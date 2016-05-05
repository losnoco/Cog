/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2016 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2004 Dag Lem <resid@nimrod.no>
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

#define EXTERNALFILTER_CPP

#include "ExternalFilter.h"

namespace reSIDfp
{

/**
 * Get the 3 dB attenuation point.
 *
 * @param res the resistance value in Ohms
 * @param cap the capacitance value in Farads
 */
inline double getW0(double res, double cap)
{
    return 1. / (res * cap);
}

ExternalFilter::ExternalFilter() :
    Vlp(0),
    Vhp(0),
    w0lp_1_s7(0),
    w0hp_1_s17(0)
{
    reset();
}

void ExternalFilter::setClockFrequency(double frequency)
{
    // Low-pass: R = 10kOhm, C = 1000pF; w0l = 1/RC = 1/(1e4*1e-9) = 100000
    w0lp_1_s7 = static_cast<int>(getW0(10e3, 1000e-12) / frequency * (1 << 7) + 0.5);

    // High-pass: R = 1kOhm, C = 10uF; w0h = 1/RC = 1/(1e3*1e-5) = 100
    w0hp_1_s17 = static_cast<int>(getW0(1e3, 10e-6) / frequency * (1 << 17) + 0.5);
}

void ExternalFilter::reset()
{
    // State of filter.
    Vlp = 0; //1 << (15 + 11);
    Vhp = 0;
}

} // namespace reSIDfp
