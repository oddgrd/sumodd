#include "main.h"

#include <stdint.h>
#include "ring_buffer.h"
#include "state.h"
#include "drivers/line_sensor.h"
#include "drivers/motor_driver.h"
#include "ranging.h"
#include "debug.h"
#include "state_common.h"
#include "state_retreat.h"
#include "state_standby.h"
#include "state_search.h"
#include "state_attack.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define TIMER_RESET_VALUE (0U)
// Range of distance change in millimeters required to trigger an enemy event.
#define RANGING_DEADBAND_MM (10U)

static struct RobotContext
{
    State state;
    uint32_t timer;
    struct StateAttackCtx state_attack;
    struct StateCommonCtx state_common;
    struct StateRetreatCtx state_retreat;
    struct StateSearchCtx state_search;
    struct StateStandbyCtx state_standby;
} ctx;

struct StateTransition
{
    State from;
    StateEvent event;
    State to;
};

// See docs/state.png for state machine transitions.
static const struct StateTransition state_transitions[] = {
    {STATE_STANDBY, EVT_ENEMY, STATE_STANDBY},
    {STATE_STANDBY, EVT_LINE, STATE_STANDBY},
    {STATE_STANDBY, EVT_NONE, STATE_STANDBY},
    {STATE_STANDBY, EVT_IR_CMD, STATE_SEARCH},
    {STATE_SEARCH, EVT_IR_CMD, STATE_STANDBY},
    {STATE_SEARCH, EVT_ENEMY, STATE_ATTACK},
    {STATE_SEARCH, EVT_LINE, STATE_RETREAT},
    {STATE_SEARCH, EVT_NONE, STATE_SEARCH},
    {STATE_ATTACK, EVT_ENEMY, STATE_ATTACK},
    {STATE_ATTACK, EVT_NONE, STATE_SEARCH},
    {STATE_ATTACK, EVT_LINE, STATE_RETREAT},
    {STATE_ATTACK, EVT_IR_CMD, STATE_STANDBY},
    {STATE_RETREAT, EVT_TIMEOUT, STATE_SEARCH},
    {STATE_RETREAT, EVT_LINE, STATE_RETREAT},
    {STATE_RETREAT, EVT_IR_CMD, STATE_STANDBY},
    {STATE_RETREAT, EVT_ENEMY, STATE_RETREAT},
    {STATE_RETREAT, EVT_NONE, STATE_RETREAT},
};

static const char *state_to_str(State state)
{
    switch (state)
    {
    case STATE_STANDBY:
        return "STANDBY";
    case STATE_SEARCH:
        return "SEARCH";
    case STATE_ATTACK:
        return "ATTACK";
    case STATE_RETREAT:
        return "RETREAT";
    }
    return "";
}

static const char *state_event_to_str(StateEvent event)
{
    switch (event)
    {
    case EVT_ENEMY:
        return "ENEMY";
    case EVT_IR_CMD:
        return "IR CMD";
    case EVT_LINE:
        return "LINE DETECTED";
    case EVT_TIMEOUT:
        return "TIMEOUT";
    case EVT_NONE:
        return "NONE";
    }
    return "";
}

static void state_enter(State from, StateEvent event, State to)
{

    if (from != to)
    {
        ctx.timer = TIMER_RESET_VALUE;
        ctx.state = to;
        DEBUG_PRINTF("%s to %s (%s)\n", state_to_str(from), state_to_str(to), state_event_to_str(event));
    }
    switch (to)
    {
    case STATE_STANDBY:
        state_standby_enter(&ctx.state_standby, from, event);
        break;
    case STATE_SEARCH:
        state_search_enter(&ctx.state_search, from, event);
        break;
    case STATE_ATTACK:
        state_attack_enter(&ctx.state_attack, from, event);
        break;
    case STATE_RETREAT:
        state_retreat_enter(&ctx.state_retreat, from, event);
        break;
    }
}

/**
 * @brief Iterate through possible state transitions, entering a state on the first match.
 *
 * Enter a state when we match both the previous state and the current event in the transitions
 * table.
 */
static void process_event(StateEvent event)
{
    for (uint16_t i = 0; i < ARRAY_SIZE(state_transitions); i++)
    {
        if (ctx.state == state_transitions[i].from && event == state_transitions[i].event)
        {
            state_enter(state_transitions[i].from, event, state_transitions[i].to);
            return;
        }
    }
    // TODO: failure handling in case of invalid state.
    DEBUG_PRINTF("Unable to process event using state transitions table!\n");
    DEBUG_PRINTF("State: %s, event: %s!\n", state_to_str(ctx.state), state_event_to_str(event));
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

    return abs(distance_delta) > RANGING_DEADBAND_MM;
}

/**
 * @brief Process and record inputs from all sensors, check the state of the timer, and return an
 * event.
 */
static StateEvent process_input(void)
{
    IrCommand cmd = ir_remote_get_cmd();
    LineType line = get_line();
    // TODO: if data is ready this will do I2C reads which can take a few ms, we should consider
    // only doing it in relevant states.
    Enemy enemy = ranging_get_enemy();

    ctx.state_common.line = line;
    ctx.state_common.enemy = enemy;
    ctx.state_common.cmd = cmd;

    if (ctx.state == STATE_SEARCH || ctx.state == STATE_ATTACK)
    {
        // DEBUG_PRINTF("Enemy bearing: %d, distance: %dmm, state: %d\n", enemy.bearing, enemy.distance_mm, ctx.state);
    }

    if (cmd != IR_NONE)
    {
        return EVT_IR_CMD;
    }

    if (line != LINE_NONE)
    {
        return EVT_LINE;
    }

    // TODO: consider when we want to reset timer
    if (*ctx.state_common.timer != TIMER_RESET_VALUE && HAL_GetTick() >= *ctx.state_common.timer)
    {
        return EVT_TIMEOUT;
    }

    if (enemy.bearing != BEARING_NONE && enemy_changed(enemy, ctx.state_common.enemy))
    {
        // DEBUG_PRINTF("Enemy bearing: %d, distance: %dmm, state: %d\n", enemy.bearing, enemy.distance_mm, ctx.state);
        return EVT_ENEMY;
    }

    return EVT_NONE;
}

void state_machine_init(void)
{
    ctx.state = STATE_STANDBY;
    ctx.timer = TIMER_RESET_VALUE;

    ctx.state_common.timer = &ctx.timer;
    ctx.state_common.cmd = IR_NONE;
    Enemy init_enemy = {.bearing = BEARING_NONE, .distance_mm = 0};
    ctx.state_common.enemy = init_enemy;
    ctx.state_common.line = LINE_NONE;

    ctx.state_retreat.common = &ctx.state_common;
    ctx.state_retreat.state = RETREAT_STATE_REVERSE;

    ctx.state_standby.state_common = &ctx.state_common;
    ctx.state_standby.last_blink_ms = 0;

    ctx.state_search.state_common = &ctx.state_common;

    ctx.state_attack.state_common = &ctx.state_common;
}

void state_machine_run(void)
{
    StateEvent next_event = process_input();

    // if (next_event != EVT_NONE)
    // {
    //     DEBUG_PRINTF("StateEvent: %s\r\n", state_event_to_str(next_event));
    // }

    process_event(next_event);
}