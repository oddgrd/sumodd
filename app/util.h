#pragma once

#include <stdint.h>

/**
 * @brief Delay execution by input microseconds, backed by a 1MHz prescaled timer peripheral.
 *
 * IMPORTANT: depends on the initialization of TIM17.
 */
void delay_us(uint16_t us);