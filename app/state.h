#pragma once

typedef enum
{
    STANDBY,
    SEARCH,
    ATTACK,
    RETREAT
} State;

extern volatile State sumo_bot_state;