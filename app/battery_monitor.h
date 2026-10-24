#pragma once

#include <stdbool.h>

/**
 * @brief Check whether the battery voltage is below the safe threshold.
 */
bool battery_monitor_check_low_voltage();

/**
 * @brief Shut down the motor, and indicate failure with LEDs in an infinite loop.
 */
void battery_monitor_low_voltage_shutdown(void);

/**
 * @brief Initialize the battery monitor.
 *
 * Initialize and start the conversion triggering timer peripheral and the ADC.
 */
void battery_monitor_init(void);