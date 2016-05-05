/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2011-2015 Leandro Nini
 *  Copyright (C) 2009 Antti S. Lankila
 *  Copyright (C) 2001 Simon White
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef EVENTSCHEDULER_H
#define EVENTSCHEDULER_H

#include "Event.h"

#include "sidcxx11.h"


namespace libsidplayfp
{

/**
 * C64 system runs actions at system clock high and low
 * states. The PHI1 corresponds to the auxiliary chip activity
 * and PHI2 to CPU activity. For any clock, PHI1s are before
 * PHI2s.
 */
typedef enum
{
    EVENT_CLOCK_PHI1 = 0,
    EVENT_CLOCK_PHI2 = 1
} event_phase_t;


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
 */
class EventScheduler
{
private:
    /// The first event of the chain.
    Event *firstEvent;

    /// EventScheduler's current clock.
    event_clock_t currentTime;

private:
    /**
     * Scan the event queue and schedule event for execution.
     *
     * @param event The event to add
     */
    void schedule(Event &event)
    {
        // find the right spot where to tuck this new event
        Event **scan = &firstEvent;
        for (;;)
        {
            if (*scan == nullptr || (*scan)->triggerTime > event.triggerTime)
            {
                 event.next = *scan;
                 *scan = &event;
                 break;
             }
             scan = &((*scan)->next);
         }
    }

public:
    EventScheduler() :
        firstEvent(nullptr),
        currentTime(0) {}

    /**
     * Add event to pending queue.
     *
     * At PHI2, specify cycles=0 and Phase=PHI1 to fire on the very next PHI1.
     *
     * @param event the event to add
     * @param cycles how many cycles from now to fire
     * @param phase the phase when to fire the event
     */
    void schedule(Event &event, unsigned int cycles, event_phase_t phase)
    {
        // this strange formulation always selects the next available slot regardless of specified phase.
        event.triggerTime = currentTime + ((currentTime & 1) ^ phase) + (cycles << 1);
        schedule(event);
    }

    /**
     * Add event to pending queue in the same phase as current event.
     *
     * @param event the event to add
     * @param cycles how many cycles from now to fire
     */
    void schedule(Event &event, unsigned int cycles)
    {
        event.triggerTime = currentTime + (cycles << 1);
        schedule(event);
    }

    /**
     * Cancel event if pending.
     *
     * @param event the event to cancel
     */
    void cancel(Event &event);

    /**
     * Cancel all pending events and reset time.
     */
    void reset();

    /**
     * Fire next event, advance system time to that event.
     */
    void clock()
    {
        Event &event = *firstEvent;
        firstEvent = firstEvent->next;
        currentTime = event.triggerTime;
        event.event();
    }

    /**
     * Check if an event is in the queue.
     *
     * @param event the event
     * @return true when pending
     */
    bool isPending(Event &event) const;

    /**
     * Get time with respect to a specific clock phase.
     *
     * @param phase the phase
     * @return the time according to specified phase.
     */
    event_clock_t getTime(event_phase_t phase) const
    {
        return (currentTime + (phase ^ 1)) >> 1;
    }

    /**
     * Get clocks since specified clock in given phase.
     *
     * @param clock the time to compare to
     * @param phase the phase to comapre to
     * @return the time between specified clock and now
     */
    event_clock_t getTime(event_clock_t clock, event_phase_t phase) const
    {
        return getTime(phase) - clock;
    }

    /**
     * Return current clock phase.
     *
     * @return The current phase
     */
    event_phase_t phase() const { return static_cast<event_phase_t>(currentTime & 1); }
};

}

#endif // EVENTSCHEDULER_H
