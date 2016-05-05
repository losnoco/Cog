/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2016 Leandro Nini <drfiemost@users.sourceforge.net>
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

#define ENVELOPEGENERATOR_CPP

#include "EnvelopeGenerator.h"

#include "Dac.h"

namespace reSIDfp
{

const unsigned int DAC_BITS = 8;

const unsigned int EnvelopeGenerator::adsrtable[16] =
{
    0x007f,
    0x3000,
    0x1e00,
    0x0660,
    0x0182,
    0x5573,
    0x000e,
    0x3805,
    0x2424,
    0x2220,
    0x090c,
    0x0ecd,
    0x010e,
    0x23f7,
    0x5237,
    0x64a8
};

void EnvelopeGenerator::set_exponential_counter()
{
    // Check for change of exponential counter period.
    //
    // For a detailed description see:
    // http://ploguechipsounds.blogspot.it/2010/03/sid-6581r3-adsr-tables-up-close.html
    switch (envelope_counter)
    {
    case 0xff:
        exponential_counter_period = 1;
        break;

    case 0x5d:
        exponential_counter_period = 2;
        break;

    case 0x36:
        exponential_counter_period = 4;
        break;

    case 0x1a:
        exponential_counter_period = 8;
        break;

    case 0x0e:
        exponential_counter_period = 16;
        break;

    case 0x06:
        exponential_counter_period = 30;
        break;

    case 0x00:
        // FIXME: Check whether 0x00 really changes the period.
        // E.g. set R = 0xf, gate on to 0x06, gate off to 0x00, gate on to 0x04,
        // gate off, sample.
        exponential_counter_period = 1;

        // When the envelope counter is changed to zero, it is frozen at zero.
        // This has been verified by sampling ENV3.
        hold_zero = true;
        break;
    }
}

void EnvelopeGenerator::setChipModel(ChipModel chipModel)
{
    Dac dacBuilder(DAC_BITS);
    dacBuilder.kinkedDac(chipModel);

    for (unsigned int i = 0; i < (1 << DAC_BITS); i++)
    {
        dac[i] = static_cast<float>(dacBuilder.getOutput(i));
    }
}

void EnvelopeGenerator::reset()
{
    envelope_counter = 0;
    envelope_pipeline = false;

    attack = 0;
    decay = 0;
    sustain = 0;
    release = 0;

    gate = false;

    lfsr = 0x7fff;
    exponential_counter = 0;
    exponential_counter_period = 1;

    state = RELEASE;
    rate = adsrtable[release];
    hold_zero = true;
}

void EnvelopeGenerator::writeCONTROL_REG(unsigned char control)
{
    const bool gate_next = (control & 0x01) != 0;

    if (gate_next == gate)
        return;

    // The rate counter is never reset, thus there will be a delay before the
    // envelope counter starts counting up (attack) or down (release).

    if (gate_next)
    {
        // Gate bit on: Start attack, decay, sustain.
        state = ATTACK;
        // FIXME: there's a single cycle where SID is using the rate from 'decay' rather than attack
        // http://csdb.dk/forums/?roomid=14&topicid=110119&showallposts=1
        rate = adsrtable[attack];

        // Switching to attack state unlocks the zero freeze and aborts any
        // pipelined envelope decrement.
        hold_zero = false;

        // FIXME: This is an assumption which should be checked using cycle exact
        // envelope sampling.
        envelope_pipeline = false;
    }
    else
    {
        // Gate bit off: Start release.
        state = RELEASE;
        rate = adsrtable[release];
    }

    gate = gate_next;
}

void EnvelopeGenerator::writeATTACK_DECAY(unsigned char attack_decay)
{
    attack = (attack_decay >> 4) & 0x0f;
    decay = attack_decay & 0x0f;

    if (state == ATTACK)
    {
        rate = adsrtable[attack];
    }
    else if (state == DECAY_SUSTAIN)
    {
        rate = adsrtable[decay];
    }
}

void EnvelopeGenerator::writeSUSTAIN_RELEASE(unsigned char sustain_release)
{
    // From the sustain levels it follows that both the low and high 4 bits
    // of the envelope counter are compared to the 4-bit sustain value.
    // This has been verified by sampling ENV3.
    //
    // For a detailed description see:
    // http://ploguechipsounds.blogspot.it/2010/11/new-research-on-sid-adsr.html
    sustain = (sustain_release & 0xf0) | ((sustain_release >> 4) & 0x0f);

    release = sustain_release & 0x0f;

    if (state == RELEASE)
    {
        rate = adsrtable[release];
    }
}

} // namespace reSIDfp
