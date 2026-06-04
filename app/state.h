#pragma once

#include "drivers/ir_remote.h"
#include "drivers/line_sensor.h"
#include "ranging.h"

typedef enum
{
    STATE_STANDBY,
    STATE_SEARCH,
    STATE_ATTACK,
    STATE_RETREAT
} State;

typedef enum
{
    EVT_NONE,
    EVT_IR_CMD,
    EVT_ENEMY,
    EVT_LINE,
    EVT_TIMEOUT,
} StateEvent;

/**
 * @brief Initialize the state machine state.
 */
void state_machine_init(void);

/**
 * @brief Run a step in the state machine.
 */
void state_machine_run(void);

/**
 * @brief Push a state event into the state machine event queue.
 */
void state_event_push(StateEvent *event);
