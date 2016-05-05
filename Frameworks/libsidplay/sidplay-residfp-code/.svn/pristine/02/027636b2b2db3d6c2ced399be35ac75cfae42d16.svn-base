/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2011-2014 Leandro Nini
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

#include "EventScheduler.h"


namespace libsidplayfp
{

void EventScheduler::reset()
{
    firstEvent = nullptr;
    currentTime = 0;
}

void EventScheduler::cancel(Event &event)
{
    Event **scan = &firstEvent;

    while (*scan != nullptr)
    {
        if (&event == *scan)
        {
            *scan = (*scan)->next;
            break;
        }
        scan = &((*scan)->next);
    }
}

bool EventScheduler::isPending(Event &event) const
{
    Event *scan = firstEvent;
    while (scan != nullptr)
    {
        if (&event == scan)
        {
            return true;
        }
        scan = scan->next;
    }
    return false;
}

}
