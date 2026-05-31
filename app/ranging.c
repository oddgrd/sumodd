#include "main.h"
#include "drivers/vl53l0x/vl53l0x_api.h"
#include "ranging.h"
#include "debug.h"

// Default I2C address of the device, same for all sensors after reset.
#define VL53L0X_DEFAULT_ADDRESS 0x52
// I2C address we set for each device during the initialization of the device.
#define RANGING_ADDR_LEFT 0x30
// #define RANGING_ADDR_MIDDLE 0x32
#define RANGING_ADDR_RIGHT 0x34

// We make the distance slightly larger than the 77CM max arena size to allow for inaccurate long
// distance measurements.
#define RANGING_MAX_DISTANCE_MM 250U
#define RANGING_MIN_DISTANCE_MM 10U
// Around 16ms (66Hz) is the lowest time supported by the device between measurements,
// including the ranging setup and measurement itself, whereas 32ms is the optimal for
// accuracy. We should consider a lower value here in the future, since we are only measuring
// within the dohyo.
#define RANGING_TIMING_BUDGET_US 20000U

I2C_HandleTypeDef hi2c1;

typedef struct
{
    GPIO_TypeDef *xshut_port;
    uint16_t xshut_pin;
    uint8_t device_address;
} RangingConfig;

// XSHUT pin and device address configurations for range sensors.
static const RangingConfig ranging_config[RANGING_COUNT] = {
    [RANGING_LEFT] = {GPIOA, GPIO_PIN_11, RANGING_ADDR_LEFT},
    // [RANGING_MIDDLE] = {GPIOA, GPIO_PIN_8, RANGING_ADDR_MIDDLE},
    [RANGING_RIGHT] = {GPIOB, GPIO_PIN_1, RANGING_ADDR_RIGHT},
};

RangingState ranging_state = {0};

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    // Fast mode, 400 KHz, to speed up the I2C transmissions required to read the ranging data.
    hi2c1.Init.Timing = 0x0010020A;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }

    /** Configure Analogue filter
     */
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
    {
        Error_Handler();
    }

    /** Configure Digital filter
     */
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
    {
        Error_Handler();
    }
}

// TODO: consider whether we should return any error from this, or simply log and continue, in case
// some sensors fail, but not all.
void ranging_update(void)
{
    VL53L0X_RangingMeasurementData_t RangingData = {0};

    for (int i = 0; i < RANGING_COUNT; i++)
    {
        if (ranging_state.sensor[i].data_ready)
        {
            // TODO: debug log more ranging data to inspect it.
            int ret = VL53L0X_GetRangingMeasurementData(&ranging_state.sensor[i].dev, &RangingData);
            if (ret != VL53L0X_ERROR_NONE)
            {
                DEBUG_PRINTF("Failed to get ranging data for sensor: %d, error: %d\n", i, ret);
            };

            int16_t distance_mm = RangingData.RangeMilliMeter;

            ranging_state.sensor[i].range_mm = distance_mm;
            ranging_state.sensor[i].range_status = RangingData.RangeStatus;

            // DEBUG_PRINTF(
            //     "Distance: %d mm, status: %d, max: %d, device: %x\n",
            //     distance_mm,
            //     RangingData.RangeStatus,
            //     RangingData.RangeDMaxMilliMeter,
            //     ranging_state.sensor[i].dev.I2cDevAddr);

            ranging_state.sensor[i].data_ready = false;

            // Clear the interrupt so the next measurement can complete
            ret = VL53L0X_ClearInterruptMask(&ranging_state.sensor[i].dev, 0);
            if (ret != VL53L0X_ERROR_NONE)
            {
                DEBUG_PRINTF("Failed to clear interrupt mask for sensor: %d, error: %d\n", i, ret);
            };
        }
    }

    // DEBUG_PRINTF("l: %d\nr:%d\n", ranging_state.sensor[RANGING_LEFT].range_mm, ranging_state.sensor[RANGING_RIGHT].range_mm);
}

bool valid_range(int16_t range_mm)
{
    return range_mm < RANGING_MAX_DISTANCE_MM && range_mm > RANGING_MIN_DISTANCE_MM;
}

Enemy ranging_get_enemy(void)
{
    ranging_update();
    Enemy enemy = {.bearing = BEARING_NONE};

    // TODO: also check ranging status? We will  get not-null for bad readings, e.g. max distance
    // due to looking into space, e.g:
    // 13:12:16.418: Distance: 8190 mm, status: 4, max: 1167
    // 13:12:16.418: Distance: 8191 mm, status: 2, max: 1169

    bool enemy_left = valid_range(ranging_state.sensor[RANGING_LEFT].range_mm);
    bool enemy_right = valid_range(ranging_state.sensor[RANGING_RIGHT].range_mm);
    bool enemy_front = enemy_left && enemy_right;

    if (enemy_front)
    {
        enemy.bearing = BEARING_FRONT;
        enemy.distance_mm = (ranging_state.sensor[RANGING_LEFT].range_mm + ranging_state.sensor[RANGING_RIGHT].range_mm) >> 1;
        return enemy;
    }

    if (enemy_left)
    {
        enemy.bearing = BEARING_LEFT;
        enemy.distance_mm = ranging_state.sensor[RANGING_LEFT].range_mm;
        return enemy;
    }

    if (enemy_right)
    {
        enemy.bearing = BEARING_RIGHT;
        enemy.distance_mm = ranging_state.sensor[RANGING_RIGHT].range_mm;
        return enemy;
    }

    return enemy;
}

VL53L0X_Error ranging_init(void)
{
    MX_I2C1_Init();

    for (int i = 0; i < RANGING_COUNT; i++)
    {
        ranging_state.sensor[i].xshut_port = ranging_config[i].xshut_port;
        ranging_state.sensor[i].xshut_pin = ranging_config[i].xshut_pin;
        ranging_state.sensor[i].data_ready = false;

        // Set all low to reset them, and we will bring them up one by one to set their address.
        HAL_GPIO_WritePin(ranging_state.sensor[i].xshut_port, ranging_state.sensor[i].xshut_pin, GPIO_PIN_RESET);
    }
    HAL_Delay(10);
    int ret = VL53L0X_ERROR_NONE;

    for (int i = 0; i < RANGING_COUNT; i++)
    {
        DEBUG_PRINTF("Initializing device with address: %x\n", ranging_config[i].device_address);

        // First, set the xshut of the sensor we are configuring high.
        HAL_GPIO_WritePin(ranging_state.sensor[i].xshut_port, ranging_state.sensor[i].xshut_pin, GPIO_PIN_SET);
        HAL_Delay(2);

        // Use the default address for the change address I2C call, since it will be the address of
        // all the devices after the reset.
        ranging_state.sensor[i].dev.I2cDevAddr = VL53L0X_DEFAULT_ADDRESS;

        ret = VL53L0X_SetDeviceAddress(&ranging_state.sensor[i].dev, ranging_config[i].device_address);
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to set device address for device, error: %d\n", ret);
            return ret;
        };

        // Change the device instance address to the new address, we will use that from here on out.
        ranging_state.sensor[i].dev.I2cDevAddr = ranging_config[i].device_address;

        ret = VL53L0X_DataInit(&ranging_state.sensor[i].dev);
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to initialize data for device, error: %d\n", ret);
            return ret;
        };

        ret = VL53L0X_StaticInit(&ranging_state.sensor[i].dev);
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to statically initialize device, error: %d\n", ret);
            return ret;
        };

        uint8_t VhvSettings = 0;
        uint8_t PhaseCal = 0;
        ret = VL53L0X_PerformRefCalibration(&ranging_state.sensor[i].dev, &VhvSettings, &PhaseCal);
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to perform ref calibration for device, error: %d\n", ret);
            return ret;
        };

        // Load SPAD (Single Photon Avalanche Diode) calibration data, needs to be done after each
        // reset. This is an array of diodes that are used for detecting the reflected IR light
        // emitted from the VCSEL (vertical-cavity surface-emitting laser).
        uint32_t refSpadCount = 0;
        uint8_t isApertureSpads = 0;
        ret = VL53L0X_PerformRefSpadManagement(&ranging_state.sensor[i].dev, &refSpadCount, &isApertureSpads);
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to perform spad management for device, error: %d\n", ret);
            return ret;
        };

        DEBUG_PRINTF(
            "Initialized sensor with spad count: %d, aperture spads enabled: %d\n",
            refSpadCount,
            isApertureSpads);

        ret = VL53L0X_SetDeviceMode(&ranging_state.sensor[i].dev, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING);
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to set device mode for device, error: %d\n", ret);
            return ret;
        };

        ret = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&ranging_state.sensor[i].dev, RANGING_TIMING_BUDGET_US);
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to set measurement timing budget for device, error: %d\n", ret);
            return ret;
        };

        // Raise the signal rate limit so that weak signals, e.g. from rough arena surfaces, are
        // ignored. Chip default is 0.25 MCPS.
        ret = VL53L0X_SetLimitCheckEnable(&ranging_state.sensor[i].dev, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1);
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to enable limit check for signal rate, error: %d\n", ret);
            return ret;
        };
        ret = VL53L0X_SetLimitCheckValue(&ranging_state.sensor[i].dev, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
                                         (FixPoint1616_t)(0.40 * 65536));
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to increase signal rate limit, error: %d\n", ret);
            return ret;
        };

        // Reject noisy / low-confidence measurements. This limit, sigma, is concerned with the standard
        // deviation of the signal, so whether the signal is precise.
        ret = VL53L0X_SetLimitCheckEnable(&ranging_state.sensor[i].dev, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1);
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to enable limit check for sigma, error: %d\n", ret);
            return ret;
        };
        ret = VL53L0X_SetLimitCheckValue(&ranging_state.sensor[i].dev, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
                                         (FixPoint1616_t)(18 * 65536)); // mm; smaller = stricter
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to increase sigma limit, error: %d\n", ret);
            return ret;
        };

        // Configure the GPIO pin on the device, AKA the DRDY interrupt pin, which will be pulled
        // low when data is ready, and which will trigger an interrupt in an EXTI pin on the MCU.
        ret = VL53L0X_SetGpioConfig(&ranging_state.sensor[i].dev, 0,
                                    VL53L0X_DEVICEMODE_CONTINUOUS_RANGING,
                                    VL53L0X_GPIOFUNCTIONALITY_NEW_MEASURE_READY,
                                    VL53L0X_INTERRUPTPOLARITY_LOW);
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to configure gpio pin for device, error: %d\n", ret);
            return ret;
        };
    }

    for (int i = 0; i < RANGING_COUNT; i++)
    {
        ret = VL53L0X_StartMeasurement(&ranging_state.sensor[i].dev);
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to start measurements for device, error: %d\n", ret);
            return ret;
        };
    }

    return ret;
}

// VL53L0X data ready interrupt ISR. Set flag to read from I2C in main loop.
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_4)
    {
        ranging_state.sensor[RANGING_LEFT].data_ready = true;
    }
    // if (GPIO_Pin == GPIO_PIN_0)
    // {
    //     ranging_state.sensor[RANGING_MIDDLE].data_ready = true;
    // }
    if (GPIO_Pin == GPIO_PIN_12)
    {
        ranging_state.sensor[RANGING_RIGHT].data_ready = true;
    }
}
