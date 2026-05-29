#include "main.h"

#include <stdint.h>
#include "ring_buffer.h"
#include "state.h"
#include "drivers/line_sensor.h"
#include "drivers/motor_driver.h"
#include "ranging.h"
#include "debug.h"

#define STATE_RETREAT_DURATION_MS (1000U)
#define BLINK_INTERVAL_MS (500U)
#define TIMER_RESET_VALUE (0U)
// Range of distance change in millimeters required to trigger an enemy event.
#define RANGING_DEADBAND_MM (10U)

typedef struct
{
    State state;
    LineType last_line;
    Enemy last_enemy;
    uint32_t timer;
    DriveDirection retreat_direction;
    uint32_t last_blink_ms;
} RobotContext;

static RobotContext ctx;

const char *state_event_type_str(StateEventType type)
{
    switch (type)
    {
    case EVT_ENEMY:
        return "ENEMY";
    case EVT_IR_CMD:
        return "IR CMD";
    case EVT_LINE_DETECTED:
        return "LINE DETECTED";
    case EVT_TIMEOUT:
        return "TIMEOUT";
    case EVT_NONE:
        return "NONE";
    default:
        return "UNKOWN";
    }
};

static void state_enter(State new_state)
{
    ctx.state = new_state;

    switch (new_state)
    {
    case STATE_SEARCH:
        // TODO: spin left and then right on a short timer.
        motor_drive(0, DRIVE_STOP);
        break;
    case STATE_ATTACK:
        if (ctx.last_enemy.bearing == BEARING_FRONT)
        {
            if (ctx.last_enemy.distance_mm > 75)
            {
                motor_drive(35, DRIVE_FORWARD);
            }
            else
            {
                motor_drive(0, DRIVE_STOP);
            }
        }
        else if (ctx.last_enemy.bearing == BEARING_LEFT)
        {
            if (ctx.last_enemy.distance_mm > 100)
            {
                motor_drive(25, DRIVE_ARC_LEFT);
            }
            else
            {
                motor_drive(25, DRIVE_SPIN_LEFT);
            }
        }
        else if (ctx.last_enemy.bearing == BEARING_RIGHT)
        {
            if (ctx.last_enemy.distance_mm > 100)
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
        break;
    case STATE_RETREAT:
        ctx.timer = HAL_GetTick() + STATE_RETREAT_DURATION_MS;
        if (ctx.retreat_direction == DRIVE_REVERSE)
        {
            motor_drive(25, DRIVE_REVERSE);
        }
        else
        {
            motor_drive(25, DRIVE_FORWARD);
        }
        break;
    case STATE_STANDBY:
        ctx.last_blink_ms = HAL_GetTick();
        motor_drive(0, DRIVE_STOP);
        break;
    }
}

static void process_event(StateEvent event)
{
    switch (event.type)
    {
    case EVT_IR_CMD:
        if (event.ir_cmd == IR_START && ctx.state == STATE_STANDBY)
        {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, RESET);
            state_enter(STATE_SEARCH);
        }
        else
        {
            // TODO: refine IR commands handling.
            state_enter(STATE_STANDBY);
        }
        break;
    case EVT_ENEMY:
        if (event.enemy.bearing == BEARING_NONE)
        {
            state_enter(STATE_SEARCH);
        }
        else
        {
            state_enter(STATE_ATTACK);
        }
        break;
    case EVT_LINE_DETECTED:
        if (ctx.state == STATE_SEARCH || ctx.state == STATE_ATTACK)
        {
            DriveDirection new_retreat_direction = DRIVE_REVERSE;
            if (event.line == LINE_FRONT)
            {
                new_retreat_direction = DRIVE_REVERSE;
            }
            else if (event.line == LINE_BACK)
            {
                new_retreat_direction = DRIVE_FORWARD;
            }

            if (new_retreat_direction != ctx.retreat_direction)
            {
                ctx.retreat_direction = new_retreat_direction;
                state_enter(STATE_RETREAT);
            }
        }
        break;
    case EVT_TIMEOUT:
        if (ctx.state == STATE_RETREAT)
        {
            ctx.timer = TIMER_RESET_VALUE;
            state_enter(STATE_SEARCH);
        }
        break;
    case EVT_NONE:
        break;
    default:
        break;
    }
}

/**
 * @brief Whether the enemy bearing changed, or distance significantly changed.
 */
static bool enemy_changed(Enemy a, Enemy b)
{
    if (a.bearing != b.bearing)
    {
        return true;
    }

    int32_t distance_delta = (int32_t)a.distance_mm - (int32_t)b.distance_mm;

    return distance_delta > RANGING_DEADBAND_MM || distance_delta < -RANGING_DEADBAND_MM;
}

static StateEvent process_input(void)
{
    IrCommand cmd = ir_remote_get_cmd();
    LineType line = get_line();
    Enemy enemy = ranging_get_enemy();

    if (ctx.state == STATE_SEARCH || ctx.state == STATE_ATTACK)
    {
        DEBUG_PRINTF("Enemy bearing: %d, distance: %dmm, state: %d\n", enemy.bearing, enemy.distance_mm, ctx.state);
    }

    StateEvent next_event = {.type = EVT_NONE};

    if (cmd != IR_NONE)
    {
        next_event.type = EVT_IR_CMD;
        next_event.ir_cmd = cmd;
        return next_event;
    }

    // TODO: extract this logic into a helper function
    // Only emit a line event if it is not NONE, and it is not the same as the previous line
    // detected. We still set the line in the state here, so that if it goes from e.g. LINE_FRONT
    // to LINE_NONE, it is recorded, even though the conditional below excludes LINE_NONE.
    LineType previous_line = ctx.last_line;
    ctx.last_line = line;

    // TODO: this makes it so that if the next line is the same, but happens after a retreat, it
    // still does not trigger a new retreat.
    if (line != LINE_NONE && line != previous_line)
    {
        next_event.type = EVT_LINE_DETECTED;
        next_event.line = line;
        return next_event;
    }

    if (enemy_changed(enemy, ctx.last_enemy))
    {
        next_event.type = EVT_ENEMY;
        next_event.enemy = enemy;
        ctx.last_enemy = enemy;
        return next_event;
    }

    // TODO: consider when we want to reset timer
    if (ctx.timer != TIMER_RESET_VALUE && HAL_GetTick() >= ctx.timer)
    {
        next_event.type = EVT_TIMEOUT;
        return next_event;
    }
    return next_event;
}

void state_machine_init(void)
{
    ctx.timer = TIMER_RESET_VALUE;
    ctx.last_blink_ms = 0;
    ctx.retreat_direction = DRIVE_STOP;
    ctx.state = STATE_STANDBY;
    ctx.last_line = LINE_NONE;
}

void state_machine_run(void)
{
    StateEvent next_event = process_input();

    if (next_event.type != EVT_NONE)
    {
        DEBUG_PRINTF("StateEvent type: %s\r\n", state_event_type_str(next_event.type));
    }

    process_event(next_event);

    switch (ctx.state)
    {
    case STATE_STANDBY:
        if ((HAL_GetTick() - ctx.last_blink_ms) >= BLINK_INTERVAL_MS)
        {
            ctx.last_blink_ms = HAL_GetTick();
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
        }
        break;
    case STATE_SEARCH:
    case STATE_ATTACK:
        ranging_update();
        break;
    default:
        break;
    }
}