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

static struct RobotState
{
    State state;
    uint32_t retreat_start_ms;
    uint32_t last_blink_ms;

} robot_state;

static uint8_t event_buffer[EVENT_BUFFER_SIZE] = {0};
static RingBuffer event_queue = {0};

void state_event_push(Event event)
{
    // TODO: return something to caller?
    ring_buffer_push(&event_queue, event);
}

static void state_enter(State new_state)
{
    switch (new_state)
    {
    case SEARCH:
        // TODO: rotate? Poll i2c data ready?
        motor_driver_set_speed(HARE);
        motor_driver_set_direction(FORWARD);
        break;
    case ATTACK:
        motor_driver_set_speed(JAGUAR);
        // TODO: set direction to what we read from distances sensor, shared in statemachine state.
        motor_driver_set_direction(FORWARD);
        break;
    case RETREAT:
        robot_state.retreat_start_ms = HAL_GetTick();
        motor_driver_set_speed(TURTLE);
        // TODO: start a timer before reversing, using a timer peripheral, and interrupt when it expires?
        motor_driver_set_direction(REVERSE);
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
static void process_event(Event event)
{
    switch (event)
    {
    case IR_START:
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, RESET);
        robot_state.state = SEARCH;
        state_enter(SEARCH);
        break;
    case IR_STOP:
        robot_state.state = STANDBY;
        state_enter(STANDBY);
        break;
    case OPPONENT_DETECTED:
        robot_state.state = ATTACK;
        state_enter(ATTACK);
        break;
    case OPPONENT_LOST:
        robot_state.state = SEARCH;
        state_enter(SEARCH);
        break;
    // TODO: different action depending on which line sensor triggered.
    // TODO: what do we do if this event already triggered, and we get a new event? We need more
    // state about the trigger state of all sensors.
    case FRONT_LEFT_EDGE_DETECTED:
        if (robot_state.state != RETREAT)
        {
            robot_state.state = RETREAT;
            state_enter(RETREAT);
        }
        break;
    // TODO: different action depending on which line sensor triggered.
    case FRONT_RIGHT_EDGE_DETECTED:
        if (robot_state.state != RETREAT)
        {
            robot_state.state = RETREAT;
            state_enter(RETREAT);
        }
        break;
    case RETREAT_DONE:
        robot_state.state = SEARCH;
        state_enter(SEARCH);
        break;
    default:
        break;
    }
}

void state_machine_init(void)
{
    ring_buffer_init(&event_queue, event_buffer, sizeof(event_buffer));
    robot_state.retreat_start_ms = 0;
    robot_state.last_blink_ms = 0;
    robot_state.state = STANDBY;
}

void state_machine_run(void)
{

    Event next_event = IR_STOP;
    while (ring_buffer_pop(&event_queue, &next_event))
    {
        process_event(next_event);
    }

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
        if ((HAL_GetTick() - robot_state.retreat_start_ms) > RETREAT_DURATION_MS)
        {
            state_event_push(RETREAT_DONE);
        }
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