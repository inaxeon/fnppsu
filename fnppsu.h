 /*
  *   File:   fnppsu.h
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

#ifndef __FNPPSU_H__
#define __FNPPSU_H__

#include "fnppsu.h"

#include <stdint.h>
#include <stdbool.h>

#define FNPPSU_I2C_ADDR_MIN     0x41
#define FNPPSU_I2C_ADDR_MAX     0x60

#define FNPPSU_MAX_MFG          9
#define FNPPSU_MAX_MODEL        17
#define FNPPSU_MAX_SERIAL       12
#define FNPPSU_MAX_REV          4

typedef struct {
    char mfg[FNPPSU_MAX_MODEL + 1];
    char model[FNPPSU_MAX_MODEL + 1];
    char serial[FNPPSU_MAX_SERIAL + 1];
    char rev[FNPPSU_MAX_REV + 1];
    uint16_t mfg_year;
    uint8_t mfg_month;
    uint8_t mfg_day;
    uint32_t hours_in_service;
} fnppsu_dev_info_t;

bool fnppsu_get_dev_info(uint8_t addr, fnppsu_dev_info_t *info);
bool fnppsu_output1_read_meas_voltage(uint8_t addr, uint16_t *result);
bool fnppsu_output1_read_meas_current(uint8_t addr, uint16_t *result);
bool fnppsu_output1_write_set_voltage(uint8_t addr, uint16_t voltage);
bool fnppsu_output1_read_set_voltage(uint8_t addr, uint16_t *result);

#endif /* __fnppsu_H__ */
