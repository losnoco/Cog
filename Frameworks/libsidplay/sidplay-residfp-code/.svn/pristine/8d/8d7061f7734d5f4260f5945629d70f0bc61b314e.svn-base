/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "sidplayfp/siddefs.h"

typedef int_fast64_t event_clock_t;

/**
 * C64 system runs actions at system clock high and low
 * states. The PHI1 corresponds to the auxiliary chip activity
 * and PHI2 to CPU activity. For any clock, PHI1s are before
 * PHI2s.
 */
typedef enum {EVENT_CLOCK_PHI1 = 0, EVENT_CLOCK_PHI2 = 1} event_phase_t;


/**
 * Event scheduler (based on alarm from Vice). Created in 2001 by Simon A.
 * White.
 *
 * Optimized EventScheduler and corresponding Event class by Antti S. Lankila
 * in 2009.
 *
 * @author Antti Lankila
 */
class SID_EXTERN Event
{
    friend class EventScheduler;

private:
    /**
     * The next event in sequence.
     */
    Event *next;

    /**
     * The clock this event fires.
     */
    event_clock_t triggerTime;

    /**
     * Describe event for humans.
     */
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

/**
 * Fast EventScheduler, which maintains a linked list of Events.
 * This scheduler takes neglible time even when it is used to
 * schedule events for nearly every clock.
 *
 * Events occur on an internal clock which is 2x the visible clock.
 * The visible clock is divided to two phases called phi1 and phi2.
 *
 * The phi1 clocks are used by VIC and CIA chips, phi2 clocks by CPU.
 *
 * Scheduling an event for a phi1 clock when system is in phi2 causes the
 * event to be moved to the next phi1 cycle. Correspondingly, requesting
 * a phi1 time when system is in phi2 returns the value of the next phi1.
 *
 * @author Antti S. Lankila
 */
class EventContext
{
public:
    /**
     * Cancel the specified event.
     *
     * @param event the event to cancel
     */
    virtual void cancel(Event &event) = 0;

    /**
     * Add event to pending queue.
     *
     * At PHI2, specify cycles=0 and Phase=PHI1 to fire on the very next PHI1.
     *
     * @param event the event to add
     * @param cycles how many cycles from now to fire
     * @param phase the phase when to fire the event
     */
    virtual void schedule(Event &event, event_clock_t cycles,
                          event_phase_t phase) = 0;

    /**
     * Add event to pending queue in the same phase as current event.
     *
     * @param event the event to add
     * @param cycles how many cycles from now to fire
     */
    virtual void schedule(Event &event, event_clock_t cycles) = 0;

    /**
     * Is the event pending in this scheduler?
     *
     * @param event the event
     * @return true when pending
     */
    virtual bool isPending(Event &event) const = 0;

    /**
     * Get time with respect to a specific clock phase.
     *
     * @param phase the phase
     * @return the time according to specified phase.
     */
    virtual event_clock_t getTime(event_phase_t phase) const = 0;

    /**
     * Get clocks since specified clock in given phase.
     *
     * @param clock the time to compare to
     * @param phase the phase to comapre to
     * @return the time between specified clock and now
     */
    virtual event_clock_t getTime(event_clock_t clock, event_phase_t phase) const = 0;

    /**
     * Return current clock phase.
     *
     * @return The current phase
     */
    virtual event_phase_t phase() const = 0;

protected:
    ~EventContext() {}
};

#endif // EVENT_H
