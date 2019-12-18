/*
 *   File:   project.h
 *   Author: Matthew Millman
 *
 *   FNP600/850/1000 Adapter Board
 *
 *   Created on 11 May 2018, 11:51
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


#ifndef __PROJECT_H__
#define __PROJECT_H__

#define _I2C_XFER_
#define _I2C_XFER_MANY_
#define _I2C_XFER_BYTE_

#define _USART1_
#define _CONSOLE1_

#define F_CPU               14745600

#define CONFIG_MAGIC        0x4650

#define OUTPUT_VOLTAGE_DEFAULT  1200
#define OUTPUT_VOLTAGE_MAX      1245 // PSU Will not accept anything above this
#define OUTPUT_VOLTAGE_MIN      100

#define CLRWDT() asm("wdr")

#define g_irq_disable cli
#define g_irq_enable sei

#define MAX_PSU            8

#define PS_ON_DDR          DDRC
#define PS_ON              PC3
#define PS_ON_PORT         PORTC
#define PS_ON_STATE        ((PS_ON_PORT & _BV(PS_ON)) == 0)

#define LCD_DDR1           DDRC
#define LCD_DDR2           DDRD
#define LCD_PORT1          PORTC
#define LCD_PORT2          PORTD
#define LCD_PIN1           PINC
#define LCD_PIN2           PIND
#define LCD_PORT1_D_MASK   0x07
#define LCD_PORT2_D_MASK   0xF8

#define LCD_E_DDR          DDRB
#define LCD_E_PORT         PORTB
#define LCD_E              PB2
#define LCD_RS_DDR         DDRB
#define LCD_RS_PORT        PORTB
#define LCD_RS             PB1
#define LCD_RW_DDR         DDRB
#define LCD_RW_PORT        PORTB
#define LCD_RW             PB0

#define USART1_DDR         DDRD
#define USART1_TX          PD1
#define USART1_RX          PD0
#define USART1_XCK         PD4

#define UART1_BAUD         9600

#define TIMEOUT_TICK_PER_SECOND  (10)
#define TIMEOUT_MS_PER_TICK      (1000 / TIMEOUT_TICK_PER_SECOND)

#define console1_busy         usart1_busy
#define console1_put          usart1_put
#define console1_data_ready   usart1_data_ready
#define console1_get          usart1_get
#define console1_clear_oerr   usart1_clear_oerr

#endif /* __PROJECT_H__ */