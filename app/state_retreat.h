#pragma once
#include "state.h"
#include "state_common.h"

/**
 * In the retreat state we retreat from the most recently detected line, until the common state
 * timer expires.
 */

typedef enum
{
    // TODO: do we need all these none variants?
    RETREAT_STATE_NONE,
    RETREAT_STATE_REVERSE,
    RETREAT_STATE_FORWARD,
    RETREAT_STATE_FORWARD_ARC_LEFT,
    RETREAT_STATE_FORWARD_ARC_RIGHT,
    RETREAT_STATE_REVERSE_ARC_LEFT,
    RETREAT_STATE_REVERSE_ARC_RIGHT,
} RetreatState;

struct StateRetreatCtx
{
    const struct StateCommonCtx *common;
    RetreatState state;
    // TODO: add this when we have retreats that are multiple moves, e.g. reverse, then turn.
    // int move_idx;
};

void state_retreat_enter(struct StateRetreatCtx *ctx, State from, StateEvent event);