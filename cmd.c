/* 
 *   File:   cmd.c
 *   Author: Matthew Millman
 *
 *   FNP600/850/1000 Adapter Board
 *
 *   Created on 05 December 2019, 21:46
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <avr/pgmspace.h>

#include "config.h"
#include "main.h"
#include "cmd.h"
#include "usart_buffered.h"
#include "util.h"
#include "i2c.h"
#include "lcd.h"
#include "fnppsu.h"

#define CMD_NONE              0x00
#define CMD_READLINE          0x01
#define CMD_COMPLETE          0x02
#define CMD_ESCAPE            0x03
#define CMD_AWAIT_NAV         0x04
#define CMD_PREVCOMMAND       0x05
#define CMD_NEXTCOMMAND       0x06
#define CMD_DEL               0x07
#define CMD_DROP_NAV          0x08
#define CMD_CANCEL            0x09

#define CMD_MEASURE           0x0A
#define CMD_ON                0x0B
#define CMD_OFF               0x0C

#define CTL_CANCEL            0x03
#define CTL_E                 0x05
#define CTL_S                 0x13
#define CTL_XOFF              0x13
#define CTL_U                 0x15

#define SEQ_ESCAPE_CHAR       0x1B
#define SEQ_CTRL_CHAR1        0x5B
#define SEQ_ARROW_UP          0x41
#define SEQ_ARROW_DOWN        0x42
#define SEQ_HOME              0x31
#define SEQ_INS               0x32
#define SEQ_DEL               0x33
#define SEQ_END               0x34
#define SEQ_PGUP              0x35
#define SEQ_PGDN              0x36
#define SEQ_NAV_END           0x7E

#define PARAM_U8              0
#define PARAM_U16             1
#define PARAM_U8_BIT          2
#define PARAM_U8_MAXPSU       3
#define PARAM_U16_2DP_OUTVOLT 4

#define CMD_MAX_CONSOLE       1
#define CMD_MAX_LINE          64
#define CMD_MAX_HISTORY       4

#define CONSOLE_1             0

typedef struct
{
    char cmd_buf[CMD_MAX_LINE];
    char cmd_history[CMD_MAX_HISTORY][CMD_MAX_LINE];
    uint8_t next_history;
    uint8_t show_history;
    uint8_t max_history;
    int8_t count;
    uint8_t state;
    uint8_t ignore_lf;
} cmd_state_t;

static void cmd_process_char(uint8_t c, uint8_t idx);
static void cmd_prompt(cmd_state_t *ccmd);
static void cmd_erase_line(cmd_state_t *ccmd);
static bool do_measure(sys_runstate_t *rs);
static void do_show(sys_config_t *config);
static void do_default_config(sys_runstate_t *rs);
static bool parse_param(void *param, uint8_t type, char *arg);

uint8_t _g_current_console;
cmd_state_t _g_cmd[CMD_MAX_CONSOLE];

static void do_help(void)
{
    printf(
        "\r\nCommands:\r\n\r\n"
        "\tmeasure|ctrl+e\r\n"
        "\t\tShow the measured output voltage/current readings\r\n\r\n"
        "\toutvoltage|o [%u.%02u to %u.%02u]\r\n"
        "\t\tSets the output voltage of the attached power supplies\r\n\r\n"
        "\ton|<pgup>\r\n"
        "\t\tEnable the main power output\r\n\r\n"
        "\toff|<pgdn>\r\n"
        "\t\tDisable the main power output\r\n\r\n"
        "\tstartmode [0 or 1]\r\n"
        "\t\tSet to '1' if the power output is to be ON after AC power on\r\n\r\n"
        "\texpectedpsus [0 to %u]\r\n"
        "\t\tDo not power up unless N number of power supplies are detected\r\n"
        "\t\tSet to 0 to disable this check\r\n\r\n"
        "\tmeasuredvoltage [0 or 1]\r\n"
        "\t\tSet to '1' to show the measured voltage on the LCD instead of\r\n"
        "\t\tconfigured voltage\r\n\r\n"
        "\tshow\r\n"
        "\t\tShow the persisted configuration\r\n\r\n"
        "\tdefault\r\n"
        "\t\tLoad the default configuration\r\n\r\n"
        "\treset\r\n"
        "\t\tReset this board\r\n\r\n",
        fixedpoint_arg_u_2dp(OUTPUT_VOLTAGE_MIN),
        fixedpoint_arg_u_2dp(OUTPUT_VOLTAGE_MAX),
        MAX_PSU
    );
}

static bool command_prompt_handler(sys_runstate_t *rs, char *text)
{
    char *command;
    char *arg;
    bool ret;

    command = strtok(text, " ");
    arg = strtok(NULL, "");

    if (!stricmp(command, "measure")) {
        return do_measure(rs);
    }
    else if (!stricmp(command, "outvoltage") || !stricmp(command, "o")) {
        ret = parse_param(&rs->config->output_voltage, PARAM_U16_2DP_OUTVOLT, arg);
        if (ret)
        {
            save_configuration(rs->config);

            if (PS_ON_STATE && rs->psu_num)
            {
                psu_adjust_voltages(rs);
            }
            else
            {
                printf("Configuration saved but no supplies were updated\r\n");
                printf("Either none were present or output disabled\r\n");
                rs->outvoltage_stale = true;
            }
        }
        return ret;
    }
    else if (!stricmp(command, "startmode")) {
        ret = parse_param(&rs->config->start_mode, PARAM_U8_BIT, arg);
        if (ret)
            save_configuration(rs->config);
        return ret;
    }
    else if (!stricmp(command, "expectedpsus")) {
        ret = parse_param(&rs->config->expected_psus, PARAM_U8_MAXPSU, arg);
        if (ret)
            save_configuration(rs->config);
        return ret;
    }
    else if (!stricmp(command, "measuredvoltage")) {
        ret = parse_param(&rs->config->show_measured_volts, PARAM_U8_BIT, arg);
        if (ret)
            save_configuration(rs->config);
        return ret;
    }
    else if (!stricmp(command, "on")) {
        return psu_change_state(rs, true);
    }
    else if (!stricmp(command, "off")) {
        return psu_change_state(rs, false);
    }
    else if (!stricmp(command, "reset")) {
        printf("\r\n");
        reset();
        return true;
    }
    else if (!stricmp(command, "show")) {
        do_show(rs->config);
        return true;
    }
    else if (!stricmp(command, "default")) {
        do_default_config(rs);
        return true;
    }
    else if ((!stricmp(command, "help") || !stricmp(command, "?"))) {
        do_help();
        return true;
    }

    printf("Error: No such command (%s)\r\n", command);
    return false;
}

static void do_show(sys_config_t *config)
{
    printf(
            "\r\nCurrent configuration:\r\n\r\n"
            "\toutvoltage ...........: %u.%02u\r\n"
            "\tstartmode ............: %u\r\n"
            "\texpectedpsus .........: %u\r\n"
            "\tmeasuredvoltage ......: %u\r\n"
            "\r\n",
                fixedpoint_arg_u_2dp(config->output_voltage),
                config->start_mode,
                config->expected_psus,
                config->show_measured_volts
            );
}

static void do_default_config(sys_runstate_t *rs)
{
    default_configuration(rs->config);
    save_configuration(rs->config);

    if (PS_ON_STATE && rs->psu_num)
        psu_adjust_voltages(rs);
    else
        rs->outvoltage_stale = true;

    printf("Default configuration loaded\r\n");
}

static bool do_measure(sys_runstate_t *rs)
{
    uint8_t i;
    uint16_t average_voltage = 0;
    uint16_t total_amps = 0;

    if (!PS_ON_STATE) {
        printf("Error: Output is currently switched off\r\n");
        return false;
    }

    if (!rs->psu_num) {
        printf("Error: No power supplies detected\r\n");
        return false;
    }

    for (i = 0; i < rs->psu_num; i++)
    {
        uint16_t volts;
        uint16_t amps;
        uint8_t addr = rs->psu_addrs[i];

        if (!fnppsu_output1_read_meas_voltage(addr, &volts)) {
            printf("Error reading voltage from PSU @ 0x%02X\r\n", addr);
            continue;
        }

        if (!fnppsu_output1_read_meas_current(addr, &amps)) {
            printf("Error reading current from PSU @ 0x%02X\r\n", addr);
            continue;
        }

        average_voltage += volts;
        total_amps += amps;

        printf("PSU @ 0x%02X:\r\n", addr);
        printf("Voltage : %u.%02u V\r\n", fixedpoint_arg_u_2dp(volts));
        printf("Current : %u.%02u A\r\n\r\n", fixedpoint_arg_u_2dp(amps));

    }

    average_voltage /= rs->psu_num;

    printf("Total   : Average voltage / Sum of current\r\n");
    printf("Voltage : %u.%02u V\r\n", fixedpoint_arg_u_2dp(average_voltage));
    printf("Current : %u.%02u A\r\n\r\n", fixedpoint_arg_u_2dp(total_amps));

    return true;
}

static bool parse_param(void *param, uint8_t type, char *arg)
{
    int16_t i16param;
    uint8_t u8param;
    uint8_t dp = 0;
    uint8_t un = 0;
    uint8_t dpmul = 1;
    char *s;
 
    if (!arg || !*arg) {
        printf("Error: Missing parameter\r\n");
        return false;
    }

    switch (type)
    {
    case PARAM_U8:
    case PARAM_U8_BIT:
    case PARAM_U8_MAXPSU:
        if (*arg == '-')
            return false;
        u8param = (uint8_t)atoi(arg);
        if (type == PARAM_U8_BIT && u8param > 1)
            return false;
        if (type == PARAM_U8_MAXPSU && u8param > MAX_PSU)
            return false;
        *(uint8_t *)param = u8param;
        break;
    case PARAM_U16:
    case PARAM_U16_2DP_OUTVOLT:
        // Note to self: All this arse about face dealing with fixed point integers
        // is not necessary anymore. Do not copy this crap to another project.
        s = strtok(arg, ".");
        i16param = atoi(s);

        switch (type)
        {
            case PARAM_U16:
                un = 1;
                break;
            case PARAM_U16_2DP_OUTVOLT:
                i16param *= _2DP_BASE;
                dp = 2;
                un = 1;
                dpmul = 10;
                if (i16param > OUTPUT_VOLTAGE_MAX || i16param < OUTPUT_VOLTAGE_MIN)
                    return 1;
                break;
        }

        if (un && *arg == '-')
            return 1;

        s = strtok(NULL, "");

        if (s && *s != 0) {
            if (dp == 2 && strlen(s) > 2)
                return 1;
            
            if (*arg == '-') {
                if (strlen(s) == 1)
                    i16param -= atoi(s) * (dpmul / 1);
                else
                    i16param -= atoi(s) * (dpmul / 10);
            } else {
                if (strlen(s) == 1)
                    i16param += atoi(s) * (dpmul / 1);
                else
                    i16param += atoi(s) * (dpmul / 10);
            }
        }
        if (type == PARAM_U16_2DP_OUTVOLT) {
            if (i16param < OUTPUT_VOLTAGE_MIN)
                i16param = OUTPUT_VOLTAGE_MIN;
            if (i16param > OUTPUT_VOLTAGE_MAX)
                i16param = OUTPUT_VOLTAGE_MAX;
        }
        *(int16_t *)param = i16param;
        break;
    }
    
    return true;
}

void putch(char byte)
{
#ifdef _CONSOLE1_
    if (_g_current_console == CONSOLE_1)
        console1_put(byte);
#endif /* _CONSOLE1_ */

#ifdef _CONSOLE2_
    if (_g_current_console == CONSOLE_2)
        console2_put(byte);
#endif /* _CONSOLE1_ */
}

void cmd_init(void)
{
    uint8_t i;

    for (i = 0; i < CMD_MAX_CONSOLE; i++)
    {
        _g_cmd[i].count = 0;
        _g_cmd[i].state = CMD_NONE;
        _g_cmd[i].next_history = 0;
        _g_cmd[i].show_history = 0;
        _g_cmd[i].max_history = 0;
        _g_cmd[i].ignore_lf = 0;
        memset(_g_cmd[i].cmd_history, 0, CMD_MAX_HISTORY * CMD_MAX_LINE);
        memset(_g_cmd[i].cmd_buf, 0, CMD_MAX_LINE);
    }
    
    _g_current_console = CONSOLE_1;
}

void cmd_process(sys_runstate_t *rs)
{
    uint8_t idx, i;

#ifdef _CONSOLE1_
    while (console1_data_ready())
    {
        char c = console1_get();
        cmd_process_char(c, CONSOLE_1);
    }
#endif /* _CONSOLE1_ */

#ifdef _CONSOLE2_
    while (console2_data_ready())
    {
        char c = console2_get();
        cmd_process_char(c, CONSOLE_2);
    }
#endif /* _CONSOLE1_ */

    for (idx = 0; idx < CMD_MAX_CONSOLE; idx++)
    {
        cmd_state_t *ccmd = &_g_cmd[idx];
        _g_current_console = idx;
        
        if (ccmd->state == CMD_NONE) {
            printf("\r\n");
            cmd_prompt(ccmd);
        } else if (ccmd->state == CMD_PREVCOMMAND) {
            uint8_t previdx;

            if (!ccmd->max_history) {
                ccmd->state = CMD_READLINE;
                return;
            }

            if (ccmd->count)
                cmd_erase_line(ccmd);

            if (ccmd->show_history == 0)
                ccmd->show_history = CMD_MAX_HISTORY;

            previdx = --ccmd->show_history;

            strcpy(ccmd->cmd_buf, ccmd->cmd_history[previdx]);
            ccmd->count = strlen(ccmd->cmd_buf);
            printf("%s", ccmd->cmd_buf);

            ccmd->state = CMD_READLINE;

        }
        else if (ccmd->state == CMD_NEXTCOMMAND)
        {
            uint8_t previdx;

            if (!ccmd->max_history) {
                ccmd->state = CMD_READLINE;
                return;
            }

            if (ccmd->count)
                cmd_erase_line(ccmd);

            previdx = ++ccmd->show_history;

            if (ccmd->show_history == CMD_MAX_HISTORY) {
                ccmd->show_history = 0;
                previdx = 0;
            }

            strcpy(ccmd->cmd_buf, ccmd->cmd_history[previdx]);
            ccmd->count = strlen(ccmd->cmd_buf);
            printf("%s", ccmd->cmd_buf);

            ccmd->state = CMD_READLINE;
        }
        else if (ccmd->state == CMD_COMPLETE) {
            ccmd->cmd_buf[ccmd->count] = 0;
            printf("\r\n");
            
            if (ccmd->count > 0) {
                int8_t tostore = -1;
				bool ret;

				if (ccmd->next_history >= CMD_MAX_HISTORY)
					ccmd->next_history = 0;
				else
					ccmd->max_history++;

				for (i = 0; i < CMD_MAX_HISTORY; i++) {
					if (!stricmp(ccmd->cmd_history[i], ccmd->cmd_buf)) {
						tostore = i;
						break;
					}
				}

				if (tostore < 0) {
					// Don't have this command in history. Store it
					strcpy(ccmd->cmd_history[ccmd->next_history], ccmd->cmd_buf);
					ccmd->next_history++;
					ccmd->show_history = ccmd->next_history;
				} else {
					// Already have this command in history, set the 'up' arrow to retrieve it.
					tostore++;

					if (tostore == CMD_MAX_HISTORY)
						tostore = 0;

					ccmd->show_history = tostore;
				}
                
                ret = command_prompt_handler(rs, ccmd->cmd_buf);

                if (!ret)
					printf("Error: Command failed\r\n");
            }
            
            cmd_prompt(ccmd);
        }
        else if (ccmd->state == CMD_CANCEL)
        {
            ccmd->cmd_buf[ccmd->count] = 0;
            printf("\r\n");
            cmd_prompt(ccmd);
        }
        else if (ccmd->state == CMD_ON) {
            if (rs->psu_num) {
                printf("\r\n");
                psu_change_state(rs, true);
                cmd_prompt(ccmd);
            }

            ccmd->state = CMD_DROP_NAV;
        }
        else if (ccmd->state == CMD_OFF) {
            if (rs->psu_num) {
                printf("\r\n");
                psu_change_state(rs, false);
                cmd_prompt(ccmd);
            }
            
            ccmd->state = CMD_DROP_NAV;
        }
        else if (ccmd->state == CMD_MEASURE) {
            printf("\r\n");
            do_measure(rs);
            cmd_prompt(ccmd);
        }
    }
}

static void cmd_erase_line(cmd_state_t *ccmd)
{
    printf("%c[%dD%c[K", SEQ_ESCAPE_CHAR, ccmd->count, SEQ_ESCAPE_CHAR);
}

static void cmd_prompt(cmd_state_t *ccmd)
{
    ccmd->state = CMD_READLINE;
    ccmd->count = 0;
    printf("cmd>");
}

void cmd_process_char(uint8_t c, uint8_t idx)
{
    cmd_state_t *ccmd = &_g_cmd[idx];
    _g_current_console = idx;
    
    if (ccmd->state == CMD_ESCAPE) {
        if (c == SEQ_CTRL_CHAR1) {
            ccmd->state = CMD_AWAIT_NAV;
            return;
        }
        else {
            ccmd->state = CMD_READLINE;
            return;
        }
    }
    else if (ccmd->state == CMD_AWAIT_NAV)
    {
        if (c == SEQ_ARROW_UP) {
            ccmd->state = CMD_PREVCOMMAND;
            return;
        }
        else if (c == SEQ_ARROW_DOWN) {
            ccmd->state = CMD_NEXTCOMMAND;
            return;
        }
        else if (c == SEQ_DEL) {
            ccmd->state = CMD_DEL;
            return;
        }
        else if (c == SEQ_PGUP) {
            ccmd->state = CMD_ON;
            return;
        }
        else if (c == SEQ_PGDN) {
            ccmd->state = CMD_OFF;
            return;
        }
        else if (c == SEQ_HOME || c == SEQ_END || c == SEQ_INS) {
            ccmd->state = CMD_DROP_NAV;
            return;
        }
        else {
            ccmd->state = CMD_READLINE;
            return;
        }
    }
    else if (ccmd->state == CMD_DEL) {
        if (c == SEQ_NAV_END && ccmd->count) {
            putch('\b');
            putch(' ');
            putch('\b');
            ccmd->count--;
        }

        ccmd->state = CMD_READLINE;
        return;
    }
    else if (ccmd->state == CMD_DROP_NAV) {
        ccmd->state = CMD_READLINE;
        return;
    }
    else
    {
        if (ccmd->count >= (int8_t)sizeof(ccmd->cmd_buf)) {
            ccmd->count--;
            ccmd->state = CMD_COMPLETE;
            return;
        }

        if (c == CTL_XOFF) /* Swallow XOFF */
            return;
        
		if (c == CTL_U) {
			if (ccmd->count) {
				cmd_erase_line(ccmd);
				*(ccmd->cmd_buf) = 0;
				ccmd->count = 0;
			}
			return;
		}

        if (c == SEQ_ESCAPE_CHAR) {
            ccmd->state = CMD_ESCAPE;
            return;
        }

        /* Unix telnet sends:    <CR> <NUL>
         * Windows telnet sends: <CR> <LF>
         */
        if (ccmd->ignore_lf && (c == '\n' || c == 0x00)) {
            ccmd->ignore_lf = 0;
            return;
        }

        if (c == CTL_CANCEL) { /* Ctrl+C */
            ccmd->state = CMD_CANCEL;
            return;
        }

        if (c == CTL_E) { /* Ctrl+E */ 
            ccmd->state = CMD_MEASURE;
            return;
        }

        if (c == '\b' || c == 0x7F) {
            if (!ccmd->count)
                return;

            putch('\b');
            putch(' ');
            putch('\b');
            ccmd->count--;
            return;
        }
        if (c != '\n' && c != '\r') {
            putch(c);
        }
        else {
            if (c == '\r') {
                ccmd->ignore_lf = 1;
                ccmd->state = CMD_COMPLETE;
                return;
            }

            if (c == '\n') {
                ccmd->state = CMD_COMPLETE;
                return;
            }
        }

        ccmd->cmd_buf[ccmd->count] = c;
        ccmd->count++;
    }
}
