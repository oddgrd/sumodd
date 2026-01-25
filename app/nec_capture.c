#include <stm32f3xx_hal_tim.h>
#include <stdint.h>

#define FINAL_PULSE 34U
#define B1_PULSE_WIDTH_MS 2U

static uint8_t pulse_counter = 0;
static uint8_t void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
    }
}