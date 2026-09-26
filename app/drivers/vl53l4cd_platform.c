/**
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/**
 * Platform specific implementation for the VL53L4CD ULD driver, defining the function that the
 * driver will use to communicate with the device over I2C.
 *
 * The implementation is quite simple, using the ST HAL I2C functions. The VL53L4CD is big endian,
 * so we need to convert to little endian when reading, and to big endian when writing.
 */
#include <i2c.h>

#include "platform.h"

#define VL53L4CD_PLATFORM_OK 0U
#define VL53L4CD_PLATFORM_I2C_ERROR 1U

#define VL53L4CD_PLATFORM_I2C_TIMEOUT 100U

uint8_t VL53L4CD_RdDWord(Dev_t dev, uint16_t RegisterAdress, uint32_t *value)
{
    uint8_t buf[4];

    // The VL53L4CD uses 16 bit addresses, and we need to read four bytes for a double word.
    if (HAL_I2C_Mem_Read(&hi2c1, dev, RegisterAdress, I2C_MEMADD_SIZE_16BIT, buf, sizeof(buf),
                         VL53L4CD_PLATFORM_I2C_TIMEOUT) != HAL_OK)
    {
        return VL53L4CD_PLATFORM_I2C_ERROR;
    }

    *value = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];

    return VL53L4CD_PLATFORM_OK;
}

uint8_t VL53L4CD_RdWord(Dev_t dev, uint16_t RegisterAdress, uint16_t *value)
{
    uint8_t buf[2];

    if (HAL_I2C_Mem_Read(&hi2c1, dev, RegisterAdress, I2C_MEMADD_SIZE_16BIT, buf, sizeof(buf),
                         VL53L4CD_PLATFORM_I2C_TIMEOUT) != HAL_OK)
    {
        return VL53L4CD_PLATFORM_I2C_ERROR;
    }

    *value = ((uint16_t)buf[0] << 8) | buf[1];

    return VL53L4CD_PLATFORM_OK;
}

uint8_t VL53L4CD_RdByte(Dev_t dev, uint16_t RegisterAdress, uint8_t *value)
{
    if (HAL_I2C_Mem_Read(&hi2c1, dev, RegisterAdress, I2C_MEMADD_SIZE_16BIT, value, 1,
                         VL53L4CD_PLATFORM_I2C_TIMEOUT) != HAL_OK)
    {
        return VL53L4CD_PLATFORM_I2C_ERROR;
    }

    return VL53L4CD_PLATFORM_OK;
}

uint8_t VL53L4CD_WrByte(Dev_t dev, uint16_t RegisterAdress, uint8_t value)
{
    if (HAL_I2C_Mem_Write(&hi2c1, dev, RegisterAdress, I2C_MEMADD_SIZE_16BIT, &value, 1,
                          VL53L4CD_PLATFORM_I2C_TIMEOUT) != HAL_OK)
    {
        return VL53L4CD_PLATFORM_I2C_ERROR;
    }

    return VL53L4CD_PLATFORM_OK;
}

uint8_t VL53L4CD_WrWord(Dev_t dev, uint16_t RegisterAdress, uint16_t value)
{
    uint8_t buf[2] = {(uint8_t)(value >> 8), (uint8_t)value};

    if (HAL_I2C_Mem_Write(&hi2c1, dev, RegisterAdress, I2C_MEMADD_SIZE_16BIT, buf, sizeof(buf),
                          VL53L4CD_PLATFORM_I2C_TIMEOUT) != HAL_OK)
    {
        return VL53L4CD_PLATFORM_I2C_ERROR;
    }

    return VL53L4CD_PLATFORM_OK;
}

uint8_t VL53L4CD_WrDWord(Dev_t dev, uint16_t RegisterAdress, uint32_t value)
{
    uint8_t buf[4] = {(uint8_t)(value >> 24), (uint8_t)(value >> 16), (uint8_t)(value >> 8),
                      (uint8_t)value};

    if (HAL_I2C_Mem_Write(&hi2c1, dev, RegisterAdress, I2C_MEMADD_SIZE_16BIT, buf, sizeof(buf),
                          VL53L4CD_PLATFORM_I2C_TIMEOUT) != HAL_OK)
    {
        return VL53L4CD_PLATFORM_I2C_ERROR;
    }

    return VL53L4CD_PLATFORM_OK;
}

uint8_t VL53L4CD_WaitMs(Dev_t dev, uint32_t TimeMs)
{
    (void)dev;

    HAL_Delay(TimeMs);
    return VL53L4CD_PLATFORM_OK;
}
