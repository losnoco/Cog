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

#define BUFFERSIZE 128

#define VERSION_LO      5
#define DATAOFFSET_HI   6
#define DATAOFFSET_LO   7
#define LOADADDRESS_HI  8
#define LOADADDRESS_LO  9
#define INITADDRESS_HI 10
#define INITADDRESS_LO 11
#define PLAYADDRESS_HI 12
#define PLAYADDRESS_LO 13
#define SONGS_HI       14
#define SONGS_LO       15
#define STARTSONG_LO   17
#define SPEED_LO_LO    21

#define FLAGS              119
#define STARTPAGE          120
#define PAGELENGTH         121
#define SECONDSIDADDRESS   122
#define THIRDSIDADDRESS    123

using namespace UnitTest;

uint8_t const bufferRSID[BUFFERSIZE] = {
    0x52, 0x53, 0x49, 0x44, // magicID
    0x00, 0x02,             // version
    0x00, 0x7C,             // dataOffset
    0x00, 0x00,             // loadAddress
    0x00, 0x00,             // initAddress
    0x00, 0x00,             // playAddress
    0x00, 0x01,             // songs
    0x00, 0x00,             // startSong
    0x00, 0x00, 0x00, 0x00, // speed
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // name
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // author
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // released
    0x00, 0x00,             // flags
    0x00,                   // startPage
    0x00,                   // pageLength
    0x00,                   // secondSIDAddress
    0x00,                   // thirdSIDAddress
    0xe8, 0x07, 0x00, 0x00  // data
};

SUITE(RSID)
{

struct TestFixture
{
    // Test setup
    TestFixture() { memcpy(data, bufferRSID, BUFFERSIZE); }

    uint8_t data[BUFFERSIZE];
};

/*
 * Check that unmodified data loads ok.
 */
TEST_FIXTURE(TestFixture, TestLoadOk)
{
    SidTune tune(data, BUFFERSIZE);
    CHECK(tune.getStatus());

    CHECK_EQUAL("No errors", tune.statusString());
}

/*
 * Version must be at least 2 for RSID files.
 */
TEST_FIXTURE(TestFixture, TestUnsupportedVersion)
{
    data[VERSION_LO] = 0x01;

    SidTune tune(data, BUFFERSIZE);
    CHECK(!tune.getStatus());

    CHECK_EQUAL("Unsupported RSID version", tune.statusString());
}

/*
 * Load address must always be 0 for RSID files.
 */
TEST_FIXTURE(TestFixture, TestWrongLoadAddress)
{
    data[LOADADDRESS_LO] = 0xff;

    SidTune tune(data, BUFFERSIZE);
    CHECK(!tune.getStatus());

    CHECK_EQUAL("SIDTUNE ERROR: File contains invalid data", tune.statusString());
}

/*
 * Actual Load Address must NOT be less than $07E8 for RSID files.
 */
TEST_FIXTURE(TestFixture, TestWrongActualLoadAddress)
{
    data[124] = 0xe7;
    data[125] = 0x07;

    SidTune tune(data, BUFFERSIZE);
    CHECK(!tune.getStatus());

    CHECK_EQUAL("SIDTUNE ERROR: Bad address data", tune.statusString());
}

/*
 * PlayAddress must always be 0 for RSID files.
 */
TEST_FIXTURE(TestFixture, TestWrongPlayAddress)
{
    data[PLAYADDRESS_LO] = 0xff;

    SidTune tune(data, BUFFERSIZE);
    CHECK(!tune.getStatus());

    CHECK_EQUAL("SIDTUNE ERROR: File contains invalid data", tune.statusString());
}

/*
 * Speed must always be 0 for RSID files.
 */
TEST_FIXTURE(TestFixture, TestWrongSpeed)
{
    data[SPEED_LO_LO] = 0xff;

    SidTune tune(data, BUFFERSIZE);
    CHECK(!tune.getStatus());

    CHECK_EQUAL("SIDTUNE ERROR: File contains invalid data", tune.statusString());
}

/*
 * DataOffset must always be 0x007C for RSID files.
 */
TEST_FIXTURE(TestFixture, TestWrongDataOffset)
{
    data[DATAOFFSET_LO] = 0x76;

    SidTune tune(data, BUFFERSIZE);
    CHECK(!tune.getStatus());

    CHECK_EQUAL("SIDTUNE ERROR: Bad address data", tune.statusString());
}

/*
 * InitAddress must never point to a ROM area for RSID files.
 */
TEST_FIXTURE(TestFixture, TestWrongInitAddressRom)
{
    data[INITADDRESS_HI] = 0xb0;

    SidTune tune(data, BUFFERSIZE);
    CHECK(!tune.getStatus());

    CHECK_EQUAL("SIDTUNE ERROR: Bad address data", tune.statusString());
}

/*
 * InitAddress must never be lower than $07E8 for RSID files.
 */
TEST_FIXTURE(TestFixture, TestWrongInitAddressTooLow)
{
    data[INITADDRESS_HI] = 0x07;
    data[INITADDRESS_LO] = 0xe7;

    SidTune tune(data, BUFFERSIZE);
    CHECK(!tune.getStatus());

    CHECK_EQUAL("SIDTUNE ERROR: Bad address data", tune.statusString());
}

/*
 * The maximum number of songs is 256.
 */
TEST_FIXTURE(TestFixture, TestTooManySongs)
{
    data[SONGS_HI] = 0x01;
    data[SONGS_LO] = 0x01;

    SidTune tune(data, BUFFERSIZE);
    //CHECK(!tune.getStatus());

    //CHECK_EQUAL("SIDTUNE ERROR: ", tune.statusString());
    CHECK_EQUAL(256, tune.getInfo()->songs());
}

/*
 * The song number to be played by default has a default of 1.
 */
TEST_FIXTURE(TestFixture, TestDefaultStartSong)
{
    SidTune tune(data, BUFFERSIZE);

    CHECK_EQUAL(1, tune.getInfo()->startSong());
}

/*
 * If 'startPage' is 0 or 0xFF, 'pageLength' must be set to 0.
 */
TEST_FIXTURE(TestFixture, TestWrongPageLength)
{
    data[STARTPAGE] = 0xff;
    data[PAGELENGTH] = 0x77;

    SidTune tune(data, BUFFERSIZE);

    CHECK_EQUAL(0, tune.getInfo()->relocPages());
}

/////////////
// TEST v3 //
/////////////

/*
 * $d420 is a valid second SID address.
 */
TEST_FIXTURE(TestFixture, TestSecondSIDAddressOk)
{
    data[VERSION_LO] = 0x03;
    data[SECONDSIDADDRESS] = 0x42;

    SidTune tune(data, BUFFERSIZE);

    CHECK_EQUAL(0xd420, tune.getInfo()->sidChipBase(1));
}

/*
 * SecondSIDAddress: only even values are valid.
 */
TEST_FIXTURE(TestFixture, TestWrongSecondSIDAddressOdd)
{
    data[VERSION_LO] = 0x03;
    data[SECONDSIDADDRESS] = 0x43;

    SidTune tune(data, BUFFERSIZE);

    CHECK_EQUAL(0, tune.getInfo()->sidChipBase(1));
}

/*
 * SecondSIDAddress: Ranges $00-$41 ($D000-$D410) and
 * $80-$DF ($D800-$DDF0) are invalid.
 */
TEST_FIXTURE(TestFixture, TestWrongSecondSIDAddressOutOfRange)
{
    data[VERSION_LO] = 0x03;
    data[SECONDSIDADDRESS] = 0x80;

    SidTune tune(data, BUFFERSIZE);

    CHECK_EQUAL(0, tune.getInfo()->sidChipBase(1));
}

/////////////
// TEST v4 //
/////////////

/*
 * $d500 is a valid third SID address.
 */
TEST_FIXTURE(TestFixture, TestThirdSIDAddressOk)
{
    data[VERSION_LO] = 0x04;
    data[SECONDSIDADDRESS] = 0x42;
    data[THIRDSIDADDRESS] = 0x50;

    SidTune tune(data, BUFFERSIZE);

    CHECK_EQUAL(0xd500, tune.getInfo()->sidChipBase(2));
}

/*
 * ThirdSIDAddress: only even values are valid.
 */
TEST_FIXTURE(TestFixture, TestWrongThirdSIDAddressOdd)
{
    data[VERSION_LO] = 0x04;
    data[SECONDSIDADDRESS] = 0x42;
    data[THIRDSIDADDRESS] = 0x43;

    SidTune tune(data, BUFFERSIZE);

    CHECK_EQUAL(0, tune.getInfo()->sidChipBase(2));
}

/*
 * ThirdSIDAddress: Ranges $00-$41 ($D000-$D410) and
 * $80-$DF ($D800-$DDF0) are invalid.
 */
TEST_FIXTURE(TestFixture, TestWrongThirdSIDAddressOutOfRange)
{
    data[VERSION_LO] = 0x04;
    data[SECONDSIDADDRESS] = 0x42;
    data[THIRDSIDADDRESS] = 0x80;

    SidTune tune(data, BUFFERSIZE);

    CHECK_EQUAL(0, tune.getInfo()->sidChipBase(2));
}

/*
 * The address of the third SID cannot be the same as the second SID.
 */
TEST_FIXTURE(TestFixture, TestWrongThirdSIDAddressLikeSecond)
{
    data[VERSION_LO] = 0x04;
    data[SECONDSIDADDRESS] = 0x42;
    data[THIRDSIDADDRESS] = 0x42;

    SidTune tune(data, BUFFERSIZE);

    CHECK_EQUAL(0, tune.getInfo()->sidChipBase(2));
}

}
