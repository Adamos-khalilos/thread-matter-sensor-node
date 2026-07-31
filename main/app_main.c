/**
 * app_main.c
 *
 * Entry point. Creates the shared queues, then starts the three tasks
 * that make up the node: sensor_task, radio_task, power_task.
 *
 * This file should stay thin — it's just wiring. Real logic belongs in
 * the individual task files under tasks/.
 */

#include "app_queues.h"
#include "sensor_task.h"
#include "radio_task.h"
#include "power_task.h"

#include "esp_log.h"

static const char *TAG = "app_main";

// Definitions for the queues declared `extern` in app_queues.h
QueueHandle_t g_sensor_reading_queue = NULL;
QueueHandle_t g_power_event_queue = NULL;

void app_main(void)
{
    ESP_LOGI(TAG, "booting thread-matter-sensor-node");

    g_sensor_reading_queue = xQueueCreate(SENSOR_READING_QUEUE_LEN, sizeof(sensor_reading_t));
    g_power_event_queue    = xQueueCreate(POWER_EVENT_QUEUE_LEN, sizeof(power_event_t));

    if (g_sensor_reading_queue == NULL || g_power_event_queue == NULL) {
        ESP_LOGE(TAG, "failed to create queues, halting");
        return;
    }

    sensor_task_start();
    radio_task_start();
    power_task_start();

    ESP_LOGI(TAG, "all tasks started");
}
