/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2014 Leandro Nini
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "UnitTest++/UnitTest++.h"
#include "UnitTest++/TestReporter.h"

#define private public
#define protected public
#define class struct

#include "../src/builders/residfp-builder/residfp/EnvelopeGenerator.h"

using namespace UnitTest;

SUITE(EnvelopeGenerator)
{

struct TestFixture
{
    // Test setup
    TestFixture() { generator.reset(); }

    reSIDfp::EnvelopeGenerator generator;
};

TEST_FIXTURE(TestFixture, TestADSRDelayBug)
{
    // If the rate counter comparison value is set below the current value of the
    // rate counter, the counter will continue counting up until it wraps around
    // to zero at 2^15 = 0x8000, and then count rate_period - 1 before the
    // envelope can constly be stepped.

    generator.writeCONTROL_REG(0x01);

    generator.writeATTACK_DECAY(0xf0);

    for (int i=0; i<0x4000; i++)
    {
        generator.clock();
    }

    generator.writeATTACK_DECAY(0x70);

    for (int i=0; i<0x4000; i++)
    {
        generator.clock();
    }

    CHECK_EQUAL(0, (int)generator.readENV());
}

TEST_FIXTURE(TestFixture, TestFlipFFto00)
{
    // The envelope counter can flip from 0xff to 0x00 by changing state to
    // release, then to attack. The envelope counter is then frozen at
    // zero; to unlock this situation the state must be changed to release,
    // then to attack.

    generator.writeCONTROL_REG(0x01);

    generator.writeATTACK_DECAY(0xff);

    for (int i=0; i<0x7a13*0xff; i++)
    {
        generator.clock();
    }

    CHECK_EQUAL(0xff, (int)generator.readENV());

    generator.writeCONTROL_REG(0x00);
    generator.writeCONTROL_REG(0x01);

    for (int i=0; i<0x7a13; i++)
    {
        generator.clock();
    }

    CHECK_EQUAL(0, (int)generator.readENV());
    CHECK(generator.hold_zero);
}

TEST_FIXTURE(TestFixture, TestFlip00toFF)
{
    // The envelope counter can flip from 0x00 to 0xff by changing state to
    // attack, then to release. The envelope counter will then continue
    // counting down in the release state.

    generator.writeATTACK_DECAY(0xff);
    generator.writeSUSTAIN_RELEASE(0xff);

    generator.hold_zero = false;

    CHECK_EQUAL(0, (int)generator.readENV());

    generator.writeCONTROL_REG(0x01);
    generator.writeCONTROL_REG(0x00);
    for (int i=0; i<0x7a13; i++)
    {
        generator.clock();
    }

    CHECK_EQUAL(0xff, (int)generator.readENV());
    CHECK(!generator.hold_zero);
}

}
