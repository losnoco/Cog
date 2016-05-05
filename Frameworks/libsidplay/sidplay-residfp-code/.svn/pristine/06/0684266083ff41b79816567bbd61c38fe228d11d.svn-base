/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2015 Leandro Nini
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

#include "../src/sidplayfp/SidTune.h"
#include "../src/sidplayfp/SidTuneInfo.h"

#include <stdint.h>
#include <cstring>

#define BUFFERSIZE 22

#define LOADADDRESS_HI  1
#define LOADADDRESS_LO  0

using namespace UnitTest;

uint8_t const bufferMUS[BUFFERSIZE] =
{
    0x52, 0x53,             // load address
    0x04, 0x00,             // length of the data for Voice 1
    0x04, 0x00,             // length of the data for Voice 2
    0x04, 0x00,             // length of the data for Voice 3
    0x00, 0x00, 0x01, 0x4F, // data for Voice 1
    0x00, 0x00, 0x01, 0x4F, // data for Voice 2
    0x00, 0x01, 0x01, 0x4F, // data for Voice 3
    0x00, 0x00,             // text description
};

SUITE(MUS)
{

struct TestFixture
{
    // Test setup
    TestFixture() { memcpy(data, bufferMUS, BUFFERSIZE); }

    uint8_t data[BUFFERSIZE];
};

TEST_FIXTURE(TestFixture, TestPlayerAddress)
{
    SidTune tune(data, BUFFERSIZE);

    CHECK_EQUAL(0xec60, tune.getInfo()->initAddr());
    CHECK_EQUAL(0xec80, tune.getInfo()->playAddr());
}

}
