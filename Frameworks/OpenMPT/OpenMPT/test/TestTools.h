/*
 * TestTools.h
 * -----------
 * Purpose: Unit test framework.
 * Notes  : 
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "TestToolsTracker.h"
#include "TestToolsLib.h"

#include "../common/mptPathString.h"

OPENMPT_NAMESPACE_BEGIN

#ifdef ENABLE_TESTS

namespace Test
{

mpt::PathString GetPathPrefix();

} // namespace Test

OPENMPT_NAMESPACE_END

#endif
