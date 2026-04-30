#pragma once
#include <stm32f3xx_hal_tim.h>
#include <stdint.h>

/**
 * @brief Direction of the motor.
 *
 */
typedef enum
{
    STOP,
    FORWARD,    // Clockwise (CW)
    REVERSE,    // Counterclockwise (CCW)
    SPIN_LEFT,  // Left wheel CW, right wheel CCW
    SPIN_RIGHT, // Right wheel CW, left wheel CCW
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
void motor_forward(uint8_t speed);

/**
 * @brief Set the motor to reverse at the given speed.
 *
 * @param speed  Speed in levels
 */
void motor_reverse(uint8_t speed);

/**
 * @brief Set the motor to spin left at the given speed.
 *
 * @param speed  Speed in levels
 */
void motor_spin_left(uint8_t speed);

/**
 * @brief Set the motor to spin right at the given speed.
 *
 * @param speed  Speed in levels
 */
void motor_spin_right(uint8_t speed);

/**
 * @brief Stop all motors.
 */
void motor_stop(void);