/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2014-2016 Leandro Nini
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

#include "../src/builders/residfp-builder/residfp/Dac.h"

using namespace UnitTest;
using namespace reSIDfp;

#define DAC_BITS 8

SUITE(Dac)
{

void buildDac(float dac[], ChipModel chipModel)
{
    Dac dacBuilder(DAC_BITS);
    dacBuilder.kinkedDac(chipModel);

    for (unsigned int i = 0; i < (1 << DAC_BITS); i++)
    {
        dac[i] = dacBuilder.getOutput(i);
    }
}

bool isDacLinear(ChipModel chipModel)
{
    float dac[1 << DAC_BITS];
    buildDac(dac, chipModel);

    for (int i = 1; i < (1 << DAC_BITS); i++)
    {
        if (dac[i] <= dac[i-1])
            return false;
    }

    return true;
}

TEST(TestDac6581)
{
    // Test the non-linearity of the 6581 DACs

    CHECK(!isDacLinear(MOS6581));
}

TEST(TestDac8580)
{
    // Test the linearity of the 8580 DACs

    CHECK(isDacLinear(MOS8580));
}

}
