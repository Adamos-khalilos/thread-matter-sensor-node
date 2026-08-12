#include "bme280.h"

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "bme280";

/* ---- Registers ---- */
#define REG_CALIB_00    0x88   // dig_T1..dig_P9, 26 bytes
#define REG_CHIP_ID     0xD0
#define REG_RESET       0xE0
#define REG_CALIB_26    0xE1   // dig_H2..dig_H6, 7 bytes
#define REG_CTRL_HUM    0xF2
#define REG_STATUS      0xF3
#define REG_CTRL_MEAS   0xF4
#define REG_CONFIG      0xF5
#define REG_DATA_START  0xF7   // press(3) + temp(3) + hum(2) = 8 bytes

#define CHIP_ID_EXPECTED 0x60

// ctrl_meas: osrs_t=001, osrs_p=001, mode=01 (forced)
// Forced mode (vs. normal/continuous) means the sensor only converts
// when we ask it to, then goes back to idle — a better fit for a node
// that's going to spend most of its life in deep sleep between reads.
#define CTRL_MEAS_FORCED_OSRS1  0x25
#define CTRL_HUM_OSRS1          0x01

static i2c_master_bus_handle_t s_bus = NULL;
static i2c_master_dev_handle_t s_dev = NULL;

// Factory calibration coefficients (read once at init, per datasheet §4.2.2)
static struct {
    uint16_t dig_T1;
    int16_t  dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4, dig_H5;
    int8_t   dig_H6;
} s_calib;

// t_fine carries fine temperature resolution from the temperature
// compensation into the pressure/humidity compensation, per datasheet.
static int32_t s_t_fine;

/* ---- Low-level register I/O ---- */

static esp_err_t reg_write(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return i2c_master_transmit(s_dev, buf, sizeof(buf), pdMS_TO_TICKS(100));
}

static esp_err_t reg_read(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

/* ---- Calibration ---- */

static esp_err_t read_calibration(void)
{
    uint8_t c1[26];
    uint8_t c2[7];

    esp_err_t err = reg_read(REG_CALIB_00, c1, sizeof(c1));
    if (err != ESP_OK) return err;

    err = reg_read(REG_CALIB_26, c2, sizeof(c2));
    if (err != ESP_OK) return err;

    s_calib.dig_T1 = (uint16_t)(c1[1] << 8 | c1[0]);
    s_calib.dig_T2 = (int16_t)(c1[3] << 8 | c1[2]);
    s_calib.dig_T3 = (int16_t)(c1[5] << 8 | c1[4]);

    s_calib.dig_P1 = (uint16_t)(c1[7] << 8 | c1[6]);
    s_calib.dig_P2 = (int16_t)(c1[9] << 8 | c1[8]);
    s_calib.dig_P3 = (int16_t)(c1[11] << 8 | c1[10]);
    s_calib.dig_P4 = (int16_t)(c1[13] << 8 | c1[12]);
    s_calib.dig_P5 = (int16_t)(c1[15] << 8 | c1[14]);
    s_calib.dig_P6 = (int16_t)(c1[17] << 8 | c1[16]);
    s_calib.dig_P7 = (int16_t)(c1[19] << 8 | c1[18]);
    s_calib.dig_P8 = (int16_t)(c1[21] << 8 | c1[20]);
    s_calib.dig_P9 = (int16_t)(c1[23] << 8 | c1[22]);

    s_calib.dig_H1 = c1[25];

    s_calib.dig_H2 = (int16_t)(c2[1] << 8 | c2[0]);
    s_calib.dig_H3 = c2[2];
    s_calib.dig_H4 = (int16_t)((c2[3] << 4) | (c2[4] & 0x0F));
    s_calib.dig_H5 = (int16_t)((c2[5] << 4) | (c2[4] >> 4));
    s_calib.dig_H6 = (int8_t)c2[6];

    return ESP_OK;
}

/* ---- Compensation formulas (Bosch BME280 datasheet §4.2.3, 32-bit integer variant) ---- */

static int32_t compensate_temperature(int32_t adc_T)
{
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)s_calib.dig_T1 << 1))) * (int32_t)s_calib.dig_T2) >> 11;
    int32_t var2 = (((((adc_T >> 4) - (int32_t)s_calib.dig_T1) * ((adc_T >> 4) - (int32_t)s_calib.dig_T1)) >> 12) * (int32_t)s_calib.dig_T3) >> 14;
    s_t_fine = var1 + var2;
    return (s_t_fine * 5 + 128) >> 8;   // hundredths of a degree C
}

static uint32_t compensate_pressure(int32_t adc_P)
{
    int64_t var1 = (int64_t)s_t_fine - 128000;
    int64_t var2 = var1 * var1 * (int64_t)s_calib.dig_P6;
    var2 += (var1 * (int64_t)s_calib.dig_P5) << 17;
    var2 += ((int64_t)s_calib.dig_P4) << 35;
    var1 = ((var1 * var1 * (int64_t)s_calib.dig_P3) >> 8) + ((var1 * (int64_t)s_calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)s_calib.dig_P1) >> 33;

    if (var1 == 0) {
        return 0; // avoid divide-by-zero (would indicate a bad read)
    }

    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = ((int64_t)s_calib.dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    var2 = ((int64_t)s_calib.dig_P8 * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)s_calib.dig_P7) << 4);

    return (uint32_t)p;   // Q24.8 format, i.e. Pa * 256
}

static uint32_t compensate_humidity(int32_t adc_H)
{
    int32_t v_x1 = s_t_fine - 76800;
    v_x1 = ((((adc_H << 14) - (((int32_t)s_calib.dig_H4) << 20) - (((int32_t)s_calib.dig_H5) * v_x1))
             + 16384) >> 15)
           * (((((((v_x1 * (int32_t)s_calib.dig_H6) >> 10)
                  * (((v_x1 * (int32_t)s_calib.dig_H3) >> 11) + 32768)) >> 10)
                + 2097152) * (int32_t)s_calib.dig_H2 + 8192) >> 14);
    v_x1 = v_x1 - (((((v_x1 >> 15) * (v_x1 >> 15)) >> 7) * (int32_t)s_calib.dig_H1) >> 4);
    v_x1 = v_x1 < 0 ? 0 : v_x1;
    v_x1 = v_x1 > 419430400 ? 419430400 : v_x1;
    return (uint32_t)(v_x1 >> 12);   // Q22.10 format, i.e. %RH * 1024
}

/* ---- Public API ---- */

esp_err_t bme280_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = BME280_I2C_PORT,
        .sda_io_num = BME280_I2C_SDA_GPIO,
        .scl_io_num = BME280_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &s_bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
        return err;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BME280_I2C_ADDR,
        .scl_speed_hz = BME280_I2C_FREQ_HZ,
    };
    err = i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t chip_id = 0;
    err = reg_read(REG_CHIP_ID, &chip_id, 1);
    if (err != ESP_OK) {
        // Most likely: nothing wired up yet at this address.
        ESP_LOGW(TAG, "no response at 0x%02X (sensor not connected?)", BME280_I2C_ADDR);
        return ESP_ERR_NOT_FOUND;
    }
    if (chip_id != CHIP_ID_EXPECTED) {
        ESP_LOGE(TAG, "unexpected chip id 0x%02X (expected 0x%02X)", chip_id, CHIP_ID_EXPECTED);
        return ESP_ERR_NOT_FOUND;
    }

    err = read_calibration();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to read calibration data: %s", esp_err_to_name(err));
        return err;
    }

    // ctrl_hum must be written before ctrl_meas for the humidity
    // oversampling setting to take effect (datasheet §5.4.3).
    err = reg_write(REG_CTRL_HUM, CTRL_HUM_OSRS1);
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "BME280 found and calibrated (addr 0x%02X)", BME280_I2C_ADDR);
    return ESP_OK;
}

esp_err_t bme280_read(bme280_data_t *out)
{
    if (out == NULL || s_dev == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Kick off one forced-mode conversion.
    esp_err_t err = reg_write(REG_CTRL_MEAS, CTRL_MEAS_FORCED_OSRS1);
    if (err != ESP_OK) return err;

    // Poll the "measuring" bit (status reg, bit 3) until conversion
    // finishes. With osrs=1 for all three this typically takes ~8ms;
    // we cap the wait so a disconnected/stuck sensor can't hang the task.
    for (int i = 0; i < 20; i++) {
        uint8_t status = 0;
        err = reg_read(REG_STATUS, &status, 1);
        if (err != ESP_OK) return err;
        if ((status & 0x08) == 0) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    uint8_t raw[8];
    err = reg_read(REG_DATA_START, raw, sizeof(raw));
    if (err != ESP_OK) return err;

    int32_t adc_P = (int32_t)((raw[0] << 12) | (raw[1] << 4) | (raw[2] >> 4));
    int32_t adc_T = (int32_t)((raw[3] << 12) | (raw[4] << 4) | (raw[5] >> 4));
    int32_t adc_H = (int32_t)((raw[6] << 8) | raw[7]);

    int32_t temp_hundredths = compensate_temperature(adc_T);   // must run first, sets s_t_fine
    uint32_t press_q24_8 = compensate_pressure(adc_P);
    uint32_t hum_q22_10 = compensate_humidity(adc_H);

    out->temperature_c = temp_hundredths / 100.0f;
    out->pressure_hpa = (press_q24_8 / 256.0f) / 100.0f;   // Pa -> hPa
    out->humidity_pct = hum_q22_10 / 1024.0f;

    return ESP_OK;
}
