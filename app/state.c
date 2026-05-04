#include "main.h"

#include <stdint.h>
#include "ring_buffer.h"
#include "state.h"
#include "drivers/line_sensor.h"
#include "drivers/motor_driver.h"
#include "ranging.h"
#include "debug.h"

#define STATE_RETREAT_DURATION_MS (2000U)
#define BLINK_INTERVAL_MS (500U)
#define EVENT_BUFFER_SIZE (16U)
#define TIMER_RESET_VALUE (0U)

typedef struct
{
    State state;
    LineType last_line;
    Enemy last_enemy;
    uint32_t timer;
    DriveDirection retreat_direction;
    uint32_t last_blink_ms;
} RobotState;

static RobotState robot_state;

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
    robot_state.state = new_state;

    switch (new_state)
    {
    case STATE_SEARCH:
        // TODO: spin left and then right on a short timer.
        motor_drive(0, DRIVE_STOP);
        break;
    case STATE_ATTACK:
        if (robot_state.last_enemy.bearing == BEARING_FRONT || robot_state.last_enemy.bearing == BEARING_NONE)
        {
            motor_drive(0, DRIVE_STOP);
        }
        else if (robot_state.last_enemy.bearing == BEARING_LEFT)
        {
            motor_drive(25, DRIVE_SPIN_LEFT);
        }
        else if (robot_state.last_enemy.bearing == BEARING_RIGHT)
        {
            motor_drive(25, DRIVE_SPIN_RIGHT);
        }
        break;
    case STATE_RETREAT:
        robot_state.timer = HAL_GetTick() + STATE_RETREAT_DURATION_MS;
        if (robot_state.retreat_direction == DRIVE_REVERSE)
        {
            motor_drive(25, DRIVE_REVERSE);
        }
        else
        {
            motor_drive(25, DRIVE_FORWARD);
        }
        break;
    case STATE_STANDBY:
        robot_state.last_blink_ms = HAL_GetTick();
        motor_drive(0, DRIVE_STOP);
        break;
    }
}

static void process_event(StateEvent event)
{
    switch (event.type)
    {
    case EVT_IR_CMD:
        if (event.ir_cmd == IR_START && robot_state.state == STATE_STANDBY)
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
        if (robot_state.state == STATE_SEARCH || robot_state.state == STATE_ATTACK)
        {
            if (event.line == LINE_FRONT)
            {
                robot_state.retreat_direction = DRIVE_REVERSE;
            }
            else if (event.line == LINE_BACK)
            {
                robot_state.retreat_direction = DRIVE_FORWARD;
            }
            state_enter(STATE_RETREAT);
        }
        break;
    case EVT_TIMEOUT:
        if (robot_state.state == STATE_RETREAT)
        {
            robot_state.timer = TIMER_RESET_VALUE;
            state_enter(STATE_SEARCH);
        }
        break;
    case EVT_NONE:
        break;
    default:
        break;
    }
}

static StateEvent process_input(void)
{
    IrCommand cmd = ir_remote_get_cmd();
    LineType line = get_line();
    Enemy enemy = ranging_get_enemy();

    if (robot_state.state == STATE_SEARCH || robot_state.state == STATE_ATTACK)
    {
        DEBUG_PRINTF("Enemy bearing: %d, distance: %dmm, state: %d\n", enemy.bearing, enemy.distance_mm, robot_state.state);
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
    LineType previous_line = robot_state.last_line;
    robot_state.last_line = line;

    if (line != LINE_NONE && line != previous_line)
    {
        next_event.type = EVT_LINE_DETECTED;
        next_event.line = line;
        return next_event;
    }

    if (enemy.bearing != robot_state.last_enemy.bearing)
    {
        next_event.type = EVT_ENEMY;
        next_event.enemy = enemy;
        robot_state.last_enemy = enemy;
        return next_event;
    }

    // TODO: consider when we want to reset timer
    if (robot_state.timer != TIMER_RESET_VALUE && HAL_GetTick() >= robot_state.timer)
    {
        next_event.type = EVT_TIMEOUT;
        return next_event;
    }
    return next_event;
}

void state_machine_init(void)
{
    robot_state.timer = TIMER_RESET_VALUE;
    robot_state.last_blink_ms = 0;
    robot_state.retreat_direction = DRIVE_STOP;
    robot_state.state = STATE_STANDBY;
    robot_state.last_line = LINE_NONE;
}

void state_machine_run(void)
{
    StateEvent next_event = process_input();

    if (next_event.type != EVT_NONE)
    {
        DEBUG_PRINTF("StateEvent type: %s\r\n", state_event_type_str(next_event.type));
    }

    process_event(next_event);

    switch (robot_state.state)
    {
    case STATE_STANDBY:
        if ((HAL_GetTick() - robot_state.last_blink_ms) >= BLINK_INTERVAL_MS)
        {
            robot_state.last_blink_ms = HAL_GetTick();
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