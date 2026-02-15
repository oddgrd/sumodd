#pragma once

#include <stm32f3xx_hal_tim.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Decoded NECx frame.
 */
typedef struct
{
    uint16_t addr;
    uint8_t cmd;
    uint8_t cmd_inverted;
} NecxDecoded;

/**
 * @brief Initialize the IR remote driver.
 *
 * Start the input capture timer peripheral, and initialize the interrupt handler.
 */
void ir_remote_init(void);

/**
 * @brief Try to read a command from the IR remote ring buffer.
 *
 * @param out  Output byte buffer.
 *
 * @return true if an item was popped, false if the ring buffer was empty.
 */
bool ir_remote_read(uint8_t *out);

/**
 * @brief Interrupt callback for input capture.
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);
