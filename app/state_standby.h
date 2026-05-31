#pragma once
#include "state.h"
#include "state_common.h"

/**
 * In the standby state we simply blink a debug LED to indicate the state, and ignore all events
 * except for IR commands. If the IR start command is received, we proceed to the search state.
 */

struct StateStandbyCtx
{
    const struct StateCommonCtx *state_common;
    // Last time the debug LED blinked.
    uint32_t last_blink_ms;
};

void state_standby_enter(struct StateStandbyCtx *ctx, State from, StateEvent event);