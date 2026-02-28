#pragma once

typedef enum
{
    STANDBY,
    SEARCH,
    ATTACK,
    RETREAT
} State;

typedef enum
{
    IR_STOP,
    IR_START,
    OPPONENT_DETECTED,
    OPPONENT_LOST,
    // TODO: just keep one event type for edge detected, and rather keep state about which sensors
    // are triggered?
    FRONT_LEFT_EDGE_DETECTED,
    FRONT_RIGHT_EDGE_DETECTED,
    RETREAT_DONE,
} Event;

// TODO: doc comments
void state_machine_init(void);
void state_machine_run(void);
void state_event_push(Event event);
