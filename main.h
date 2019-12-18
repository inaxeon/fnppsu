/*
 *   File:   main.h
 *   Author: Matthew Millman
 *
 *   FNP600/850/1000 Adapter Board
 *
 *   Created on 05 December 2019, 07:43
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

#ifndef __MAIN_H__
#define __MAIN_H__

typedef struct
{
    sys_config_t *config;
    uint8_t psu_addrs[MAX_PSU];
    uint8_t psu_num;
    bool outvoltage_stale;
} sys_runstate_t;

bool psu_adjust_voltages(sys_runstate_t *rs);
bool psu_change_state(sys_runstate_t *rs, bool on);

#endif /* __MAIN_H__ */
