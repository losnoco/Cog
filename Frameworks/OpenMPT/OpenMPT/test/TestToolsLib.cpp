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


int fail_count = 0;


static std::string remove_newlines(std::string str)
{
	return mpt::String::Replace(mpt::String::Replace(str, "\n", " "), "\r", " ");
}


Context::Context(const char * file, int line)
	: file(file)
	, line(line)
{
	return;
}


Context::Context(const Context &c)
	: file(c.file)
	, line(c.line)
{
	return;
}


Testcase::Testcase(Fatality fatality, Verbosity verbosity, const char * const desc, const Context &context)
	: fatality(fatality)
	, verbosity(verbosity)
	, desc(desc)
	, context(context)
{
	return;
}


std::string Testcase::AsString() const
{
	return mpt::format(std::string("%1(%2): %3"))(context.file, context.line, remove_newlines(desc));
}


void Testcase::ShowStart() const
{
	switch(verbosity)
	{
		case VerbosityQuiet:
			break;
		case VerbosityNormal:
			std::cout << "TEST..: " << AsString() << ": " << std::endl;
			break;
		case VerbosityVerbose:
			std::cout << "TEST..: " << AsString() << ": " << std::endl;
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
			std::cout << "TEST..: " << AsString() << ": " << text << std::endl;
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
			std::cout << "RESULT: PASS" << std::endl;
			break;
		case VerbosityVerbose:
			std::cout << "PASS..: " << AsString() << std::endl;
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

MPT_NOINLINE void AssertHandler(const char *file, int line, const char *function, const char *expr, const char *msg)
{
	Test::fail_count++;
	if(msg)
	{
		mpt::log::Logger().SendLogMessage(mpt::log::Context(file, line, function), LogError, "ASSERT",
			MPT_USTRING("ASSERTION FAILED: ") + mpt::ToUnicode(mpt::CharsetASCII, msg) + MPT_USTRING(" (") + mpt::ToUnicode(mpt::CharsetASCII, expr) + MPT_USTRING(")")
			);
	} else
	{
		mpt::log::Logger().SendLogMessage(mpt::log::Context(file, line, function), LogError, "ASSERT",
			MPT_USTRING("ASSERTION FAILED: ") + mpt::ToUnicode(mpt::CharsetASCII, expr)
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
