#pragma once

#include <stm32f3xx_hal_tim.h>
#include <stdint.h>

#include "ring_buffer.h"

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
 * @brief Input-capture callback for NEC IR signal decoding.
 *
 * Triggered by a timer peripheral set to input capture on the falling edges of an NEC
 * IR signal. Pulse timing is accumulated to assemble and decode an NEC frame.
 *
 * @param htim        Handle to input capture timer peripheral.
 * @param nec_buffer  Output buffer for the decoded frame.
 */
void nec_capture_isr(TIM_HandleTypeDef *htim, ring_buffer *nec_buffer);
