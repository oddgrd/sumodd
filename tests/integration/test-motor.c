#include "main.h"
#include "drivers/motor_driver.h"
#include "app.h"

void app_init(void)
{
    motor_driver_init();
}

void app_run(void)
{
    motor_spin_left(25);
    HAL_Delay(5000);
    motor_spin_right(25);
    HAL_Delay(5000);
}