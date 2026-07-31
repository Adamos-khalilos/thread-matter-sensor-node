#pragma once

/**
 * Starts the sensor task. Call once from app_main().
 *
 * Responsibilities (to implement):
 *   - Initialize I2C and the sensor driver (BME280 / SHT40)
 *   - Periodically sample temperature/humidity
 *   - Push each reading onto g_sensor_reading_queue
 *   - Never touch the radio directly — that's radio_task's job
 */
void sensor_task_start(void);
