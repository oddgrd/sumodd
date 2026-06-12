#pragma once
#include <stm32f3xx_hal_tim.h>
#include <stdint.h>

/**
 * @brief Direction to drive in.
 *
 */
typedef enum
{
    DRIVE_STOP,
    DRIVE_FORWARD,
    DRIVE_REVERSE,
    // Wide turn in given direction.
    DRIVE_FORWARD_ARC_LEFT,
    DRIVE_FORWARD_ARC_RIGHT,
    DRIVE_REVERSE_ARC_LEFT,
    DRIVE_REVERSE_ARC_RIGHT,
    // Turn in place.
    DRIVE_SPIN_LEFT,
    DRIVE_SPIN_RIGHT,
} DriveDirection;

void motor_drive(uint8_t speed, DriveDirection direction);

/**
 * @brief Initialize and start the motor driver.
 */
void motor_driver_init();
