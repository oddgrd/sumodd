#pragma once

/**
 * @brief Initialize the line sensor driver.
 *
 * Start the timer peripheral that triggers ADC conversion, start the ADC and the ADC watchdog.
 */
void line_sensor_init(void);

/**
 * @brief Interrupt callback for ADC conversion completed.
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc);
