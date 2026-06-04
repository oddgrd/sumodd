#pragma once

#include "main.h"
#include "drivers/vl53l0x/vl53l0x_platform.h"

#include "drivers/vl53l0x/vl53l0x_api.h"
#include <stdbool.h>

extern I2C_HandleTypeDef hi2c1;

typedef enum
{
    RANGING_LEFT = 0,
    // RANGING_MIDDLE,
    RANGING_RIGHT,
    RANGING_COUNT
} RangingSensor;

typedef struct
{
    VL53L0X_Dev_t dev;
    GPIO_TypeDef *xshut_port;
    uint16_t xshut_pin;
    int16_t range_mm;
    uint8_t range_status;
    volatile bool data_ready;
} RangingInstance;

typedef struct
{
    RangingInstance sensor[RANGING_COUNT];
} RangingState;
extern RangingState ranging_state;

// TODO: bearings where the opponent is detected on both sensors, but at significantly different
// ranges, indicating the opponent is at an angle, and we may want to adjust our approach.
typedef enum
{
    BEARING_NONE,
    BEARING_FRONT,
    BEARING_LEFT,
    BEARING_RIGHT
} EnemyBearing;

typedef struct
{
    EnemyBearing bearing;
    uint16_t distance_mm;
} Enemy;

// TODO: document this function.
// TODO: pass pointer to statemachine ranging state? Or just keep bearing in statemachine, leave
// the details in here, behind api?
VL53L0X_Error ranging_init(void);

Enemy ranging_get_enemy(void);