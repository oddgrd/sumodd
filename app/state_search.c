#include "main.h"

#include "state.h"
#include "state_search.h"
#include "drivers/motor_driver.h"

void state_search_enter(struct StateSearchCtx *ctx, State from, StateEvent event)
{
    // TODO: use timer to search smarter than just spinning.
    motor_drive(30, DRIVE_SPIN_LEFT);

    UNUSED(event);
    UNUSED(from);
    UNUSED(ctx);
}
