#pragma once

/**
 * @brief Callback for ADC WD out of range interrupt.
 *
 */
void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef *hadc);

/**
 * @brief Initialize the line sensor driver.
 *
 * Start the timer peripheral that triggers ADC conversion, start the ADC and the ADC watchdog.
 */
void line_sensor_init(void);