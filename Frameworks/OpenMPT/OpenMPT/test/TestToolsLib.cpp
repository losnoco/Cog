/*
 * TestToolsLib.cpp
 * ----------------
 * Purpose: Unit test framework for libopenmpt.
 * Notes  : Currently somewhat unreadable :/
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "TestToolsLib.h"


#ifdef ENABLE_TESTS
#ifndef MODPLUG_TRACKER


#include <exception>
#include <iostream>

#include <cstdlib>


OPENMPT_NAMESPACE_BEGIN


namespace Test {



void mpt_test_reporter::case_run(const mpt::source_location& loc)
{
	#if !MPT_OS_DJGPP
		std::cout << "TEST..: " << MPT_AFORMAT("{}({}):")(loc.file_name() ? loc.file_name() : "", loc.line()) << ": " << std::endl;
	#else
		MPT_UNUSED(loc);
	#endif
}

void mpt_test_reporter::case_run(const mpt::source_location& loc, const char* text_e)
{
	#if !MPT_OS_DJGPP
		std::cout << "TEST..: " << MPT_AFORMAT("{}({}): {}")(loc.file_name() ? loc.file_name() : "", loc.line(), text_e) << ": " << std::endl;
	#else
		MPT_UNUSED(loc);
		MPT_UNUSED(text_e);
	#endif
}

void mpt_test_reporter::case_run(const mpt::source_location& loc, const char* text_ex, const char* text_e)
{
	if(text_ex)
	{
		#if !MPT_OS_DJGPP
			std::cout << "TEST..: " << MPT_AFORMAT("{}({}): {} throws {}")(loc.file_name() ? loc.file_name() : "", loc.line(), text_e, text_ex) << ": " << std::endl;
		#else
			MPT_UNUSED(loc);
			MPT_UNUSED(text_ex);
			MPT_UNUSED(text_e);
		#endif
	} else
	{
		#if !MPT_OS_DJGPP
			std::cout << "TEST..: " << MPT_AFORMAT("{}({}): {} throws")(loc.file_name() ? loc.file_name() : "", loc.line(), text_e) << ": " << std::endl;
		#else
			MPT_UNUSED(loc);
			MPT_UNUSED(text_ex);
			MPT_UNUSED(text_e);
		#endif
	}
}

void mpt_test_reporter::case_run(const mpt::source_location& loc, const char* text_a, const char* text_cmp, const char* text_b)
{
	#if !MPT_OS_DJGPP
		std::cout << "TEST..: " << MPT_AFORMAT("{}({}): {} {} {}")(loc.file_name() ? loc.file_name() : "", loc.line(), text_a, text_cmp, text_b) << ": " << std::endl;
	#else
		MPT_UNUSED(loc);
		MPT_UNUSED(text_a);
		MPT_UNUSED(text_cmp);
		MPT_UNUSED(text_b);
	#endif
}

void mpt_test_reporter::case_result(const mpt::source_location& loc, const mpt::test::result& result)
{
	MPT_UNUSED(loc);
	if(std::holds_alternative<mpt::test::result_success>(result.info))
	{
		#if !MPT_OS_DJGPP
			std::cout << "RESULT: PASS" << std::endl;
		#endif
	} else if(std::holds_alternative<mpt::test::result_failure>(result.info))
	{
		fail_count++;
		std::cout << "RESULT: FAIL" << std::endl;
		std::cout.flush();
		std::cerr << "FAIL: " << "FAILURE: " << std::get<mpt::test::result_failure>(result.info).text << std::endl;
		std::cerr.flush();
	} else if(std::holds_alternative<mpt::test::result_unexpected_exception>(result.info))
	{
		fail_count++;
		std::cout << "RESULT: FAIL" << std::endl;
		std::cout.flush();
		std::cerr << "FAIL: " << "UNEXPECTED EXCEPTION: " << std::get<mpt::test::result_unexpected_exception>(result.info).text << std::endl;
		std::cerr.flush();
	} else
	{
		fail_count++;
		std::cout << "RESULT: FAIL" << std::endl;
		std::cout.flush();
		std::cerr << "FAIL: " << "UNKOWN" << std::endl;
		std::cerr.flush();
	}
}



int fail_count = 0;


static std::string remove_newlines(std::string str)
{
	return mpt::replace(mpt::replace(str, std::string("\n"), std::string(" ")), std::string("\r"), std::string(" "));
}


Testcase::Testcase(Fatality fatality, Verbosity verbosity, const char * const desc, const mpt::source_location &loc)
	: fatality(fatality)
	, verbosity(verbosity)
	, desc(desc)
	, loc(loc)
{
	return;
}


std::string Testcase::AsString() const
{
	return MPT_AFORMAT("{}({}): {}")(loc.file_name() ? loc.file_name() : "", loc.line(), remove_newlines(desc));
}


void Testcase::ShowStart() const
{
	switch(verbosity)
	{
		case VerbosityQuiet:
			break;
		case VerbosityNormal:
#if !MPT_OS_DJGPP
			std::cout << "TEST..: " << AsString() << ": " << std::endl;
#endif
			break;
		case VerbosityVerbose:
#if !MPT_OS_DJGPP
			std::cout << "TEST..: " << AsString() << ": " << std::endl;
#endif
			break;
	}
}


void Testcase::ShowProgress(const char * text) const
{
	switch(verbosity)
	{
		case VerbosityQuiet:
			break;
		case VerbosityNormal:
			break;
		case VerbosityVerbose:
#if !MPT_OS_DJGPP
			std::cout << "TEST..: " << AsString() << ": " << text << std::endl;
#else
			MPT_UNUSED_VARIABLE(text);
#endif
			break;
	}
}


void Testcase::ShowPass() const
{
	switch(verbosity)
	{
		case VerbosityQuiet:
			break;
		case VerbosityNormal:
#if !MPT_OS_DJGPP
			std::cout << "RESULT: PASS" << std::endl;
#endif
			break;
		case VerbosityVerbose:
#if !MPT_OS_DJGPP
			std::cout << "PASS..: " << AsString() << std::endl;
#endif
			break;
	}
}


void Testcase::ShowFail(bool exception, const char * const text) const
{
	switch(verbosity)
	{
		case VerbosityQuiet:
			break;
		case VerbosityNormal:
			std::cout << "RESULT: FAIL" << std::endl;
			break;
		case VerbosityVerbose:
			std::cout << "FAIL..: " << AsString() << std::endl;
			break;
	}
	std::cout.flush();
	if(!exception)
	{
		if(!text || (text && std::string(text).empty()))
		{
			std::cerr << "FAIL: " << AsString() << std::endl;
		} else
		{
			std::cerr << "FAIL: " << AsString() << " : " << text << std::endl;
		}
	} else
	{
		if(!text || (text && std::string(text).empty()))
		{
			std::cerr << "FAIL: " << AsString() << " EXCEPTION!" << std::endl;
		} else
		{
			std::cerr << "FAIL: " << AsString() << " EXCEPTION: " << text << std::endl;
		}
	}
	std::cerr.flush();
}


void Testcase::ReportPassed()
{
	ShowPass();
}


void Testcase::ReportFailed()
{
	fail_count++;
	ReportException();
}


void Testcase::ReportException()
{
	try
	{
		throw; // get the exception
	} catch(TestFailed & e)
	{
		ShowFail(false, e.values.c_str());
		if(fatality == FatalityStop)
		{
			throw; // rethrow
		}
	} catch(std::exception & e)
	{
		ShowFail(true, e.what());
		throw; // rethrow
	} catch(...)
	{
		ShowFail(true);
		throw; // rethrow
	}
}


} // namespace Test


#if defined(MPT_ASSERT_HANDLER_NEEDED)

MPT_NOINLINE void AssertHandler(const mpt::source_location &loc, const char *expr, const char *msg)
{
	Test::fail_count++;
	if(msg)
	{
		mpt::log::GlobalLogger().SendLogMessage(loc, LogError, "ASSERT",
			U_("ASSERTION FAILED: ") + mpt::ToUnicode(mpt::Charset::ASCII, msg) + U_(" (") + mpt::ToUnicode(mpt::Charset::ASCII, expr) + U_(")")
			);
	} else
	{
		mpt::log::GlobalLogger().SendLogMessage(loc, LogError, "ASSERT",
			U_("ASSERTION FAILED: ") + mpt::ToUnicode(mpt::Charset::ASCII, expr)
			);
	}
	#if defined(MPT_BUILD_FATAL_ASSERTS)
		std::abort();
	#endif // MPT_BUILD_FATAL_ASSERTS
}

#endif // MPT_ASSERT_HANDLER_NEEDED


OPENMPT_NAMESPACE_END


#endif // !MODPLUG_TRACKER
#endif // ENABLE_TESTS
