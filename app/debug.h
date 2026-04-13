#pragma once

/**
 * Conditionally compiled logging over RTT in debug builds. Wrapper around SEGGER_RTT_printf.
 */

#ifdef DEBUG
#include "SEGGER_RTT.h"
#define DEBUG_PRINTF(fmt, ...) SEGGER_RTT_printf(0, fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...) ((void)0)
#endif