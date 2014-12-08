/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2014 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2009-2014 VICE Project
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

#ifndef MOS6526_H
#define MOS6526_H

#include <stdint.h>

#include "timer.h"
#include "tod.h"
#include "EventScheduler.h"
#include "c64/component.h"

#include "sidcxx11.h"

class EventContext;

namespace libsidplayfp
{

class MOS6526;

/**
 * This is the timer A of this CIA.
 *
 * @author Ken Händel
 *
 */
class TimerA : public Timer
{
private:
    /**
     * Signal underflows of Timer A to Timer B.
     */
    void underFlow() override;

    void serialPort() override;

public:
    /**
     * Create timer A.
     */
    TimerA(EventContext *context, MOS6526* parent) :
        Timer("CIA Timer A", context, parent) {}
};

/**
 * This is the timer B of this CIA.
 *
 * @author Ken Händel
 *
 */
class TimerB : public Timer
{
private:
    void underFlow() override;

public:
    /**
     * Create timer B.
     */
    TimerB(EventContext *context, MOS6526* parent) :
        Timer("CIA Timer B", context, parent) {}

    /**
     * Receive an underflow from Timer A.
     */
    void cascade()
    {
        /* we pretend that we are CPU doing a write to ctrl register */
        syncWithCpu();
        state |= CIAT_STEP;
        wakeUpAfterSyncWithCpu();
    }

    /**
     * Check if start flag is set.
     *
     * @return true if start flag is set, false otherwise
     */
    bool started() const { return (state & CIAT_CR_START) != 0; }
};

/**
 * This class is heavily based on the ciacore/ciatimer source code from VICE.
 * The CIA state machine is lifted as-is. Big thanks to VICE project!
 *
 * @author alankila
 */
class MOS6526 : public component
{
    friend class TimerA;
    friend class TimerB;
    friend class Tod;

private:
    static const char *credit;

protected:
    /// Event context.
    EventContext &event_context;

    /// These are all CIA registers.
    uint8_t regs[0x10];

    /// Ports
    //@{
    uint8_t &pra, &prb, &ddra, &ddrb;
    //@}

    /// Timers A and B.
    //@{
    TimerA timerA;
    TimerB timerB;
    //@}

    /// TOD
    Tod tod;

    /// Serial Data Registers
    //@{
    uint8_t sdr_out;
    bool    sdr_buffered;
    int     sdr_count;
    //@}

    /// Interrupt control register
    uint8_t icr;

    /// Interrupt data register
    uint8_t idr;

    /// Have we already scheduled CIA->CPU interrupt transition?
    bool triggerScheduled;

    /// Events
    //@{
    EventCallback<MOS6526> bTickEvent;
    EventCallback<MOS6526> triggerEvent;
    //@}

private:
    /**
     * TOD alarm interrupt.
     */
    void todInterrupt();

    /**
     * This event exists solely to break the ambiguity of what scheduling on
     * top of PHI1 causes, because there is no ordering between events on
     * same phase. Thus it is scheduled in PHI2 to ensure the b.event() is
     * run once before the value changes.
     *
     * - PHI1 a.event() (which calls underFlow())
     * - PHI1 b.event()
     * - PHI2 bTick.event()
     * - PHI1 a.event()
     * - PHI1 b.event()
     */
    void bTick();

    /**
     * Signal interrupt to CPU.
     */
    void trigger();

    /**
     * Timer A underflow.
     */
    void underflowA();

    /** Timer B underflow. */
    void underflowB();

    /**
     * Trigger an interrupt.
     *
     * @param interruptMask Interrupt flag number
     */
    void trigger(uint8_t interruptMask);

    /**
     * Clear interrupt state.
     */
    void clear();

    /**
     * Handle the serial port.
     */
    void serialPort();

protected:
    /**
     * Create a new CIA.
     *
     * @param context the event context
     */
    MOS6526(EventContext *context);
    ~MOS6526() {}

    /**
     * Signal interrupt.
     *
     * @param state
     *            interrupt state
     */
    virtual void interrupt(bool state) = 0;

    virtual void portA() {}
    virtual void portB() {}

    /**
     * Read CIA register.
     *
     * @param addr
     *            register address to read (lowest 4 bits)
     */
    uint8_t read(uint_least8_t addr) override;

    /**
     * Write CIA register.
     *
     * @param addr
     *            register address to write (lowest 4 bits)
     * @param data
     *            value to write
     */
    void write(uint_least8_t addr, uint8_t data) override;

public:
    /**
     * Reset CIA.
     */
    virtual void reset() override;

    /**
     * Get the credits.
     *
     * @return the credits
     */
    static const char *credits() { return credit; }

    /**
     * Set day-of-time event occurence of rate.
     *
     * @param clock
     */
    void setDayOfTimeRate(unsigned int clock) { tod.setPeriod(clock); }
};

}

#endif // MOS6526_H
