/*
 * test.h
 * ------
 * Purpose: Unit tests for OpenMPT.
 * Notes  : We need FAAAAAAAR more unit tests!
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

OPENMPT_NAMESPACE_BEGIN

namespace Test {

void PrintHeader();

void PrintFooter();

void DoTests();

} // namespace Test

OPENMPT_NAMESPACE_END
