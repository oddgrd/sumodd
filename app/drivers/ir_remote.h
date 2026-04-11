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

// TODO: don't use hardcoded values?
typedef enum
{
    IR_NONE,
    IR_START, // 0x10
    IR_STOP,  // 0x11
} IrCommand;

/**
 * @brief Initialize the IR remote driver.
 *
 * Start the input capture timer peripheral, and initialize the interrupt handler.
 */
void ir_remote_init(void);

/**
 * @brief Fetch an IR command from the IR remote queue, if any have been received.
 */
IrCommand ir_remote_get_cmd(void);

/**
 * @brief Interrupt callback for input capture.
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);
