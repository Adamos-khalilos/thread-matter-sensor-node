#pragma once

/**
 * Starts the radio/comms task. Call once from app_main().
 *
 * Responsibilities (to implement):
 *   - Initialize and start the Matter stack (esp_matter::start)
 *   - Register the temperature sensor endpoint/cluster
 *   - Consume readings from g_sensor_reading_queue and update the
 *     Matter attribute (this is what makes the value visible to
 *     Home app / Google Home / Home Assistant)
 *   - Post to g_power_event_queue once a transmit cycle is complete,
 *     so power_task knows it's safe to sleep again
 */
void radio_task_start(void);
