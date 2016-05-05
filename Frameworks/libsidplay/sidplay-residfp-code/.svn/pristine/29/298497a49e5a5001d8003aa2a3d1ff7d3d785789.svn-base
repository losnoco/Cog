/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2015 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef ENVELOPEGENERATOR_H
#define ENVELOPEGENERATOR_H

#include "siddefs-fp.h"

namespace reSIDfp
{

/**
 * A 15 bit [LFSR] is used to implement the envelope rates, in effect dividing
 * the clock to the envelope counter by the currently selected rate period.
 *
 * In addition, another counter is used to implement the exponential envelope decay,
 * in effect further dividing the clock to the envelope counter.
 * The period of this counter is set to 1, 2, 4, 8, 16, 30 at the envelope counter
 * values 255, 93, 54, 26, 14, 6, respectively.
 * 
 * [LFSR]: https://en.wikipedia.org/wiki/Linear_feedback_shift_register
 */
class EnvelopeGenerator
{
private:
    /**
     * The envelope state machine's distinct states. In addition to this,
     * envelope has a hold mode, which freezes envelope counter to zero.
     */
    enum State
    {
        ATTACK, DECAY_SUSTAIN, RELEASE
    };

private:
    /// XOR shift register for ADSR prescaling.
    unsigned int lfsr;

    /// Comparison value (period) of the rate counter before next event.
    unsigned int rate;

    /**
     * During release mode, the SID arpproximates envelope decay via piecewise
     * linear decay rate.
     */
    unsigned int exponential_counter;

    /**
     * Comparison value (period) of the exponential decay counter before next
     * decrement.
     */
    unsigned int exponential_counter_period;

    /// Current envelope state
    State state;

    /// Whether hold is enabled. Only switching to ATTACK can release envelope.
    bool hold_zero;

    bool envelope_pipeline;

    /// Gate bit
    bool gate;

    /// The current digital value of envelope output.
    unsigned char envelope_counter;

    /// Attack register
    unsigned char attack;

    /// Decay register
    unsigned char decay;

    /// Sustain register
    unsigned char sustain;

    /// Release register
    unsigned char release;

    /**
     * Emulated nonlinearity of the envelope DAC.
     *
     * @See SID.kinked_dac
     */
    float dac[256];

private:
    /**
     * Lookup table to convert from attack, decay, or release value to rate
     * counter period.
     *
     * The rate counter is a 15 bit register which is left shifted each cycle.
     * When the counter reaches a specific comparison value,
     * the envelope counter is incremented (attack) or decremented
     * (decay/release) and the rate counter is resetted.
     *
     * see [kevtris.org](http://blog.kevtris.org/?p=13)
     */
    static const unsigned int adsrtable[16];

private:
    void set_exponential_counter();

public:
    /**
     * Set chip model.
     * This determines the type of the analog DAC emulation:
     * 8580 is perfectly linear while 6581 is nonlinear.
     *
     * @param chipModel
     */
    void setChipModel(ChipModel chipModel);

    /**
     * SID clocking.
     */
    void clock();

    /**
     * Get the Envelope Generator output.
     * DAC imperfections are emulated by using envelope_counter as an index
     * into a DAC lookup table. readENV() uses envelope_counter directly.
     */
    float output() const { return dac[envelope_counter]; }

    /**
     * Constructor.
     */
    EnvelopeGenerator() :
        lfsr(0),
        rate(0),
        exponential_counter(0),
        exponential_counter_period(1),
        state(RELEASE),
        hold_zero(true),
        envelope_pipeline(false),
        gate(false),
        envelope_counter(0),
        attack(0),
        decay(0),
        sustain(0),
        release(0) {}

    /**
     * SID reset.
     */
    void reset();

    /**
     * Write control register.
     *
     * @param control
     *            control register
     */
    void writeCONTROL_REG(unsigned char control);

    /**
     * Write Attack/Decay register.
     *
     * @param attack_decay
     *            attack/decay value
     */
    void writeATTACK_DECAY(unsigned char attack_decay);

    /**
     * Write Sustain/Release register.
     *
     * @param sustain_release
     *            sustain/release value
     */
    void writeSUSTAIN_RELEASE(unsigned char sustain_release);

    /**
     * Return the envelope current value.
     *
     * @return envelope counter
     */
    unsigned char readENV() const { return envelope_counter; }
};

} // namespace reSIDfp

#if RESID_INLINING || defined(ENVELOPEGENERATOR_CPP)

namespace reSIDfp
{

RESID_INLINE
void EnvelopeGenerator::clock()
{
    if (unlikely(envelope_pipeline))
    {
        --envelope_counter;
        envelope_pipeline = false;
        // Check for change of exponential counter period.
        set_exponential_counter();
    }

    // Check for ADSR delay bug.
    // If the rate counter comparison value is set below the current value of the
    // rate counter, the counter will continue counting up until it wraps around
    // to zero at 2^15 = 0x8000, and then count rate_period - 1 before the
    // envelope can constly be stepped.
    // This has been verified by sampling ENV3.
    //
    // Note: Envelope is now implemented like in the real machine with a shift register
    // so the ADSR delay bug should be correcly modeled

    // check to see if LFSR matches table value
    if (likely(lfsr != rate))
    {
        // it wasn't a match, clock the LFSR once
        // by performing XOR on last 2 bits
        const unsigned int feedback = ((lfsr << 14) ^ (lfsr << 13)) & 0x4000;
        lfsr = (lfsr >> 1) | feedback;
        return;
    }

    // reset LFSR
    lfsr = 0x7fff;

    // The first envelope step in the attack state also resets the exponential
    // counter. This has been verified by sampling ENV3.
    if (state == ATTACK || ++exponential_counter == exponential_counter_period)
    {
        // likely (~50%)
        exponential_counter = 0;

        // Check whether the envelope counter is frozen at zero.
        if (unlikely(hold_zero))
        {
            return;
        }

        switch (state)
        {
        case ATTACK:
            // The envelope counter can flip from 0xff to 0x00 by changing state to
            // release, then to attack. The envelope counter is then frozen at
            // zero; to unlock this situation the state must be changed to release,
            // then to attack. This has been verified by sampling ENV3.
            ++envelope_counter;

            if (unlikely(envelope_counter == 0xff))
            {
                state = DECAY_SUSTAIN;
                rate = adsrtable[decay];
            }

            break;

        case DECAY_SUSTAIN:
            if (likely(envelope_counter == sustain))
            {
                return;
            }

            // fall-through

        case RELEASE:
            // The envelope counter can flip from 0x00 to 0xff by changing state to
            // attack, then to release. The envelope counter will then continue
            // counting down in the release state.
            // This has been verified by sampling ENV3.
            // NB! The operation below requires two's complement integer.
            if (unlikely(exponential_counter_period != 1))
            {
                // The decrement is delayed one cycle.
                envelope_pipeline = true;
                return;
            }

            --envelope_counter;
            break;
        }

        // Check for change of exponential counter period.
        set_exponential_counter();
    }
}

} // namespace reSIDfp

#endif

#endif
