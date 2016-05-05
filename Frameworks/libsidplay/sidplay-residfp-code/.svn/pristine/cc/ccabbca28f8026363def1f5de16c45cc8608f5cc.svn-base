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

#define WAVEFORMGENERATOR_CPP

#include "WaveformGenerator.h"

#include "Dac.h"

namespace reSIDfp
{

/**
 * Number of cycles after which the waveform output fades to 0 when setting
 * the waveform register to 0.
 *
 * FIXME
 * This value has been adjusted aleatorily to ~1 sec
 * from the original reSID value (0x4000)
 * to fix /MUSICIANS/H/Hatlelid_Kris/Grand_Prix_Circuit.sid#2
 * and /MUSICIANS/P/PVCF/Thomkat_with_Strange_End.sid;
 * see [VICE Bug #290](http://sourceforge.net/p/vice-emu/bugs/290/)
 */
const int FLOATING_OUTPUT_TTL = 0xF4240;

/**
 * Number of cycles after which the shift register is reset
 * when the test bit is set.
 */
const int SHIFT_REGISTER_RESET = 0x8000;

const int DAC_BITS = 12;

void WaveformGenerator::clock_shift_register(unsigned int bit0)
{
    write_shift_register();

    shift_register = (shift_register >> 1) | bit0;

    // New noise waveform output.
    set_noise_output();
}

void WaveformGenerator::write_shift_register()
{
    if (unlikely(waveform > 0x8))
    {
        // Write changes to the shift register output caused by combined waveforms
        // back into the shift register. This happens only when the register is clocked
        // (see $D1+$81_wave_test [1]) or when the test bit is set.
        // A bit once set to zero cannot be changed, hence the and'ing.
        //
        // [1] ftp://ftp.untergrund.net/users/nata/sid_test/$D1+$81_wave_test.7z
        //
        // FIXME: Write test program to check the effect of 1 bits and whether
        // neighboring bits are affected.

        shift_register &=
            ~(
                (1 <<  2) |  // Bit 20
                (1 <<  4) |  // Bit 18
                (1 <<  8) |  // Bit 14
                (1 << 11) |  // Bit 11
                (1 << 13) |  // Bit  9
                (1 << 17) |  // Bit  5
                (1 << 20) |  // Bit  2
                (1 << 22)    // Bit  0
            ) |
            ((waveform_output & (1 << 11)) >>  9) |  // Bit 11 -> bit 20
            ((waveform_output & (1 << 10)) >>  6) |  // Bit 10 -> bit 18
            ((waveform_output & (1 <<  9)) <<  1) |  // Bit  9 -> bit 14
            ((waveform_output & (1 <<  8)) <<  3) |  // Bit  8 -> bit 11
            ((waveform_output & (1 <<  7)) <<  6) |  // Bit  7 -> bit  9
            ((waveform_output & (1 <<  6)) << 11) |  // Bit  6 -> bit  5
            ((waveform_output & (1 <<  5)) << 15) |  // Bit  5 -> bit  2
            ((waveform_output & (1 <<  4)) << 18);   // Bit  4 -> bit  0

        noise_output &= waveform_output;
        no_noise_or_noise_output = no_noise | noise_output;
    }
}

void WaveformGenerator::reset_shift_register()
{
    shift_register = 0x7fffff;
    shift_register_reset = 0;
}

void WaveformGenerator::set_noise_output()
{
    noise_output =
        ((shift_register & (1 <<  2)) <<  9) |  // Bit 20 -> bit 11
        ((shift_register & (1 <<  4)) <<  6) |  // Bit 18 -> bit 10
        ((shift_register & (1 <<  8)) <<  1) |  // Bit 14 -> bit  9
        ((shift_register & (1 << 11)) >>  3) |  // Bit 11 -> bit  8
        ((shift_register & (1 << 13)) >>  6) |  // Bit  9 -> bit  7
        ((shift_register & (1 << 17)) >> 11) |  // Bit  5 -> bit  6
        ((shift_register & (1 << 20)) >> 15) |  // Bit  2 -> bit  5
        ((shift_register & (1 << 22)) >> 18);   // Bit  0 -> bit  4

    no_noise_or_noise_output = no_noise | noise_output;
}

void WaveformGenerator::setWaveformModels(matrix_t* models)
{
    model_wave = models;
}

void WaveformGenerator::setChipModel(ChipModel chipModel)
{
    Dac dacBuilder(DAC_BITS);
    dacBuilder.kinkedDac(chipModel);

    const float offset = dacBuilder.getOutput(chipModel == MOS6581 ? 0x380 : 0x800);

    for (unsigned int i = 0; i < (1 << DAC_BITS); i++)
    {
        const double dacValue = dacBuilder.getOutput(i);
        dac[i] = static_cast<float>(dacValue - offset);
    }
}

void WaveformGenerator::synchronize(WaveformGenerator* syncDest, const WaveformGenerator* syncSource) const
{
    // A special case occurs when a sync source is synced itself on the same
    // cycle as when its MSB is set high. In this case the destination will
    // not be synced. This has been verified by sampling OSC3.
    if (unlikely(msb_rising) && syncDest->sync && !(sync && syncSource->msb_rising))
    {
        syncDest->accumulator = 0;
    }
}

void WaveformGenerator::writeCONTROL_REG(unsigned char control)
{
    const unsigned int waveform_prev = waveform;
    const bool test_prev = test;

    waveform = (control >> 4) & 0x0f;
    test = (control & 0x08) != 0;
    sync = (control & 0x02) != 0;

    // Substitution of accumulator MSB when sawtooth = 0, ring_mod = 1.
    ring_msb_mask = ((~control >> 5) & (control >> 2) & 0x1) << 23;

    if (waveform != waveform_prev)
    {
        // Set up waveform table.
        wave = (*model_wave)[waveform & 0x7];

        // no_noise and no_pulse are used in set_waveform_output() as bitmasks to
        // only let the noise or pulse influence the output when the noise or pulse
        // waveforms are selected.
        no_noise = (waveform & 0x8) != 0 ? 0x000 : 0xfff;
        no_noise_or_noise_output = no_noise | noise_output;
        no_pulse = (waveform & 0x4) != 0 ? 0x000 : 0xfff;

        if (waveform == 0)
        {
            // Change to floating DAC input.
            // Reset fading time for floating DAC input.
            floating_output_ttl = FLOATING_OUTPUT_TTL;
        }
    }

    if (test != test_prev)
    {
        if (test)
        {
            // Reset accumulator.
            accumulator = 0;

            // Flush shift pipeline.
            shift_pipeline = 0;

            // Set reset time for shift register.
            shift_register_reset = SHIFT_REGISTER_RESET;

            // Write back to the shift register.
            write_shift_register();

            // New noise waveform output.
            set_noise_output();
        }
        else
        {
            // When the test bit is falling, the second phase of the shift is
            // completed by enabling SRAM write.

            // bit0 = (bit22 | test) ^ bit17 = 1 ^ bit17 = ~bit17
            clock_shift_register((~shift_register << 17) & (1 << 22));
        }
    }
}

void WaveformGenerator::reset()
{
    accumulator = 0;
    freq = 0;
    pw = 0;

    msb_rising = false;

    waveform = 0;
    test = false;
    sync = false;

    wave = model_wave ? (*model_wave)[0] : nullptr;

    ring_msb_mask = 0;
    no_noise = 0xfff;
    no_pulse = 0xfff;
    pulse_output = 0xfff;

    reset_shift_register();
    set_noise_output();
    shift_pipeline = 0;

    waveform_output = 0;
    floating_output_ttl = 0;
}

} // namespace reSIDfp
