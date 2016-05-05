/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2015 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2001 Simon White
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

#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>


namespace libsidplayfp
{

typedef int_fast64_t event_clock_t;


/**
 * An Event object that can be inserted in the Event Scheduler.
 */
class Event
{
    friend class EventScheduler;

private:
    /// The next event in sequence.
    Event *next;

    /// The clock this event fires.
    event_clock_t triggerTime;

    /// Describe event for humans.
    const char * const m_name;

public:
    /**
     * Events are used for delayed execution. Name is
     * not used by code, but is useful for debugging.
     *
     * @param name Descriptive string of the event.
     */
    Event(const char * const name) :
        m_name(name) {}

    /**
     * Event code to be executed. Events are allowed to safely
     * reschedule themselves with the EventScheduler during
     * invocations.
     */
    virtual void event() = 0;

protected:
    ~Event() {}
};

}

#endif // EVENT_H
