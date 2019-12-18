/*
 *   File:   timeout.c
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

#include "project.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <avr/interrupt.h>

#include "timeout.h"

#define MAX_SOFT_TIMERS 10

#define F_ACTIVE        0x01
#define F_RUNNING       0x02
#define F_REPEAT        0x04

#define MS(x) ((x) / TIMEOUT_MS_PER_TICK)

#define TIMER1H_RELOAD  0xA6 /* Approx 100ms */

typedef struct
{
    uint8_t flags;
    int32_t next_fires;
    uint32_t interval;
    void *data;
    void (*callback)(void *);
} timeout_t;

timeout_t _g_timers[MAX_SOFT_TIMERS];
int32_t _g_tick_count;

ISR(TIMER1_OVF_vect)
{
    _g_tick_count++;

    TCNT1H = TIMER1H_RELOAD;
    TCNT1L = 0x00;
}

void timeout_init(void)
{
    _g_tick_count = 0;
    memset(&_g_timers, 0, sizeof(_g_timers));

    TCCR1A = 0x00;
    // CLK(i/o) prescaler 64
    TCCR1B &= ~(1 << CS12);
    TCCR1B |= (1 << CS11);
    TCCR1B |= (1 << CS10);

    TIMSK1 |= (1 << TOIE1);

    TCNT1H = TIMER1H_RELOAD;
    TCNT1L = 0x00;

    // Note to self: AVR Timers do not seem to have a 'go' bit
    // They're always going...
}

void timeout_check(void)
{
    uint8_t i;

    for (i = 0; i < MAX_SOFT_TIMERS; i++)
    {
        timeout_t *timer = &_g_timers[i];

        if (timer->flags & F_ACTIVE && timer->flags & F_RUNNING)
        {
            bool timedout;

            g_irq_disable();
            timedout = _g_tick_count >= timer->next_fires;
            g_irq_enable();

            if (timedout)
            {
                timer->flags &= ~F_RUNNING;                
                timer->callback(timer->data);
                if (timer->flags & F_REPEAT)
                    timeout_start(i);
            }
        }
    }
}

int8_t timeout_create(uint32_t interval, bool start, bool repeat, void (*callback)(void *), void *data)
{
    int8_t timer_index = 0;
    timeout_t *timer = NULL;

    for (timer_index = 0; timer_index < MAX_SOFT_TIMERS; timer_index++)
    {
        if (!(_g_timers[timer_index].flags & F_ACTIVE))
        {
            timer = &_g_timers[timer_index];
            timer->flags |= F_ACTIVE;
            break;
        }
    }

    if (!timer)
        return -1;

    timer->interval = interval;
    timer->callback = callback;
    timer->data = data;

    if (repeat)
        timer->flags |= F_REPEAT;

    if (start)
        timeout_start(timer_index);

    return timer_index;
}

void timeout_destroy(int8_t index)
{
    timeout_t *timer = &_g_timers[index];
    timer->flags &= ~F_ACTIVE;
}

void timeout_start(int8_t index)
{
    timeout_t *timer = &_g_timers[index];
    g_irq_disable();
    timer->next_fires = _g_tick_count + MS(timer->interval);
    g_irq_enable();
    timer->flags |= F_RUNNING;
}

void timeout_stop(int8_t index)
{
    timeout_t *timer = &_g_timers[index];
    timer->flags &= ~F_RUNNING;
}

int32_t get_tick_count(void)
{
    return _g_tick_count;
}