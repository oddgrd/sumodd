#include "main.h"
#include "line_sensor.h"
#include "state.h"
#include <stdint.h>
#include <stdbool.h>

// TODO: IR remote command to adjust threshold?
#define LINE_DETECTED_THRESHOLD 500

ADC_HandleTypeDef hadc2;
TIM_HandleTypeDef htim1;

// Buffer for the ADC conversion value of all four channels, representing all four line sensors.
static volatile uint16_t adc_buffer[4] = {0};

struct LineSamples
{
    uint16_t front_left;
    uint16_t front_right;
    uint16_t rear_left;
    uint16_t rear_right;
};

/**
 * @brief ADC2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC2_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    /** Common config
     */
    hadc2.Instance = ADC2;
    hadc2.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
    hadc2.Init.Resolution = ADC_RESOLUTION_10B;
    hadc2.Init.ScanConvMode = ADC_SCAN_ENABLE;
    hadc2.Init.ContinuousConvMode = DISABLE;
    hadc2.Init.DiscontinuousConvMode = DISABLE;
    hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc2.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_TRGO;
    hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc2.Init.NbrOfConversion = 4;
    hadc2.Init.DMAContinuousRequests = ENABLE;
    hadc2.Init.EOCSelection = ADC_EOC_SEQ_CONV;
    hadc2.Init.LowPowerAutoWait = DISABLE;
    hadc2.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    if (HAL_ADC_Init(&hadc2) != HAL_OK)
    {
        Error_Handler();
    }

    /** Configure Regular Channel
     */
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    // IMPORTANT: Setting the sample time lower than this led to crosstalk between channels on
    // optimized builds.
    sConfig.SamplingTime = ADC_SAMPLETIME_4CYCLES_5;

    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    /** Configure Regular Channel
     */
    sConfig.Channel = ADC_CHANNEL_2;
    sConfig.Rank = ADC_REGULAR_RANK_2;
    if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    /** Configure Regular Channel
     */
    sConfig.Channel = ADC_CHANNEL_3;
    sConfig.Rank = ADC_REGULAR_RANK_3;
    if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    /** Configure Regular Channel
     */
    sConfig.Channel = ADC_CHANNEL_4;
    sConfig.Rank = ADC_REGULAR_RANK_4;
    if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief TIM1 Initialization Function
 * @param None
 * @retval None
 *
 * This timer peripheral is used to emit TRG0 update events, which are used to trigger conversions
 * of all four channels in the ADC used for the line sensors. This is to control the frequency of
 * these conversions, rather than running continuous conversions.
 * TODO: This was done at a time when we were relying on the conversion interrupts and did not
 * want to spam them, but since we no longer use those, we just use DMA to update a buffer on
 * each conversion, we could consider dropping this timer and doing continuous conversions.
 */
static void MX_TIM1_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    // With prescaler set to 64 (-1 because stm32 TIM prescalers are zero-based), and a source clock
    // of 64MHz, this timer will have a frequency of 1MHz, which means each tick is 1us. With the
    // period set to 100, the timer will reload and trigger an update event every 100us. That update
    // event will trigger a conversion in the line sensor ADC.
    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 63;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 100;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
    {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

LineType get_line(void)
{
    struct LineSamples samples = {
        .front_left = adc_buffer[0],
        .front_right = adc_buffer[1],
        .rear_left = adc_buffer[2],
        .rear_right = adc_buffer[3]};

    // TODO: make more granular, right now we only act on front or back, we should also handle
    // corners, sides etc.
    if (samples.front_left < LINE_DETECTED_THRESHOLD || samples.front_right < LINE_DETECTED_THRESHOLD)
    {
        return LINE_FRONT;
    }

    if (samples.rear_left < LINE_DETECTED_THRESHOLD || samples.rear_right < LINE_DETECTED_THRESHOLD)
    {
        return LINE_BACK;
    }

    return LINE_NONE;
}

void line_sensor_init(void)
{
    MX_TIM1_Init();
    MX_ADC2_Init();

    if (HAL_ADC_Start_DMA(&hadc2, (uint32_t *)adc_buffer, 4) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_Base_Start(&htim1) != HAL_OK)
    {
        Error_Handler();
    }
}