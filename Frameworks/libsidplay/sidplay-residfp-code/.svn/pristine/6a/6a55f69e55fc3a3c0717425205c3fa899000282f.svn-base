/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2015 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000-2002 Simon White
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

#include "hardsid-emu.h"

#include <cstdio>
#include <sstream>
#include <string>

#include "hardsid.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

/***************************************************************************
Hardsid support interface.
Created from Jarnos original
Sidplay2 patch
***************************************************************************/

extern libsidplayfp::HsidDLL2 hsid2;

namespace libsidplayfp
{

const  unsigned int HardSID::voices = HARDSID_VOICES;
unsigned int HardSID::sid = 0;

const char* HardSID::getCredits()
{
    static std::string credits;

    if (credits.empty())
    {
        // Setup credits
        std::ostringstream ss;
        ss << "HardSID V" << VERSION << " Engine:\n";
        ss << "\t(C) 1999-2002 Simon White\n";
        credits = ss.str();
    }

    return credits.c_str();
}

HardSID::HardSID(sidbuilder *builder) :
    sidemu(builder),
    Event("HardSID Delay"),
    m_instance(sid++)
{
    if (m_instance >= hsid2.Devices())
    {
        m_error = "HARDSID WARNING: System dosen't have enough SID chips.";
        return;
    }

    m_status = true;
    sidemu::reset();
}


HardSID::~HardSID()
{
    sid--;
}

event_clock_t HardSID::delay()
{
    event_clock_t cycles = eventScheduler->getTime(m_accessClk, EVENT_CLOCK_PHI1);
    m_accessClk += cycles;

    while (cycles > 0xFFFF)
    {
        hsid2.Delay((BYTE) m_instance, 0xFFFF);
        cycles -= 0xFFFF;
    }

    return cycles;
}

void HardSID::clock()
{
    return;
}

uint8_t HardSID::read(uint_least8_t addr)
{
    const event_clock_t cycles = delay();

    return hsid2.Read((BYTE) m_instance, (WORD) cycles, (BYTE) addr);
}

void HardSID::write(uint_least8_t addr, uint8_t data)
{
    const event_clock_t cycles = delay();

    hsid2.Write((BYTE) m_instance, (WORD) cycles, (BYTE) addr, (BYTE) data);
}

void HardSID::reset(uint8_t volume)
{
    m_accessClk = 0;
    // Clear hardsid buffers
    hsid2.Flush ((BYTE) m_instance);
    if (hsid2.Version >= HSID_VERSION_204)
        hsid2.Reset2((BYTE) m_instance, volume);
    else
        hsid2.Reset((BYTE) m_instance);
    hsid2.Sync((BYTE) m_instance);

    if (eventScheduler != nullptr)
        eventScheduler->schedule(*this, HARDSID_DELAY_CYCLES, EVENT_CLOCK_PHI1);
}

void HardSID::voice(unsigned int num, bool mute)
{
    if (hsid2.Version >= HSID_VERSION_207)
        hsid2.Mute2((BYTE) m_instance, (BYTE) num, (BOOL) mute, FALSE);
    else
        hsid2.Mute((BYTE) m_instance, (BYTE) num, (BOOL) mute);
}

// Set execution environment and lock sid to it
bool HardSID::lock(EventScheduler *env)
{
    if (hsid2.Version >= HSID_VERSION_204)
    {
        if (hsid2.Lock(m_instance) == FALSE)
            return false;
    }

    sidemu::lock(env);
    eventScheduler->schedule(*this, HARDSID_DELAY_CYCLES, EVENT_CLOCK_PHI1);

    return true;
}

// Unlock sid
void HardSID::unlock()
{
    if (hsid2.Version >= HSID_VERSION_204)
        hsid2.Unlock(m_instance);

    eventScheduler->cancel(*this);
    sidemu::unlock();
}

void HardSID::event()
{
    event_clock_t cycles = eventScheduler->getTime(m_accessClk, EVENT_CLOCK_PHI1);
    if (cycles < HARDSID_DELAY_CYCLES)
    {
        eventScheduler->schedule(*this, HARDSID_DELAY_CYCLES - cycles,
                  EVENT_CLOCK_PHI1);
    }
    else
    {
        m_accessClk += cycles;
        hsid2.Delay((BYTE) m_instance, (WORD) cycles);
        eventScheduler->schedule(*this, HARDSID_DELAY_CYCLES, EVENT_CLOCK_PHI1);
    }
}

// Disable/Enable SID filter
void HardSID::filter(bool enable)
{
    hsid2.Filter((BYTE) m_instance, (BOOL) enable);
}

void HardSID::flush()
{
    hsid2.Flush((BYTE) m_instance);
}

}
