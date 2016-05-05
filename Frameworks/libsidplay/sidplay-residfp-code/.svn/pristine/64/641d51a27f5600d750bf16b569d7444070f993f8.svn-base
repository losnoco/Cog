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

#include "residfp-emu.h"

#include <sstream>
#include <string>
#include <algorithm>

#include "residfp/siddefs-fp.h"
#include "sidplayfp/siddefs.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

namespace libsidplayfp
{

const char* ReSIDfp::getCredits()
{
    static std::string credits;

    if (credits.empty())
    {
        // Setup credits
        std::ostringstream ss;
        ss << "ReSIDfp V" << VERSION << " Engine:\n";
        ss << "\t(C) 1999-2002 Simon White\n";
        ss << "MOS6581 (SID) Emulation (ReSIDfp V" << residfp_version_string << "):\n";
        ss << "\t(C) 1999-2002 Dag Lem\n";
        ss << "\t(C) 2005-2011 Antti S. Lankila\n";
        ss << "\t(C) 2010-2015 Leandro Nini\n";
        credits = ss.str();
    }

    return credits.c_str();
}

ReSIDfp::ReSIDfp(sidbuilder *builder) :
    sidemu(builder),
    m_sid(*(new reSIDfp::SID))
{
    m_buffer = new short[OUTPUTBUFFERSIZE];
    reset(0);
}

ReSIDfp::~ReSIDfp()
{
    delete &m_sid;
    delete[] m_buffer;
}

void ReSIDfp::filter6581Curve(double filterCurve)
{
   m_sid.setFilter6581Curve(filterCurve);
}

void ReSIDfp::filter8580Curve(double filterCurve)
{
   m_sid.setFilter8580Curve(filterCurve);
}

// Standard component options
void ReSIDfp::reset(uint8_t volume)
{
    m_accessClk = 0;
    m_sid.reset();
    m_sid.write(0x18, volume);
}

uint8_t ReSIDfp::read(uint_least8_t addr)
{
    clock();
    return m_sid.read(addr);
}

void ReSIDfp::write(uint_least8_t addr, uint8_t data)
{
    clock();
    m_sid.write(addr, data);
}

void ReSIDfp::clock()
{
    const event_clock_t cycles = eventScheduler->getTime(m_accessClk, EVENT_CLOCK_PHI1);
    m_accessClk += cycles;
    m_bufferpos += m_sid.clock(cycles, m_buffer+m_bufferpos);
}

void ReSIDfp::filter(bool enable)
{
      m_sid.enableFilter(enable);
}

void ReSIDfp::sampling(float systemclock, float freq,
        SidConfig::sampling_method_t method, bool)
{
    reSIDfp::SamplingMethod sampleMethod;
    switch (method)
    {
    case SidConfig::INTERPOLATE:
        sampleMethod = reSIDfp::DECIMATE;
        break;
    case SidConfig::RESAMPLE_INTERPOLATE:
        sampleMethod = reSIDfp::RESAMPLE;
        break;
    default:
        m_status = false;
        m_error = ERR_INVALID_SAMPLING;
        return;
    }

    try
    {
        // Round half frequency to the nearest multiple of 5000
        const int halfFreq = 5000*((static_cast<int>(freq)+5000)/10000);
        m_sid.setSamplingParameters(systemclock, sampleMethod, freq, std::min(halfFreq, 20000));
    }
    catch (reSIDfp::SIDError const &)
    {
        m_status = false;
        m_error = ERR_UNSUPPORTED_FREQ;
        return;
    }

    m_status = true;
}

// Set the emulated SID model
void ReSIDfp::model(SidConfig::sid_model_t model)
{
    reSIDfp::ChipModel chipModel;
    switch (model)
    {
        case SidConfig::MOS6581:
            chipModel = reSIDfp::MOS6581;
            break;
        case SidConfig::MOS8580:
            chipModel = reSIDfp::MOS8580;
            break;
        default:
            m_status = false;
            m_error = ERR_INVALID_CHIP;
            return;
    }

    m_sid.setChipModel(chipModel);
    m_status = true;
}

}
