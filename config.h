/* 
 *   File:   config.h
 *   Author: Matthew Millman
 *
 *   FNP600/850/1000 Adapter Board
 *
 *   Created on 05 December 2019, 07:55
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

#ifndef __CONFIG_H__
#define __CONFIG_H__

typedef struct {
    uint16_t magic;
    uint16_t output_voltage;
    uint8_t start_mode;
    uint8_t expected_psus;
    uint8_t show_measured_volts;
} sys_config_t;

void configuration_bootprompt(sys_config_t *config);
void load_configuration(sys_config_t *config);
void save_configuration(sys_config_t *config);
void default_configuration(sys_config_t *config);
int8_t configuration_prompt_handler(char *message, sys_config_t *config, bool sms);

#endif /* __CONFIG_H__ */