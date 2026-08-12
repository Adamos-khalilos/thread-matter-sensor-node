/**
 * bme280.h
 *
 * Minimal I2C driver for the Bosch BME280 temperature / humidity /
 * pressure sensor, built on ESP-IDF's new i2c_master driver
 * (driver/i2c_master.h — the one to use on IDF v5.2+; the legacy
 * driver/i2c.h API is deprecated).
 *
 * Usage:
 *   ESP_ERROR_CHECK(bme280_init());
 *   bme280_data_t data;
 *   if (bme280_read(&data) == ESP_OK) { ... }
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- Wiring / bus config — adjust these once the board is wired up ----
// Defaults below are just common ESP32-C6-DevKitC breakout pins; swap
// them for whatever pins you actually route to the sensor.
#define BME280_I2C_PORT        0
#define BME280_I2C_SDA_GPIO    6
#define BME280_I2C_SCL_GPIO    7
#define BME280_I2C_FREQ_HZ     100000

// BME280 ships with SDO tied either high or low, which selects the
// address. Most breakout boards default to SDO=GND -> 0x76.
#define BME280_I2C_ADDR        0x76    // use 0x77 if your board's SDO is pulled high

typedef struct {
    float temperature_c;
    float humidity_pct;
    float pressure_hpa;
} bme280_data_t;

/**
 * Initializes the I2C bus, probes the sensor (checks chip ID),
 * reads factory calibration data, and configures it for normal-mode
 * sampling. Safe to call once at startup.
 *
 * Returns ESP_ERR_NOT_FOUND if no BME280 responds at BME280_I2C_ADDR
 * (e.g. sensor not wired up yet) — sensor_task can use that to fall
 * back to placeholder readings during bring-up.
 */
esp_err_t bme280_init(void);

/**
 * Triggers a forced-mode measurement and reads back compensated
 * temperature, humidity, and pressure. Blocks briefly while the
 * sensor completes conversion.
 */
esp_err_t bme280_read(bme280_data_t *out);

#ifdef __cplusplus
}
#endif
