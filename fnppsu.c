 /*
  *   File:   fnppsu.c
  *   Author: Matthew Millman
  *
  *   FNP600/850/1000 Adapter Board
  *
  *   Created on 12 July 2016, 10:32
  *
  *   This is free software: you can redistribute it and/or modify
  *   it under the terms of the GNU General Public License as published by
  *   the Free Software Foundation, either version 2 of the License, or
  *   (at your option) any later version.
  *   This software is distributed in the hope that it will be useful,
  *   but WITHOUT ANY WARRANTY; without even the implied warranty of
  *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  *   GNU General Public License for more details.
  *   You should have received a copy of the GNU General Public License
  *   along with this software.  If not, see <http://www.gnu.org/licenses/>.
  */

#include "project.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <util/delay.h>

#include "fnppsu.h"
#include "i2c.h"

#define PSU_MODEL_LEN               0x00
#define PSU_MODEL                   0x01
#define PSU_SERIAL_LEN              0x12
#define PSU_SERIAL                  0x13
#define PSU_REV_LEN                 0x19
#define PSU_REV                     0x20
#define PSU_MFG_YEAR                0x24
#define PSU_MFG_MONTH               0x25
#define PSU_MFG_DAY                 0x26
#define PSU_MFG_NAME_LEN            0x27
#define PSU_MFG_NAME                0x28
#define PSU_HOURS_IN_SERVICE        0x86

#define OUTPUT1_MEAS_VOLTAGE_MSB    0x8A
#define OUTPUT1_MEAS_VOLTAGE_LSB    0x8B
#define OUTPUT1_MEAS_VOLTAGE_SCALE  0x8C

#define OUTPUT1_MEAS_CURRENT_MSB    0x96
#define OUTPUT1_MEAS_CURRENT_LSB    0x97
#define OUTPUT1_MEAS_CURRENT_SCALE  0x98

#define OUTPUT1_SET_VOLTAGE_MSB     0xA3
#define OUTPUT1_SET_VOLTAGE_LSB     0xA4
#define OUTPUT1_SET_VOLTAGE_SCALE   0xA5

#define psu_pow(value, exp) (exp == 0 ? (value / 10) : (exp == 1 ? value : (exp == 2 ? (value * 10) : value * 100)))
#define swap_32(x) (((x) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | ((x) << 24))

bool fnppsu_get_dev_info(uint8_t addr, fnppsu_dev_info_t *info)
{
    uint8_t i;
    uint8_t str_len;
    uint8_t tmp;
    
    /* Model */
    if (!i2c_read(addr, PSU_MODEL, &str_len))
        return false;
    
    str_len = str_len > FNPPSU_MAX_MODEL ? FNPPSU_MAX_MODEL : str_len;

    for (i = 0; i < str_len; i++)
    {
        if (!i2c_read(addr, PSU_MODEL + i, (uint8_t *)&info->model[i]))
            return false;
    }

    info->model[i] = 0;

    /* Serial */
    if (!i2c_read(addr, PSU_SERIAL_LEN, &str_len))
        return false;

    str_len = str_len > FNPPSU_MAX_SERIAL ? FNPPSU_MAX_SERIAL : str_len;

    for (i = 0; i < str_len; i++)
    {
        if (!i2c_read(addr, PSU_SERIAL + i, (uint8_t *)&info->serial[i]))
            return false;
    }

    info->serial[i] = 0;

    /* Rev */
    if (!i2c_read(addr, PSU_REV_LEN, &str_len))
        return false;

    str_len = str_len > FNPPSU_MAX_REV ? FNPPSU_MAX_REV : str_len;

    for (i = 0; i < str_len; i++)
    {
        if (!i2c_read(addr, PSU_REV + i, (uint8_t *)&info->rev[i]))
            return false;
    }

    info->rev[i] = 0;

    /* Mfg */
    if (!i2c_read(addr, PSU_MFG_NAME_LEN, &str_len))
        return false;

    str_len = str_len > FNPPSU_MAX_MFG ? FNPPSU_MAX_MFG : str_len;

    for (i = 0; i < str_len; i++)
    {
        if (!i2c_read(addr, PSU_MFG_NAME + i, (uint8_t *)&info->mfg[i]))
            return false;
    }

    info->mfg[i] = 0;

    if (!i2c_read(addr, PSU_MFG_YEAR, &tmp))
        return false;
    if (!i2c_read(addr, PSU_MFG_MONTH, &info->mfg_month))
        return false;
    if (!i2c_read(addr, PSU_MFG_DAY, &info->mfg_day))
        return false;

    info->mfg_year = tmp + 2000;

    if (!i2c_read_buf(addr, PSU_HOURS_IN_SERVICE, (uint8_t *)&info->hours_in_service, 4))
        return false;

    info->hours_in_service = swap_32(info->hours_in_service);
    
    return true;
}

bool fnppsu_output1_read_meas_voltage(uint8_t addr, uint16_t *result)
{
    uint8_t msb;
    uint8_t lsb;
    uint8_t scale;

    if (!i2c_read(addr, OUTPUT1_MEAS_VOLTAGE_MSB, &msb))
        return false;
    if (!i2c_read(addr, OUTPUT1_MEAS_VOLTAGE_LSB, &lsb))
        return false;
    if (!i2c_read(addr, OUTPUT1_MEAS_VOLTAGE_SCALE, &scale))
        return false;

    *result = psu_pow((uint16_t)(msb << 8 | lsb), scale);
    return true;
}

bool fnppsu_output1_read_meas_current(uint8_t addr, uint16_t *result)
{
    uint8_t msb;
    uint8_t lsb;
    uint8_t scale;

    if (!i2c_read(addr, OUTPUT1_MEAS_CURRENT_MSB, &msb))
        return false;
    if (!i2c_read(addr, OUTPUT1_MEAS_CURRENT_LSB, &lsb))
        return false;
    if (!i2c_read(addr, OUTPUT1_MEAS_CURRENT_SCALE, &scale))
        return false;

    *result = psu_pow((uint16_t)(msb << 8 | lsb), scale);
    return true;
}

bool fnppsu_output1_write_set_voltage(uint8_t addr, uint16_t voltage)
{
    uint8_t msb;
    uint8_t lsb;
    uint8_t scale;

    if (!i2c_read(addr, OUTPUT1_SET_VOLTAGE_SCALE, &scale))
        return false;

    if (scale != 0x00)
        return false;

    voltage *= 10;

    msb = (uint8_t)(voltage >> 8);
    lsb = (uint8_t)(voltage & 0xFF);

    if (!i2c_write(addr, OUTPUT1_SET_VOLTAGE_MSB, msb))
        return false;
    
    _delay_ms(8);

    if (!i2c_write(addr, OUTPUT1_SET_VOLTAGE_LSB, lsb))
        return false;
    
    _delay_ms(8);
    
    return true;
}

bool fnppsu_output1_read_set_voltage(uint8_t addr, uint16_t *result)
{
    uint8_t msb;
    uint8_t lsb;
    uint8_t scale;

    if (!i2c_read(addr, OUTPUT1_SET_VOLTAGE_MSB, &msb))
        return false;
    if (!i2c_read(addr, OUTPUT1_SET_VOLTAGE_LSB, &lsb))
        return false;
    if (!i2c_read(addr, OUTPUT1_SET_VOLTAGE_SCALE, &scale))
        return false;

    *result = psu_pow((uint16_t)(msb << 8 | lsb), scale);
    return true;
}


