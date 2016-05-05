/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2014-2015 Leandro Nini
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

#include <limits>

#include "../src/builders/residfp-builder/residfp/Spline.h"

using namespace UnitTest;
using namespace reSIDfp;

const unsigned int OPAMP_SIZE = 33;

const Spline::Point opamp_voltage[OPAMP_SIZE] =
{
  {  0.81, 10.31 },  // Approximate start of actual range
  {  2.40, 10.31 },
  {  2.60, 10.30 },
  {  2.70, 10.29 },
  {  2.80, 10.26 },
  {  2.90, 10.17 },
  {  3.00, 10.04 },
  {  3.10,  9.83 },
  {  3.20,  9.58 },
  {  3.30,  9.32 },
  {  3.50,  8.69 },
  {  3.70,  8.00 },
  {  4.00,  6.89 },
  {  4.40,  5.21 },
  {  4.54,  4.54 },  // Working point (vi = vo)
  {  4.60,  4.19 },
  {  4.80,  3.00 },
  {  4.90,  2.30 },  // Change of curvature
  {  4.95,  2.03 },
  {  5.00,  1.88 },
  {  5.05,  1.77 },
  {  5.10,  1.69 },
  {  5.20,  1.58 },
  {  5.40,  1.44 },
  {  5.60,  1.33 },
  {  5.80,  1.26 },
  {  6.00,  1.21 },
  {  6.40,  1.12 },
  {  7.00,  1.02 },
  {  7.50,  0.97 },
  {  8.50,  0.89 },
  { 10.00,  0.81 },
  { 10.31,  0.81 },  // Approximate end of actual range
};

SUITE(Spline)
{

TEST(TestMonotonicity)
{
    Spline s(opamp_voltage, OPAMP_SIZE);

    double old = std::numeric_limits<double>::max();
    for (double x = 0.0; x < 12.0; x+=0.01)
    {
        Spline::Point out = s.evaluate(x);

        CHECK(out.x <= old);

        old = out.x;
    }
}

TEST(TestPoints)
{
    Spline s(opamp_voltage, OPAMP_SIZE);

    for (int i = 0; i < OPAMP_SIZE; i++)
    {
        Spline::Point out = s.evaluate(opamp_voltage[i].x);

        CHECK_EQUAL(opamp_voltage[i].y, out.x);
    }
}

TEST(TestInterpolateOutsideBounds)
{
    const Spline::Point values[5] = {
        { 10, 15 },
        { 15, 20 },
        { 20, 30 },
        { 25, 40 },
        { 30, 45 },
    };

    Spline s(values, 5);

    Spline::Point out;

    out = s.evaluate(5);
    CHECK_CLOSE(6.66667, out.x, 0.00001);

    out = s.evaluate(40);
    CHECK_CLOSE(75.0, out.x, 0.00001);
}

}
