#include "stm32f3xx_hal.h"
#include <stdint.h>
#include <inttypes.h>

#include "nec_capture.h"

#define FINAL_PULSE 34U
#define B1_PULSE_WIDTH_TICKS 1800U

static necx_decoded nec_decoded = {0};
static uint32_t raw_message = 0;

static uint8_t pulse_count = 0;
static uint16_t last = 0;

// Reverse the bit order of a byte, making the LSB the MSB, the second LSB the second MSB, and so
// on.
static uint8_t reverse_bits(uint8_t b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

void nec_handle_edge(TIM_HandleTypeDef *htim, necx_decoded nec_buffer[1])
{
    uint16_t now = (uint16_t)HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
    uint16_t dt = (uint16_t)(now - last);
    last = now;

    pulse_count++;

    if (pulse_count >= 3 && pulse_count <= FINAL_PULSE)
    {
        // TODO: more granular validation of range.
        if (dt > 3000)
        {
            pulse_count = 1;
            raw_message = 0;
            return;
        }
        raw_message <<= 1;
        raw_message += (dt >= B1_PULSE_WIDTH_TICKS) ? 1 : 0;
    }

    if (pulse_count == FINAL_PULSE)
    {
        nec_decoded.addr = reverse_bits((raw_message >> 16) & 0xFFFF);
        nec_decoded.cmd = reverse_bits((raw_message >> 8) & 0xFF);
        nec_decoded.cmd_inverted = reverse_bits(raw_message & 0xFF);
        nec_buffer[0] = nec_decoded;
        raw_message = 0;
        pulse_count = 0;
    }
}
