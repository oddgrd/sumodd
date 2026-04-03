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

// TODO: retreat event?
typedef enum
{
    EVT_IR_CMD,
    EVT_ENEMY,
    EVT_LINE_DETECTED,
} state_event_type_t;

/**
 * @brief The type of event, and associated context for the given event type.
 */
typedef struct
{
    state_event_type_t type;
    union
    {
        line_type_detected_t line;
        ir_command_t ir_cmd;
    };
} state_event_t;

// TODO: doc comments
void state_machine_init(void);
void state_machine_run(void);
void state_event_push(state_event_t *event);
