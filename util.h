/* 
 *   File:   util.h
 *   Author: Matthew Millman
 *
 *   FNP600/850/1000 Adapter Board
 *
 *   Created on 09 August 2015, 14:30
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

#ifndef __UTIL_H__
#define	__UTIL_H__


void reset(void);
void eeprom_read_data(uint8_t addr, uint8_t *bytes, uint8_t len);
void eeprom_write_data(uint8_t addr, uint8_t *bytes, uint8_t len);
int print_char(char byte, FILE *stream);

#undef printf
#define printf(fmt, ...) printf_P(PSTR(fmt) __VA_OPT__(,) __VA_ARGS__)
#define sprintf(buf, fmt, ...) sprintf_P(buf, PSTR(fmt) __VA_OPT__(,) __VA_ARGS__)
#define strcmp_p(str, to) strcmp_P(str, PSTR(to))
#define strcpy_p(str, to) strcpy_P(str, PSTR(to))
#define strncmp_p(str, to, n) strncmp_P(str, PSTR(to), n)
#define stricmp_p(str, to) strcasecmp_P(str, PSTR(to))
#define stricmp strcasecmp

#define _1DP_BASE 10
#define _2DP_BASE 100

#define I_1DP               0
#define I_2DP               1
#define U_1DP               4
#define U_2DP               5

#define fixedpoint_sign(value, tag) \
    char tag##_sign[2]; \
    tag##_sign[1] = 0; \
    if (value < 0 ) \
        tag##_sign[0] = '-'; \
    else \
        tag##_sign[0] = 0; \

#define format_i16_1dp(buf, value) format_fixedpoint(buf, (int16_t)value, I_1DP)
#define format_u16_1dp(buf, value) format_fixedpoint(buf, (int16_t)value, U_1DP)

#define fixedpoint_arg(value, tag) tag##_sign, (abs(value) / _1DP_BASE), (abs(value) % _1DP_BASE)
#define fixedpoint_arg_u(value) (value / _1DP_BASE), (value % _1DP_BASE)
#define fixedpoint_arg_u_2dp(value) (value / _2DP_BASE), (value % _2DP_BASE)

#define max_(x, y) (x > y ? x : y)
#define min_(x, y) (x < y ? x : y)

#endif	/* __UTIL_H__ */

