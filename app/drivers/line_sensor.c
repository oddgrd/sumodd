#include "main.h"
#include "line_sensor.h"
#include "state.h"
#include <stdint.h>
#include <stdbool.h>
#include <adc.h>
#include <tim.h>

// TODO: IR remote command to adjust threshold?
#define LINE_DETECTED_THRESHOLD 500

// Buffer for the ADC conversion value of all four channels, representing all four line sensors.
static volatile uint16_t adc_buffer[4] = {0};

struct LineSamples
{
    uint16_t front_left;
    uint16_t front_right;
    uint16_t rear_left;
    uint16_t rear_right;
};

LineType get_line(void)
{
    struct LineSamples samples = {
        .front_left = adc_buffer[0],
        .front_right = adc_buffer[1],
        .rear_right = adc_buffer[2],
        .rear_left = adc_buffer[3]};

    if (samples.front_left < LINE_DETECTED_THRESHOLD && samples.front_right < LINE_DETECTED_THRESHOLD)
    {
        return LINE_FRONT;
    }

    if (samples.rear_left < LINE_DETECTED_THRESHOLD && samples.rear_right < LINE_DETECTED_THRESHOLD)
    {
        return LINE_BACK;
    }

    if (samples.front_left < LINE_DETECTED_THRESHOLD && samples.rear_left < LINE_DETECTED_THRESHOLD)
    {
        return LINE_LEFT;
    }

    if (samples.front_right < LINE_DETECTED_THRESHOLD && samples.rear_right < LINE_DETECTED_THRESHOLD)
    {
        return LINE_RIGHT;
    }

    if (samples.front_left < LINE_DETECTED_THRESHOLD)
    {
        return LINE_FRONT_LEFT;
    }

    if (samples.front_right < LINE_DETECTED_THRESHOLD)
    {
        return LINE_FRONT_RIGHT;
    }

    if (samples.rear_left < LINE_DETECTED_THRESHOLD)
    {
        return LINE_BACK_LEFT;
    }

    if (samples.rear_right < LINE_DETECTED_THRESHOLD)
    {
        return LINE_BACK_RIGHT;
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