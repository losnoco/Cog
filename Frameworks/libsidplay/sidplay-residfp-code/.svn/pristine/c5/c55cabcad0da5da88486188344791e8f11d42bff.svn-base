/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2015 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright (C) 2000 Simon White
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef MIXER_H
#define MIXER_H

#include "sidcxx11.h"

#include <stdint.h>
#include <cstdlib>

#include <vector>

namespace libsidplayfp
{

class sidemu;

/**
 * This class implements the mixer.
 */
class Mixer
{
public:
    /// Maximum number of supported SIDs
    static const unsigned int MAX_SIDS = 3;

    static const int_least32_t SCALE_FACTOR = 1 << 16;
#ifdef HAVE_CXX11
    static constexpr double SQRT_0_5 = 0.70710678118654746;
#else
#  define SQRT_0_5 0.70710678118654746
#endif
    static const int_least32_t C1 = static_cast<int_least32_t>(1.0 / (1.0 + SQRT_0_5) * SCALE_FACTOR);
    static const int_least32_t C2 = static_cast<int_least32_t>(SQRT_0_5 / (1.0 + SQRT_0_5) * SCALE_FACTOR);

private:
    typedef int_least32_t (Mixer::*mixer_func_t)() const;

public:
    /// Maximum allowed volume, must be a power of 2.
    static const int_least32_t VOLUME_MAX = 1024;

private:
    std::vector<sidemu*> m_chips;
    std::vector<short*> m_buffers;

    std::vector<int_least32_t> m_iSamples;
    std::vector<int_least32_t> m_volume;

    std::vector<mixer_func_t> m_mix;

    int oldRandomValue;
    int m_fastForwardFactor;

    // Mixer settings
    short         *m_sampleBuffer;
    uint_least32_t m_sampleCount;
    uint_least32_t m_sampleIndex;

    bool m_stereo;

private:
    void updateParams();

    int triangularDithering()
    {
        const int prevValue = oldRandomValue;
        oldRandomValue = rand() & (VOLUME_MAX-1);
        return oldRandomValue - prevValue;
    }

    /*
     * Channel matrix
     *
     *   C1
     * L 1.0
     * R 1.0
     *
     *   C1   C2
     * L 1.0  0.0
     * R 0.0  1.0
     *
     *   C1       C2           C3
     * L 1/1.707  0.707/1.707  0.0
     * R 0.0      0.707/1.707  1/1.707
     * 
     * FIXME
     * it seems that scaling down the summed signals is not the correct way of mixing, see:
     * http://dsp.stackexchange.com/questions/3581/algorithms-to-mix-audio-signals-without-clipping
     * maybe we should consider some form of soft/hard clipping instead to avoid possible overflows
     */

    // Mono mixing
    template <int Chips>
    int_least32_t mono() const
    {
        int_least32_t res = 0;
        for (int i = 0; i < Chips; i++)
            res += m_iSamples[i];
        return res /= Chips;
    }

    // Stereo mixing
    int_least32_t stereo_OneChip() const { return m_iSamples[0]; }

    int_least32_t stereo_ch1_TwoChips() const { return m_iSamples[0]; }
    int_least32_t stereo_ch2_TwoChips() const { return m_iSamples[1]; }

    int_least32_t stereo_ch1_ThreeChips() const { return (C1*m_iSamples[0] + C2*m_iSamples[1]) / SCALE_FACTOR; }
    int_least32_t stereo_ch2_ThreeChips() const { return (C2*m_iSamples[1] + C1*m_iSamples[2]) / SCALE_FACTOR; }

public:
    /**
     * Create a new mixer.
     */
    Mixer() :
        oldRandomValue(0),
        m_fastForwardFactor(1),
        m_sampleCount(0),
        m_stereo(false)
    {
        m_mix.push_back(&Mixer::mono<1>);
    }

    /**
     * Do the mixing.
     */
    void doMix();

    /**
     * This clocks the SID chips to the present moment, if they aren't already.
     */
    void clockChips();

    /**
     * Reset sidemu buffer position discarding produced samples.
     */
    void resetBufs();

    /**
     * Prepare for mixing cycle.
     *
     * @param buffer output buffer
     * @param count size of the buffer in samples
     */
    void begin(short *buffer, uint_least32_t count);

    /**
     * Remove all SIDs from the mixer.
     */
    void clearSids();

    /**
     * Add a SID to the mixer.
     *
     * @param chip the sid emu to add
     */
    void addSid(sidemu *chip);

    /**
     * Get a SID from the mixer.
     *
     * @param i the number of the SID to get
     * @return a pointer to the requested sid emu or 0 if not found
     */
    sidemu* getSid(unsigned int i) const { return (i < m_chips.size()) ? m_chips[i] : nullptr; }

    /**
     * Set the fast forward ratio.
     *
     * @param ff the fast forward ratio, from 1 to 32
     * @return true if parameter is valid, false otherwise
     */
    bool setFastForward(int ff);

    /**
     * Set mixing volumes, from 0 to #VOLUME_MAX.
     *
     * @param left volume for left or mono channel
     * @param right volume for right channel in stereo mode
     */
    void setVolume(int_least32_t left, int_least32_t right);

    /**
     * Set mixing mode.
     *
     * @param stereo true for stereo mode, false for mono
     */
    void setStereo(bool stereo);

    /**
     * Check if the buffer have been filled.
     */
    bool notFinished() const { return m_sampleIndex != m_sampleCount; }

    /**
     * Get the number of samples generated up to now.
     */
    uint_least32_t samplesGenerated() const { return m_sampleIndex; }
};

}

#endif // MIXER_H
