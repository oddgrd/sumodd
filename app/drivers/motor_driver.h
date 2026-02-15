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
    FORWARD, // Clockwise (CW)
    REVERSE  // Counterclockwise (CCW)
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
 * @brief Set the speed of the motor by adjusting the PWM duty cycle.
 *
 * TODO
 *
 * @param speed  Speed in levels
 */
void motor_driver_set_speed(MotorSpeed speed);
/**
 * @brief Control whether the motor should drive forward or in reverse.
 *
 * TODO
 *
 * @param direction  Direction of the motor.
 */
void motor_driver_set_direction(MotorDirection direction);