#include "stm32f3xx_hal.h"
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

#include "nec_capture.h"
#include "app_config.h"
#include "ring_buffer.h"

#define FINAL_PULSE 34U
#define B1_PULSE_WIDTH_TICKS 1800U

static uint32_t raw_message = 0;

static uint8_t pulse_count = 0;
static uint16_t last = 0;

/**
 * Reverse the bit order of a byte, making the LSB the MSB, the second LSB the second MSB, and so
 * on.
 */
static uint8_t reverse_bits(uint8_t b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

typedef enum
{
    NEC_OK = 0,
    NEC_ERR_ADDR,
    NEC_ERR_CMD_INVERT
} nec_status_t;

/**
 * @brief Parse an NECx message from a raw 32-bit frame.
 *
 * @param raw  Raw decoded NEC frame bits.
 * @param out  Output buffer for the decoded message.
 *
 * @retval NEC_OK               Message is valid.
 * @retval NEC_ERR_ADDR         Address does not match IR_REMOTE_ADDR.
 * @retval NEC_ERR_CMD_INVERT   Command inverse check failed.
 */
static nec_status_t parse_necx(const uint32_t raw, necx_decoded *out)
{
    necx_decoded decoded = {0};

    // Address is received in as two bytes, LSB first, with bits reversed.
    uint8_t addr_lsb = reverse_bits((raw >> 24) & 0xFF);
    uint8_t addr_msb = reverse_bits((raw >> 16) & 0xFF);
    decoded.addr = ((uint16_t)addr_msb << 8) | addr_lsb;
    // Shift right by n, mask the remaining bits, and reverse the bits.
    decoded.cmd = reverse_bits((raw >> 8) & 0xFF);
    decoded.cmd_inverted = reverse_bits(raw & 0xFF);

    if (decoded.addr != IR_REMOTE_ADDR)
    {
        return NEC_ERR_ADDR;
    }

    // The result of XOR is 1 if the bits are different, so if these are inversed, it should
    // produce a fully set byte.
    if ((decoded.cmd ^ decoded.cmd_inverted) != 0xFF)
    {
        return NEC_ERR_CMD_INVERT;
    }

    *out = decoded;
    return NEC_OK;
}

void nec_capture_isr(TIM_HandleTypeDef *htim, ring_buffer *nec_buffer)
{
    // Safe cast, TIM16 has a 16-bit auto-reload upcounter.
    uint16_t now = (uint16_t)HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
    uint16_t dt = now - last;
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
        necx_decoded decoded = {0};

        int ret = parse_necx(raw_message, &decoded);
        if (ret == 0)
        {
            ring_buffer_push(nec_buffer, decoded.cmd);
        }
        else
        {
            // TODO: print error when we have non-blocking UART logging in debug builds.
        }

        raw_message = 0;
        pulse_count = 0;
    }
}
