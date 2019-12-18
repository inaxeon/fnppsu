/*
 *   File:   usart_buffered.c
 *   Author: Matthew Millman
 *
 *   FNP600/850/1000 Adapter Board
 *
 *   Created on 22 May 2018, 16:32
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
#include <stdbool.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "usart_buffered.h"

#define UART_TX_BUFFER_SIZE 64
#define UART_RX_BUFFER_SIZE 64

#ifdef _USART1_

/* size of RX/TX buffers */
#define UART_RX_BUFFER_MASK (UART_RX_BUFFER_SIZE - 1)
#define UART_TX_BUFFER_MASK (UART_TX_BUFFER_SIZE - 1)

#if (UART_RX_BUFFER_SIZE & UART_RX_BUFFER_MASK)
#error RX buffer size is not a power of 2
#endif
#if (UART_TX_BUFFER_SIZE & UART_TX_BUFFER_MASK)
#error TX buffer size is not a power of 2
#endif

static volatile uint8_t _g_usart_txbuf[UART_TX_BUFFER_SIZE];
static volatile uint8_t _g_usart_rxbuf[UART_RX_BUFFER_SIZE];
static volatile uint8_t _g_usart_txhead;
static volatile uint8_t _g_usart_txtail;
static volatile uint8_t _g_usart_rxhead;
static volatile uint8_t _g_usart_rxtail;
static volatile uint8_t _g_usart_last_rx_error;

ISR(USART_RX_vect)
{
    uint8_t tmphead;
    uint8_t data;
    uint8_t usr;
    uint8_t lastRxError;
 
    usr  = UCSR0A;
    data = UDR0;
    
    lastRxError = (usr & (_BV(FE0) | _BV(DOR0)));
    tmphead = (_g_usart_rxhead + 1) & UART_RX_BUFFER_MASK;
    
    if (tmphead == _g_usart_rxtail)
    {
        lastRxError = UART_BUFFER_OVERFLOW >> 8;
    }
    else
    {
        _g_usart_rxhead = tmphead;
        _g_usart_rxbuf[tmphead] = data;
    }

    _g_usart_last_rx_error = lastRxError;   
}

ISR(USART_UDRE_vect)
{
    uint8_t tmptail;
    
    if (_g_usart_txhead != _g_usart_txtail)
    {
        tmptail = (_g_usart_txtail + 1) & UART_TX_BUFFER_MASK;
        _g_usart_txtail = tmptail;
        UDR0 = _g_usart_txbuf[tmptail];
    }
    else
    {
        UCSR0B &= ~_BV(UDRIE0);
    }
}

void usart1_open(uint8_t flags, uint16_t brg)
{
    _g_usart_txhead = 0;
    _g_usart_txtail = 0;
    _g_usart_rxhead = 0;
    _g_usart_rxtail = 0;
    
    UCSR0C |= _BV(UMSEL01);

    if (flags & USART_SYNC)
        UCSR0C |= _BV(UMSEL00);
    else
        UCSR0C &= ~_BV(UMSEL00);

    if (flags & USART_9BIT)
    {
        UCSR0C |= _BV(UCSZ00);
        UCSR0C |= _BV(UCSZ01);
        UCSR0B |= _BV(UCSZ02);
    }
    else
    {
        UCSR0C |= _BV(UCSZ00);
        UCSR0C |= _BV(UCSZ01);
        UCSR0B &= ~_BV(UCSZ02);
    }

    if (flags & USART_SYNC)
    {
        if (flags & USART_SYNC_MASTER)
            USART1_DDR |= _BV(USART1_XCK);
        else
            USART1_DDR &= ~_BV(USART1_XCK);
    }

    if (flags & USART_CONT_RX)
        UCSR0B |= _BV(RXEN0);
    else
        UCSR0B &= ~_BV(RXEN0);

    UCSR0B |= _BV(RXCIE0);

    UBRR0L = (brg & 0xFF);
    UBRR0H = (brg >> 8);

    UCSR0B |= _BV(TXEN0);

    USART1_DDR |= _BV(USART1_TX);
    USART1_DDR &= ~_BV(USART1_RX);
}

bool usart1_data_ready(void)
{
    if (_g_usart_rxhead == _g_usart_rxtail)
        return false;
    return true;
}

char usart1_get(void)
{
    uint8_t tmptail;

    if (_g_usart_rxhead == _g_usart_rxtail)
        return 0x00;
    
    tmptail = (_g_usart_rxtail + 1) & UART_RX_BUFFER_MASK;
    _g_usart_rxtail = tmptail;
    
    return _g_usart_rxbuf[tmptail];
}

void usart1_put(char c)
{
    uint8_t tmphead = (_g_usart_txhead + 1) & UART_TX_BUFFER_MASK;
    
    while (tmphead == _g_usart_txtail);
    
    _g_usart_txbuf[tmphead] = c;
    _g_usart_txhead = tmphead;

    UCSR0B |= _BV(UDRIE0);
}

bool usart1_busy(void)
{
    return (_g_usart_txhead != _g_usart_txtail || (UCSR0A & _BV(UDRE0)) == 0);
}

uint8_t usart1_get_last_rx_error(void)
{
    return _g_usart_last_rx_error;
}

#endif /* _USART1_ */