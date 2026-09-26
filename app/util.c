#include "tim.h"

void delay_us(uint16_t us)
{
    uint16_t start = (uint16_t)__HAL_TIM_GET_COUNTER(&htim17);

    while ((uint16_t)((uint16_t)__HAL_TIM_GET_COUNTER(&htim17) - start) < us)
    {
    }
}