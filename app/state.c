#include "main.h"

#include <stdint.h>
#include "ring_buffer.h"
#include "state.h"
#include "drivers/line_sensor.h"
#include "drivers/motor_driver.h"
#include "debug.h"

#define STATE_RETREAT_DURATION_MS (2000U)
#define BLINK_INTERVAL_MS (500U)
#define EVENT_BUFFER_SIZE (16U)
#define TIMER_RESET_VALUE (0U)

typedef struct
{
    State state;
    LineType last_line;
    uint32_t timer;
    MotorDirection retreat_direction;
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
        // TODO: spin in place is difficult with 4 wheels, implement scanning left and right in
        // place.
        // motor_forward(TURTLE);
        motor_spin_left(HARE);
        break;
    case STATE_ATTACK:
        // TODO: set direction to bearing we determine from range sensors, shared in statemachine state.
        motor_forward(TURTLE);
        break;
    case STATE_RETREAT:
        robot_state.timer = HAL_GetTick() + STATE_RETREAT_DURATION_MS;
        if (robot_state.retreat_direction == REVERSE)
        {
            motor_reverse(TURTLE);
        }
        else
        {
            motor_forward(TURTLE);
        }
        break;
    case STATE_STANDBY:
        robot_state.last_blink_ms = HAL_GetTick();
        motor_stop();
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
        break;
    case EVT_LINE_DETECTED:
        if (robot_state.state == STATE_SEARCH || robot_state.state == STATE_ATTACK)
        {
            if (event.line == LINE_FRONT)
            {
                robot_state.retreat_direction = REVERSE;
            }
            else if (event.line == LINE_BACK)
            {
                robot_state.retreat_direction = FORWARD;
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

    StateEvent next_event = {.type = EVT_NONE};

    if (cmd != IR_NONE)
    {
        next_event.type = EVT_IR_CMD;
        next_event.ir_cmd = cmd;
        return next_event;
    }

    // TODO: extract this logic into a helper function
    LineType previous_line = robot_state.last_line;
    robot_state.last_line = line;

    if (line != LINE_NONE && line != previous_line)
    {
        next_event.type = EVT_LINE_DETECTED;
        next_event.line = line;
        return next_event;
    }

    if (robot_state.timer != TIMER_RESET_VALUE && HAL_GetTick() >= robot_state.timer)
    {
        next_event.type = EVT_TIMEOUT;
        return next_event;
    }
    // else if enemy detected
    return next_event;
}

void state_machine_init(void)
{
    robot_state.timer = TIMER_RESET_VALUE;
    robot_state.last_blink_ms = 0;
    robot_state.retreat_direction = STOP;
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
    default:
        break;
    }
}