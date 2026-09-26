#include "main.h"

#include "motor_driver.h"
#include "debug.h"
#include <tim.h>

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
    if (speed_left > 99 || speed_right > 99)
    {
        DEBUG_PRINTF(
            "Speed should be between 0 and 99, received speed left: %d, speed right: %d", speed_left, speed_right);
        Error_Handler();
    }

    // Clamp speed to valid value.
    speed_left = speed_left > 99 ? 99 : speed_left;
    speed_right = speed_right > 99 ? 99 : speed_right;

    // TODO: solve this properly, the driver needs to be HIGH for 100us after having slept,
    // which it does automatically when inactive for 0.9-2.6ms. Just set the speed of each driver
    // in a uint8_t, and check it when setting motor speed.
    if (speed_left > 0)
    {
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 99);
    }
    if (speed_right > 0)
    {
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, 99);
    }

    if (speed_left > 0 || speed_right > 0)
    {
        // TODO: use timer peripheral to create microsecond delay function.
        HAL_Delay(1);
    }

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, speed_left);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, speed_right);
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
