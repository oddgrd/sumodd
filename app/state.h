#pragma once

#include "drivers/ir_remote.h"
#include "drivers/line_sensor.h"

typedef enum
{
    STANDBY,
    SEARCH,
    ATTACK,
    RETREAT
} State;

typedef enum
{
    EVT_IR_CMD,
    EVT_ENEMY,
    EVT_LINE_DETECTED,
    EVT_TIMEOUT,
} StateEventType;

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
    };
} StateEvent;

// TODO: doc comments
void state_machine_init(void);
void state_machine_run(void);

/**
 * @brief Push a state event into the state machine event queue.
 */
void state_event_push(StateEvent *event);
