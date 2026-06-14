#pragma once

/**
 * This driver is concerned with configuring and initializing the peripherals needed to read our
 * line detection sensors, specifically an ADC and a timer peripheral. It also defines the DMA
 * buffer where ADC conversion are written to, as well as an API for reading the buffer.
 *
 * For more information on line detection, refer to the line detection docs in
 * `docs/line-detection.md`.
 */

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
    LINE_NONE,
    LINE_FRONT,
    LINE_BACK,
    LINE_LEFT,
    LINE_RIGHT,
    LINE_FRONT_LEFT,
    LINE_FRONT_RIGHT,
    LINE_BACK_LEFT,
    LINE_BACK_RIGHT,
} LineType;

LineType get_line(void);
