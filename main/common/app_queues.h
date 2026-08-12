/**
 * app_queues.h
 *
 * Shared message types and queue handles used to pass data between tasks.
 * Keeping this in one place is what lets sensor_task, radio_task, and
 * power_task stay decoupled from each other — nobody calls another task's
 * functions directly, everyone just posts to a queue.
 */

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdint.h>

/* ---- Message types ---- */

// Sent from sensor_task -> radio_task whenever a new reading is ready.
typedef struct {
    float temperature_c;
    float humidity_pct;
    float pressure_hpa;
    int64_t timestamp_us;
} sensor_reading_t;

// Sent from radio_task or sensor_task -> power_task to signal that
// whatever needed to happen (transmit, retry, etc.) is done, and it's
// safe to go back to sleep.
typedef enum {
    POWER_EVENT_WORK_DONE,
    POWER_EVENT_WORK_FAILED,
} power_event_t;

/* ---- Queue handles (defined in app_main.c, extern'd here) ---- */

extern QueueHandle_t g_sensor_reading_queue;   // sensor_task -> radio_task
extern QueueHandle_t g_power_event_queue;      // radio_task -> power_task

/* ---- Tunables ---- */

#define SENSOR_READING_QUEUE_LEN   4
#define POWER_EVENT_QUEUE_LEN      4
