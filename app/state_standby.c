#include "main.h"

#include "state_standby.h"
#include "drivers/motor_driver.h"

#define BLINK_INTERVAL_MS (500U)

void state_standby_enter(struct StateStandbyCtx *ctx, State from, StateEvent event)
{
    motor_drive(0, DRIVE_STOP);

    UNUSED(event);
    uint32_t now = HAL_GetTick();

    if ((now - ctx->last_blink_ms) >= BLINK_INTERVAL_MS)
    {
        ctx->last_blink_ms = now;
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
    }
}
