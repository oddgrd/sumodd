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

typedef enum
{
    LINE_FRONT,
    LINE_BACK,
    LINE_LEFT,
    LINE_RIGHT,
    LINE_FRONT_LEFT,
    LINE_FRONT_RIGHT,
    LINE_BACK_LEFT,
    LINE_BACK_RIGHT,
} LineType;
