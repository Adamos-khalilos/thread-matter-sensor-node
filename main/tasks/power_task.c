#include "power_task.h"
#include "app_queues.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "power_task";

// Set to 1 once you're ready to test real deep sleep on battery power.
// Keep at 0 while developing on USB so your flash/monitor session
// doesn't get killed every cycle.
#define DEEP_SLEEP_ENABLED 0

static void power_task_fn(void *pvParameters)
{
    ESP_LOGI(TAG, "power_task started");

    power_event_t evt;

    while (1) {
        if (xQueueReceive(g_power_event_queue, &evt, portMAX_DELAY) == pdTRUE) {
            if (evt == POWER_EVENT_WORK_DONE) {
                ESP_LOGI(TAG, "work done, would sleep here");

#if DEEP_SLEEP_ENABLED
                // TODO: tune sleep duration once you have real timing
                // requirements from the sensor sampling interval.
                //   esp_sleep_enable_timer_wakeup(60 * 1000000ULL); // 60s
                //   esp_deep_sleep_start();
#endif
            } else {
                ESP_LOGW(TAG, "work failed — would retry or back off here");
            }
        }
    }
}

void power_task_start(void)
{
    xTaskCreate(power_task_fn, "power_task", 2048, NULL, 5, NULL);
}
