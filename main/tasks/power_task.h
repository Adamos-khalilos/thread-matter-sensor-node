#pragma once

/**
 * Starts the power management task. Call once from app_main().
 *
 * Responsibilities (to implement):
 *   - Listen on g_power_event_queue for "work done" signals from radio_task
 *   - Once idle, configure and enter deep sleep (esp_sleep_enable_timer_wakeup)
 *   - On wake, the whole chip resets and app_main() runs again — so any
 *     "resume" state needs to be saved in RTC memory (RTC_DATA_ATTR) if
 *     you need it to survive sleep
 *
 * NOTE: leave deep sleep disabled while developing on USB power — it will
 * disconnect your debugger/monitor session on every sleep cycle. Only
 * enable it once you move to battery power for the final measurement.
 */
void power_task_start(void);
