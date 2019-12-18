/*
 *   File:   timeout.h
 *   Author: Matthew Millman
 *
 *   FNP600/850/1000 Adapter Board
 *
 *   Created on 19 May 2018, 13:46
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

#ifndef __TIMEOUT_H__
#define __TIMEOUT_H__

void timeout_init(void);
void timeout_check(void);
int8_t timeout_create(uint32_t interval, bool start, bool reapeat, void (*callback)(void *), void *data);
void timeout_destroy(int8_t index);
void timeout_start(int8_t index);
void timeout_stop(int8_t index);
int32_t get_tick_count(void);

#endif /* __TIMEOUT_H__ */