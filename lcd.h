/* 
 *   File:   lcd.h
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

#ifndef __LCD_H__
#define __LCD_H__

#define LCD_ROWS            2
#define LCD_COLS            8

#define LCD_ROW1            0
#define LCD_ROW2            1

void lcd_init(void);
void lcd_clear(void);
void lcd_start_update(void);
void lcd_process(void);

extern char _g_lcd_data[LCD_ROWS][LCD_COLS + 1];

#endif /* __LCD_H__ */
