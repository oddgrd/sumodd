#include "main.h"

#include "motor_driver.h"
#include "debug.h"

// Handle to the TIM2 peripheral used for motor controller PWM.
TIM_HandleTypeDef htim2;

/**
 * @brief TIM2 Initialization Function
 */
static void MX_TIM2_Init(void)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    /**
     * With the system clock running at 64MHz, we configure the period and prescaler to arrive at
     * 20KHz frequency:
     * f_PWM = f_TIM / ((PSC + 1)(ARR + 1))
     * f_PWM = 64MHz / (32 * 100)
     * f_PWM = 20KHz
     */
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 31;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 100 - 1; // 0..=99
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0; // Initial CCR value.
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim2);
}

/**
 * @brief Direction of the motors.
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
 * @brief Set the direction of the motors, by configuring the GPIO output pins connected to the
 * driver.
 */
static void motor_driver_set_direction(MotorDirection direction)
{
    switch (direction)
    {
    case FORWARD:
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_1, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
        break;
    case SPIN_LEFT:
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_1, GPIO_PIN_SET); // CCW
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET); // CW
        break;
    case SPIN_RIGHT:
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_1, GPIO_PIN_RESET); // CW
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); // CCW
        break;
    case REVERSE:
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_1, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
        break;
    case STOP:
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_1, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
        break;
    default:
        break;
    }
}

// TODO: assert that number is within 0-99 range, and/or use a newtype if possible.
// TODO: make each motor's speed individually configurable.
static void motor_driver_set_speed(uint8_t speed_left, uint8_t speed_right)
{
    if (speed_left > 99 || speed_right > 99)
    {
        DEBUG_PRINTF(
            "Speed should be between 0 and 99, received speed left: %d, speed right: %d", speed_left, speed_right);
        Error_Handler();
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
    // To achieve the wide arc turns, we simply halve the speed on one side.
    case DRIVE_ARC_LEFT:
        motor_driver_set_direction(FORWARD);
        uint8_t speed_right = speed >> 1;
        motor_driver_set_speed(speed, speed_right);
        break;
    case DRIVE_ARC_RIGHT:
        motor_driver_set_direction(FORWARD);
        uint8_t speed_left = speed >> 1;
        motor_driver_set_speed(speed_left, speed);
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
        motor_driver_set_direction(STOP);
        motor_driver_set_speed(0, 0);
    default:
        break;
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
