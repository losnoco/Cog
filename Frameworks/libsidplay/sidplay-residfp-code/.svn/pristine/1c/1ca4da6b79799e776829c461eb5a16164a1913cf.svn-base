/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2015 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "SincResampler.h"

#include <cassert>
#include <cstring>
#include <cmath>
#include <iostream>
#include <sstream>
#include <limits>

#include "siddefs-fp.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_MMINTRIN_H
#  include <mmintrin.h>
#endif

namespace reSIDfp
{

typedef std::map<std::string, matrix_t> fir_cache_t;

/// Cache for the expensive FIR table computation results.
fir_cache_t FIR_CACHE;

/// Maximum error acceptable in I0 is 1e-6, or ~96 dB.
const double I0E = 1e-6;

const int BITS = 16;

/**
 * Compute the 0th order modified Bessel function of the first kind.
 * This function is originally from resample-1.5/filterkit.c by J. O. Smith.
 * It is used to build the Kaiser window for resampling.
 *
 * @param x evaluate I0 at x
 * @return value of I0 at x.
 */
double I0(double x)
{
    double sum = 1.;
    double u = 1.;
    double n = 1.;
    const double halfx = x / 2.;

    do
    {
        const double temp = halfx / n;
        u *= temp * temp;
        sum += u;
        n += 1.;
    }
    while (u >= I0E * sum);

    return sum;
}

/**
 * Calculate convolution with sample and sinc.
 *
 * @param a sample buffer input
 * @param b sinc buffer
 * @param bLength length of the sinc buffer
 * @return convolved result
 */
int convolve(const short* a, const short* b, int bLength)
{
#ifdef HAVE_MMINTRIN_H
    __m64 acc = _mm_setzero_si64();

    const int n = bLength / 4;

    for (int i = 0; i < n; i++)
    {
        const __m64 tmp = _mm_madd_pi16(*(__m64*)a, *(__m64*)b);
        acc = _mm_add_pi16(acc, tmp);
        a += 4;
        b += 4;
    }

    int out = _mm_cvtsi64_si32(acc) + _mm_cvtsi64_si32(_mm_srli_si64(acc, 32));
    _mm_empty();

    bLength &= 3;
#else
    int out = 0;
#endif

    for (int i = 0; i < bLength; i++)
    {
        out += *a++ * *b++;
    }

    return (out + (1 << 14)) >> 15;
}

int SincResampler::fir(int subcycle)
{
    // Find the first of the nearest fir tables close to the phase
    int firTableFirst = (subcycle * firRES >> 10);
    const int firTableOffset = (subcycle * firRES) & 0x3ff;

    // Find firN most recent samples, plus one extra in case the FIR wraps.
    int sampleStart = sampleIndex - firN + RINGSIZE - 1;

    const int v1 = convolve(sample + sampleStart, (*firTable)[firTableFirst], firN);

    // Use next FIR table, wrap around to first FIR table using
    // previous sample.
    if (unlikely(++firTableFirst == firRES))
    {
        firTableFirst = 0;
        ++sampleStart;
    }

    const int v2 = convolve(sample + sampleStart, (*firTable)[firTableFirst], firN);

    // Linear interpolation between the sinc tables yields good
    // approximation for the exact value.
    return v1 + (firTableOffset * (v2 - v1) >> 10);
}

SincResampler::SincResampler(double clockFrequency, double samplingFrequency, double highestAccurateFrequency) :
    sampleIndex(0),
    cyclesPerSample(static_cast<int>(clockFrequency / samplingFrequency * 1024.)),
    sampleOffset(0),
    outputValue(0)
{
    // 16 bits -> -96dB stopband attenuation.
    const double A = -20. * log10(1.0 / (1 << BITS));
    // A fraction of the bandwidth is allocated to the transition band, which we double
    // because we design the filter to transition halfway at nyquist.
    const double dw = (1. - 2.*highestAccurateFrequency / samplingFrequency) * M_PI * 2.;

    // For calculation of beta and N see the reference for the kaiserord
    // function in the MATLAB Signal Processing Toolbox:
    // http://www.mathworks.com/help/signal/ref/kaiserord.html
    const double beta = 0.1102 * (A - 8.7);
    const double I0beta = I0(beta);
    const double cyclesPerSampleD = clockFrequency / samplingFrequency;

    {
        // The filter order will maximally be 124 with the current constraints.
        // N >= (96.33 - 7.95)/(2 * pi * 2.285 * (maxfreq - passbandfreq) >= 123
        // The filter order is equal to the number of zero crossings, i.e.
        // it should be an even number (sinc is symmetric with respect to x = 0).
        int N = static_cast<int>((A - 7.95) / (2.285 * dw) + 0.5);
        N += N & 1;

        // The filter length is equal to the filter order + 1.
        // The filter length must be an odd number (sinc is symmetric with respect to
        // x = 0).
        firN = static_cast<int>(N * cyclesPerSampleD) + 1;
        firN |= 1;

        // Check whether the sample ring buffer would overflow.
        assert(firN < RINGSIZE);

        // Error is bounded by err < 1.234 / L^2, so L = sqrt(1.234 / (2^-16)) = sqrt(1.234 * 2^16).
        firRES = static_cast<int>(ceil(sqrt(1.234 * (1 << BITS)) / cyclesPerSampleD));

        // firN*firRES represent the total resolution of the sinc sampling. JOS
        // recommends a length of 2^BITS, but we don't quite use that good a filter.
        // The filter test program indicates that the filter performs well, though.
    }

    // Create the map key
    std::ostringstream o;
    o << firN << "," << firRES << "," << cyclesPerSampleD;
    const std::string firKey = o.str();
    fir_cache_t::iterator lb = FIR_CACHE.lower_bound(firKey);

    // The FIR computation is expensive and we set sampling parameters often, but
    // from a very small set of choices. Thus, caching is used to speed initialization.
    if (lb != FIR_CACHE.end() && !(FIR_CACHE.key_comp()(firKey, lb->first)))
    {
        firTable = &(lb->second);
    }
    else
    {
        // Allocate memory for FIR tables.
        matrix_t tempTable(firRES, firN);
        firTable = &(FIR_CACHE.insert(lb, fir_cache_t::value_type(firKey, tempTable))->second);

        // The cutoff frequency is midway through the transition band, in effect the same as nyquist.
        const double wc = M_PI;

        // Calculate the sinc tables.
        const double scale = 32768.0 * wc / cyclesPerSampleD / M_PI;

        for (int i = 0; i < firRES; i++)
        {
            const double jPhase = (double) i / firRES + firN / 2;

            for (int j = 0; j < firN; j++)
            {
                const double x = j - jPhase;

                const double xt = x / (firN / 2);
                const double kaiserXt = fabs(xt) < 1. ? I0(beta * sqrt(1. - xt * xt)) / I0beta : 0.;

                const double wt = wc * x / cyclesPerSampleD;
                const double sincWt = fabs(wt) >= 1e-8 ? sin(wt) / wt : 1.;

                (*firTable)[i][j] = static_cast<short>(scale * sincWt * kaiserXt);
            }
        }
    }
}

template<typename I, typename O>
inline O clip(I input)
{
    if (input < std::numeric_limits<O>::min()) input = std::numeric_limits<O>::min();
    if (input > std::numeric_limits<O>::max()) input = std::numeric_limits<O>::max();
    return static_cast<O>(input);
}

bool SincResampler::input(int input)
{
    bool ready = false;

    /*
     * Clip the input as it may overflow the 16 bit range.
     *
     * Approximate measured input ranges:
     * 6581: ]-20000,+20000[
     * 8580: ]-33000,+33000[
     */
    sample[sampleIndex] = sample[sampleIndex + RINGSIZE] = clip<int, short>(input);
    sampleIndex = (sampleIndex + 1) & (RINGSIZE - 1);

    if (sampleOffset < 1024)
    {
        outputValue = fir(sampleOffset);
        ready = true;
        sampleOffset += cyclesPerSample;
    }

    sampleOffset -= 1024;

    return ready;
}

void SincResampler::reset()
{
    memset(sample, 0, sizeof(sample));
    sampleOffset = 0;
}

} // namespace reSIDfp
