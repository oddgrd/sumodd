#include "main.h"

#include "tb6612fng.h"

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
     * With the system clock running at 32MHz, we configure the period and prescaler to arrive at
     * 20KHz frequency:
     * f_PWM = f_TIM / ((PSC + 1)(ARR + 1))
     * f_PWM = 32MHz / (1 * 1600)
     * f_PWM = 20KHz
     */
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
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
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim2);
}

void motor_driver_set_direction(tb6612fng_direction_e direction)
{
    switch (direction)
    {
    case STOP:
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_1, GPIO_PIN_RESET);
        break;
    case FORWARD:
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_1, GPIO_PIN_RESET);
        break;
    case REVERSE:
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_1, GPIO_PIN_SET);
        break;
    }
    // TODO: default for bad input
}

void motor_driver_set_speed(tb6612fng_speed_e speed)
{
    switch (speed)
    {
    case OFF:
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
        break;
    case TURTLE:
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 400);
        break;
    case HUMAN:
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 800);
        break;
    case HARE:
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 1200);
        break;
    case JAGUAR:
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 1599);
        break;
    }
    // TODO: default for bad input
}

void motor_driver_start(void)
{
    motor_driver_set_direction(FORWARD);
    motor_driver_set_speed(STOP);

    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    // Set STBY pin HIGH.
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
}

void motor_driver_init()
{
    MX_TIM2_Init();
}
