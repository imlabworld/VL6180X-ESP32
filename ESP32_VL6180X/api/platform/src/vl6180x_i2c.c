
/*******************************************************************************
Copyright � 2014, STMicroelectronics International N.V.
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
 * @file vl6180x_i2c.c
 *
 * Copyright (C) 2014 ST MicroElectronics
 *
 * provide variable word size byte/Word/dword VL6180x register access via i2c
 *
 */

#include "vl6180x_platform.h"
#include "vl6180x_api.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"

#define ACK_CHECK_EN true
#define I2C_TOOL_TIMEOUT_VALUE_MS   (50)

/**
 * Writes the supplied byte buffer to the device
 * @param   Dev       Device Handle
 * @param   index     The register index
 * @param   pdata     Pointer to uint8_t buffer containing the data to be written
 * @param   count     Number of bytes in the supplied byte buffer
 * @return  0         on success
 */
static int VL6180x_WriteMulti(i2c_master_dev_handle_t* handle, uint16_t index, uint8_t *pdata, uint32_t count)
{
    uint8_t *buf = malloc(count + 2);
    buf[0] = index >> 8;
    buf[1] = index & 0xff;
    for (int i = 0; i < count; i++) {
        buf[2 + i] = pdata[i];
    }
    esp_err_t ret = i2c_master_transmit(*handle, buf, 2 + count, I2C_TOOL_TIMEOUT_VALUE_MS);

    free(buf);
    int status = (ret == ESP_OK) ? 0 : -1;
    return status;
}

/**
 * Write single byte register
 * @param   Dev       Device Handle
 * @param   index     The register index
 * @param   data      8 bit register data
 * @return  0         on success
 */
int VL6180x_WrByte(i2c_master_dev_handle_t* handle, uint16_t index, uint8_t data)
{
    return VL6180x_WriteMulti(handle, index, &data, 1);
}

/**
 * Threat safe Update (read/modify/write) single byte register
 *
 * Final_reg = (Initial_reg & and_data) |or_data
 *
 * @param   Dev        Device Handle
 * @param   index      The register index
 * @param   AndData    8 bit and data
 * @param   OrData     8 bit or data
 * @return  0         on success
 */
int VL6180x_UpdateByte(i2c_master_dev_handle_t* handle, uint16_t index, uint8_t AndData, uint8_t OrData)
{
    int status;
    uint8_t data = 0;

    status = VL6180x_RdByte(handle, index, &data);

    if (status != 0)
        return status;

    ESP_LOGI("VL6180", "update byte: %d, andData: %d, orData: %d", data, AndData, OrData);
    data = (data & AndData) | OrData;
    return VL6180x_WrByte(handle, index, data);
}

/**
 * Write word register
 * @param   Dev       Device Handle
 * @param   index     The register index
 * @param   data      16 bit register data
 * @return  0         on success
 */
int VL6180x_WrWord(i2c_master_dev_handle_t* handle, uint16_t index, uint16_t data)
{
    uint8_t buffer[2]; // 2
    buffer[0] = (uint8_t)(data >> 8);
    buffer[1] = (uint8_t)(data &  0x00FF);

    return VL6180x_WriteMulti(handle, index, buffer, 2);
}

/**
 * Write double word (4 byte) register
 * @param   Dev       Device Handle
 * @param   index     The register index
 * @param   data      32 bit register data
 * @return  0         on success
 */
int VL6180x_WrDWord(i2c_master_dev_handle_t* handle, uint16_t index, uint32_t data)
{
    uint8_t buffer[4]; // 4

    buffer[0] = (uint8_t) (data >> 24);
    buffer[1] = (uint8_t)((data &  0x00FF0000) >> 16);
    buffer[2] = (uint8_t)((data &  0x0000FF00) >> 8);
    buffer[3] = (uint8_t) (data &  0x000000FF);

    return VL6180x_WriteMulti(handle, index, buffer, 4);
}

/**
 * Read single byte register
 * @param   Dev       Device Handle
 * @param   index     The register index
 * @param   data      pointer to 8 bit data
 * @return  0         on success
 */
int VL6180x_RdByte(i2c_master_dev_handle_t* handle, uint16_t index, uint8_t *data)
{
    return VL6180x_RdMulti(handle, index, data, 1);
}

/**
 * Read word (2byte) register
 * @param   Dev       Device Handle
 * @param   index     The register index
 * @param   data      pointer to 16 bit data
 * @return  0         on success
 */
int VL6180x_RdWord(i2c_master_dev_handle_t* handle, uint16_t index, uint16_t *data)
{
    int status;
    uint8_t  buffer[2] = {};

    status = VL6180x_RdMulti(handle, index, buffer, 2);
    *data = ((uint16_t)buffer[0]<<8) + (uint16_t)buffer[1];

    return status;
}

/**
 * Read dword (4byte) register
 * @param   Dev       Device Handle
 * @param   index     The register index
 * @param   data      pointer to 32 bit data
 * @return  0         on success
 */
int VL6180x_RdDWord(i2c_master_dev_handle_t* handle, uint16_t index, uint32_t *data)
{
    int status;
    uint8_t  buffer[4] = {};

    status = VL6180x_RdMulti(handle, index, buffer, 4);
    *data = ((uint32_t)buffer[0]<<24) + ((uint32_t)buffer[1]<<16) +
             ((uint32_t)buffer[2]<<8) + (uint32_t)buffer[3];

    return status;
}

/**
 * Read VL6180x multiple bytes
 * @note required only if #VL6180x_HAVE_MULTI_READ is set
 * @param dev   The device
 * @param index The register index
 * @param data  pointer to 8 bit data
 * @param nData number of data bytes to read
 * @return 0 on success
 */
int VL6180x_RdMulti(i2c_master_dev_handle_t* handle, uint16_t index, uint8_t *data, int count)
{
    esp_err_t ret = ESP_OK;

    uint8_t buf[2] = {0, };
    buf[0] = index >> 8;
    buf[1] = index & 0xff;
    ret = i2c_master_transmit_receive(*handle, buf, 2, data, count, I2C_TOOL_TIMEOUT_VALUE_MS);

    int status = (ret == ESP_OK) ? 0 : -1;
    return status;
}
