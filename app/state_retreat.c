#include "main.h"

#include "state_retreat.h"
#include "state.h"
#include "drivers/motor_driver.h"

#define STATE_RETREAT_DURATION_MS (1000U)

static void state_retreat_run(struct StateRetreatCtx *ctx)
{
    *ctx->common->timer = HAL_GetTick() + STATE_RETREAT_DURATION_MS;
    // TODO: handle all line types, not just front and back
    if (ctx->common->line == LINE_FRONT)
    {
        motor_drive(25, DRIVE_REVERSE);
    }
    else
    {
        motor_drive(25, DRIVE_FORWARD);
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
            // TODO: handle (ignore) remaining events
        }
        break;
    case STATE_RETREAT:
        switch (event)
        {
        case EVT_LINE:
            state_retreat_run(ctx);
            break;
        case EVT_TIMEOUT:
            // Currently, if state retreat receives timeout event, it will enter search. But we may
            // need to do this differently if we want multiple moves.
            // TODO: verify rest of events can be ignored
            break;
        }
        break;
    }
}
