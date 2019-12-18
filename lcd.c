/* 
 *   File:   lcd.c
 *   Author: Matthew Millman
 *
 *   FNP600/850/1000 Adapter Board
 *
 *   Created on 05 December 2019, 21:46
 *
 *   Asynchronous HD44780 driver
 *
 *   With the clock of this IC typically only a few hundred kilohertz
 *   it can take quite a while to write stuff to the display, holding
 *   our CPU when it could be doing something useful. 
 *   
 *   Not good for this project which has a full-time CLI which needs to
 *   be responsive to the user. In this driver the CPU is able to go do
 *   other stuff while we're waiting for commands to complete.
 *
 *   Additionally the data path is 8-bit bi-directional, so we can get the
 *   data into the display as fast as possible and poll the 'busy' bit.
 *   No need for arbitrary, larger than necessary delays.
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
#include <string.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/io.h>

#include "lcd.h"

#define LCD_CMD_ADDR        0x00
#define LCD_DATA_ADDR       0x01

#define CMD_CLEAR           0x01
#define CMD_HOME            0x02
#define CMD_ONOFF           0x08
#define CMD_FUNCTIONSET     0x20

#define DISP_ON             0x04
#define MODE_8BIT           0x10
#define MODE_2LINE          0x08

#define CMD_DDADDR          0x80
#define CMD_CGADDR          0x40

#define STATUS_BUSY         0x80

#define DDRAM_LINE1_OFFSET  0x00
#define DDRAM_LINE2_OFFSET  0x40

static inline void lcd_set_gpio_out(void);
static inline void lcd_set_gpio_in(void);
static uint8_t lcd_read_byte(uint8_t reg);
static void lcd_write_byte(uint8_t reg, uint8_t data);

#define LCD_CMD(cmd) do { \
        lcd_write_byte(LCD_CMD_ADDR, cmd); \
        while (lcd_read_byte(LCD_CMD_ADDR) & STATUS_BUSY); \
} while (0)

#define LCD_DATA(cmd) do { \
        lcd_write_byte(LCD_DATA_ADDR, cmd); \
        while (lcd_read_byte(LCD_CMD_ADDR) & STATUS_BUSY); \
} while (0)

char _g_lcd_data[LCD_ROWS][LCD_COLS + 1];
static uint8_t _g_lcd_cur_row;
static uint8_t _g_lcd_cur_col;
static bool _g_lcd_updating;

void lcd_init(void)
{
    // Data GPIO init
    lcd_set_gpio_out();

    // Control GPIO init
    LCD_E_PORT &= ~_BV(LCD_E);
    LCD_RW_DDR |= _BV(LCD_RW);
    LCD_RS_DDR |= _BV(LCD_RS);
    LCD_E_DDR |= _BV(LCD_E);

    // Wait for power up
    while (lcd_read_byte(LCD_CMD_ADDR) & STATUS_BUSY);
    
    // Initialise
	LCD_CMD(CMD_FUNCTIONSET | MODE_8BIT | MODE_2LINE);
	LCD_CMD(CMD_FUNCTIONSET | MODE_8BIT | MODE_2LINE);
	LCD_CMD(CMD_ONOFF | DISP_ON);
	LCD_CMD(CMD_CLEAR);
	LCD_CMD(CMD_HOME);
}

void lcd_start_update(void)
{
    uint8_t i, j;

    if (_g_lcd_updating)
        return; // Already updating the display

    _g_lcd_cur_row = 0;
    _g_lcd_cur_col = 0;

    // Pad out the rest of the lines with spaces, so we clear anything that was there
    for (i = 0; i < LCD_ROWS; i++)
    {
        for (j = 0; j < LCD_COLS; j++)
        {
            if (!_g_lcd_data[i][j])
                break;
        }
        memset(&_g_lcd_data[i][j], 0x20, LCD_COLS - j);
    }

    lcd_write_byte(LCD_CMD_ADDR, CMD_DDADDR | DDRAM_LINE1_OFFSET);

    _g_lcd_updating = true;
}

void lcd_process(void)
{
    if (!_g_lcd_updating)
        return; // Nothing happening here. Back to the idle loop.

    if (lcd_read_byte(LCD_CMD_ADDR) & STATUS_BUSY)
        return; // LCD busy. Back to the idle loop.
    
    if (_g_lcd_cur_row == LCD_ROWS)
    {
        // All rows written out. Finished.
        _g_lcd_updating = false;
    }
    else if (_g_lcd_cur_col == LCD_COLS)
    {
        // Move down a row
        lcd_write_byte(LCD_CMD_ADDR, CMD_DDADDR | DDRAM_LINE2_OFFSET);
        _g_lcd_cur_col = 0;
        _g_lcd_cur_row++;
        _g_lcd_updating = true;
    }
    else
    {
        // Write char and move across a column
        lcd_write_byte(LCD_DATA_ADDR, _g_lcd_data[_g_lcd_cur_row][_g_lcd_cur_col]);
        _g_lcd_cur_col++;
        _g_lcd_updating = true;
    }
}

void lcd_clear(void)
{
    LCD_CMD(CMD_CLEAR);
}

static inline void lcd_set_gpio_out(void)
{
    LCD_DDR1 |= LCD_PORT1_D_MASK;
    LCD_DDR2 |= LCD_PORT2_D_MASK;
}

static inline void lcd_set_gpio_in(void)
{
    LCD_DDR1 &= ~LCD_PORT1_D_MASK;
    LCD_DDR2 &= ~LCD_PORT2_D_MASK;
}

static uint8_t lcd_read_byte(uint8_t reg)
{
    uint8_t result;
    
    lcd_set_gpio_in();
    
    LCD_RW_PORT |= _BV(LCD_RW);
    
    if (reg)
        LCD_RS_PORT |= _BV(LCD_RS);
    else
        LCD_RS_PORT &= ~_BV(LCD_RS);
    
    LCD_E_PORT |= _BV(LCD_E);
    
    _delay_us(1);
    
    result = (LCD_PIN1 & LCD_PORT1_D_MASK);
    result |= (LCD_PIN2 & LCD_PORT2_D_MASK);
    
    LCD_E_PORT &= ~_BV(LCD_E);
    
    return result;
}

static void lcd_write_byte(uint8_t reg, uint8_t data)
{
    lcd_set_gpio_out();
    
    LCD_RW_PORT &= ~_BV(LCD_RW);
    
    if (reg)
        LCD_RS_PORT |= _BV(LCD_RS);
    else
        LCD_RS_PORT &= ~_BV(LCD_RS);
    
    LCD_E_PORT &= ~_BV(LCD_E);

    LCD_PORT1 &= ~LCD_PORT1_D_MASK;
    LCD_PORT1 |= (data & LCD_PORT1_D_MASK);
    LCD_PORT2 &= ~LCD_PORT2_D_MASK;
    LCD_PORT2 |= (data & LCD_PORT2_D_MASK);
    
    LCD_E_PORT |= _BV(LCD_E);
    _delay_us(1);
    LCD_E_PORT &= ~_BV(LCD_E);
}
