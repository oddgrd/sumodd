#pragma once

#include <stm32f3xx_hal_tim.h>
#include <stdint.h>

// An NEC Extended procotol message.
typedef struct
{
    // TODO: check that the address matches my remote, discard the message if not.
    uint16_t addr;
    uint8_t cmd;
    // TODO: check that this is the actual inverse of the cmd, discard message if not.
    uint8_t cmd_inverted;
} necx_decoded;

// TODO: implement a circular buffer.
void nec_handle_edge(TIM_HandleTypeDef *htim, necx_decoded nec_buffer[1]);
