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

#include "../src/builders/residfp-builder/residfp/Dac.h"

using namespace UnitTest;
using namespace reSIDfp;

#define DAC_BITS 8

SUITE(Dac)
{

void getDac(float dac[], double _2R_div_R, bool term)
{
    double dacBits[DAC_BITS];
    Dac::kinkedDac(dacBits, DAC_BITS, _2R_div_R, term);

    for (unsigned int i = 0; i < (1 << DAC_BITS); i++)
    {
        double dacValue = 0.;

        for (unsigned int j = 0; j < DAC_BITS; j++)
        {
            if ((i & (1 << j)) != 0)
            {
                dacValue += dacBits[j];
            }
        }

        dac[i] = static_cast<float>(dacValue);
    }
}

bool isDacLinear(double _2R_div_R, bool term)
{
    float dac[1 << DAC_BITS];
    getDac(dac, _2R_div_R, term);

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

    CHECK(!isDacLinear(2.2, false));
}

TEST(TestDac8580)
{
    // Test the linearity of the 8580 DACs

    CHECK(isDacLinear(2.0, true));
}

}
