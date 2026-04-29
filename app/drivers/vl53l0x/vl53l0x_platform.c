/*******************************************************************************
Copyright � 2015, STMicroelectronics International N.V.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of STMicroelectronics nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
NON-INFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS ARE DISCLAIMED.
IN NO EVENT SHALL STMICROELECTRONICS INTERNATIONAL N.V. BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************************/

/**
 * @file VL53L0X_i2c.c
 *
 * Copyright (C) 2014 ST MicroElectronics
 *
 * provide variable word size byte/Word/dword VL6180x register access via i2c
 *
 */
#include "main.h"
#include "vl53l0x_platform.h"
#include "vl53l0x_api.h"
#include "ranging.h"

/** Maximum buffer size to be used in i2c */
#define VL53L0X_MAX_I2C_XFER_SIZE 64

VL53L0X_Error VL53L0X_LockSequenceAccess(VL53L0X_DEV Dev)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;

    return Status;
}

VL53L0X_Error VL53L0X_UnlockSequenceAccess(VL53L0X_DEV Dev)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;

    return Status;
}

VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count)
{
    if (count >= VL53L0X_MAX_I2C_XFER_SIZE)
        return VL53L0X_ERROR_INVALID_PARAMS;

    if (HAL_I2C_Mem_Write(&hi2c1, Dev->I2cDevAddr, index, 1, pdata, count, 100) != HAL_OK)
        return VL53L0X_ERROR_CONTROL_INTERFACE;

    return VL53L0X_ERROR_NONE;
}

VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count)
{
    if (count >= VL53L0X_MAX_I2C_XFER_SIZE)
        return VL53L0X_ERROR_INVALID_PARAMS;

    if (HAL_I2C_Mem_Read(&hi2c1, Dev->I2cDevAddr, index, 1, pdata, count, 100) != HAL_OK)
        return VL53L0X_ERROR_CONTROL_INTERFACE;

    return VL53L0X_ERROR_NONE;
}
VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint8_t deviceAddress = Dev->I2cDevAddr;

    if (HAL_I2C_Mem_Write(&hi2c1, deviceAddress, index, 1, &data, 1, 100) != HAL_OK)
    {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    return Status;
}

VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint8_t deviceAddress = Dev->I2cDevAddr;
    uint8_t buf[2];

    // Sensor expects big-endian
    buf[0] = (data >> 8) & 0xFF;
    buf[1] = data & 0xFF;

    if (HAL_I2C_Mem_Write(&hi2c1, deviceAddress, index, 1, buf, 2, 100) != HAL_OK)
    {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    return Status;
}

VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint8_t deviceAddress = Dev->I2cDevAddr;
    uint8_t buf[4];

    buf[0] = (data >> 24) & 0xFF;
    buf[1] = (data >> 16) & 0xFF;
    buf[2] = (data >> 8) & 0xFF;
    buf[3] = data & 0xFF;

    if (HAL_I2C_Mem_Write(&hi2c1, deviceAddress, index, 1, buf, 4, 100) != HAL_OK)
    {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    return Status;
}

VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index, uint8_t AndData, uint8_t OrData)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint8_t deviceAddress = Dev->I2cDevAddr;
    uint8_t data;

    if (HAL_I2C_Mem_Read(&hi2c1, deviceAddress, index, 1, &data, 1, 100) != HAL_OK)
        return VL53L0X_ERROR_CONTROL_INTERFACE;

    data = (data & AndData) | OrData;

    if (HAL_I2C_Mem_Write(&hi2c1, deviceAddress, index, 1, &data, 1, 100) != HAL_OK)
        return VL53L0X_ERROR_CONTROL_INTERFACE;

    return Status;
}

VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint8_t deviceAddress = Dev->I2cDevAddr;

    if (HAL_I2C_Mem_Read(&hi2c1, deviceAddress, index, 1, data, 1, 100) != HAL_OK)
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;

    return Status;
}

VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint8_t deviceAddress = Dev->I2cDevAddr;
    uint8_t buf[2];

    if (HAL_I2C_Mem_Read(&hi2c1, deviceAddress, index, 1, buf, 2, 100) != HAL_OK)
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    else
        *data = ((uint16_t)buf[0] << 8) | buf[1];

    return Status;
}

VL53L0X_Error VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint8_t deviceAddress = Dev->I2cDevAddr;
    uint8_t buf[4];

    if (HAL_I2C_Mem_Read(&hi2c1, deviceAddress, index, 1, buf, 4, 100) != HAL_OK)
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    else
        *data = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
                ((uint32_t)buf[2] << 8) | buf[3];

    return Status;
}

VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev)
{
    HAL_Delay(1);
    return VL53L0X_ERROR_NONE;
}
