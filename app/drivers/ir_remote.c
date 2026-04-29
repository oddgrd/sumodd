#include "main.h"
#include <stdint.h>
#include <inttypes.h>

#include "ir_remote.h"
#include "ring_buffer.h"
#include "state.h"
#include "app_config.h"
#include "debug.h"

#define CMD_BUFFER_SIZE (8U)
#define FINAL_PULSE 34U
#define B1_PULSE_WIDTH_TICKS 1800U

TIM_HandleTypeDef htim17;

static uint32_t raw_message = 0;

static uint8_t pulse_count = 0;
static uint16_t last = 0;

static IrCommand cmd_buffer[CMD_BUFFER_SIZE] = {0};
static RingBuffer cmd_queue = {0};

/**
 * @brief TIM17 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM17_Init(void)
{
    TIM_IC_InitTypeDef sConfigIC = {0};

    htim17.Instance = TIM17;
    htim17.Init.Prescaler = 64 - 1;
    htim17.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim17.Init.Period = 65536 - 1;
    htim17.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim17.Init.RepetitionCounter = 0;
    htim17.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim17) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_IC_Init(&htim17) != HAL_OK)
    {
        Error_Handler();
    }
    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 0;
    if (HAL_TIM_IC_ConfigChannel(&htim17, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }
}

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
} NecStatus;

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
static NecStatus parse_necx(const uint32_t raw, NecxDecoded *out)
{
    NecxDecoded decoded = {0};

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

/**
 * @brief Input-capture callback for NEC IR signal decoding.
 *
 * Triggered by a timer peripheral set to input capture on the falling edges of an NEC
 * IR signal. Pulse timing is accumulated to assemble and decode an NEC frame.
 *
 * @param htim        Handle to input capture timer peripheral.
 */
static void nec_capture_isr(TIM_HandleTypeDef *htim)
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
        NecxDecoded decoded = {0};

        int ret = parse_necx(raw_message, &decoded);
        IrCommand cmd = IR_NONE;
        if (ret == 0)
        {
            switch (decoded.cmd)
            {
            case 0x10:
                cmd = IR_START;
                ring_buffer_push(&cmd_queue, &cmd);
                break;
            case 0x11:
                cmd = IR_STOP;
                ring_buffer_push(&cmd_queue, &cmd);
            default:
                break;
            }
        }
        else
        {
            DEBUG_PRINTF("Failed to parse IR command, raw message %d\r\n", raw_message);
        }

        raw_message = 0;
        pulse_count = 0;
    }
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM17 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
        nec_capture_isr(htim);
    }
}

IrCommand ir_remote_get_cmd(void)
{
    IrCommand cmd = IR_NONE;

    ring_buffer_pop(&cmd_queue, &cmd);

    return cmd;
}

void ir_remote_init(void)
{
    MX_TIM17_Init();
    ring_buffer_init(&cmd_queue, (uint8_t *)cmd_buffer, CMD_BUFFER_SIZE, sizeof(IrCommand));

    if (HAL_TIM_IC_Start_IT(&htim17, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    };
}