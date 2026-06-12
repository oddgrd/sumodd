#include "main.h"
#include "drivers/motor_driver.h"
#include "app.h"

void app_init(void)
{
    motor_driver_init();
}

void app_run(void)
{
    // Enable LED and wait for 5 seconds before starting.
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, SET);
    motor_drive(0, DRIVE_STOP);
    HAL_Delay(5000);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, RESET);
    motor_drive(25, DRIVE_FORWARD);
    HAL_Delay(3000);
    motor_drive(25, DRIVE_REVERSE);
    HAL_Delay(3000);
    motor_drive(25, DRIVE_FORWARD_ARC_LEFT);
    HAL_Delay(3000);
    motor_drive(25, DRIVE_FORWARD_ARC_RIGHT);
    HAL_Delay(3000);
    motor_drive(25, DRIVE_REVERSE_ARC_LEFT);
    HAL_Delay(3000);
    motor_drive(25, DRIVE_REVERSE_ARC_RIGHT);
    HAL_Delay(3000);
    motor_drive(25, DRIVE_SPIN_LEFT);
    HAL_Delay(3000);
    motor_drive(25, DRIVE_SPIN_RIGHT);
    HAL_Delay(3000);
}