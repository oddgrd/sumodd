#include "stm32f3xx_hal.h"
#include <stdint.h>
#include <inttypes.h>

#include "nec_capture.h"

#define FINAL_PULSE 34U
#define B1_PULSE_WIDTH_TICKS 1800U

static nec_message working_message = {0};

static uint8_t pulse_counter = 0;
static uint16_t last = 0;

void nec_handle_edge(TIM_HandleTypeDef *htim, nec_message nec_buffer[1])
{
    uint16_t now = (uint16_t)HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
    uint16_t dt = (uint16_t)(now - last);
    last = now;

    // TODO: validate this range, and also that repeat frames aren't captured.
    // See protocol for repeat frames: https://sibotic.wordpress.com/wp-content/uploads/2013/12/adoh-necinfraredtransmissionprotocol-281113-1713-47344.pdf
    if (dt > 12000 && dt < 15000)
    {
        pulse_counter = 1;
        working_message.raw = 0;
        return;
    }

    // If no leader, ignore
    if (pulse_counter == 0)
    {
        return;
    }

    pulse_counter++;

    // TODO: validate that dt is within valid range for bits.
    if (pulse_counter >= 3 && pulse_counter <= FINAL_PULSE)
    {
        working_message.raw <<= 1;
        working_message.raw += (dt >= B1_PULSE_WIDTH_TICKS) ? 1 : 0;
    }

    if (pulse_counter == FINAL_PULSE)
    {
        nec_buffer[0] = working_message;
        working_message.raw = 0;
        pulse_counter = 0;
    }
}