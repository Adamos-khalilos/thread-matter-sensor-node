#include "sensor_task.h"
#include "app_queues.h"
#include "bme280.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <stdbool.h>

static const char *TAG = "sensor_task";

// How often to sample, in milliseconds. Once you add deep sleep, this
// loop won't run continuously — power_task will wake the chip on a timer
// instead. For now (dev-board-on-USB stage) a simple delay loop is fine.
#define SAMPLE_INTERVAL_MS 5000

static void sensor_task_fn(void *pvParameters)
{
    ESP_LOGI(TAG, "sensor_task started");

    esp_err_t init_err = bme280_init();
    bool sensor_present = (init_err == ESP_OK);
    if (!sensor_present) {
        ESP_LOGW(TAG, "BME280 not found (%s) — falling back to placeholder readings until it's wired up",
                 esp_err_to_name(init_err));
    }

    while (1) {
        sensor_reading_t reading = {
            .temperature_c = 22.5f,   // placeholder, used if no sensor is connected yet
            .humidity_pct  = 45.0f,
            .pressure_hpa  = 1013.25f,
            .timestamp_us  = esp_timer_get_time(),
        };

        if (sensor_present) {
            bme280_data_t data;
            esp_err_t err = bme280_read(&data);
            if (err == ESP_OK) {
                reading.temperature_c = data.temperature_c;
                reading.humidity_pct  = data.humidity_pct;
                reading.pressure_hpa  = data.pressure_hpa;
            } else {
                ESP_LOGW(TAG, "sensor read failed (%s), reusing placeholder for this cycle", esp_err_to_name(err));
            }
        }

        if (xQueueSend(g_sensor_reading_queue, &reading, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGW(TAG, "reading queue full, dropping sample");
        } else {
            ESP_LOGI(TAG, "queued reading: %.1fC, %.1f%%, %.1fhPa",
                     reading.temperature_c, reading.humidity_pct, reading.pressure_hpa);
        }

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }
}

void sensor_task_start(void)
{
    xTaskCreate(sensor_task_fn, "sensor_task", 4096, NULL, 5, NULL);
}
