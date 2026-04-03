#include "main.h"

#include <stdint.h>
#include "ring_buffer.h"
#include "state.h"
#include "drivers/line_sensor.h"
#include "drivers/motor_driver.h"

#define RETREAT_DURATION_MS (2000U)
#define BLINK_INTERVAL_MS (500U)
#define EVENT_BUFFER_SIZE (16U)

_Static_assert((EVENT_BUFFER_SIZE & (EVENT_BUFFER_SIZE - 1)) == 0, "EVENT_BUFFER_SIZE must be a power of two");

// TODO: how much of this do we still need after refactoring to event structs?
static struct RobotState
{
    State state;
    uint32_t retreat_start_ms;
    bool retreat_direction;
    uint32_t last_blink_ms;

} robot_state;

static state_event_t event_buffer[EVENT_BUFFER_SIZE] = {0};
static RingBuffer event_queue = {0};

void state_event_push(state_event_t *event)
{
    // TODO: return something to caller?
    ring_buffer_push(&event_queue, event);
}

// TODO: refactor now that we use state event struct.
static void state_enter(State new_state)
{
    switch (new_state)
    {
    case SEARCH:
        // TODO: rotate? Poll i2c data ready?
        motor_driver_set_speed(TURTLE);
        motor_driver_set_direction(FORWARD);
        break;
    case ATTACK:
        motor_driver_set_speed(JAGUAR);
        // TODO: set direction to what we read from distances sensor, shared in statemachine state.
        motor_driver_set_direction(FORWARD);
        break;
    case RETREAT:
        // robot_state.retreat_start_ms = HAL_GetTick();
        motor_driver_set_speed(TURTLE);
        // TODO: start a timer before reversing, using a timer peripheral, and interrupt when it expires?
        if (robot_state.retreat_direction == false)
        {
            motor_driver_set_direction(REVERSE);
        }
        else
        {
            motor_driver_set_direction(FORWARD);
        }
        // TODO: turn when timer expires, then return to SEARCH state.
        break;
    case STANDBY:
        robot_state.last_blink_ms = HAL_GetTick();
        motor_driver_set_speed(OFF);
        motor_driver_set_direction(STOP);
        break;
    }
}

// TODO: pass pointer to robot_state, rather than using global.
static void process_event(state_event_t event)
{
    switch (event.type)
    {
    case EVT_IR_CMD:
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, RESET);
        if (event.ir_cmd == IR_START)
        {
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
    // TODO: different action depending on which line sensor triggered.
    // TODO: what do we do if this event already triggered, and we get a new event? In general,
    // how do we handle transition of one state to another?
    case EVT_LINE_DETECTED:
        // TODO: check line type, and base retreat direction on that.
        robot_state.state = RETREAT;
        state_enter(RETREAT);
        break;
    default:
        break;
    }
}

void state_machine_init(void)
{
    ring_buffer_init(&event_queue, (uint8_t *)event_buffer, EVENT_BUFFER_SIZE, sizeof(state_event_t));
    robot_state.retreat_start_ms = 0;
    robot_state.last_blink_ms = 0;
    robot_state.retreat_direction = false;
    robot_state.state = STANDBY;
}

void state_machine_run(void)
{
    state_event_t next_event = {.type = EVT_IR_CMD, .ir_cmd = IR_STOP};
    while (ring_buffer_pop(&event_queue, &next_event))
    {
        process_event(next_event);
    }

    // TODO: consider what to do here now that we have state events as structs.
    switch (robot_state.state)
    {
    case SEARCH:
        // poll distance sensor data ready, update state machine context when we do.
        break;
    case ATTACK:
        // poll distance sensor
        // steer towards opponent
        break;
    case RETREAT:
        // check retreat timer, push RETREAT_DONE when it times out.
        // if ((HAL_GetTick() - robot_state.retreat_start_ms) > RETREAT_DURATION_MS)
        // {
        //     state_event_push(RETREAT_DONE);
        // }
        break;
    case STANDBY:
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