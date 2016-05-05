/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2014 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2004,2010 Dag Lem <resid@nimrod.no>
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

#ifndef FILTER8580_H
#define FILTER8580_H

#include <cmath>
#include <cstring>

#include <stdint.h>

#include "siddefs-fp.h"

#include "Filter.h"

#include "sidcxx11.h"

namespace reSIDfp
{

/**
 * Simple white noise generator.
 * Generates small low quality pseudo random numbers
 * useful to prevent float denormals.
 *
 * Based on the paper [Denormal numbers in floating point signal
 * processing applications](http://ldesoras.free.fr/prod.html#doc_denormal)
 * from Laurent de Soras.
 */
class antiDenormalNoise
{
private:
    uint32_t rand_state;

private:
    /**
     * Reduce 32bit integer to float with a magnitude of about 10^–20.
     */
    static inline float reduce(uint32_t val)
    {
        // FIXME may not be fully portable
        // This code assumes IEEE-754 floating point representation
        // and same endianness for integers and floats
        const uint32_t mantissa = val & 0x807F0000; // Keep only most significant bits
        const uint32_t flt_rnd = mantissa | 0x1E000000; // Set exponent
        float temp;
        memcpy(&temp, &flt_rnd, sizeof(float));
        return temp;
    }

public:
    antiDenormalNoise() :
        rand_state(1) {}

    inline float get()
    {
        // LCG from Numerical Recipes
        rand_state = rand_state * 1664525 + 1013904223;

        return reduce(rand_state);
    }
};

/**
 * Filter for 8580 chip based on simple linear approximation
 * of the FC control.
 */
class Filter8580 final : public Filter
{
private:
    /// Cutoff frequency in Hertz
    double highFreq;

    /// Lowpass filter voltage
    float Vlp;

    /// Bandpass filter voltage
    float Vbp;

    /// Highpass filter voltage
    float Vhp;

    float w0;

    /// Resonance parameter
    float _1_div_Q;

    /// External input voltage
    int ve;

    antiDenormalNoise noise;

public:
    Filter8580() :
        highFreq(12500.),
        Vlp(0.f),
        Vbp(0.f),
        Vhp(0.f),
        w0(0.f),
        _1_div_Q(0.f),
        ve(0) {}

    int clock(int voice1, int voice2, int voice3) override;

    /**
     * Set filter cutoff frequency.
     */
    void updatedCenterFrequency() override { w0 = static_cast<float>(2. * M_PI * highFreq * fc / 2047. / 1e6); }

    /**
     * Set filter resonance.
     *
     * The following function for 1/Q has been modeled in the MOS 8580:
     *
     * 1/Q = 2^(1/2)*2^(-x/8) = 2^(1/2 - x/8) = 2^((4 - x)/8)
     *
     * @param res the new resonance value
     */
    void updateResonance(unsigned char res) override { _1_div_Q = static_cast<float>(pow(2., (4 - res) / 8.)); }

    void input(int input) override { ve = input << 4; }

    void updatedMixing() override {}

    /**
     * Set filter curve type based on single parameter.
     *
     * @param curvePosition filter's center frequency expressed in Hertz, default is 12500
     */
    void setFilterCurve(double curvePosition) { highFreq = curvePosition; }
};

} // namespace reSIDfp

#if RESID_INLINING || defined(FILTER8580_CPP)

#include <cassert>

namespace reSIDfp
{

RESID_INLINE
int Filter8580::clock(int voice1, int voice2, int voice3)
{
    int Vi = 0;
    int Vo = 0;

    (filt1 ? Vi : Vo) += voice1;
    (filt2 ? Vi : Vo) += voice2;

    // NB! Voice 3 is not silenced by voice3off if it is routed
    // through the filter.
    if (filt3) Vi += voice3;
    else if (!voice3off) Vo += voice3;

    (filtE ? Vi : Vo) += ve;

    Vlp -= w0 * Vbp;
    Vbp -= w0 * Vhp;
    Vhp = (Vbp * _1_div_Q) - Vlp - static_cast<float>(Vi >> 7) + noise.get();

    assert(std::fpclassify(Vlp) != FP_SUBNORMAL);
    assert(std::fpclassify(Vbp) != FP_SUBNORMAL);
    assert(std::fpclassify(Vhp) != FP_SUBNORMAL);

    float Vof = static_cast<float>(Vo >> 7);

    if (lp) Vof += Vlp;
    if (bp) Vof += Vbp;
    if (hp) Vof += Vhp;

    return static_cast<int>(floor(Vof + 0.5f)) * vol >> 4;
}

} // namespace reSIDfp

#endif

#endif
