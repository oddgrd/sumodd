#include "main.h"

#include "state.h"
#include "state_search.h"
#include "drivers/motor_driver.h"

void state_search_enter(struct StateSearchCtx *ctx, State from, StateEvent event)
{
    motor_drive(50, DRIVE_SPIN_LEFT);

    UNUSED(event);
    UNUSED(from);
    UNUSED(ctx);
}
