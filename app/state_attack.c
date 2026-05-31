#include "main.h"

#include "state.h"
#include "state_attack.h"
#include "drivers/motor_driver.h"

void state_attack_enter(struct StateAttackCtx *ctx, State from, StateEvent event)
{
    UNUSED(event);
    UNUSED(from);

    // TODO: switch on from, if we were already attacking in the same way, do nothing. Keep
    // previous attack direction in attack state.
    Enemy enemy = ctx->state_common->enemy;
    if (enemy.bearing == BEARING_FRONT)
    {
        if (enemy.distance_mm > 75)
        {
            motor_drive(35, DRIVE_FORWARD);
        }
        else
        {
            motor_drive(0, DRIVE_STOP);
        }
    }
    else if (enemy.bearing == BEARING_LEFT)
    {
        if (enemy.distance_mm > 100)
        {
            motor_drive(25, DRIVE_ARC_LEFT);
        }
        else
        {
            motor_drive(25, DRIVE_SPIN_LEFT);
        }
    }
    else if (enemy.bearing == BEARING_RIGHT)
    {
        if (enemy.distance_mm > 100)
        {
            motor_drive(25, DRIVE_ARC_RIGHT);
        }
        else
        {
            motor_drive(25, DRIVE_SPIN_RIGHT);
        }
    }
    else
    {
        motor_drive(0, DRIVE_STOP);
    }
}