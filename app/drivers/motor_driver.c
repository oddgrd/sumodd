#include "main.h"

#include "motor_driver.h"

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
     * f_PWM = 64MHz / (2 * 1600)
     * f_PWM = 20KHz
     */
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 1600 - 1; // 0..=1599
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
    case REVERSE:
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_1, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
        break;
    }
    // TODO: default for bad input
}

// TODO: refactor to work for both motor drivers, left is CH4 and right is CH3, both motors on
// each side will have the same speed, they use the same PWM channel, so the same CRR value to
// adjust duty cycle.
static void motor_driver_set_speed(MotorSpeed speed)
{
    switch (speed)
    {
    case OFF:
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 0);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, 0);

        break;
    case TURTLE:
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 400);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, 400);

        break;
    case HUMAN:
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 800);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, 800);

        break;
    case HARE:
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 1200);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, 1200);

        break;
    case JAGUAR:
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 1599);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, 1599);

        break;
    }
    // TODO: default for bad input
}

void motor_forward(MotorSpeed speed)
{
    motor_driver_set_direction(FORWARD);
    motor_driver_set_speed(speed);
}

void motor_reverse(MotorSpeed speed)
{
    motor_driver_set_direction(REVERSE);
    motor_driver_set_speed(speed);
}

void motor_stop(void)
{
    motor_driver_set_direction(STOP);
    motor_driver_set_speed(OFF);
}

// TODO: refactor, make part of init?
void motor_driver_start(void)
{
    motor_driver_set_direction(FORWARD);
    motor_driver_set_speed(STOP);

    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler();
    }
}

void motor_driver_init()
{
    MX_TIM2_Init();
}
