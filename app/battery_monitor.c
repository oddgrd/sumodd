#include <stdbool.h>

#include "adc.h"
#include "debug.h"
#include "main.h"
#include "tim.h"

#define ADC_REF_VOLTAGE_MV 3300U
#define ADC_12_BIT_RESOLUTION 4095U
#define BATTERY_MONITOR_R1_OHM 75000U
#define BATTERY_MONITOR_R2_OHM 24000U

volatile uint32_t adc_val = 0;

uint32_t battery_monitor_get_voltage_mv()
{
    // Convert the 12 bit ADC reading to millivolts.
    uint32_t adc_raw = adc_val;
    uint32_t adc_voltage_mv = adc_raw * ADC_REF_VOLTAGE_MV / ADC_12_BIT_RESOLUTION;

    // Apply the battery monitor voltage divider ratio to get the battery voltage.
    uint32_t battery_voltage_mv =
        adc_voltage_mv * (BATTERY_MONITOR_R1_OHM + BATTERY_MONITOR_R2_OHM) / BATTERY_MONITOR_R2_OHM;

    return battery_voltage_mv;
};

// cppcheck-suppress constParameterPointer
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        adc_val = HAL_ADC_GetValue(&hadc1);
    }
};

void battery_monitor_init(void)
{
    MX_TIM6_Init();
    MX_ADC1_Init();

    if (HAL_ADC_Start_IT(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_Base_Start(&htim6) != HAL_OK)
    {
        Error_Handler();
    }

    // Trigger the initial ADC conversion immediately, so we don't need to wait for the first
    // timer reload.
    HAL_TIM_GenerateEvent(&htim6, TIM_EVENTSOURCE_UPDATE);
};