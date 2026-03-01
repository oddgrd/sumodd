#pragma once

#include <stdint.h>

/**
 * @brief Callback for data ready pin interrupt.
 * // TODO: should this live in this driver? It would apply to all GPIO_EXTI callbacks, but for now
 * all of them will be used for VL53LOX.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

/**
 * @brief Initialize the IR remote driver.
 *
 * Start the I2C bus, and initialize the data ready interrupt handler.
 */
void vl53lox_init(void);
