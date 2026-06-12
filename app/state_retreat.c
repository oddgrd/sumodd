#include "main.h"

#include "state_retreat.h"
#include "state.h"
#include "debug.h"
#include "drivers/motor_driver.h"

#define STATE_RETREAT_DURATION_MS (600U)

static RetreatState next_retreat_state(const struct StateRetreatCtx *ctx)
{
    RetreatState state = RETREAT_STATE_NONE;
    switch (ctx->common->line)
    {
    case LINE_FRONT:
        state = RETREAT_STATE_REVERSE;
        break;
    case LINE_BACK:
        state = RETREAT_STATE_FORWARD;
        break;
    case LINE_LEFT:
        state = RETREAT_STATE_REVERSE_ARC_RIGHT;
        break;
    case LINE_RIGHT:
        state = RETREAT_STATE_REVERSE_ARC_LEFT;
        break;
    case LINE_FRONT_LEFT:
        state = RETREAT_STATE_REVERSE_ARC_RIGHT;
        break;
    case LINE_FRONT_RIGHT:
        state = RETREAT_STATE_REVERSE_ARC_LEFT;
        break;
    case LINE_BACK_LEFT:
        state = RETREAT_STATE_FORWARD_ARC_RIGHT;
        break;
    case LINE_BACK_RIGHT:
        state = RETREAT_STATE_FORWARD_ARC_LEFT;
        break;
    // TODO: failure handling here, should not be possible.
    case LINE_NONE:
        break;
    }
    DEBUG_PRINTF("Set retreat state: %d\n", state);

    return state;
}

static void state_retreat_run(struct StateRetreatCtx *ctx)
{
    *ctx->common->timer = HAL_GetTick() + STATE_RETREAT_DURATION_MS;
    switch (next_retreat_state(ctx))
    {
    case RETREAT_STATE_REVERSE:
        ctx->state = RETREAT_STATE_REVERSE;
        motor_drive(60, DRIVE_REVERSE);
        break;
    case RETREAT_STATE_FORWARD:
        ctx->state = RETREAT_STATE_FORWARD;
        motor_drive(60, DRIVE_FORWARD);
        break;
    case RETREAT_STATE_FORWARD_ARC_LEFT:
        ctx->state = RETREAT_STATE_FORWARD_ARC_LEFT;
        motor_drive(45, DRIVE_FORWARD_ARC_LEFT);
        break;
    case RETREAT_STATE_FORWARD_ARC_RIGHT:
        ctx->state = RETREAT_STATE_FORWARD_ARC_RIGHT;
        motor_drive(45, DRIVE_FORWARD_ARC_RIGHT);
        break;
    case RETREAT_STATE_REVERSE_ARC_LEFT:
        ctx->state = RETREAT_STATE_REVERSE_ARC_LEFT;
        motor_drive(45, DRIVE_REVERSE_ARC_LEFT);
        break;
    case RETREAT_STATE_REVERSE_ARC_RIGHT:
        ctx->state = RETREAT_STATE_REVERSE_ARC_RIGHT;
        motor_drive(45, DRIVE_REVERSE_ARC_RIGHT);
        break;
    case RETREAT_STATE_NONE:
        break;
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
