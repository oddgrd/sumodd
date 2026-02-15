#pragma once

#include <stm32f3xx_hal_tim.h>
#include <stdint.h>
#include <stdbool.h>

#include "ring_buffer.h"

extern ring_buffer nec_commands;

/**
 * @brief Decoded NECx frame.
 */
typedef struct
{
    uint16_t addr;
    uint8_t cmd;
    uint8_t cmd_inverted;
} necx_decoded;

/**
 * @brief Initialize the NEC capture driver.
 *
 * Start the input capture timer peripheral, and initialize the interrupt handler.
 */
void nec_capture_init(void);

/**
 * @brief Try to read a command from the NEC capture ring buffer.
 *
 * @param out  Output byte buffer.
 *
 * @return true if an item was popped, false if the ring buffer was empty.
 */
bool nec_capture_read(uint8_t *out);

/**
 * @brief Interrupt callback for input capture.
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);
