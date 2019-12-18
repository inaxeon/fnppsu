/*
 *   File:   main.c
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

#include "project.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/io.h>
#include <avr/wdt.h> 
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "config.h"
#include "main.h"
#include "i2c.h"
#include "usart_buffered.h"
#include "util.h"
#include "cmd.h"
#include "lcd.h"
#include "fnppsu.h"
#include "timeout.h"

#define MAX_DESC           8
#define PS_ON_DELAY_MS     500

char _g_dotBuf[MAX_DESC];

sys_config_t _g_cfg;
sys_runstate_t _g_rs;

FILE uart_str = FDEV_SETUP_STREAM(print_char, NULL, _FDEV_SETUP_RW);

static void io_init(void);
static void update_lcd(void *param);
static bool psu_init(sys_runstate_t *rs);
static uint8_t psu_find(uint8_t *addrs);

int main(void)
{
    sys_runstate_t *rs = &_g_rs;
    sys_config_t *config = &_g_cfg;
    rs->config = config;
    rs->outvoltage_stale = false;

    io_init();
    wdt_enable(WDTO_1S);
    g_irq_enable();
    i2c_init(400);

    usart1_open(USART_CONT_RX, (((F_CPU / UART1_BAUD) / 16) - 1));
    stdout = &uart_str;

    printf("\r\nStarting up...\r\n");

    lcd_init();

    load_configuration(rs->config);

    if (rs->config->start_mode)
        psu_init(rs);
    else
        printf("Output disabled. Not probing attached power supplies\r\n");

    printf("Found %u of max %u attached power supplies\r\n", rs->psu_num, MAX_PSU);

    timeout_init();
    timeout_create(500, true, true, &update_lcd, (void *)rs);

    cmd_init();

    // Idle loop
    for (;;) {
        timeout_check();
        cmd_process(rs);
        lcd_process();
        CLRWDT();
    }
}

static void psu_enable(bool enable)
{
    if (enable) {
        if (PS_ON_STATE)
            return;

        printf("Enabling output...\r\n");
        PS_ON_PORT &= ~_BV(PS_ON); // On
    } else {
        if (!PS_ON_STATE)
            return;

        printf("Disabling output...\r\n");
        PS_ON_PORT |= _BV(PS_ON); // Off
    }
}

static bool psu_init(sys_runstate_t *rs)
{
    psu_enable(true);
    _delay_ms(PS_ON_DELAY_MS);
    CLRWDT();

    rs->psu_num = psu_find(rs->psu_addrs);

    if (rs->config->expected_psus && rs->psu_num < rs->config->expected_psus) {
        printf("Error: Number of power supplies detected (%u) does not match expected number (%u)\r\n",
            rs->psu_num, rs->config->expected_psus);
        psu_enable(false);
        return false;
    }

    psu_adjust_voltages(rs);
    return rs->psu_num > 0;
}

static uint8_t psu_find(uint8_t *addrs)
{
    uint8_t addr;
    uint8_t idx = 0;

    printf("\r\n");

    for (addr = FNPPSU_I2C_ADDR_MIN; addr <= FNPPSU_I2C_ADDR_MAX; addr++) {
        fnppsu_dev_info_t info;

        if (idx >= MAX_PSU) {
            printf("Maximum number of supported power supplies found. Aborting\r\n");
            return idx;
        }

        if (fnppsu_get_dev_info(addr, &info)) {
            printf("Detected PSU @ 0x%02X\r\n", addr);
            printf("Manufacturer     : %s\r\n", info.mfg);
            printf("Model            : %s\r\n", info.model);
            printf("Serial           : %s\r\n", info.serial);
            printf("Rev              : %s\r\n", info.rev);
            printf("Mfg. Date        : %u.%u.%u\r\n", info.mfg_year, info.mfg_month, info.mfg_day);
            printf("Hours in service : %lu\r\n\r\n", info.hours_in_service);

            addrs[idx++] = addr;
        }

        CLRWDT();
    }

    return idx;
}

bool psu_adjust_voltages(sys_runstate_t *rs)
{
    uint8_t i;
    bool adjusted = false;

    for (i = 0; i < rs->psu_num; i++) {
        uint8_t addr = rs->psu_addrs[i];
        uint16_t sv;

        if (!fnppsu_output1_read_set_voltage(addr, &sv)) {
            printf("Error reading set voltage from PSU @ 0x%02X\r\n", addr);
            return false;
        }

        if (sv != rs->config->output_voltage) {
            printf("Changing set voltage for PSU @ 0x%02X from %u.%02u to %u.%02u\r\n",
                addr, fixedpoint_arg_u_2dp(sv), fixedpoint_arg_u_2dp(rs->config->output_voltage));

            if (!fnppsu_output1_write_set_voltage(addr, rs->config->output_voltage)) {
                printf("Error changing set voltage on PSU @ 0x%02X\r\n", addr);
                return false;
            }

            adjusted = true;
        }
    }

    if (adjusted) {
        psu_enable(false);
        for (i = 0; i < 5; i++) {
            _delay_ms(100);
            CLRWDT();
        }
        psu_enable(true);
    }

    return true;
}

bool psu_change_state(sys_runstate_t *rs, bool on)
{
    if (on) {
        if (!PS_ON_STATE && !rs->psu_num)
            return psu_init(rs);
        else if (!PS_ON_STATE && rs->psu_num) {
            psu_enable(true);
            if (rs->outvoltage_stale) {
                psu_adjust_voltages(rs);
                rs->outvoltage_stale = false;
            }
        }
    }
    else {
        psu_enable(false);
    }

    return true;
}

static void io_init(void)
{
    PS_ON_PORT |= _BV(PS_ON); // Off
    PS_ON_DDR |= _BV(PS_ON);
}

static void update_lcd(void *param)
{
    sys_runstate_t *rs = (sys_runstate_t *)param;
    uint16_t display_voltage = 0;
    uint16_t total_amps = 0;
    int len;

    if (PS_ON_STATE && rs->psu_num) {
        for (uint8_t i = 0; i < rs->psu_num; i++) {
            uint16_t volts;
            uint16_t amps;
            uint8_t addr = rs->psu_addrs[i];

            if (rs->config->show_measured_volts) {
                if (!fnppsu_output1_read_meas_voltage(addr, &volts)) {
                    printf("Error reading voltage from PSU @ 0x%02X\r\n", addr);
                    goto i2cerror;
                }
            }
            if (!fnppsu_output1_read_meas_current(addr, &amps)) {
                printf("Error reading current from PSU @ 0x%02X\r\n", addr);
                goto i2cerror;
            }

            display_voltage += volts;
            total_amps += amps;
        }

        if (rs->config->show_measured_volts)
            display_voltage /= rs->psu_num;
        else
            display_voltage = rs->config->output_voltage;
        
        memset(_g_lcd_data[LCD_ROW1], 0x20, LCD_COLS);
        memset(_g_lcd_data[LCD_ROW2], 0x20, LCD_COLS);

        _g_lcd_data[LCD_ROW1][LCD_COLS - 1] = 'V';
        _g_lcd_data[LCD_ROW2][LCD_COLS - 1] = 'A';

        len = sprintf(_g_lcd_data[LCD_ROW1], "%u.%02u", fixedpoint_arg_u_2dp(display_voltage));
        _g_lcd_data[LCD_ROW1][len] = 0x20; // Remove null terminator

        len = sprintf(_g_lcd_data[LCD_ROW2], "%u.%02u", fixedpoint_arg_u_2dp(total_amps));
        _g_lcd_data[LCD_ROW2][len] = 0x20; // Remove null terminator

        goto done;
    }
    
    if (!PS_ON_STATE && rs->psu_num) {
        strcpy_p(_g_lcd_data[LCD_ROW1], "OUTPUT");
        strcpy_p(_g_lcd_data[LCD_ROW2], "OFF");
        goto done;
    }
    
    if (!rs->psu_num) {
        strcpy_p(_g_lcd_data[LCD_ROW1], "NO PSUS");
        strcpy_p(_g_lcd_data[LCD_ROW2], "");
        goto done;
    }

i2cerror:
    strcpy_p(_g_lcd_data[LCD_ROW1], "I2C");
    strcpy_p(_g_lcd_data[LCD_ROW2], "ERROR");
done:
    lcd_start_update();
}
