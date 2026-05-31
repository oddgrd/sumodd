#pragma once
#include <stdint.h>
#include "drivers/line_sensor.h"
#include "ranging.h"

/**
 * @brief Common state shared across the state machine states.
 */
struct StateCommonCtx
{
    uint32_t *timer;
    LineType line;
    Enemy enemy;
    IrCommand cmd;
};