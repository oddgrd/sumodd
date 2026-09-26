#include "ranging.h"

#include "VL53L4CD_api.h"
#include "debug.h"
#include "drivers/vl53l0x/vl53l0x_api.h"
#include "i2c.h"
#include "main.h"

// Default I2C address of the device, same for all sensors after reset.
#define VL53L4CD_DEFAULT_ADDRESS 0x52
// I2C address we set for each device during the initialization of the device.
#define RANGING_ADDR_LEFT 0x30
#define RANGING_ADDR_MIDDLE 0x32
#define RANGING_ADDR_RIGHT 0x34

// We set the range lower than the max size of the arena since it will rarely need it, and it makes
// testing easier. The sensor can handle up to 1M well with the lowest timing budget, however,
// depending on the amount of ambient light.
#define RANGING_MAX_DISTANCE_MM 400U
#define RANGING_MIN_DISTANCE_MM 10U
// Around 10ms is the lowest timing budget supported by the VL53L4CD, with some loss of accuracy,
// whereas 30ms is the default. Since we are only measuring within a 77cm dohyo, we go for the
// fastest.
#define RANGING_TIMING_BUDGET_MS 10U

typedef struct
{
    GPIO_TypeDef *xshut_port;
    uint16_t xshut_pin;
    uint8_t device_address;
} RangingConfig;

// XSHUT pin and device address configurations for range sensors.
static const RangingConfig ranging_config[RANGING_COUNT] = {
    // [RANGING_LEFT] = {GPIOA, GPIO_PIN_11, RANGING_ADDR_LEFT},
    [RANGING_MIDDLE] = {GPIOA, GPIO_PIN_8, RANGING_ADDR_MIDDLE},
    // [RANGING_RIGHT] = {GPIOB, GPIO_PIN_1, RANGING_ADDR_RIGHT},
};

RangingState ranging_state = {0};

/**
 * @brief Read the ranging sensors and update the ranging state.
 *
 * If a failure occurs, the function will simply log the error (in debug builds) and continue.
 */
static void ranging_update(void)
{

    for (int i = 0; i < RANGING_COUNT; i++)
    {
        if (!ranging_state.sensor[i].data_ready)
        {
            continue;
        }

        VL53L4CD_ResultsData_t RangingData = {0};

        int ret = VL53L4CD_GetResult(ranging_state.sensor[i].dev, &RangingData);
        if (ret != VL53L4CD_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to get ranging data for sensor: %d, error: %d\n", i, ret);
        };

        ranging_state.sensor[i].range_mm = RangingData.distance_mm;
        ranging_state.sensor[i].range_status = RangingData.range_status;

        DEBUG_PRINTF("Distance: %d mm, status: %d, sigma mm: %d, device: %x\n",
                     RangingData.distance_mm, RangingData.range_status, RangingData.sigma_mm,
                     ranging_state.sensor[i].dev);

        ranging_state.sensor[i].data_ready = false;

        // Clear the interrupt so the next measurement can complete
        ret = VL53L4CD_ClearInterrupt(ranging_state.sensor[i].dev);
        if (ret != VL53L4CD_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to clear interrupt mask for sensor: %d, error: %d\n", i, ret);
        };
    }

    // DEBUG_PRINTF("l:%d m:%d r:%d\n", ranging_state.sensor[RANGING_LEFT].range_mm,
    // ranging_state.sensor[RANGING_MIDDLE].range_mm, ranging_state.sensor[RANGING_RIGHT].range_mm);
}

static bool valid_range(int16_t range_mm)
{
    return range_mm < RANGING_MAX_DISTANCE_MM && range_mm > RANGING_MIN_DISTANCE_MM;
}

Enemy ranging_get_enemy(void)
{
    ranging_update();
    Enemy enemy = {.bearing = BEARING_NONE};

    // bool enemy_left = valid_range(ranging_state.sensor[RANGING_LEFT].range_mm);
    bool enemy_front = valid_range(ranging_state.sensor[RANGING_MIDDLE].range_mm);
    // bool enemy_right = valid_range(ranging_state.sensor[RANGING_RIGHT].range_mm);

    if (enemy_front)
    {
        enemy.bearing = BEARING_FRONT;
        enemy.distance_mm = ranging_state.sensor[RANGING_MIDDLE].range_mm;
        return enemy;
    }

    // if (enemy_left)
    // {
    //     enemy.bearing = BEARING_LEFT;
    //     enemy.distance_mm = ranging_state.sensor[RANGING_LEFT].range_mm;
    //     return enemy;
    // }

    // if (enemy_right)
    // {
    //     enemy.bearing = BEARING_RIGHT;
    //     enemy.distance_mm = ranging_state.sensor[RANGING_RIGHT].range_mm;
    //     return enemy;
    // }

    return enemy;
}

VL53L4CD_Error ranging_init(void)
{
    MX_I2C1_Init();

    for (int i = 0; i < RANGING_COUNT; i++)
    {
        ranging_state.sensor[i].xshut_port = ranging_config[i].xshut_port;
        ranging_state.sensor[i].xshut_pin = ranging_config[i].xshut_pin;
        ranging_state.sensor[i].data_ready = false;

        // Set all low to reset them, and we will bring them up one by one to set their address.
        HAL_GPIO_WritePin(ranging_state.sensor[i].xshut_port, ranging_state.sensor[i].xshut_pin,
                          GPIO_PIN_RESET);
    }
    HAL_Delay(10);
    int ret = VL53L4CD_ERROR_NONE;

    for (int i = 0; i < RANGING_COUNT; i++)
    {
        DEBUG_PRINTF("Initializing device with address: %x\n", ranging_config[i].device_address);

        // First, set the xshut of the sensor we are configuring high.
        HAL_GPIO_WritePin(ranging_state.sensor[i].xshut_port, ranging_state.sensor[i].xshut_pin,
                          GPIO_PIN_SET);
        HAL_Delay(2);

        // Use the default address for the change address I2C call, since it will be the address of
        // all the devices after the reset.
        ranging_state.sensor[i].dev = VL53L4CD_DEFAULT_ADDRESS;

        ret = VL53L4CD_SetI2CAddress(ranging_state.sensor[i].dev, ranging_config[i].device_address);
        if (ret != VL53L4CD_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to set device address for device %d, error: %d\n",
                         ranging_state.sensor[i].dev, ret);
            return ret;
        };

        // Change the device instance address to the new address, we will use that from here on out.
        ranging_state.sensor[i].dev = ranging_config[i].device_address;

        ret = VL53L4CD_SensorInit(ranging_state.sensor[i].dev);
        if (ret != VL53L4CD_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to initialize data for device %d, error: %d\n",
                         ranging_state.sensor[i].dev, ret);
            return ret;
        };

        // Configure for fast ranging with the minimum timing budget, and continuous ranging,
        // 0 ms between measurements.
        ret = VL53L4CD_SetRangeTiming(ranging_state.sensor[i].dev, RANGING_TIMING_BUDGET_MS, 0);
        if (ret != VL53L4CD_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to set range timing for device %d, error: %d\n",
                         ranging_state.sensor[i].dev, ret);
            return ret;
        };
    }

    for (int i = 0; i < RANGING_COUNT; i++)
    {
        ret = VL53L4CD_StartRanging(ranging_state.sensor[i].dev);
        if (ret != VL53L4CD_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to start measurements for device %d, error: %d\n",
                         ranging_state.sensor[i].dev, ret);
            return ret;
        };
    }

    return ret;
}

// VL53L0X data ready interrupt ISR. Set flag to read data over I2C in main loop.
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // if (GPIO_Pin == GPIO_PIN_4)
    // {
    //     ranging_state.sensor[RANGING_LEFT].data_ready = true;
    // }
    if (GPIO_Pin == GPIO_PIN_0)
    {
        ranging_state.sensor[RANGING_MIDDLE].data_ready = true;
    }
    // if (GPIO_Pin == GPIO_PIN_12)
    // {
    //     ranging_state.sensor[RANGING_RIGHT].data_ready = true;
    // }
}
