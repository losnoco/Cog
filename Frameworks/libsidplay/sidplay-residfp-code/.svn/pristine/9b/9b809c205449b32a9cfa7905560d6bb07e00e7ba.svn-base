/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000 Simon White
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "timer.h"

#include "sidendian.h"

namespace libsidplayfp
{

void Timer::setControlRegister(uint8_t cr)
{
    state &= ~CIAT_CR_MASK;
    state |= (cr & CIAT_CR_MASK) ^ CIAT_PHI2IN;
    lastControlValue = cr;
}

void Timer::syncWithCpu()
{
    if (ciaEventPauseTime > 0)
    {
        eventScheduler.cancel(m_cycleSkippingEvent);
        const event_clock_t elapsed = eventScheduler.getTime(EVENT_CLOCK_PHI2) - ciaEventPauseTime;

        // It's possible for CIA to determine that it wants to go to sleep starting from the next
        // cycle, and then have its plans aborted by CPU. Thus, we must avoid modifying
        // the CIA state if the first sleep clock was still in the future.
        if (elapsed >= 0)
        {
            timer -= elapsed;
            clock();
        }
    }
    if (ciaEventPauseTime == 0)
    {
        eventScheduler.cancel(*this);
    }
    ciaEventPauseTime = -1;
}

void Timer::wakeUpAfterSyncWithCpu()
{
    ciaEventPauseTime = 0;
    eventScheduler.schedule(*this, 0, EVENT_CLOCK_PHI1);
}

void Timer::event()
{
    clock();
    reschedule();
}

void Timer::cycleSkippingEvent()
{
    const event_clock_t elapsed = eventScheduler.getTime(EVENT_CLOCK_PHI1) - ciaEventPauseTime;
    ciaEventPauseTime = 0;
    timer -= elapsed;
    event();
}

void Timer::clock()
{
    if (timer != 0 && (state & CIAT_COUNT3) != 0)
    {
        timer--;
    }

    /* ciatimer.c block start */
    int_least32_t adj = state & (CIAT_CR_START | CIAT_CR_ONESHOT | CIAT_PHI2IN);
    if ((state & (CIAT_CR_START | CIAT_PHI2IN)) == (CIAT_CR_START | CIAT_PHI2IN))
    {
        adj |= CIAT_COUNT2;
    }
    if ((state & CIAT_COUNT2) != 0
            || (state & (CIAT_STEP | CIAT_CR_START)) == (CIAT_STEP | CIAT_CR_START))
    {
        adj |= CIAT_COUNT3;
    }
    // CR_FLOAD -> LOAD1, CR_ONESHOT -> ONESHOT0, LOAD1 -> LOAD, ONESHOT0 -> ONESHOT
    adj |= (state & (CIAT_CR_FLOAD | CIAT_CR_ONESHOT | CIAT_LOAD1 | CIAT_ONESHOT0)) << 8;
    state = adj;
    /* ciatimer.c block end */

    if (timer == 0 && (state & CIAT_COUNT3) != 0)
    {
        state |= CIAT_LOAD | CIAT_OUT;

        if ((state & (CIAT_ONESHOT | CIAT_ONESHOT0)) != 0)
        {
            state &= ~(CIAT_CR_START | CIAT_COUNT2);
        }

        // By setting bits 2&3 of the control register,
        // PB6/PB7 will be toggled between high and low at each underflow.
        const bool toggle = (lastControlValue & 0x06) == 6;
        pbToggle = toggle && !pbToggle;

        // Implementation of the serial port
        serialPort();

        // Timer A signals underflow handling: IRQ/B-count
        underFlow();
    }

    if ((state & CIAT_LOAD) != 0)
    {
        timer = latch;
        state &= ~CIAT_COUNT3;
    }
}

void Timer::reset()
{
    eventScheduler.cancel(*this);
    timer = latch = 0xffff;
    pbToggle = false;
    state = 0;
    lastControlValue = 0;
    ciaEventPauseTime = 0;
    eventScheduler.schedule(*this, 1, EVENT_CLOCK_PHI1);
}

void Timer::latchLo(uint8_t data)
{
    endian_16lo8(latch, data);
    if (state & CIAT_LOAD)
        endian_16lo8(timer, data);
}

void Timer::latchHi(uint8_t data)
{
    endian_16hi8(latch, data);
    // Reload timer if stopped
    if ((state & CIAT_LOAD) || !(state & CIAT_CR_START))
        timer = latch;
}

}
