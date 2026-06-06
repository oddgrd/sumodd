#include "main.h"

#include "state_retreat.h"
#include "state.h"
#include "debug.h"
#include "drivers/motor_driver.h"

#define STATE_RETREAT_DURATION_MS (450U)

static void state_retreat_run(struct StateRetreatCtx *ctx)
{
    *ctx->common->timer = HAL_GetTick() + STATE_RETREAT_DURATION_MS;
    // TODO: handle all line types, not just front and back
    if (ctx->common->line == LINE_FRONT)
    {
        ctx->state = RETREAT_STATE_REVERSE;
        motor_drive(50, DRIVE_REVERSE);
    }
    else
    {
        ctx->state = RETREAT_STATE_FORWARD;
        motor_drive(50, DRIVE_FORWARD);
    }
}

static RetreatState next_retreat_state(const struct StateRetreatCtx *ctx)
{
    if (ctx->common->line == LINE_FRONT)
    {
        return RETREAT_STATE_REVERSE;
    }
    else
    {
        return RETREAT_STATE_FORWARD;
    }
}

void state_retreat_enter(struct StateRetreatCtx *ctx, State from, StateEvent event)
{
    switch (from)
    {
    case STATE_SEARCH:
    case STATE_ATTACK:
        switch (event)
        {
        case EVT_LINE:
            state_retreat_run(ctx);
            break;
        case EVT_ENEMY:
        case EVT_IR_CMD:
        case EVT_NONE:
        case EVT_TIMEOUT:
            // TODO: handle these invalid cases.
            break;
        }
        break;
    case STATE_RETREAT:
        switch (event)
        {
        case EVT_LINE:
            if (next_retreat_state(ctx) != ctx->state)
            {
                state_retreat_run(ctx);
            }
            break;
        case EVT_ENEMY:
        case EVT_IR_CMD:
        case EVT_NONE:
        case EVT_TIMEOUT:
            // Currently, if state retreat receives timeout event, it will enter search. But we may
            // need to do this differently if we want multiple moves.
            // TODO: handle the other invalid cases.
            break;
        }
        break;
    case STATE_STANDBY:
        // TODO: handle these invalid cases.
        break;
    }
}
