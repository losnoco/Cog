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

#ifndef C64ENV_H
#define C64ENV_H

#include "EventScheduler.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

namespace libsidplayfp
{

/**
 * An implementation of of this class can be created to perform the C64
 * specifics.  A pointer to this child class can then be passed to
 * each of the components so they can interact with it.
 */
class c64env
{
private:
    EventScheduler &eventScheduler;

public:
    c64env(EventScheduler &scheduler) :
        eventScheduler(scheduler) {}

    EventScheduler &scheduler() const { return eventScheduler; }

    virtual uint8_t cpuRead(uint_least16_t addr) =0;
    virtual void cpuWrite(uint_least16_t addr, uint8_t data) =0;

#ifdef PC64_TESTSUITE
    virtual void loadFile(const char *file) =0;
#endif

    virtual void interruptIRQ(bool state) = 0;
    virtual void interruptNMI() = 0;
    virtual void interruptRST() = 0;

    virtual void setBA(bool state) = 0;
    virtual void lightpen(bool state) = 0;

protected:
    ~c64env() {}
};

}

#endif // C64ENV_H
