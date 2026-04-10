#include "main.h"

#include <stdint.h>
#include "ring_buffer.h"
#include "state.h"
#include "drivers/line_sensor.h"
#include "drivers/motor_driver.h"

#define RETREAT_DURATION_MS (2000U)
#define BLINK_INTERVAL_MS (500U)
#define EVENT_BUFFER_SIZE (16U)
#define TIMER_RESET_VALUE (0U)

_Static_assert((EVENT_BUFFER_SIZE & (EVENT_BUFFER_SIZE - 1)) == 0, "EVENT_BUFFER_SIZE must be a power of two");

typedef struct
{
    State state;
    uint32_t timer;
    MotorDirection retreat_direction;
    uint32_t last_blink_ms;
} RobotState;

static RobotState robot_state;

static StateEvent event_buffer[EVENT_BUFFER_SIZE] = {0};
static RingBuffer event_queue = {0};

void state_event_push(StateEvent *event)
{
    ring_buffer_push(&event_queue, event);
}

static void state_enter(State new_state)
{
    switch (new_state)
    {
    case SEARCH:
        // TODO: turn in place until target is acquired?
        motor_driver_set_speed(TURTLE);
        motor_driver_set_direction(FORWARD);
        break;
    case ATTACK:
        motor_driver_set_speed(JAGUAR);
        // TODO: set direction to bearing we determine from range sensors, shared in statemachine state.
        motor_driver_set_direction(FORWARD);
        break;
    case RETREAT:
        robot_state.timer = HAL_GetTick() + RETREAT_DURATION_MS;
        motor_driver_set_speed(TURTLE);
        if (robot_state.retreat_direction == REVERSE)
        {
            motor_driver_set_direction(REVERSE);
        }
        else
        {
            motor_driver_set_direction(FORWARD);
        }
        break;
    case STANDBY:
        robot_state.last_blink_ms = HAL_GetTick();
        motor_driver_set_speed(OFF);
        motor_driver_set_direction(STOP);
        break;
    }
}

static void process_event(StateEvent event)
{
    switch (event.type)
    {
    case EVT_IR_CMD:
        if (event.ir_cmd == IR_START && robot_state.state == STANDBY)
        {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, RESET);
            robot_state.state = SEARCH;
            state_enter(SEARCH);
        }
        else
        {
            // TODO: refine IR commands handling.
            robot_state.state = STANDBY;
            state_enter(STANDBY);
        }
        break;
    case EVT_ENEMY:
        break;
    case EVT_LINE_DETECTED:
        if (robot_state.state == SEARCH || robot_state.state == ATTACK)
        {
            robot_state.state = RETREAT;
            if (event.line == LINE_FRONT)
            {
                robot_state.retreat_direction = REVERSE;
            }
            else if (event.line == LINE_BACK)
            {
                robot_state.retreat_direction = FORWARD;
            }
            state_enter(RETREAT);
        }
        break;
    case EVT_TIMEOUT:
        if (robot_state.state == RETREAT)
        {
            robot_state.state = SEARCH;
            state_enter(SEARCH);
        }
        break;
    default:
        break;
    }
}

void state_machine_init(void)
{
    ring_buffer_init(&event_queue, (uint8_t *)event_buffer, EVENT_BUFFER_SIZE, sizeof(StateEvent));
    robot_state.timer = TIMER_RESET_VALUE;
    robot_state.last_blink_ms = 0;
    robot_state.retreat_direction = REVERSE;
    robot_state.state = STANDBY;
}

void state_machine_run(void)
{
    StateEvent next_event = {.type = EVT_IR_CMD, .ir_cmd = IR_STOP};
    while (ring_buffer_pop(&event_queue, &next_event))
    {

        process_event(next_event);
    }

    switch (robot_state.state)
    {
    case STANDBY:
        if ((HAL_GetTick() - robot_state.last_blink_ms) >= BLINK_INTERVAL_MS)
        {
            robot_state.last_blink_ms = HAL_GetTick();
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
        }
        break;
    case RETREAT:
        if (robot_state.timer != TIMER_RESET_VALUE && HAL_GetTick() >= robot_state.timer)
        {
            StateEvent timeout_event = {.type = EVT_TIMEOUT};
            state_event_push(&timeout_event);

            robot_state.timer = TIMER_RESET_VALUE;
        }
        break;
    default:
        break;
    }
}