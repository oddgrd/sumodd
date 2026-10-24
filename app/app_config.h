#pragma once
#include <stdint.h>

// IR Remote
#define IR_REMOTE_ADDR ((uint16_t)0xBF00)

// Battery
#define LOW_BATTERY_THRESHOLD_MV 6700U
// The duration the battery threshold needs to be crossed before taking action.
#define LOW_BATTERY_THRESHOLD_DURATION_MS 3000U

// Ranging
#define RANGING_MAX_DISTANCE_MM 400U
