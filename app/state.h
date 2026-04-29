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
    EVT_LINE_DETECTED,
    EVT_TIMEOUT,
} StateEventType;

/**
 * @brief Generate a string representation of the event type.
 */
const char *state_event_type_str(StateEventType t);

/**
 * @brief The type of event, and associated context for the given event type.
 */
typedef struct
{
    StateEventType type;
    union
    {
        LineType line;
        IrCommand ir_cmd;
        Enemy enemy;
    };
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
