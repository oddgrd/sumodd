#pragma once
#include <stm32f3xx_hal_tim.h>
#include <stdint.h>

/**
 * @brief Speed of the motor.
 *
 */
typedef enum
{
    OFF,    // 0%
    TURTLE, // 25%
    HUMAN,  // 50%
    HARE,   // 75%
    JAGUAR  // 100%
} MotorSpeed;

/**
 * @brief Direction of the motor.
 *
 */
typedef enum
{
    STOP,
    FORWARD,   // Clockwise (CW)
    REVERSE,   // Counterclockwise (CCW)
    SPIN_LEFT, // Left wheels CW, right wheels CCW
} MotorDirection;

/**
 * @brief Initialize the motor driver.
 *
 * TODO
 */
void motor_driver_init();

/**
 * @brief Start the motor driver.
 *
 * TODO
 *
 * @param htim        Handle to input capture timer peripheral.
 * @param nec_buffer  Output buffer for the decoded frame.
 */
void motor_driver_start(void);

/**
 * @brief Set the motor to drive forward at the given speed.
 *
 * @param speed  Speed in levels
 */
void motor_forward(MotorSpeed speed);

/**
 * @brief Set the motor to reverse at the given speed.
 *
 * @param speed  Speed in levels
 */
void motor_reverse(MotorSpeed speed);

void motor_spin_left(MotorSpeed speed);
/**
 * @brief Stop all motors.
 */
void motor_stop(void);