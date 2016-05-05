/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2015 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef TOD_H
#define TOD_H

#include <stdint.h>

#include "EventScheduler.h"

namespace libsidplayfp
{

class MOS6526;

/**
 * TOD implementation taken from Vice.
 */
class Tod : private Event
{
private:
    enum
    {
        TENTHS  = 0,
        SECONDS = 1,
        MINUTES = 2,
        HOURS   = 3
    };

private:
    /// Event scheduler.
    EventScheduler &eventScheduler;

    /// Pointer to the MOS6526 which this Timer belongs to.
    MOS6526 &parent;

    const uint8_t &cra;
    const uint8_t &crb;

    event_clock_t cycles;
    event_clock_t period;

    bool isLatched;
    bool isStopped;

    uint8_t clock[4];
    uint8_t latch[4];
    uint8_t alarm[4];

private:
    inline void checkAlarm();

    void event();

public:
    Tod(EventScheduler &scheduler, MOS6526 &parent, uint8_t regs[0x10]) :
        Event("CIA Time of Day"),
        eventScheduler(scheduler),
        parent(parent),
        cra(regs[0x0e]),
        crb(regs[0x0f]),
        period(~0) // Dummy
    {}

    /**
     * Reset TOD.
     */
    void reset();

    /**
     * Read TOD register.
     *
     * @param addr
     *            register register to read
     */
    uint8_t read(uint_least8_t reg);

    /**
     * Write TOD register.
     *
     * @param addr
     *            register to write
     * @param data
     *            value to write
     */
    void write(uint_least8_t reg, uint8_t data);

    /**
     * Set TOD period.
     *
     * @param clock
     */
    void setPeriod(event_clock_t clock) { period = clock * (1 << 7); }
};

}

#endif
