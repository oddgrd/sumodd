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
    IR_START,
    IR_STOP,
    OPPONENT_DETECTED,
    OPPONENT_LOST,
    EDGE_DETECTED,
    RETREAT_DONE,
} Event;

// TODO: doc comments
void state_machine_init(void);
void state_machine_run(void);
void state_event_push(Event event);
