#include "debug.h"
#include "drivers/ir_remote.h"
#include "drivers/line_sensor.h"
#include "drivers/motor_driver.h"
#include "drivers/vl53l0x/vl53l0x_api.h"
#include "main.h"
#include "ranging.h"
#include "state.h"
#include "tim.h"

void app_init(void)
{
    // Used by microsecond sleep utility, and for IR remote input capture.
    MX_TIM17_Init();
    if (HAL_TIM_Base_Start(&htim17) != HAL_OK)
    {
        Error_Handler();
    }

    ir_remote_init();
    line_sensor_init();
    int ret = ranging_init();
    if (ret != VL53L0X_ERROR_NONE)
    {
        DEBUG_PRINTF("Encountered an error during ranging init, error: %d\n", ret);
        Error_Handler();
    }
    motor_driver_init();
    state_machine_init();
}

void app_run(void)
{
    state_machine_run();
}