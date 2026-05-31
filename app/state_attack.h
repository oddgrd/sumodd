#pragma once

#include "state.h"
#include "state_common.h"

/**
 * In the attack state we drive at the opponent, with whatever bearing we get from the ranging
 * sensors, to push them out of the dohyo.
 */

struct StateAttackCtx
{
    const struct StateCommonCtx *state_common;
};

void state_attack_enter(struct StateAttackCtx *ctx, State from, StateEvent event);