#pragma once

#include <stm32f3xx_hal_tim.h>
#include <stdint.h>

typedef union
{
    struct
    {
        uint8_t cmd_inverted;
        uint8_t cmd;
        uint8_t addr_inverted;
        uint8_t addr;
    } decoded;
    uint32_t raw;
} nec_message;

// TODO: implement a circular buffer.

void nec_handle_edge(TIM_HandleTypeDef *htim, nec_message nec_buffer[1]);