#include "main.h"
#include "drivers/vl53l0x/vl53l0x_api.h"
#include "ranging.h"
#include "debug.h"

#define VL53L0X_DEFAULT_ADDRESS 0x52
#define RANGING_ADDR_LEFT 0x30
#define RANGING_ADDR_MIDDLE 0x31
#define RANGING_ADDR_RIGHT 0x32

I2C_HandleTypeDef hi2c1;

typedef struct
{
    GPIO_TypeDef *xshut_port;
    uint16_t xshut_pin;
    // TODO: is it needed to have these here? We don't use them in interrupt.
    GPIO_TypeDef *drdy_port;
    uint16_t drdy_pin;
} RangingConfig;

// XSHUT and DRDY pin configurations for range sensors.
static const RangingConfig ranging_config[RANGING_COUNT] = {
    // [RANGING_LEFT] = {GPIOB, GPIO_PIN_1, GPIOB, GPIO_PIN_4},
    [RANGING_MIDDLE] = {GPIOA, GPIO_PIN_8, GPIOB, GPIO_PIN_0},
    // [RANGING_RIGHT] = {GPIOA, GPIO_PIN_11, GPIOA, GPIO_PIN_12},
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

    // TODO: add other addresses once in use.
    uint8_t addresses[RANGING_COUNT] = {RANGING_ADDR_MIDDLE};
    for (int i = 0; i < RANGING_COUNT; i++)
    {
        // First, set the xshut of the sensor we are configuring high.
        HAL_GPIO_WritePin(ranging_state.sensor[i].xshut_port, ranging_state.sensor[i].xshut_pin, GPIO_PIN_SET);
        HAL_Delay(2);

        // Use the default address for the change address I2C call, since it will be the address of
        // all the devices after the reset.
        ranging_state.sensor[i].dev.I2cDevAddr = VL53L0X_DEFAULT_ADDRESS;

        ret = VL53L0X_SetDeviceAddress(&ranging_state.sensor[i].dev, addresses[i]);
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to set device address for device, error: %d\n", ret);
            return ret;
        };

        // Change the devices address to the new address, we will use that from here on out.
        ranging_state.sensor[i].dev.I2cDevAddr = addresses[i];

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

        uint8_t VhvSettings, PhaseCal;
        ret = VL53L0X_PerformRefCalibration(&ranging_state.sensor[i].dev, &VhvSettings, &PhaseCal);
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to perform ref calibration for device, error: %d\n", ret);
            return ret;
        };

        // TODO: document spad management.
        uint32_t refSpadCount;
        uint8_t isApertureSpads;
        ret = VL53L0X_PerformRefSpadManagement(&ranging_state.sensor[i].dev, &refSpadCount, &isApertureSpads);
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to perform spad management for device, error: %d\n", ret);
            return ret;
        };

        ret = VL53L0X_SetDeviceMode(&ranging_state.sensor[i].dev, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING);
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to set device mode for device, error: %d\n", ret);
            return ret;
        };

        // Around 18ms (55Hz) is the lowest time supported by the device between measurements, and since
        // we are working with short ranges, we'll use the fastest, at the cost of some accuracy.
        ret = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&ranging_state.sensor[i].dev, 20000);
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to set measurement timing budget for device, error: %d\n", ret);
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

        ret = VL53L0X_StartMeasurement(&ranging_state.sensor[i].dev);
        if (ret != VL53L0X_ERROR_NONE)
        {
            DEBUG_PRINTF("Failed to start measurements for device, error: %d\n", ret);
            return ret;
        };
    }

    return ret;
};

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        // VL53L0X data ready interrupt triggered. Set flag to read from I2C in main loop.
        ranging_state.sensor[RANGING_MIDDLE].data_ready = true;
    }
    // TODO: add other sensors EXTI pins.
}
