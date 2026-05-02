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

// TODO: refactor to work with two motor drivers, where each control both motors on one side,
// skid steering. Left is GPIO F0 and F1, right is A0 and A1.
void motor_driver_set_direction(MotorDirection direction)
{
    switch (direction)
    {
    case STOP:
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_1, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
        break;
    case FORWARD:
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_1, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
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
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
        break;
    }
    // TODO: default for bad input
}

// TODO: assert that number is within 0-99 range, and/or use a newtype if possible.
// TODO: make each motor's speed individually configurable.
static void motor_driver_set_speed(uint8_t speed)
{
    if (speed > 99 || speed < 0)
    {
        DEBUG_PRINTF("Speed should be between 0 and 99, received: %d", speed);
        return;
    }
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, speed);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, speed);
}

void motor_forward(uint8_t speed)
{
    motor_driver_set_direction(FORWARD);
    motor_driver_set_speed(speed);
}

void motor_reverse(uint8_t speed)
{
    motor_driver_set_direction(REVERSE);
    motor_driver_set_speed(speed);
}

void motor_spin_left(uint8_t speed)
{
    motor_driver_set_direction(SPIN_LEFT);
    motor_driver_set_speed(speed);
}

void motor_spin_right(uint8_t speed)
{
    motor_driver_set_direction(SPIN_RIGHT);
    motor_driver_set_speed(speed);
}

void motor_stop(void)
{
    motor_driver_set_direction(STOP);
    motor_driver_set_speed(0);
}

void motor_driver_init()
{
    MX_TIM2_Init();

    motor_driver_set_direction(FORWARD);
    motor_driver_set_speed(0);

    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler();
    }
}
