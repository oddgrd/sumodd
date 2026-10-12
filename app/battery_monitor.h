#pragma once

/**
 * @brief Read the latest battery monitor voltage measurement in mV.
 */
uint32_t battery_monitor_get_voltage_mv();

/**
 * @brief Initialize the battery monitor.
 *
 * Initialize and start the conversion triggering timer peripheral and the ADC.
 */
void battery_monitor_init(void);