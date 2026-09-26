#include "motor_driver.h"

#include <stdbool.h>

#include "debug.h"
#include "main.h"
#include "tim.h"
#include "util.h"

// 100% duty cycle: CCR = 100 with TIM2 ARR = 99 (100 counts per period).
#define MOTOR_MAX_SPEED 100U

// Record previous speeds to know whether we need to wake the motor driver.
static uint8_t prev_speed_left = 0;
static uint8_t prev_speed_right = 0;

/**
 * @brief Direction of the motors.
 */
typedef enum
{
    FORWARD,    // Clockwise (CW)
    REVERSE,    // Counterclockwise (CCW)
    SPIN_LEFT,  // Left wheel CW, right wheel CCW
    SPIN_RIGHT, // Right wheel CW, left wheel CCW
} MotorDirection;

/**
 * @brief Set the direction of the motors, by configuring the GPIO output pins connected to the
 * driver.
 */
static void motor_driver_set_direction(MotorDirection direction)
{
    switch (direction)
    {
    case FORWARD:
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_SET);
        break;
    case SPIN_LEFT:
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_RESET);
        break;
    case SPIN_RIGHT:
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_SET);
        break;
    case REVERSE:
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_RESET);
        break;
    default:
        break;
    }
}

static void motor_driver_set_speed(uint8_t speed_left, uint8_t speed_right)
{
    if (speed_left > MOTOR_MAX_SPEED || speed_right > MOTOR_MAX_SPEED)
    {
        DEBUG_PRINTF("Motor speed out of range, clamping to %d. Left: %d, right: %d",
                     MOTOR_MAX_SPEED, speed_left, speed_right);
    }

    // Clamp speed to valid value.
    speed_left = speed_left > MOTOR_MAX_SPEED ? MOTOR_MAX_SPEED : speed_left;
    speed_right = speed_right > MOTOR_MAX_SPEED ? MOTOR_MAX_SPEED : speed_right;

    const bool should_wake_left = prev_speed_left == 0 && speed_left > 0;
    const bool should_wake_right = prev_speed_right == 0 && speed_right > 0;

    if (should_wake_left)
    {
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, MOTOR_MAX_SPEED);
    }
    if (should_wake_right)
    {
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, MOTOR_MAX_SPEED);
    }

    // The DRV8212 motor driver goes into sleep mode after the EN pin (PWM input) is low for
    // between 0.9 and 2.6ms. To wake it, the EN pin must be held high for at least 100us.
    // To achieve that, we set our duty cycle to 100%, then busy wait for the needed duration.
    // See DRV8212 datasheet section 7.5 for details.
    if (should_wake_left || should_wake_right)
    {
        delay_us(110);
    }

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, speed_left);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, speed_right);

    prev_speed_left = speed_left;
    prev_speed_right = speed_right;
}

void motor_drive(uint8_t speed, DriveDirection direction)
{
    switch (direction)
    {
    case DRIVE_FORWARD:
        motor_driver_set_direction(FORWARD);
        motor_driver_set_speed(speed, speed);
        break;
    case DRIVE_REVERSE:
        motor_driver_set_direction(REVERSE);
        motor_driver_set_speed(speed, speed);
        break;
    // To achieve the wide arc turns, we simply reduce the speed on one side.
    case DRIVE_FORWARD_ARC_LEFT:
        motor_driver_set_direction(FORWARD);
        // Set right speed to 75% of left to turn widely to the left.
        uint8_t speed_right = speed - (speed >> 2);
        motor_driver_set_speed(speed, speed_right);
        break;
    case DRIVE_FORWARD_ARC_RIGHT:
        motor_driver_set_direction(FORWARD);
        uint8_t speed_left = speed - (speed >> 2);
        motor_driver_set_speed(speed_left, speed);
        break;
    case DRIVE_REVERSE_ARC_LEFT:
        motor_driver_set_direction(REVERSE);
        uint8_t speed_rev_right = speed - (speed >> 2);
        motor_driver_set_speed(speed, speed_rev_right);
        break;
    case DRIVE_REVERSE_ARC_RIGHT:
        motor_driver_set_direction(REVERSE);
        uint8_t speed_rev_left = speed - (speed >> 2);
        motor_driver_set_speed(speed_rev_left, speed);
        break;
    case DRIVE_SPIN_LEFT:
        motor_driver_set_direction(SPIN_LEFT);
        motor_driver_set_speed(speed, speed);
        break;
    case DRIVE_SPIN_RIGHT:
        motor_driver_set_direction(SPIN_RIGHT);
        motor_driver_set_speed(speed, speed);
        break;
    case DRIVE_STOP:
        motor_driver_set_speed(0, 0);
    }
}

void motor_driver_init()
{
    MX_TIM2_Init();

    motor_driver_set_direction(FORWARD);
    motor_driver_set_speed(0, 0);

    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler();
    }
}
