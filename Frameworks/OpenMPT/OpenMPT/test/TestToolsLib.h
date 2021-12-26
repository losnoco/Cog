/*
 * TestToolsLib.h
 * --------------
 * Purpose: Unit test framework for libopenmpt.
 * Notes  : This is more complex than the OpenMPT version because we cannot
 *          rely on a debugger and have to deal with exceptions ourselves.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"


#ifdef ENABLE_TESTS
#ifndef MODPLUG_TRACKER


//#define MPT_TEST_CXX11


#include "mpt/test/test.hpp"

#include <type_traits>

#include "mpt/base/bit.hpp"
#include "openmpt/base/FlagSet.hpp"
#include "../soundlib/Snd_defs.h"


OPENMPT_NAMESPACE_BEGIN


namespace Test {



class mpt_test_reporter
	: public mpt::test::silent_reporter
{
public:
	mpt_test_reporter() = default;
	~mpt_test_reporter() override = default;
public:
	void case_run(const mpt::source_location & loc) override;
	void case_run(const mpt::source_location & loc, const char * text_e) override;
	void case_run(const mpt::source_location & loc, const char * text_ex, const char * text_e) override;
	void case_run(const mpt::source_location & loc, const char * text_a, const char * text_cmp, const char * text_b) override;
	void case_result(const mpt::source_location & loc, const mpt::test::result & result) override;
};



extern int fail_count;


enum Verbosity
{
	VerbosityQuiet,
	VerbosityNormal,
	VerbosityVerbose,
};

enum Fatality
{
	FatalityContinue,
	FatalityStop
};


struct TestFailed
{
	std::string values;
	TestFailed(const std::string &values) : values(values) { }
	TestFailed() { }
};

} // namespace Test

template<typename T>
struct ToStringHelper
{
	std::string operator () (const T &x)
	{
		return mpt::afmt::val(x);
	}
};

#ifdef MPT_TEST_CXX11

template<>
struct ToStringHelper<mpt::endian>
{
	std::string operator () (const mpt::endian &x)
	{
		if(x == mpt::endian::big) return "big";
		if(x == mpt::endian::little) return "little";
		return "unknown";
	}
};

template<typename enum_t, typename store_t>
struct ToStringHelper<FlagSet<enum_t, store_t> >
{
	std::string operator () (const FlagSet<enum_t, store_t> &x)
	{
		return mpt::afmt::val(x.GetRaw());
	}
};

template<typename enum_t>
struct ToStringHelper<enum_value_type<enum_t> >
{
	std::string operator () (const enum_value_type<enum_t> &x)
	{
		return mpt::afmt::val(x.as_bits());
	}
};

template<typename Ta, typename Tb>
struct ToStringHelper<std::pair<Ta, Tb> >
{
	std::string operator () (const std::pair<Ta, Tb> &x)
	{
		return std::string("{") + mpt::afmt::val(x.first) + std::string(",") + mpt::afmt::val(x.second) + std::string("}");
	}
};

template<std::size_t FRACT, typename T>
struct ToStringHelper<FPInt<FRACT, T> >
{
	std::string operator () (const FPInt<FRACT, T> &x)
	{
		return std::string("FPInt<") + mpt::afmt::val(FRACT) + std::string(",") + mpt::afmt::val(typeid(T).name()) + std::string(">{") + mpt::afmt::val(x.GetInt()) + std::string(".") + mpt::afmt::val(x.GetFract()) + std::string("}");
	}
};

template<>
struct ToStringHelper<SamplePosition>
{
	std::string operator () (const SamplePosition &x)
	{
		return mpt::afmt::val(x.GetInt()) + std::string(".") + std::string("0x") + mpt::afmt::hex0<8>(x.GetFract());
	}
};

#endif // MPT_TEST_CXX11


namespace Test {

class Testcase
{

private:

	Fatality const fatality;
	Verbosity const verbosity;
	const char * const desc;
	mpt::source_location const loc;

public:

	Testcase(Fatality fatality, Verbosity verbosity, const char * const desc, const mpt::source_location &loc);

public:

	std::string AsString() const;

	void ShowStart() const;
	void ShowProgress(const char * text) const;
	void ShowPass() const;
	void ShowFail(bool exception = false, const char * const text = nullptr) const;

	void ReportPassed();
	void ReportFailed();

	void ReportException();

private:

	template <typename Tx, typename Ty>
	inline bool IsEqual(const Tx &x, const Ty &y, std::false_type, std::false_type)
	{
		return (x == y);
	}

	template <typename Tx, typename Ty>
	inline bool IsEqual(const Tx &x, const Ty &y, std::false_type, std::true_type)
	{
		return (x == y);
	}

	template <typename Tx, typename Ty>
	inline bool IsEqual(const Tx &x, const Ty &y, std::true_type, std::false_type)
	{
		return (x == y);
	}

	template <typename Tx, typename Ty>
	inline bool IsEqual(const Tx &x, const Ty &y, std::true_type /* is_integer */, std::true_type /* is_integer */ )
	{
		// Avoid signed-unsigned-comparison warnings and test equivalence in case of either type conversion direction.
		return ((x == static_cast<Tx>(y)) && (static_cast<Ty>(x) == y));
	}

	template <typename Tx, typename Ty, typename Teps>
	inline bool IsEqualEpsilon(const Tx &x, const Ty &y, const Teps &eps)
	{
		return std::abs(x - y) <= eps;
	}

public:

#ifdef MPT_TEST_CXX11

private:

	template <typename Tx, typename Ty>
	MPT_NOINLINE void TypeCompareHelper(const Tx &x, const Ty &y)
	{
		if(!IsEqual(x, y, std::is_integral<Tx>(), std::is_integral<Ty>()))
		{
			throw TestFailed(MPT_AFORMAT("{} != {}")(ToStringHelper<Tx>()(x), ToStringHelper<Ty>()(y)));
			//throw TestFailed();
		}
	}

	template <typename Tx, typename Ty, typename Teps>
	MPT_NOINLINE void TypeCompareHelper(const Tx &x, const Ty &y, const Teps &eps)
	{
		if(!IsEqualEpsilon(x, y, eps))
		{
			throw TestFailed(MPT_AFORMAT("{} != {}")(ToStringHelper<Tx>()(x), ToStringHelper<Ty>()(y)));
			//throw TestFailed();
		}
	}

public:

	template <typename Tfx, typename Tfy>
	MPT_NOINLINE void operator () (const Tfx &fx, const Tfy &fy)
	{
		ShowStart();
		try
		{
			ShowProgress("Calculate x ...");
			const auto x = fx();
			ShowProgress("Calculate y ...");
			const auto y = fy();
			ShowProgress("Compare ...");
			TypeCompareHelper(x, y);
			ReportPassed();
		} catch(...)
		{
			ReportFailed();
		}
	}

	template <typename Tfx, typename Tfy, typename Teps>
	MPT_NOINLINE void operator () (const Tfx &fx, const Tfy &fy, const Teps &eps)
	{
		ShowStart();
		try
		{
			ShowProgress("Calculate x ...");
			const auto x = fx();
			ShowProgress("Calculate y ...");
			const auto y = fy();
			ShowProgress("Compare ...");
			TypeCompareHelper(x, y, eps);
			ReportPassed();
		} catch(...)
		{
			ReportFailed();
		}
	}

	#define VERIFY_EQUAL(x,y)               Test::Testcase(Test::FatalityContinue, Test::VerbosityNormal, #x " == " #y , MPT_SOURCE_LOCATION_CURRENT() )( [&](){return (x) ;}, [&](){return (y) ;} )
	#define VERIFY_EQUAL_NONCONT(x,y)       Test::Testcase(Test::FatalityStop    , Test::VerbosityNormal, #x " == " #y , MPT_SOURCE_LOCATION_CURRENT() )( [&](){return (x) ;}, [&](){return (y) ;} )
	#define VERIFY_EQUAL_QUIET_NONCONT(x,y) Test::Testcase(Test::FatalityStop    , Test::VerbosityQuiet , #x " == " #y , MPT_SOURCE_LOCATION_CURRENT() )( [&](){return (x) ;}, [&](){return (y) ;} )

	#define VERIFY_EQUAL_EPS(x,y,eps)       Test::Testcase(Test::FatalityContinue, Test::VerbosityNormal, #x " == " #y , MPT_SOURCE_LOCATION_CURRENT() )( [&](){return (x) ;}, [&](){return (y) ;}, (eps) )

#else

public:

	template <typename Tx, typename Ty>
	MPT_NOINLINE void operator () (const Tx &x, const Ty &y)
	{
		ShowStart();
		try
		{
			if(!IsEqual(x, y, std::is_integral<Tx>(), std::is_integral<Ty>()))
			{
				//throw TestFailed(MPT_AFORMAT("{} != {}")(x, y));
				throw TestFailed();
			}
			ReportPassed();
		} catch(...)
		{
			ReportFailed();
		}
	}

	template <typename Tx, typename Ty, typename Teps>
	MPT_NOINLINE void operator () (const Tx &x, const Ty &y, const Teps &eps)
	{
		ShowStart();
		try
		{
			if(!IsEqualEpsilon(x, y, eps))
			{
				//throw TestFailed(MPT_AFORMAT("{} != {}")(x, y));
				throw TestFailed();
			}
			ReportPassed();
		} catch(...)
		{
			ReportFailed();
		}
	}

	#define VERIFY_EQUAL(x,y)               Test::Testcase(Test::FatalityContinue, Test::VerbosityNormal, #x " == " #y , MPT_SOURCE_LOCATION_CURRENT() )( (x) , (y) )
	#define VERIFY_EQUAL_NONCONT(x,y)       Test::Testcase(Test::FatalityStop    , Test::VerbosityNormal, #x " == " #y , MPT_SOURCE_LOCATION_CURRENT() )( (x) , (y) )
	#define VERIFY_EQUAL_QUIET_NONCONT(x,y) Test::Testcase(Test::FatalityStop    , Test::VerbosityQuiet , #x " == " #y , MPT_SOURCE_LOCATION_CURRENT() )( (x) , (y) )

	#define VERIFY_EQUAL_EPS(x,y,eps)       Test::Testcase(Test::FatalityContinue, Test::VerbosityNormal, #x " == " #y , MPT_SOURCE_LOCATION_CURRENT() )( (x) , (y), (eps) )

#endif

};


#define DO_TEST(func) \
do { \
	Test::Testcase test(Test::FatalityStop, Test::VerbosityVerbose, #func , MPT_SOURCE_LOCATION_CURRENT() ); \
	try { \
		test.ShowStart(); \
		fail_count = 0; \
		func(); \
		if(fail_count > 0) { \
			throw Test::TestFailed(); \
		} \
		test.ReportPassed(); \
	} catch(...) { \
		test.ReportException(); \
	} \
} while(0)


} // namespace Test


OPENMPT_NAMESPACE_END


#endif // !MODPLUG_TRACKER
#endif // ENABLE_TESTS
