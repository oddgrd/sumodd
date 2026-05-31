#pragma once

#include "state.h"
#include "state_common.h"

/**
 * In the search state we drive the motors in a way that allows us to scan the arena with the
 * time-of-flight sensors, which are continuously ranging.
 */

struct StateSearchCtx
{
    const struct StateCommonCtx *state_common;
};

void state_search_enter(struct StateSearchCtx *ctx, State from, StateEvent event);