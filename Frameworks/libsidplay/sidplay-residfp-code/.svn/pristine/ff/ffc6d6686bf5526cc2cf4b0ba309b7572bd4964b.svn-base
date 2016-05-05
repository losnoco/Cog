/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2016 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000-2001 Simon White
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

#ifndef SIDCONFIG_H
#define SIDCONFIG_H

#include <stdint.h>

#include "sidplayfp/siddefs.h"


class sidbuilder;

/**
 * SidConfig
 *
 * An instance of this class is used to transport emulator settings
 * to and from the interface class.
 */
class SID_EXTERN SidConfig
{
public:
    /// Playback mode
    typedef enum
    {
        MONO = 1,
        STEREO
    } playback_t;

    /// SID chip model
    typedef enum
    {
        MOS6581,
        MOS8580
    } sid_model_t;

    /// C64 model
    typedef enum
    {
        PAL,
        NTSC,
        OLD_NTSC,
        DREAN
    } c64_model_t;

    /// Sampling method
    typedef enum
    {
        INTERPOLATE,
        RESAMPLE_INTERPOLATE
    } sampling_method_t;

public:
    /**
     * Maximum power on delay.
     * - Delays <= MAX produce constant results
     * - Delays >  MAX produce random results
     */
    SID_DEPRECATED static const uint_least16_t MAX_POWER_ON_DELAY = 0x1FFF;
    SID_DEPRECATED static const uint_least16_t DEFAULT_POWER_ON_DELAY = MAX_POWER_ON_DELAY + 1;

    static const uint_least32_t DEFAULT_SAMPLING_FREQ  = 44100;

public:
    /**
     * Intended c64 model when unknown or forced.
     * - PAL
     * - NTSC
     * - OLD_NTSC
     * - DREAN
     */
    c64_model_t defaultC64Model;

    /**
     * Force the model to #defaultC64Model ignoring tune's clock setting.
     */
    bool forceC64Model;

    /**
     * Intended sid model when unknown or forced.
     * - MOS6581
     * - MOS8580
     */
    sid_model_t defaultSidModel;

    /**
     * Force the sid model to #defaultSidModel.
     */
    bool forceSidModel;

    /**
     * Playbak mode.
     * - MONO
     * - STEREO
     */
    playback_t playback;

    /**
     * Sampling frequency.
     */
    uint_least32_t frequency;

    /**
     * Extra SID chips addresses.
     */
    //@{
    uint_least16_t secondSidAddress;
    uint_least16_t thirdSidAddress;
    //@}

    /**
     * Pointer to selected emulation,
     * reSIDfp, reSID or hardSID.
     */
    sidbuilder *sidEmulation;

    /**
     * Left channel volume.
     */
    uint_least32_t leftVolume;

    /**
     * Right channel volume.
     */
    uint_least32_t rightVolume;

    /**
     * Power on delay cycles.
     */
    SID_DEPRECATED uint_least16_t powerOnDelay;

    /**
     * Sampling method.
     * - INTERPOLATE
     * - RESAMPLE_INTERPOLATE
     */
    sampling_method_t samplingMethod;

    /**
     * Faster low-quality emulation,
     * available only for reSID.
     */
    bool fastSampling;

    /**
     * Compare two config objects.
     *
     * @return true if different
     */
    bool compare(const SidConfig &config);

public:
    SidConfig();
};

#endif // SIDCONFIG_H
