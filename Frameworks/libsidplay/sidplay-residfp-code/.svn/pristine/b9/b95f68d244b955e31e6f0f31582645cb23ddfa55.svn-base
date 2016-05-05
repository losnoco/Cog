/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2015 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2001-2001 by Jarno Paananen
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

#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdio>
#include <sstream>
#include <string>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

namespace libsidplayfp
{

// Move these to common header file
#define HSID_IOCTL_RESET     _IOW('S', 0, int)
#define HSID_IOCTL_FIFOSIZE  _IOR('S', 1, int)
#define HSID_IOCTL_FIFOFREE  _IOR('S', 2, int)
#define HSID_IOCTL_SIDTYPE   _IOR('S', 3, int)
#define HSID_IOCTL_CARDTYPE  _IOR('S', 4, int)
#define HSID_IOCTL_MUTE      _IOW('S', 5, int)
#define HSID_IOCTL_NOFILTER  _IOW('S', 6, int)
#define HSID_IOCTL_FLUSH     _IO ('S', 7)
#define HSID_IOCTL_DELAY     _IOW('S', 8, int)
#define HSID_IOCTL_READ      _IOWR('S', 9, int*)

bool HardSID::m_sidFree[16] = {0};
const unsigned int HardSID::voices = HARDSID_VOICES;
unsigned int HardSID::sid = 0;

const char* HardSID::getCredits()
{
    static std::string credits;

    if (credits.empty())
    {
        // Setup credits
        std::ostringstream ss;
        ss << "HardSID V" << VERSION << " Engine:\n";
        ss << "\t(C) 2001-2002 Jarno Paanenen\n";
        credits = ss.str();
    }

    return credits.c_str();
}

HardSID::HardSID (sidbuilder *builder) :
    sidemu(builder),
    Event("HardSID Delay"),
    m_handle(0),
    m_instance(sid++)
{
    unsigned int num = 16;
    for (unsigned int i = 0; i < 16; i++)
    {
        if (m_sidFree[i] == false)
        {
            m_sidFree[i] = true;
            num = i;
            break;
        }
    }

    // All sids in use?!?
    if (num == 16)
        return;

    m_instance = num;

    {
        char device[20];
        sprintf(device, "/dev/sid%u", m_instance);
        m_handle = open (device, O_RDWR);
        if (m_handle < 0)
        {
            if (m_instance == 0)
            {
                m_handle = open("/dev/sid", O_RDWR);
                if (m_handle < 0)
                {
                    m_error.assign("HARDSID ERROR: Cannot access \"/dev/sid\" or \"").append(device).append("\"");
                    return;
                }
            }
            else
            {
                m_error.assign("HARDSID ERROR: Cannot access \"").append(device).append("\"");
                return;
            }
        }
    }

    m_status = true;
    sidemu::reset();
}

HardSID::~HardSID()
{
    sid--;
    m_sidFree[m_instance] = false;
    if (m_handle)
        close(m_handle);
}

void HardSID::reset(uint8_t volume)
{
    for (unsigned int i= 0; i < voices; i++)
        muted[i] = false;
    ioctl(m_handle, HSID_IOCTL_RESET, volume);
    m_accessClk = 0;
    if (eventScheduler != nullptr)
        eventScheduler->schedule(*this, HARDSID_DELAY_CYCLES, EVENT_CLOCK_PHI1);
}

event_clock_t HardSID::delay()
{
    event_clock_t cycles = eventScheduler->getTime(m_accessClk, EVENT_CLOCK_PHI1);
    m_accessClk += cycles;

    while (cycles > 0xffff)
    {
        ioctl(m_handle, HSID_IOCTL_DELAY, 0xffff);
        cycles -= 0xffff;
    }

    return cycles;
}

void HardSID::clock()
{
    if (!m_handle)
        return;

    const event_clock_t cycles = delay();

    if (cycles)
        ioctl(m_handle, HSID_IOCTL_DELAY, cycles);
}

uint8_t HardSID::read(uint_least8_t addr)
{
    if (!m_handle)
        return 0;

    const event_clock_t cycles = delay();

    unsigned int packet = ((cycles & 0xffff) << 16) | ((addr & 0x1f) << 8);
    ioctl(m_handle, HSID_IOCTL_READ, &packet);

    return static_cast<uint8_t>(packet & 0xff);
}

void HardSID::write(uint_least8_t addr, uint8_t data)
{
    if (!m_handle)
        return;

    const event_clock_t cycles = delay();

    unsigned int packet = ((cycles & 0xffff) << 16) | ((addr & 0x1f) << 8)
        | (data & 0xff);

    ::write(m_handle, &packet, sizeof(packet));
}

void HardSID::voice(unsigned int num, bool mute)
{
    // Only have 3 voices!
    if (num >= voices)
        return;
    muted[num] = mute;

    int cmute = 0;
    for (unsigned int i = 0; i < voices; i++)
        cmute |= (muted[i] << i);
    ioctl(m_handle, HSID_IOCTL_MUTE, cmute);
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
        ioctl(m_handle, HSID_IOCTL_DELAY, static_cast<uint>(cycles));
        eventScheduler->schedule(*this, HARDSID_DELAY_CYCLES, EVENT_CLOCK_PHI1);
    }
}

void HardSID::filter(bool enable)
{
    ioctl(m_handle, HSID_IOCTL_NOFILTER, !enable);
}

void HardSID::flush()
{
    ioctl(m_handle, HSID_IOCTL_FLUSH);
}

bool HardSID::lock(EventScheduler* env)
{
    sidemu::lock(env);
    eventScheduler->schedule(*this, HARDSID_DELAY_CYCLES, EVENT_CLOCK_PHI1);

    return true;
}

void HardSID::unlock()
{
    eventScheduler->cancel(*this);
    sidemu::unlock();
}

}
