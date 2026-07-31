#include "radio_task.h"
#include "app_queues.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "radio_task";

static void radio_task_fn(void *pvParameters)
{
    ESP_LOGI(TAG, "radio_task started");

    // TODO: initialize the Matter stack here. Rough shape, once
    // ESP-Matter is set up (see esp-matter/examples/temperature_sensor):
    //
    //   node::config_t node_config;
    //   node_t *node = node::create(&node_config, NULL, NULL);
    //   temperature_sensor::config_t sensor_config;
    //   endpoint_t *endpoint = temperature_sensor::create(node, &sensor_config, ENDPOINT_FLAG_NONE, NULL);
    //   esp_matter::start(NULL);
    //
    // Commissioning (pairing into a Thread/Matter fabric) happens
    // automatically after esp_matter::start() — you'll scan a QR code
    // or enter a pairing code from your phone's Home app.

    sensor_reading_t reading;

    while (1) {
        if (xQueueReceive(g_sensor_reading_queue, &reading, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "got reading %.1fC -> would update Matter attribute here", reading.temperature_c);

            // TODO: update the Matter temperature attribute, e.g.
            //   attribute::update(endpoint_id, cluster_id, attribute_id, &val);

            power_event_t evt = POWER_EVENT_WORK_DONE;
            xQueueSend(g_power_event_queue, &evt, pdMS_TO_TICKS(100));
        }
    }
}

void radio_task_start(void)
{
    xTaskCreate(radio_task_fn, "radio_task", 8192, NULL, 5, NULL);
}
