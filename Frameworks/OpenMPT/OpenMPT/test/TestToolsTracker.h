/*
 * TestToolsTracker.h
 * ------------------
 * Purpose: Unit test framework for OpenMPT.
 * Notes  : Really basic functionality that relies on a debugger that catches
 *          exceptions and breaks right at the spot where it gets thrown.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"


#ifdef ENABLE_TESTS
#ifdef MODPLUG_TRACKER


#include "mpt/test/test.hpp"


OPENMPT_NAMESPACE_BEGIN


namespace Test {


#if MPT_COMPILER_MSVC
// With MSVC, break directly using __debugbreak intrinsic instead of calling DebugBreak which breaks one stackframe deeper than we want
#define MyDebugBreak() __debugbreak()
#else
#define MyDebugBreak() DebugBreak()
#endif


class mpt_test_reporter
	: public mpt::test::silent_reporter
{
public:
	mpt_test_reporter() = default;
	~mpt_test_reporter() override = default;
public:
	inline void immediate_breakpoint() override {
		MyDebugBreak();
	}
};


// Verify that given parameters are 'equal'. Break directly into the debugger if not.
// The exact meaning of equality is based on operator== of the compared types.
#define VERIFY_EQUAL(x,y)	\
	do { \
		if(!((x) == (y))) { \
			MyDebugBreak(); \
		} \
	} while(0) \
/**/

// Like VERIFY_EQUAL, only differs for libopenmpt
#define VERIFY_EQUAL_NONCONT VERIFY_EQUAL

// Like VERIFY_EQUAL, only differs for libopenmpt
#define VERIFY_EQUAL_QUIET_NONCONT VERIFY_EQUAL

#define VERIFY_EQUAL_EPS(x,y,eps)	\
	do { \
		if(std::abs((x) - (y)) > (eps)) { \
			MyDebugBreak(); \
		} \
	} while(0) \
/**/


#define DO_TEST(func) \
	do { \
		if(IsDebuggerPresent()) { \
			func(); \
		} \
	} while(0) \
/**/


} // namespace Test


OPENMPT_NAMESPACE_END


#endif // MODPLUG_TRACKER
#endif // ENABLE_TESTS
