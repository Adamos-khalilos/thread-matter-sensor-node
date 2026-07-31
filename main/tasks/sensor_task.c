#include "sensor_task.h"
#include "app_queues.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "sensor_task";

// How often to sample, in milliseconds. Once you add deep sleep, this
// loop won't run continuously — power_task will wake the chip on a timer
// instead. For now (dev-board-on-USB stage) a simple delay loop is fine.
#define SAMPLE_INTERVAL_MS 5000

static void sensor_task_fn(void *pvParameters)
{
    ESP_LOGI(TAG, "sensor_task started");

    // TODO: init I2C bus here (i2c_param_config / i2c_driver_install)
    // TODO: init BME280 / SHT40 driver here

    while (1) {
        // TODO: replace with a real sensor read.
        sensor_reading_t reading = {
            .temperature_c = 22.5f,   // placeholder value
            .humidity_pct  = 45.0f,   // placeholder value
            .timestamp_us  = esp_timer_get_time(),
        };

        if (xQueueSend(g_sensor_reading_queue, &reading, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGW(TAG, "reading queue full, dropping sample");
        } else {
            ESP_LOGI(TAG, "queued reading: %.1fC, %.1f%%", reading.temperature_c, reading.humidity_pct);
        }

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }
}

void sensor_task_start(void)
{
    xTaskCreate(sensor_task_fn, "sensor_task", 4096, NULL, 5, NULL);
}
