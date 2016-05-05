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

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#include "Event.h"
#include "EventCallback.h"
#include "EventScheduler.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

class MOS6526;

/**
 * This is the base class for the MOS6526 timers.
 */
class Timer : private Event
{
protected:
    static const int_least32_t CIAT_CR_START   = 0x01;
    static const int_least32_t CIAT_STEP       = 0x04;
    static const int_least32_t CIAT_CR_ONESHOT = 0x08;
    static const int_least32_t CIAT_CR_FLOAD   = 0x10;
    static const int_least32_t CIAT_PHI2IN     = 0x20;
    static const int_least32_t CIAT_CR_MASK    = CIAT_CR_START | CIAT_CR_ONESHOT | CIAT_CR_FLOAD | CIAT_PHI2IN;

    static const int_least32_t CIAT_COUNT2     = 0x100;
    static const int_least32_t CIAT_COUNT3     = 0x200;

    static const int_least32_t CIAT_ONESHOT0   = 0x08 << 8;
    static const int_least32_t CIAT_ONESHOT    = 0x08 << 16;
    static const int_least32_t CIAT_LOAD1      = 0x10 << 8;
    static const int_least32_t CIAT_LOAD       = 0x10 << 16;

    static const int_least32_t CIAT_OUT        = 0x80000000;

private:
    EventCallback<Timer> m_cycleSkippingEvent;

    /// Event context.
    EventScheduler &eventScheduler;

    /**
     * This is a tri-state:
     *
     * - when -1: cia is completely stopped
     * - when 0: cia 1-clock events are ticking.
     * - otherwise: cycle skip event is ticking, and the value is the first
     *   phi1 clock of skipping.
     */
    event_clock_t ciaEventPauseTime;

    /// PB6/PB7 Flipflop to signal underflows.
    bool pbToggle;

    /// Current timer value.
    uint_least16_t timer;

    /// Timer start value (Latch).
    uint_least16_t latch;

    /// Copy of regs[CRA/B]
    uint8_t lastControlValue;

protected:
    /// Pointer to the MOS6526 which this Timer belongs to.
    MOS6526 &parent;

    /// CRA/CRB control register / state.
    int_least32_t state;

private:
    /**
     * Perform scheduled cycle skipping, and resume.
     */
    void cycleSkippingEvent();

    /**
     * Execute one CIA state transition.
     */
    void clock();

    /**
     * Reschedule CIA event at the earliest interesting time.
     * If CIA timer is stopped or is programmed to just count down,
     * the events are paused.
     */
    inline void reschedule();

    /**
     * Timer ticking event.
     */
    void event() override;

    /**
     * Signal timer underflow.
     */
    virtual void underFlow() =0;

    /**
     * Handle the serial port.
     */
    virtual void serialPort() {}

protected:
    /**
     * Create a new timer.
     *
     * @param name component name
     * @param context event context
     * @param parent the MOS6526 which this Timer belongs to
     */
    Timer(const char* name, EventScheduler &scheduler, MOS6526 &parent) :
        Event(name),
        m_cycleSkippingEvent("Skip CIA clock decrement cycles", *this, &Timer::cycleSkippingEvent),
        eventScheduler(scheduler),
        pbToggle(false),
        timer(0),
        latch(0),
        lastControlValue(0),
        parent(parent),
        state(0) {}

public:
    /**
     * Set CRA/CRB control register.
     *
     * @param cr control register value
     */
    void setControlRegister(uint8_t cr);

    /**
     * Perform cycle skipping manually.
     *
     * Clocks the CIA up to the state it should be in, and stops all events.
     */
    void syncWithCpu();

    /**
     * Counterpart of syncWithCpu(),
     * starts the event ticking if it is needed.
     * No clock() call or anything such is permissible here!
     */
    void wakeUpAfterSyncWithCpu();

    /**
     * Reset timer.
     */
    void reset();

    /**
     * Set low byte of Timer start value (Latch).
     *
     * @param data
     *            low byte of latch
     */
    void latchLo(uint8_t data);

    /**
     * Set high byte of Timer start value (Latch).
     *
     * @param data
     *            high byte of latch
     */
    void latchHi(uint8_t data);

    /**
     * Set PB6/PB7 Flipflop state.
     *
     * @param state
     *            PB6/PB7 flipflop state
     */
    inline void setPbToggle(bool state) { pbToggle = state; }

    /**
     * Get current state value.
     *
     * @return current state value
     */
    inline int_least32_t getState() const { return state; }

    /**
     * Get current timer value.
     *
     * @return current timer value
     */
    inline uint_least16_t getTimer() const { return timer; }

    /**
     * Get PB6/PB7 Flipflop state.
     *
     * @param reg value of the control register
     * @return PB6/PB7 flipflop state
     */
    inline bool getPb(uint8_t reg) const { return (reg & 0x04) ? pbToggle : (state & CIAT_OUT); }
};

void Timer::reschedule()
{
    // There are only two subcases to consider.
    //
    // - are we counting, and if so, are we going to
    //   continue counting?
    // - have we stopped, and are there no conditions to force a new beginning?
    //
    // Additionally, there are numerous flags that are present only in passing manner,
    // but which we need to let cycle through the CIA state machine.
    const int_least32_t unwanted = CIAT_OUT | CIAT_CR_FLOAD | CIAT_LOAD1 | CIAT_LOAD;
    if ((state & unwanted) != 0)
    {
        eventScheduler.schedule(*this, 1);
        return;
    }

    if ((state & CIAT_COUNT3) != 0)
    {
        // Test the conditions that keep COUNT2 and thus COUNT3 alive, and also
        // ensure that all of them are set indicating steady state operation.

        const int_least32_t wanted = CIAT_CR_START | CIAT_PHI2IN | CIAT_COUNT2 | CIAT_COUNT3;
        if (timer > 2 && (state & wanted) == wanted)
        {
            // we executed this cycle, therefore the pauseTime is +1. If we are called
            // to execute on the very next clock, we need to get 0 because there's
            // another timer-- in it.
            ciaEventPauseTime = eventScheduler.getTime(EVENT_CLOCK_PHI1) + 1;
            // execute event slightly before the next underflow.
            eventScheduler.schedule(m_cycleSkippingEvent, timer - 1);
            return;
        }

        // play safe, keep on ticking.
        eventScheduler.schedule(*this, 1);
    }
    else
    {
        // Test conditions that result in CIA activity in next clocks.
        // If none, stop.
        const int_least32_t unwanted1 = CIAT_CR_START | CIAT_PHI2IN;
        const int_least32_t unwanted2 = CIAT_CR_START | CIAT_STEP;

        if ((state & unwanted1) == unwanted1
            || (state & unwanted2) == unwanted2)
        {
            eventScheduler.schedule(*this, 1);
            return;
        }

        ciaEventPauseTime = -1;
    }
}

}

#endif // TIMER_H
