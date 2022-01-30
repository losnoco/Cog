/*
 * test.cpp
 * --------
 * Purpose: Unit tests for OpenMPT.
 * Notes  : We need FAAAAAAAR more unit tests!
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "test.h"


#ifdef ENABLE_TESTS

#include "mpt/base/numbers.hpp"
#include "mpt/crc/crc.hpp"
#include "mpt/environment/environment.hpp"
#include "mpt/io/base.hpp"
#include "mpt/io/io.hpp"
#include "mpt/io/io_stdstream.hpp"
#include "mpt/io_read/filecursor_stdstream.hpp"
#include "mpt/test/test.hpp"
#include "mpt/test/test_macros.hpp"
#include "mpt/uuid/uuid.hpp"

#include "../common/version.h"
#include "../common/misc_util.h"
#include "../common/mptStringBuffer.h"
#include "../common/serialization_utils.h"
#include "../soundlib/Sndfile.h"
#include "../common/FileReader.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/MIDIEvents.h"
#include "../soundlib/MIDIMacros.h"
#include "openmpt/soundbase/Copy.hpp"
#include "openmpt/soundbase/SampleConvert.hpp"
#include "openmpt/soundbase/SampleDecode.hpp"
#include "openmpt/soundbase/SampleEncode.hpp"
#include "openmpt/soundbase/SampleFormat.hpp"
#include "../soundlib/SampleCopy.h"
#include "../soundlib/SampleNormalize.h"
#include "../soundlib/ModSampleCopy.h"
#include "../soundlib/ITCompression.h"
#include "../soundlib/tuningcollection.h"
#include "../soundlib/tuning.h"
#include "openmpt/soundbase/Dither.hpp"
#include "../common/Dither.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/Mptrack.h"
#include "../mptrack/Moddoc.h"
#include "../mptrack/ModDocTemplate.h"
#include "../mptrack/Mainfrm.h"
#include "../mptrack/Settings.h"
#include "../mptrack/HTTP.h"
#endif // MODPLUG_TRACKER
#include "../common/mptFileIO.h"
#ifdef MODPLUG_TRACKER
#include "mpt/crypto/hash.hpp"
#include "mpt/crypto/jwk.hpp"
#include "mpt/uuid_namespace/uuid_namespace.hpp"
#endif // MODPLUG_TRACKER
#ifdef LIBOPENMPT_BUILD
#include "../libopenmpt/libopenmpt_version.h"
#endif // LIBOPENMPT_BUILD
#ifndef NO_PLUGINS
#include "../soundlib/plugins/PlugInterface.h"
#endif
#include <sstream>
#include <limits>
#ifdef LIBOPENMPT_BUILD
#include <iostream>
#endif // LIBOPENMPT_BUILD
#include <istream>
#include <ostream>
#include <stdexcept>
#if MPT_COMPILER_MSVC
#include <tchar.h>
#endif
#if MPT_OS_WINDOWS
#include <windows.h>
#endif
#if defined(MPT_WITH_ZLIB)
#include <zlib.h>
#elif defined(MPT_WITH_MINIZ)
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include <miniz/miniz.h>
#endif

#define MPT_TEST_HAS_FILESYSTEM 1
#if MPT_OS_DJGPP
#undef MPT_TEST_HAS_FILESYSTEM
#define MPT_TEST_HAS_FILESYSTEM 0
#endif

#include "TestTools.h"


// enable tests which may fail spuriously
//#define FLAKY_TESTS



OPENMPT_NAMESPACE_BEGIN



namespace Test {



static MPT_NOINLINE void TestVersion();
static MPT_NOINLINE void TestTypes();
static MPT_NOINLINE void TestMisc1();
static MPT_NOINLINE void TestMisc2();
static MPT_NOINLINE void TestRandom();
static MPT_NOINLINE void TestCharsets();
static MPT_NOINLINE void TestStringFormatting();
static MPT_NOINLINE void TestSettings();
static MPT_NOINLINE void TestStringIO();
static MPT_NOINLINE void TestMIDIEvents();
static MPT_NOINLINE void TestSampleConversion();
static MPT_NOINLINE void TestITCompression();
static MPT_NOINLINE void TestPCnoteSerialization();
static MPT_NOINLINE void TestLoadSaveFile();
static MPT_NOINLINE void TestEditing();



static mpt::PathString *PathPrefix = nullptr;
static mpt::default_prng * s_PRNG = nullptr;



mpt::PathString GetPathPrefix()
{
	if((*PathPrefix).empty())
	{
		return P_("");
	}
	return *PathPrefix + P_("/");
}


void DoTests()
{

	#ifdef LIBOPENMPT_BUILD

		std::cout << std::flush;
		std::clog << std::flush;
		std::cerr << std::flush;
	
		std::cout << "libopenmpt test suite" << std::endl;

		std::cout << "Version.: " << mpt::ToCharset(mpt::Charset::ASCII, Build::GetVersionString(Build::StringVersion | Build::StringRevision | Build::StringSourceInfo | Build::StringBuildFlags | Build::StringBuildFeatures)) << std::endl;
		std::cout << "Compiler: " << mpt::ToCharset(mpt::Charset::ASCII, Build::GetBuildCompilerString()) << std::endl;

		std::cout << std::flush;

	#endif

	#if MPT_OS_WINDOWS

		// prefix for test suite
		std::wstring pathprefix = std::wstring();

		bool libopenmpt = false;
		#ifdef LIBOPENMPT_BUILD
			libopenmpt = true;
		#endif

#if !MPT_OS_WINDOWS_WINRT
		// set path prefix for test files (if provided)
		std::vector<WCHAR> buf(GetEnvironmentVariableW(L"srcdir", NULL, 0) + 1);
		if(GetEnvironmentVariableW(L"srcdir", buf.data(), static_cast<DWORD>(buf.size())) > 0)
		{
			pathprefix = buf.data();
		} else
#endif
		if(libopenmpt && IsDebuggerPresent())
		{
			pathprefix = L"../../";
		}

		PathPrefix = new mpt::PathString(mpt::PathString::FromWide(pathprefix));

	#else

		// prefix for test suite
		mpt::ustring pathprefix = mpt::ustring();

		// set path prefix for test files (if provided)
		mpt::ustring env_srcdir = mpt::getenv( U_("srcdir") ).value_or( U_("") );
		if ( !env_srcdir.empty() ) {
			pathprefix = env_srcdir;
		}

		PathPrefix = new mpt::PathString(mpt::PathString::FromUnicode(pathprefix));

	#endif

	void (*do_mpt_test)(void) = []() {
		mpt_test_reporter reporter{};
		mpt::test::run_all(reporter);
	};
	DO_TEST(do_mpt_test);

	mpt::random_device rd;
	s_PRNG = new mpt::default_prng(mpt::make_prng<mpt::default_prng>(rd));

	DO_TEST(TestVersion);
	DO_TEST(TestTypes);
	DO_TEST(TestMisc1);
	DO_TEST(TestMisc2);
	DO_TEST(TestRandom);
	DO_TEST(TestCharsets);
	DO_TEST(TestStringFormatting);
	DO_TEST(TestSettings);
	DO_TEST(TestStringIO);
	DO_TEST(TestMIDIEvents);
	DO_TEST(TestSampleConversion);
	DO_TEST(TestITCompression);

	// slower tests, require opening a CModDoc
	DO_TEST(TestPCnoteSerialization);
	DO_TEST(TestLoadSaveFile);
	DO_TEST(TestEditing);

	delete s_PRNG;
	s_PRNG = nullptr;

	delete PathPrefix;
	PathPrefix = nullptr;
}


static mpt::PathString GetTempFilenameBase();


static void RemoveFile(const mpt::PathString &filename)
{
	#if MPT_OS_WINDOWS
		for(int retry=0; retry<10; retry++)
		{
			if(DeleteFile(filename.AsNative().c_str()) != FALSE)
			{
				break;
			}
			// wait for windows virus scanners
			Sleep(10);
		}
	#else
		remove(filename.AsNative().c_str());
	#endif
}


// Test if functions related to program version data work
static MPT_NOINLINE void TestVersion()
{
	//Verify that macros and functions work.
	{
		VERIFY_EQUAL( Version::Parse(Version::Current().ToUString()), Version::Current() );
		VERIFY_EQUAL( Version::Parse(Version::Current().ToUString()).ToUString(), Version::Current().ToUString() );
		VERIFY_EQUAL( Version(18285096).ToUString(), U_("1.17.02.28") );
		VERIFY_EQUAL( Version::Parse(U_("1.17.02.28")), Version(18285096) );
		VERIFY_EQUAL( Version::Parse(U_("1.fe.02.28")), Version(0x01fe0228) );
		VERIFY_EQUAL( Version::Parse(U_("01.fe.02.28")), Version(0x01fe0228) );
		VERIFY_EQUAL( Version::Parse(U_("1.22")), Version(0x01220000) );
		VERIFY_EQUAL( MPT_V("1.17.02.28"), Version(18285096) );
		VERIFY_EQUAL( MPT_V("1.fe.02.28"), Version(0x01fe0228) );
		VERIFY_EQUAL( MPT_V("01.fe.02.28"), Version(0x01fe0228) );
		VERIFY_EQUAL( MPT_V("1.22"), Version(0x01220000) );
		VERIFY_EQUAL( MPT_V("1.19.02.00").WithoutTestNumber(), MPT_V("1.19.02.00"));
		VERIFY_EQUAL( MPT_V("1.18.03.20").WithoutTestNumber(), MPT_V("1.18.03.00"));
		VERIFY_EQUAL( MPT_V("1.18.01.13").IsTestVersion(), true);
		VERIFY_EQUAL( MPT_V("1.19.01.00").IsTestVersion(), false);
		VERIFY_EQUAL( MPT_V("1.17.02.54").IsTestVersion(), false);
		VERIFY_EQUAL( MPT_V("1.18.00.00").IsTestVersion(), false);
		VERIFY_EQUAL( MPT_V("1.18.02.00").IsTestVersion(), false);
		VERIFY_EQUAL( MPT_V("1.18.02.01").IsTestVersion(), true);

		// Ensure that versions ending in .00.00 (which are ambiguous to truncated version numbers in certain file formats (e.g. S3M and IT) do not get qualified as test builds.
		VERIFY_EQUAL( MPT_V("1.23.00.00").IsTestVersion(), false);

		static_assert( MPT_V("1.17.2.28").GetRawVersion() == 18285096 );
		static_assert( MPT_V("1.17.02.48").GetRawVersion() == 18285128 );
		static_assert( MPT_V("01.17.02.52").GetRawVersion() == 18285138 );
		// Ensure that bit-shifting works (used in some mod loaders for example)
		static_assert( MPT_V("01.17.00.00").GetRawVersion() == 0x0117 << 16 );
		static_assert( MPT_V("01.17.03.00").GetRawVersion() >> 8 == 0x011703 );

		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05-r13099")).IsNewerThan(VersionWithRevision::Parse(U_("1.30.00.05-r13100"))), false);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05-r13099")).IsNewerThan(VersionWithRevision::Parse(U_("1.30.00.05-r13099"))), false);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05-r13099")).IsNewerThan(VersionWithRevision::Parse(U_("1.30.00.05-r13098"))), true);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05-r13099")).IsNewerThan(VersionWithRevision::Parse(U_("1.30.00.05"))), false);

		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05-r13099")).IsOlderThan(VersionWithRevision::Parse(U_("1.30.00.05-r13100"))), true);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05-r13099")).IsOlderThan(VersionWithRevision::Parse(U_("1.30.00.05-r13099"))), false);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05-r13099")).IsOlderThan(VersionWithRevision::Parse(U_("1.30.00.05-r13098"))), false);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05-r13099")).IsOlderThan(VersionWithRevision::Parse(U_("1.30.00.05"))), false);

		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05")).IsNewerThan(VersionWithRevision::Parse(U_("1.30.00.05-r13100"))), false);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05")).IsNewerThan(VersionWithRevision::Parse(U_("1.30.00.05-r13099"))), false);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05")).IsNewerThan(VersionWithRevision::Parse(U_("1.30.00.05-r13098"))), false);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05")).IsNewerThan(VersionWithRevision::Parse(U_("1.30.00.05"))), false);

		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05")).IsOlderThan(VersionWithRevision::Parse(U_("1.30.00.05-r13100"))), false);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05")).IsOlderThan(VersionWithRevision::Parse(U_("1.30.00.05-r13099"))), false);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05")).IsOlderThan(VersionWithRevision::Parse(U_("1.30.00.05-r13098"))), false);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05")).IsOlderThan(VersionWithRevision::Parse(U_("1.30.00.05"))), false);

		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05-r13099")).IsEqualTo(VersionWithRevision::Parse(U_("1.30.00.05-r13100"))), false);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05-r13099")).IsEqualTo(VersionWithRevision::Parse(U_("1.30.00.05-r13099"))), true);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05-r13099")).IsEqualTo(VersionWithRevision::Parse(U_("1.30.00.05-r13098"))), false);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05-r13099")).IsEqualTo(VersionWithRevision::Parse(U_("1.30.00.05"))), false);

		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05-r13099")).IsEquivalentTo(VersionWithRevision::Parse(U_("1.30.00.05-r13100"))), false);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05-r13099")).IsEquivalentTo(VersionWithRevision::Parse(U_("1.30.00.05-r13099"))), true);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05-r13099")).IsEquivalentTo(VersionWithRevision::Parse(U_("1.30.00.05-r13098"))), false);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05-r13099")).IsEquivalentTo(VersionWithRevision::Parse(U_("1.30.00.05"))), true);

		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05")).IsEqualTo(VersionWithRevision::Parse(U_("1.30.00.05-r13100"))), false);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05")).IsEqualTo(VersionWithRevision::Parse(U_("1.30.00.05-r13099"))), false);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05")).IsEqualTo(VersionWithRevision::Parse(U_("1.30.00.05-r13098"))), false);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05")).IsEqualTo(VersionWithRevision::Parse(U_("1.30.00.05"))), true);

		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05")).IsEquivalentTo(VersionWithRevision::Parse(U_("1.30.00.05-r13100"))), true);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05")).IsEquivalentTo(VersionWithRevision::Parse(U_("1.30.00.05-r13099"))), true);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05")).IsEquivalentTo(VersionWithRevision::Parse(U_("1.30.00.05-r13098"))), true);
		VERIFY_EQUAL(VersionWithRevision::Parse(U_("1.30.00.05")).IsEquivalentTo(VersionWithRevision::Parse(U_("1.30.00.05"))), true);

	}

#ifdef MODPLUG_TRACKER
	//Verify that the version obtained from the executable file is the same as
	//defined in Version.
	{
		WCHAR szFullPath[MAX_PATH];
		DWORD dwVerHnd;
		DWORD dwVerInfoSize;

		// Get version information from the application
		::GetModuleFileNameW(NULL, szFullPath, mpt::saturate_cast<DWORD>(std::size(szFullPath)));
		dwVerInfoSize = ::GetFileVersionInfoSizeW(szFullPath, &dwVerHnd);
		if (!dwVerInfoSize)
			throw std::runtime_error("!dwVerInfoSize is true");

		std::vector<TCHAR> pVersionInfo(dwVerInfoSize);

		WCHAR *szVer = nullptr;
		UINT uVerLength;
		if (!(::GetFileVersionInfoW(szFullPath, (DWORD)dwVerHnd,
								   (DWORD)dwVerInfoSize, pVersionInfo.data())))
		{
			throw std::runtime_error("GetFileVersionInfo() returned false");
		}
		if (!(::VerQueryValueW(pVersionInfo.data(), L"\\StringFileInfo\\040904b0\\FileVersion",
							  (LPVOID*)&szVer, &uVerLength))) {
			throw std::runtime_error("VerQueryValue() returned false");
		}

		mpt::ustring version = mpt::ToUnicode(szVer);

		VERIFY_EQUAL( version, mpt::ufmt::val(Version::Current()) );
		VERIFY_EQUAL( Version::Parse(version), Version::Current() );
	}
#endif

#ifdef LIBOPENMPT_BUILD
#if MPT_TEST_HAS_FILESYSTEM
#if !MPT_OS_DJGPP
	mpt::PathString version_mk = GetPathPrefix() + P_("libopenmpt/libopenmpt_version.mk");
	mpt::ifstream f(version_mk, std::ios::in);
	VERIFY_EQUAL(f ? true : false, true);
	std::map<std::string, std::string> fields;
	std::string line;
	while(std::getline(f, line))
	{
		line = mpt::trim(line);
		if(line.empty())
		{
			continue;
		}
		std::vector<std::string> line_fields = mpt::String::Split<std::string>(line, std::string("="));
		VERIFY_EQUAL_NONCONT(line_fields.size(), 2u);
		line_fields[0] = mpt::trim(line_fields[0]);
		line_fields[1] = mpt::trim(line_fields[1]);
		VERIFY_EQUAL_NONCONT(line_fields[0].length() > 0, true);
		fields[line_fields[0]] = line_fields[1];
	}
	VERIFY_EQUAL(fields["LIBOPENMPT_VERSION_MAJOR"], mpt::afmt::val(OPENMPT_API_VERSION_MAJOR));
	VERIFY_EQUAL(fields["LIBOPENMPT_VERSION_MINOR"], mpt::afmt::val(OPENMPT_API_VERSION_MINOR));
	VERIFY_EQUAL(fields["LIBOPENMPT_VERSION_PATCH"], mpt::afmt::val(OPENMPT_API_VERSION_PATCH));
	VERIFY_EQUAL(fields["LIBOPENMPT_VERSION_PREREL"], mpt::afmt::val(OPENMPT_API_VERSION_PREREL));
	if(std::string(OPENMPT_API_VERSION_PREREL).length() > 0)
	{
		VERIFY_EQUAL(std::string(OPENMPT_API_VERSION_PREREL).substr(0, 1), "-");
	}
	VERIFY_EQUAL(OPENMPT_API_VERSION_IS_PREREL, (std::string(OPENMPT_API_VERSION_PREREL).length() > 0) ? 1 : 0);
	
	VERIFY_EQUAL(fields["LIBOPENMPT_LTVER_CURRENT"].length() > 0, true);
	VERIFY_EQUAL(fields["LIBOPENMPT_LTVER_REVISION"].length() > 0, true);
	VERIFY_EQUAL(fields["LIBOPENMPT_LTVER_AGE"].length() > 0, true);
#endif // !MPT_OS_DJGPP
#endif // MPT_TEST_HAS_FILESYSTEM
#endif // LIBOPENMPT_BUILD

}


// Test if data types are interpreted correctly
static MPT_NOINLINE void TestTypes()
{

	static_assert(sizeof(std::uintptr_t) == sizeof(void*));
	#if defined(__SIZEOF_POINTER__)
		static_assert(__SIZEOF_POINTER__ == mpt::pointer_size);
		static_assert(__SIZEOF_POINTER__ == sizeof(void*));
	#endif

	VERIFY_EQUAL(int8_min, std::numeric_limits<int8>::min());
	VERIFY_EQUAL(int8_max, std::numeric_limits<int8>::max());
	VERIFY_EQUAL(uint8_max, std::numeric_limits<uint8>::max());

	VERIFY_EQUAL(int16_min, std::numeric_limits<int16>::min());
	VERIFY_EQUAL(int16_max, std::numeric_limits<int16>::max());
	VERIFY_EQUAL(uint16_max, std::numeric_limits<uint16>::max());

	VERIFY_EQUAL(int32_min, std::numeric_limits<int32>::min());
	VERIFY_EQUAL(int32_max, std::numeric_limits<int32>::max());
	VERIFY_EQUAL(uint32_max, std::numeric_limits<uint32>::max());

	VERIFY_EQUAL(int64_min, std::numeric_limits<int64>::min());
	VERIFY_EQUAL(int64_max, std::numeric_limits<int64>::max());
	VERIFY_EQUAL(uint64_max, std::numeric_limits<uint64>::max());


	static_assert(int8_max == std::numeric_limits<int8>::max());
	static_assert(uint8_max == std::numeric_limits<uint8>::max());

	static_assert(int16_max == std::numeric_limits<int16>::max());
	static_assert(uint16_max == std::numeric_limits<uint16>::max());

	static_assert(int32_max == std::numeric_limits<int32>::max());
	static_assert(uint32_max == std::numeric_limits<uint32>::max());

	static_assert(int64_max == std::numeric_limits<int64>::max());
	static_assert(uint64_max == std::numeric_limits<uint64>::max());

}



#ifdef MODPLUG_TRACKER

// In tracker debug builds, the sprintf-like function is retained in order to be able to validate our own formatting against sprintf.

// There are 4 reasons why this is not available for library code:
//  1. printf-like functionality is not type-safe.
//  2. There are portability problems with char/wchar_t and the semantics of %s/%ls/%S .
//  3. There are portability problems with specifying format for 64bit integers.
//  4. Formatting of floating point values depends on the currently set C locale.
//     A library is not allowed to mock with that and thus cannot influence the behavior in this case.

template <typename T>
static std::string StringFormat(std::string format, T x)
{
	#if MPT_COMPILER_MSVC
		// Count the needed array size.
		const size_t nCount = _scprintf(format.c_str(), x); // null character not included.
		std::vector<char> buf(nCount + 1); // + 1 is for null terminator.
		sprintf_s(&(buf[0]), buf.size(), format.c_str(), x);
		return &(buf[0]);
	#else
		int size = snprintf(NULL, 0, format.c_str(), x); // get required size, requires c99 compliant snprintf which msvc does not have
		std::vector<char> temp(size + 1);
		snprintf(&(temp[0]), size + 1, format.c_str(), x);
		return &(temp[0]);
	#endif
}

#endif

template <typename Tfloat>
static void TestFloatFormat(Tfloat x, std::string format, mpt::FormatFlags f, std::size_t width = 0, int precision = -1)
{
#ifdef MODPLUG_TRACKER
	std::string str_sprintf = StringFormat(format, x);
#endif
	std::string str_iostreams = mpt::afmt::fmt(x, mpt::FormatSpec().SetFlags(f).SetWidth(width).SetPrecision(precision));
#ifdef MODPLUG_TRACKER
	//MPT_LOG_GLOBAL(LogDebug, "test", mpt::ToUnicode(mpt::Charset::ASCII, str_sprintf));
#endif
	//MPT_LOG_GLOBAL(LogDebug, "test", mpt::ToUnicode(mpt::Charset::ASCII, str_iostreams));
#ifdef MODPLUG_TRACKER
#if MPT_MSVC_AT_LEAST(2019,4) || MPT_GCC_AT_LEAST(11,1,0)
	// to_chars generates shortest form instead of 0-padded precision
	if(precision != -1)
#endif
	{
		VERIFY_EQUAL(str_iostreams, str_sprintf); // this will fail with a set c locale (and there is nothing that can be done about that in libopenmpt)
	}
#else
	MPT_UNREFERENCED_PARAMETER(format);
	MPT_UNUSED_VARIABLE(str_iostreams);
#endif
}


template <typename Tfloat>
static void TestFloatFormats(Tfloat x)
{

	TestFloatFormat(x, "%.8g", mpt::afmt::NotaNrm | mpt::afmt::FillOff, 0, 8);

	TestFloatFormat(x, MPT_AFORMAT("%.{}g")(std::numeric_limits<Tfloat>::max_digits10), mpt::afmt::NotaNrm | mpt::afmt::FillOff);
	TestFloatFormat(x, MPT_AFORMAT("%.{}f")(std::numeric_limits<Tfloat>::digits10), mpt::afmt::NotaFix | mpt::afmt::FillOff);
	TestFloatFormat(x, MPT_AFORMAT("%.{}e")(std::numeric_limits<Tfloat>::max_digits10 - 1), mpt::afmt::NotaSci | mpt::afmt::FillOff);

	TestFloatFormat(x, "%.0f", mpt::afmt::NotaFix | mpt::afmt::FillOff, 0, 0);
	TestFloatFormat(x, "%.1f", mpt::afmt::NotaFix | mpt::afmt::FillOff, 0, 1);
	TestFloatFormat(x, "%.2f", mpt::afmt::NotaFix | mpt::afmt::FillOff, 0, 2);
	TestFloatFormat(x, "%.3f", mpt::afmt::NotaFix | mpt::afmt::FillOff, 0, 3);
	TestFloatFormat(x, "%0.1f", mpt::afmt::NotaFix | mpt::afmt::FillNul, 0, 1);
	TestFloatFormat(x, "%02.0f", mpt::afmt::NotaFix | mpt::afmt::FillNul, 2, 0);
}



static MPT_NOINLINE void TestStringFormatting()
{
	VERIFY_EQUAL(mpt::afmt::val(1.5f), "1.5");
	VERIFY_EQUAL(mpt::afmt::val(true), "1");
	VERIFY_EQUAL(mpt::afmt::val(false), "0");
	//VERIFY_EQUAL(mpt::afmt::val('A'), "A"); // deprecated
	//VERIFY_EQUAL(mpt::afmt::val(L'A'), "A"); // deprecated

	VERIFY_EQUAL(mpt::afmt::val(0), "0");
	VERIFY_EQUAL(mpt::afmt::val(-23), "-23");
	VERIFY_EQUAL(mpt::afmt::val(42), "42");

	VERIFY_EQUAL(mpt::afmt::hex0<3>((int32)-1), "-001");
	VERIFY_EQUAL(mpt::afmt::hex((int32)-1), "-1");
	VERIFY_EQUAL(mpt::afmt::hex(-0xabcde), "-abcde");
	VERIFY_EQUAL(mpt::afmt::hex(int32_min), "-80000000");
	VERIFY_EQUAL(mpt::afmt::hex(int32_min + 1), "-7fffffff");
	VERIFY_EQUAL(mpt::afmt::hex(0x123e), "123e");
	VERIFY_EQUAL(mpt::afmt::hex0<6>(0x123e), "00123e");
	VERIFY_EQUAL(mpt::afmt::hex0<2>(0x123e), "123e");

	VERIFY_EQUAL(mpt::afmt::dec0<0>(1), "1");
	VERIFY_EQUAL(mpt::afmt::dec0<1>(1), "1");
	VERIFY_EQUAL(mpt::afmt::dec0<2>(1), "01");
	VERIFY_EQUAL(mpt::afmt::dec0<3>(1), "001");
	VERIFY_EQUAL(mpt::afmt::dec0<0>(11), "11");
	VERIFY_EQUAL(mpt::afmt::dec0<1>(11), "11");
	VERIFY_EQUAL(mpt::afmt::dec0<2>(11), "11");
	VERIFY_EQUAL(mpt::afmt::dec0<3>(11), "011");
	VERIFY_EQUAL(mpt::afmt::dec0<0>(-1), "-1");
	VERIFY_EQUAL(mpt::afmt::dec0<1>(-1), "-1");
	VERIFY_EQUAL(mpt::afmt::dec0<2>(-1), "-01");
	VERIFY_EQUAL(mpt::afmt::dec0<3>(-1), "-001");

	VERIFY_EQUAL(mpt::ufmt::HEX0<7>(0xa2345678), U_("A2345678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<8>(0xa2345678), U_("A2345678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<9>(0xa2345678), U_("0A2345678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<10>(0xa2345678), U_("00A2345678"));

#if MPT_WSTRING_FORMAT
	VERIFY_EQUAL(mpt::wfmt::hex(0x123e), L"123e");
	VERIFY_EQUAL(mpt::wfmt::hex0<6>(0x123e), L"00123e");
	VERIFY_EQUAL(mpt::wfmt::hex0<2>(0x123e), L"123e");
#endif

	VERIFY_EQUAL(mpt::afmt::val(-87.0f), "-87");
	if(mpt::afmt::val(-0.5e-6) != "-5e-007"
		&& mpt::afmt::val(-0.5e-6) != "-5e-07"
		&& mpt::afmt::val(-0.5e-6) != "-5e-7"
		&& mpt::afmt::val(-0.5e-6) != "-4.9999999999999998e-7"
		&& mpt::afmt::val(-0.5e-6) != "-4.9999999999999998e-07"
		&& mpt::afmt::val(-0.5e-6) != "-4.9999999999999998e-007"
		)
	{
		VERIFY_EQUAL(true, false);
	}
	if(mpt::afmt::val(-1.0 / 65536.0) != "-1.52587890625e-005"
		&& mpt::afmt::val(-1.0 / 65536.0) != "-1.52587890625e-05"
		&& mpt::afmt::val(-1.0 / 65536.0) != "-1.52587890625e-5"
		)
	{
		VERIFY_EQUAL(true, false);
	}
	if(mpt::afmt::val(-1.0f / 65536.0f) != "-1.52587891e-005"
		&& mpt::afmt::val(-1.0f / 65536.0f) != "-1.52587891e-05"
		&& mpt::afmt::val(-1.0f / 65536.0f) != "-1.52587891e-5"
		&& mpt::afmt::val(-1.0f / 65536.0f) != "-1.5258789e-005"
		&& mpt::afmt::val(-1.0f / 65536.0f) != "-1.5258789e-05"
		&& mpt::afmt::val(-1.0f / 65536.0f) != "-1.5258789e-5"
		)
	{
		VERIFY_EQUAL(true, false);
	}
	if(mpt::afmt::val(58.65403492763) != "58.654034927630001"
		&& mpt::afmt::val(58.65403492763) != "58.65403492763"
		)
	{
		VERIFY_EQUAL(true, false);
	}
	VERIFY_EQUAL(mpt::afmt::flt(58.65403492763, 6), "58.654");
	VERIFY_EQUAL(mpt::afmt::fix(23.42, 1), "23.4");
	VERIFY_EQUAL(mpt::afmt::fix(234.2, 1), "234.2");
	VERIFY_EQUAL(mpt::afmt::fix(2342.0, 1), "2342.0");
	
	VERIFY_EQUAL(mpt::afmt::dec(2, ';', 2345678), std::string("2;34;56;78"));
	VERIFY_EQUAL(mpt::afmt::dec(2, ';', 12345678), std::string("12;34;56;78"));
	VERIFY_EQUAL(mpt::afmt::hex(3, ':', 0xa2345678), std::string("a2:345:678"));

	VERIFY_EQUAL(mpt::ufmt::dec(2, ';', 12345678), U_("12;34;56;78"));
	VERIFY_EQUAL(mpt::ufmt::hex(3, ':', 0xa2345678), U_("a2:345:678"));

	VERIFY_EQUAL(mpt::ufmt::HEX0<7>(3, ':', 0xa2345678), U_("A2:345:678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<8>(3, ':', 0xa2345678), U_("A2:345:678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<9>(3, ':', 0xa2345678), U_("0A2:345:678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<10>(3, ':', 0xa2345678), U_("0:0A2:345:678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<11>(3, ':', 0xa2345678), U_("00:0A2:345:678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<12>(3, ':', 0xa2345678), U_("000:0A2:345:678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<7>(3, ':', -0x12345678), U_("-12:345:678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<8>(3, ':', -0x12345678), U_("-12:345:678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<9>(3, ':', -0x12345678), U_("-012:345:678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<10>(3, ':', -0x12345678), U_("-0:012:345:678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<11>(3, ':', -0x12345678), U_("-00:012:345:678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<12>(3, ':', -0x12345678), U_("-000:012:345:678"));

	VERIFY_EQUAL(mpt::ufmt::HEX0<5>(3, ':', 0x345678), U_("345:678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<6>(3, ':', 0x345678), U_("345:678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<7>(3, ':', 0x345678), U_("0:345:678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<5>(3, ':', -0x345678), U_("-345:678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<6>(3, ':', -0x345678), U_("-345:678"));
	VERIFY_EQUAL(mpt::ufmt::HEX0<7>(3, ':', -0x345678), U_("-0:345:678"));

	VERIFY_EQUAL(mpt::afmt::left(3, "a"), "a  ");
	VERIFY_EQUAL(mpt::afmt::right(3, "a"), "  a");
	VERIFY_EQUAL(mpt::afmt::center(3, "a"), " a ");
	VERIFY_EQUAL(mpt::afmt::center(4, "a"), " a  ");

	#if defined(MPT_WITH_MFC)
		VERIFY_EQUAL(mpt::cfmt::left(3, CString(_T("a"))), CString(_T("a  ")));
		VERIFY_EQUAL(mpt::cfmt::right(3, CString(_T("a"))), CString(_T("  a")));
		VERIFY_EQUAL(mpt::cfmt::center(3, CString(_T("a"))), CString(_T(" a ")));
		VERIFY_EQUAL(mpt::cfmt::center(4, CString(_T("a"))), CString(_T(" a  ")));
	#endif // MPT_WITH_MFC

	VERIFY_EQUAL(ConvertStrTo<uint32>("586"), 586u);
	VERIFY_EQUAL(ConvertStrTo<uint32>("2147483647"), (uint32)int32_max);
	VERIFY_EQUAL(ConvertStrTo<uint32>("4294967295"), uint32_max);

	VERIFY_EQUAL(ConvertStrTo<int64>("-9223372036854775808"), int64_min);
	VERIFY_EQUAL(ConvertStrTo<int64>("-159"), -159);
	VERIFY_EQUAL(ConvertStrTo<int64>("9223372036854775807"), int64_max);

	VERIFY_EQUAL(ConvertStrTo<uint64>("85059"), 85059u);
	VERIFY_EQUAL(ConvertStrTo<uint64>("9223372036854775807"), (uint64)int64_max);
	VERIFY_EQUAL(ConvertStrTo<uint64>("18446744073709551615"), uint64_max);

	VERIFY_EQUAL(ConvertStrTo<float>("-87.0"), -87.0f);
#if !MPT_OS_DJGPP
	VERIFY_EQUAL(ConvertStrTo<double>("-0.5e-6"), -0.5e-6);
#endif
#if !MPT_OS_DJGPP
	VERIFY_EQUAL(ConvertStrTo<double>("58.65403492763"), 58.65403492763);
#else
	VERIFY_EQUAL_EPS(ConvertStrTo<double>("58.65403492763"), 58.65403492763, 0.0001);
#endif

	VERIFY_EQUAL(ConvertStrTo<float>(mpt::afmt::val(-87.0)), -87.0f);
#if !MPT_OS_DJGPP
	VERIFY_EQUAL(ConvertStrTo<double>(mpt::afmt::val(-0.5e-6)), -0.5e-6);
#endif

	VERIFY_EQUAL(mpt::String::Parse::Hex<unsigned char>("fe"), 254);
#if MPT_WSTRING_FORMAT
	VERIFY_EQUAL(mpt::String::Parse::Hex<unsigned char>(L"fe"), 254);
#endif
	VERIFY_EQUAL(mpt::String::Parse::Hex<unsigned int>(U_("ffff")), 65535);

	TestFloatFormats(0.0f);
	TestFloatFormats(-0.0f);
	TestFloatFormats(1.0f);
	TestFloatFormats(-1.0f);
	TestFloatFormats(0.1f);
	TestFloatFormats(-0.1f);
	TestFloatFormats(1000000000.0f);
	TestFloatFormats(-1000000000.0f);
	TestFloatFormats(0.0000000001f);
	TestFloatFormats(-0.0000000001f);
	TestFloatFormats(6.12345f);

	TestFloatFormats(0.0);
	TestFloatFormats(-0.0);
	TestFloatFormats(1.0);
	TestFloatFormats(-1.0);
	TestFloatFormats(0.1);
	TestFloatFormats(-0.1);
	TestFloatFormats(1000000000.0);
	TestFloatFormats(-1000000000.0);
	TestFloatFormats(0.0000000001);
	TestFloatFormats(-0.0000000001);
	TestFloatFormats(6.12345);

	TestFloatFormats(42.1234567890);
	TestFloatFormats(0.1234567890);
	TestFloatFormats(1234567890000000.0);
	TestFloatFormats(0.0000001234567890);

	TestFloatFormats(mpt::numbers::pi);
	TestFloatFormats(3.14159265358979323846);
	TestFloatFormats(3.14159265358979323846f);

	VERIFY_EQUAL(mpt::afmt::flt(6.12345, 3), "6.12");
	VERIFY_EQUAL(mpt::afmt::fix(6.12345, 3), "6.123");
	VERIFY_EQUAL(mpt::afmt::flt(6.12345, 4), "6.123");
	VERIFY_EQUAL(mpt::afmt::fix(6.12345, 4), "6.1235");

#if MPT_WSTRING_FORMAT
	VERIFY_EQUAL(mpt::wfmt::flt(6.12345, 3), L"6.12");
	VERIFY_EQUAL(mpt::wfmt::fix(6.12345, 3), L"6.123");
	VERIFY_EQUAL(mpt::wfmt::flt(6.12345, 4), L"6.123");
#endif

	static_assert(mpt::parse_format_string_argument_count("") == 0);
	static_assert(mpt::parse_format_string_argument_count("{{") == 0);
	static_assert(mpt::parse_format_string_argument_count("}}") == 0);
	static_assert(mpt::parse_format_string_argument_count("{}") == 1);
	static_assert(mpt::parse_format_string_argument_count("{}{}") == 2);
	static_assert(mpt::parse_format_string_argument_count("{0}{1}") == 2);

	// basic
	VERIFY_EQUAL(MPT_AFORMAT("{}{}{}")(1,2,3), "123");
	VERIFY_EQUAL(MPT_AFORMAT("{2}{1}{0}")(1,2,3), "321");

	VERIFY_EQUAL(MPT_AFORMAT("{2}{1}{0}{4}{3}{6}{5}{7}{10}{9}{8}")(0,1,2,3,4,5,6,7,8,9,"a"), "21043657a98");

	//VERIFY_EQUAL(MPT_AFORMAT("{2}{1}{0}{2}{1}{0}{10}{9}{8}")(0,1,2,3,4,5,6,7,8,9,"a"), "210210a98");

#if MPT_WSTRING_FORMAT
	VERIFY_EQUAL(MPT_WFORMAT("{}{}{}")(1,2,3), L"123");
#endif

	// escaping behviour
	VERIFY_EQUAL(MPT_AFORMAT("%")(), "%");
	VERIFY_EQUAL(MPT_AFORMAT("%")(), "%");
	VERIFY_EQUAL(MPT_AFORMAT("%%")(), "%%");
	VERIFY_EQUAL(MPT_AFORMAT("{}")("a"), "a");
	VERIFY_EQUAL(MPT_AFORMAT("{}%")("a"), "a%");
	VERIFY_EQUAL(MPT_AFORMAT("{}%")("a"), "a%");
	VERIFY_EQUAL(MPT_AFORMAT("{}%%")("a"), "a%%");
	VERIFY_EQUAL(MPT_AFORMAT("%1")(), "%1");
	VERIFY_EQUAL(MPT_AFORMAT("%{}")("a"), "%a");
	VERIFY_EQUAL(MPT_AFORMAT("%b")(), "%b");
	VERIFY_EQUAL(MPT_AFORMAT("{{}}")(), "{}");
	VERIFY_EQUAL(MPT_AFORMAT("{{{}}}")("a"), "{a}");

#if defined(MPT_WITH_MFC)
	VERIFY_EQUAL(mpt::ufmt::val(CString(_T("foobar"))), U_("foobar"));
	VERIFY_EQUAL(mpt::ufmt::val(CString(_T("foobar"))), U_("foobar"));
	VERIFY_EQUAL(MPT_CFORMAT("{}{}{}")(1,2,3), _T("123"));
	VERIFY_EQUAL(MPT_CFORMAT("{}{}{}")(1,mpt::cfmt::dec0<3>(2),3), _T("10023"));
#endif // MPT_WITH_MFC

	FlagSet<MODTYPE> foo;
	foo.set(MOD_TYPE_MOD, true);
	VERIFY_EQUAL(MPT_UFORMAT("{}")(foo), U_("00000000000000000000000000000001"));

}


namespace {

struct Gregorian {
	int Y,M,D,h,m,s;
	static Gregorian FromTM(tm t) {
		Gregorian g;
		g.Y = t.tm_year + 1900;
		g.M = t.tm_mon + 1;
		g.D = t.tm_mday;
		g.h = t.tm_hour;
		g.m = t.tm_min;
		g.s = t.tm_sec;
		return g;
	}
	static tm ToTM(Gregorian g) {
		tm t;
		MemsetZero(t);
		t.tm_year = g.Y - 1900;
		t.tm_mon = g.M - 1;
		t.tm_mday = g.D;
		t.tm_hour = g.h;
		t.tm_min = g.m;
		t.tm_sec = g.s;
		return t;
	}
};

inline bool operator ==(Gregorian a, Gregorian b) {
	return a.Y == b.Y && a.M == b.M && a.D == b.D && a.h == b.h && a.m == b.m && a.s == b.s;
}

}

static int64 TestDate1(int s, int m, int h, int D, int M, int Y) {
	return mpt::Date::Unix::FromUTC(Gregorian::ToTM(Gregorian{Y,M,D,h,m,s}));
}

static Gregorian TestDate2(int s, int m, int h, int D, int M, int Y) {
	return Gregorian{Y,M,D,h,m,s};
}


static MPT_NOINLINE void TestMisc1()
{

	VERIFY_EQUAL(ModCommand::IsPcNote(NOTE_MAX), false);
	VERIFY_EQUAL(ModCommand::IsPcNote(NOTE_PC), true);
	VERIFY_EQUAL(ModCommand::IsPcNote(NOTE_PCS), true);

	VERIFY_EQUAL(CModSpecifications::ExtensionToType(".mod"), MOD_TYPE_MOD);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("mod"), MOD_TYPE_MOD);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(".s3m"), MOD_TYPE_S3M);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("s3m"), MOD_TYPE_S3M);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(".xm"), MOD_TYPE_XM);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("xm"), MOD_TYPE_XM);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(".it"), MOD_TYPE_IT);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("it"), MOD_TYPE_IT);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(".itp"), MOD_TYPE_NONE);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("itp"), MOD_TYPE_NONE);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("mptm"), MOD_TYPE_MPT);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("invalidExtension"), MOD_TYPE_NONE);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("ita"), MOD_TYPE_NONE);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("s2m"), MOD_TYPE_NONE);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(""), MOD_TYPE_NONE);

	// invalid
	VERIFY_EQUAL(SampleFormat::FromInt(0), SampleFormat::Invalid);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0000'11'0), SampleFormat::Invalid);

	// correct
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0001'00'1), SampleFormat::Unsigned8);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0001'00'0), SampleFormat::Int8);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0010'00'0), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0011'00'0), SampleFormat::Int24);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0100'00'0), SampleFormat::Int32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0100'00'0), SampleFormat::Float32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'1000'00'0), SampleFormat::Float64);

	// no size
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0000'00'0), SampleFormat::Invalid);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0000'00'1), SampleFormat::Unsigned8);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0000'00'0), SampleFormat::Float32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0000'00'1), SampleFormat::Invalid);

	// invalid unsigned
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0010'00'1), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0011'00'1), SampleFormat::Int24);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0100'00'1), SampleFormat::Int32);

	// invalid float
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0001'00'0), SampleFormat::Int8);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0010'00'0), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0011'00'0), SampleFormat::Int24);

	// bogus size

	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0001'00'0), SampleFormat::Int8);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0010'00'0), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0011'00'0), SampleFormat::Int24);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0100'00'0), SampleFormat::Int32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0101'00'0), SampleFormat::Int32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0110'00'0), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0111'00'0), SampleFormat::Int24);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'1000'00'0), SampleFormat::Float64);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'1001'00'0), SampleFormat::Float64);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'1010'00'0), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'1011'00'0), SampleFormat::Int24);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'1100'00'0), SampleFormat::Int32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'1101'00'0), SampleFormat::Int32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'1110'00'0), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'1111'00'0), SampleFormat::Int24);

	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0001'00'1), SampleFormat::Unsigned8);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0010'00'1), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0011'00'1), SampleFormat::Int24);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0100'00'1), SampleFormat::Int32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0101'00'1), SampleFormat::Int32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0110'00'1), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'0111'00'1), SampleFormat::Int24);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'1000'00'1), SampleFormat::Float64);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'1001'00'1), SampleFormat::Float64);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'1010'00'1), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'1011'00'1), SampleFormat::Int24);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'1100'00'1), SampleFormat::Int32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'1101'00'1), SampleFormat::Int32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'1110'00'1), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b0'1111'00'1), SampleFormat::Int24);

	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0001'00'0), SampleFormat::Int8);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0010'00'0), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0011'00'0), SampleFormat::Int24);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0100'00'0), SampleFormat::Float32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0101'00'0), SampleFormat::Float32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0110'00'0), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0111'00'0), SampleFormat::Int24);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'1000'00'0), SampleFormat::Float64);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'1001'00'0), SampleFormat::Float64);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'1010'00'0), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'1011'00'0), SampleFormat::Int24);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'1100'00'0), SampleFormat::Float32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'1101'00'0), SampleFormat::Float32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'1110'00'0), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'1111'00'0), SampleFormat::Int24);

	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0001'00'1), SampleFormat::Unsigned8);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0010'00'1), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0011'00'1), SampleFormat::Int24);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0100'00'1), SampleFormat::Float32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0101'00'1), SampleFormat::Float32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0110'00'1), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'0111'00'1), SampleFormat::Int24);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'1000'00'1), SampleFormat::Float64);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'1001'00'1), SampleFormat::Float64);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'1010'00'1), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'1011'00'1), SampleFormat::Int24);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'1100'00'1), SampleFormat::Float32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'1101'00'1), SampleFormat::Float32);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'1110'00'1), SampleFormat::Int16);
	VERIFY_EQUAL(SampleFormat::FromInt(0b1'1111'00'1), SampleFormat::Int24);

}


static MPT_NOINLINE void TestMisc2()
{

	// Check for completeness of supported effect list in mod specifications
	for(const auto &spec : ModSpecs::Collection)
	{
		VERIFY_EQUAL(strlen(spec->commands), (size_t)MAX_EFFECTS);
		VERIFY_EQUAL(strlen(spec->volcommands), (size_t)MAX_VOLCMDS);
	}

#ifdef MODPLUG_TRACKER
#ifdef MPT_ENABLE_FILEIO

	{
		std::vector<std::byte> data;
		data.push_back(mpt::as_byte(0));
		data.push_back(mpt::as_byte(255));
		data.push_back(mpt::as_byte(1));
		data.push_back(mpt::as_byte(2));
		mpt::PathString fn = GetTempFilenameBase() + P_("lazy");
		RemoveFile(fn);
		mpt::LazyFileRef f(fn);
		f = data;
		std::vector<std::byte> data2;
		data2 = f;
		VERIFY_EQUAL(data.size(), data2.size());
		for(std::size_t i = 0; i < data.size() && i < data2.size(); ++i)
		{
			VERIFY_EQUAL(data[i], data2[i]);
		}
		RemoveFile(fn);
	}

#endif
#endif // MODPLUG_TRACKER

#ifdef MPT_WITH_ZLIB
	VERIFY_EQUAL(crc32(0, mpt::byte_cast<const unsigned char*>(std::string("123456789").c_str()), 9), 0xCBF43926u);
#endif
#ifdef MPT_WITH_MINIZ
	VERIFY_EQUAL(mz_crc32(0, mpt::byte_cast<const unsigned char*>(std::string("123456789").c_str()), 9), 0xCBF43926u);
#endif

	// Check floating-point accuracy in TransposeToFrequency
	static constexpr int32 transposeToFrequency[] =
	{
		      5,       5,       5,       5,
		     31,      32,      33,      34,
		    196,     202,     207,     214,
		   1243,    1280,    1317,    1356,
		   7894,    8125,    8363,    8608,
		  50121,   51590,   53102,   54658,
		 318251,  327576,  337175,  347055,
		2020767, 2079980, 2140928, 2203663,
	};

	int freqIndex = 0;
	for(int32 transpose = -128; transpose < 128; transpose += 32)
	{
		for(int32 finetune = -128; finetune < 128; finetune += 64, freqIndex++)
		{
			const auto freq = ModSample::TransposeToFrequency(transpose, finetune);
			VERIFY_EQUAL_EPS(transposeToFrequency[freqIndex], static_cast<int32>(freq), 1);
			if(transpose >= -96)
			{
				// Verify transpose+finetune <-> frequency roundtrip
				// (not for transpose = -128 because it would require fractional precision that we don't have here)
				ModSample smp;
				smp.nC5Speed = freq;
				smp.FrequencyToTranspose();
				smp.TransposeToFrequency();
				VERIFY_EQUAL(freq, smp.nC5Speed);
			}
		}
	}

	{
		ModSample smp;
		smp.nC5Speed = 9999;
		smp.Transpose(1.5);
		VERIFY_EQUAL_EPS(28281, static_cast<int32>(smp.nC5Speed), 1);
		smp.Transpose(-1.3);
		VERIFY_EQUAL_EPS(11486, static_cast<int32>(smp.nC5Speed), 1);
	}

	// Check SamplePosition fixed-point type
	VERIFY_EQUAL(SamplePosition(1).GetRaw(), 1);
	VERIFY_EQUAL(SamplePosition(2).Set(1), SamplePosition(1, 0));
	VERIFY_EQUAL(SamplePosition(2).SetInt(1), SamplePosition(1, 2));
	VERIFY_EQUAL(SamplePosition(1).IsPositive(), true);
	VERIFY_EQUAL(SamplePosition(0).IsZero(), true);
	VERIFY_EQUAL(SamplePosition(1, 0).IsUnity(), true);
	VERIFY_EQUAL(SamplePosition(1, 1).IsUnity(), false);
	VERIFY_EQUAL(SamplePosition(-1).IsNegative(), true);
	VERIFY_EQUAL(SamplePosition(int64_max).GetRaw(), int64_max);
	VERIFY_EQUAL(SamplePosition(2, SamplePosition::fractMax).GetInt(), 2);
	VERIFY_EQUAL(SamplePosition(2, SamplePosition::fractMax).GetFract(), SamplePosition::fractMax);
	VERIFY_EQUAL(SamplePosition(1, SamplePosition::fractMax).GetInvertedFract(), SamplePosition(0, 1));
	VERIFY_EQUAL(SamplePosition(1, 0).GetInvertedFract(), SamplePosition(1, 0));
	VERIFY_EQUAL(SamplePosition(2, 0).Negate(), SamplePosition(-2, 0));
	VERIFY_EQUAL(SamplePosition::Ratio(10, 5), SamplePosition(2, 0));
	VERIFY_EQUAL(SamplePosition(1, 1) + SamplePosition(2, 2), SamplePosition(3, 3));
	VERIFY_EQUAL(SamplePosition(1, 0) * 3, SamplePosition(3, 0));
	VERIFY_EQUAL((SamplePosition(6, 0) / SamplePosition(2, 0)), 3);
	
	VERIFY_EQUAL(srlztn::ID::FromInt(static_cast<uint32>(0x87654321u)).AsString(), srlztn::ID("\x21\x43\x65\x87").AsString());

#if defined(MODPLUG_TRACKER)

	VERIFY_EQUAL(mpt::OS::Wine::Version(U_("1.1.44" )).AsString() , U_("1.1.44"));
	VERIFY_EQUAL(mpt::OS::Wine::Version(U_("1.6.2"  )).AsString() , U_("1.6.2" ));
	VERIFY_EQUAL(mpt::OS::Wine::Version(U_("1.8"    )).AsString() , U_("1.8.0" ));
	VERIFY_EQUAL(mpt::OS::Wine::Version(U_("2.0-rc" )).AsString() , U_("2.0.0" ));
	VERIFY_EQUAL(mpt::OS::Wine::Version(U_("2.0-rc4")).AsString() , U_("2.0.0" ));
	VERIFY_EQUAL(mpt::OS::Wine::Version(U_("2.0"    )).AsString() , U_("2.0.0" ));
	VERIFY_EQUAL(mpt::OS::Wine::Version(U_("2.4"    )).AsString() , U_("2.4.0" ));

#endif // MODPLUG_TRACKER

	// date

	VERIFY_EQUAL(             0, TestDate1(  0,  0,  0,  1,  1, 1970 ));
	VERIFY_EQUAL(          3600, TestDate1(  0,  0,  1,  1,  1, 1970 ));
	VERIFY_EQUAL(         86400, TestDate1(  0,  0,  0,  2,  1, 1970 ));
	VERIFY_EQUAL(      31536000, TestDate1(  0,  0,  0,  1,  1, 1971 ));
	VERIFY_EQUAL(     100000000, TestDate1( 40, 46,  9,  3,  3, 1973 ));
	VERIFY_EQUAL(     951782400, TestDate1(  0,  0,  0, 29,  2, 2000 ));
	VERIFY_EQUAL(    1000000000, TestDate1( 40, 46,  1,  9,  9, 2001 ));
	VERIFY_EQUAL(    1044057600, TestDate1(  0,  0,  0,  1,  2, 2003 ));
	VERIFY_EQUAL(    1044144000, TestDate1(  0,  0,  0,  2,  2, 2003 ));
	VERIFY_EQUAL(    1046476800, TestDate1(  0,  0,  0,  1,  3, 2003 ));
	VERIFY_EQUAL(    1064966400, TestDate1(  0,  0,  0,  1, 10, 2003 ));
	VERIFY_EQUAL(    1077926399, TestDate1( 59, 59, 23, 27,  2, 2004 ));
	VERIFY_EQUAL(    1077926400, TestDate1(  0,  0,  0, 28,  2, 2004 ));
	VERIFY_EQUAL(    1077926410, TestDate1( 10,  0,  0, 28,  2, 2004 ));
	VERIFY_EQUAL(    1078012799, TestDate1( 59, 59, 23, 28,  2, 2004 ));
	VERIFY_EQUAL(    1078012800, TestDate1(  0,  0,  0, 29,  2, 2004 ));
	VERIFY_EQUAL(    1078012820, TestDate1( 20,  0,  0, 29,  2, 2004 ));
	VERIFY_EQUAL(    1078099199, TestDate1( 59, 59, 23, 29,  2, 2004 ));
	VERIFY_EQUAL(    1078099200, TestDate1(  0,  0,  0,  1,  3, 2004 ));
	VERIFY_EQUAL(    1078099230, TestDate1( 30,  0,  0,  1,  3, 2004 ));
	VERIFY_EQUAL(    1078185599, TestDate1( 59, 59, 23,  1,  3, 2004 ));
	VERIFY_EQUAL(    1096588800, TestDate1(  0,  0,  0,  1, 10, 2004 ));
	VERIFY_EQUAL(    1413064016, TestDate1( 56, 46, 21, 11, 10, 2014 ));
	VERIFY_EQUAL(    1413064100, TestDate1( 20, 48, 21, 11, 10, 2014 ));

	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(             0).AsUTC()), TestDate2(  0,  0,  0,  1,  1, 1970 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(          3600).AsUTC()), TestDate2(  0,  0,  1,  1,  1, 1970 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(         86400).AsUTC()), TestDate2(  0,  0,  0,  2,  1, 1970 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(      31536000).AsUTC()), TestDate2(  0,  0,  0,  1,  1, 1971 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(     100000000).AsUTC()), TestDate2( 40, 46,  9,  3,  3, 1973 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(     951782400).AsUTC()), TestDate2(  0,  0,  0, 29,  2, 2000 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(    1000000000).AsUTC()), TestDate2( 40, 46,  1,  9,  9, 2001 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(    1044057600).AsUTC()), TestDate2(  0,  0,  0,  1,  2, 2003 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(    1044144000).AsUTC()), TestDate2(  0,  0,  0,  2,  2, 2003 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(    1046476800).AsUTC()), TestDate2(  0,  0,  0,  1,  3, 2003 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(    1064966400).AsUTC()), TestDate2(  0,  0,  0,  1, 10, 2003 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(    1077926399).AsUTC()), TestDate2( 59, 59, 23, 27,  2, 2004 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(    1077926400).AsUTC()), TestDate2(  0,  0,  0, 28,  2, 2004 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(    1077926410).AsUTC()), TestDate2( 10,  0,  0, 28,  2, 2004 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(    1078012799).AsUTC()), TestDate2( 59, 59, 23, 28,  2, 2004 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(    1078012800).AsUTC()), TestDate2(  0,  0,  0, 29,  2, 2004 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(    1078012820).AsUTC()), TestDate2( 20,  0,  0, 29,  2, 2004 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(    1078099199).AsUTC()), TestDate2( 59, 59, 23, 29,  2, 2004 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(    1078099200).AsUTC()), TestDate2(  0,  0,  0,  1,  3, 2004 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(    1078099230).AsUTC()), TestDate2( 30,  0,  0,  1,  3, 2004 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(    1078185599).AsUTC()), TestDate2( 59, 59, 23,  1,  3, 2004 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(    1096588800).AsUTC()), TestDate2(  0,  0,  0,  1, 10, 2004 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(    1413064016).AsUTC()), TestDate2( 56, 46, 21, 11, 10, 2014 ));
	VERIFY_EQUAL(Gregorian::FromTM(mpt::Date::Unix(    1413064100).AsUTC()), TestDate2( 20, 48, 21, 11, 10, 2014 ));


#ifdef MODPLUG_TRACKER

	// URI & HTTP

	{
		URI uri = ParseURI(U_("scheme://username:password@host:port/path?query#fragment"));
		VERIFY_EQUAL(uri.scheme, U_("scheme"));
		VERIFY_EQUAL(uri.username, U_("username"));
		VERIFY_EQUAL(uri.password, U_("password"));
		VERIFY_EQUAL(uri.host, U_("host"));
		VERIFY_EQUAL(uri.port, U_("port"));
		VERIFY_EQUAL(uri.path, U_("/path"));
		VERIFY_EQUAL(uri.query, U_("query"));
		VERIFY_EQUAL(uri.fragment, U_("fragment"));
	}
	{
		URI uri = ParseURI(U_("scheme://host/path"));
		VERIFY_EQUAL(uri.scheme, U_("scheme"));
		VERIFY_EQUAL(uri.username, U_(""));
		VERIFY_EQUAL(uri.password, U_(""));
		VERIFY_EQUAL(uri.host, U_("host"));
		VERIFY_EQUAL(uri.port, U_(""));
		VERIFY_EQUAL(uri.path, U_("/path"));
		VERIFY_EQUAL(uri.query, U_(""));
		VERIFY_EQUAL(uri.fragment, U_(""));
	}
	{
		URI uri = ParseURI(U_("scheme://host"));
		VERIFY_EQUAL(uri.scheme, U_("scheme"));
		VERIFY_EQUAL(uri.username, U_(""));
		VERIFY_EQUAL(uri.password, U_(""));
		VERIFY_EQUAL(uri.host, U_("host"));
		VERIFY_EQUAL(uri.port, U_(""));
		VERIFY_EQUAL(uri.path, U_(""));
		VERIFY_EQUAL(uri.query, U_(""));
		VERIFY_EQUAL(uri.fragment, U_(""));
	}
	{
		URI uri = ParseURI(U_("scheme://host?query"));
		VERIFY_EQUAL(uri.scheme, U_("scheme"));
		VERIFY_EQUAL(uri.username, U_(""));
		VERIFY_EQUAL(uri.password, U_(""));
		VERIFY_EQUAL(uri.host, U_("host"));
		VERIFY_EQUAL(uri.port, U_(""));
		VERIFY_EQUAL(uri.path, U_(""));
		VERIFY_EQUAL(uri.query, U_("query"));
		VERIFY_EQUAL(uri.fragment, U_(""));
	}
	{
		URI uri = ParseURI(U_("scheme://host#fragment"));
		VERIFY_EQUAL(uri.scheme, U_("scheme"));
		VERIFY_EQUAL(uri.username, U_(""));
		VERIFY_EQUAL(uri.password, U_(""));
		VERIFY_EQUAL(uri.host, U_("host"));
		VERIFY_EQUAL(uri.port, U_(""));
		VERIFY_EQUAL(uri.path, U_(""));
		VERIFY_EQUAL(uri.query, U_(""));
		VERIFY_EQUAL(uri.fragment, U_("fragment"));
	}
	{
		URI uri = ParseURI(U_("scheme://host?#"));
		VERIFY_EQUAL(uri.scheme, U_("scheme"));
		VERIFY_EQUAL(uri.username, U_(""));
		VERIFY_EQUAL(uri.password, U_(""));
		VERIFY_EQUAL(uri.host, U_("host"));
		VERIFY_EQUAL(uri.port, U_(""));
		VERIFY_EQUAL(uri.path, U_(""));
		VERIFY_EQUAL(uri.query, U_(""));
		VERIFY_EQUAL(uri.fragment, U_(""));
	}
	{
		URI uri = ParseURI(U_("scheme://host#?"));
		VERIFY_EQUAL(uri.scheme, U_("scheme"));
		VERIFY_EQUAL(uri.username, U_(""));
		VERIFY_EQUAL(uri.password, U_(""));
		VERIFY_EQUAL(uri.host, U_("host"));
		VERIFY_EQUAL(uri.port, U_(""));
		VERIFY_EQUAL(uri.path, U_(""));
		VERIFY_EQUAL(uri.query, U_(""));
		VERIFY_EQUAL(uri.fragment, U_("?"));
	}
	{
		URI uri = ParseURI(U_("scheme://username:password@[2001:db8::1]:port/path?query#fragment"));
		VERIFY_EQUAL(uri.scheme, U_("scheme"));
		VERIFY_EQUAL(uri.username, U_("username"));
		VERIFY_EQUAL(uri.password, U_("password"));
		VERIFY_EQUAL(uri.host, U_("[2001:db8::1]"));
		VERIFY_EQUAL(uri.port, U_("port"));
		VERIFY_EQUAL(uri.path, U_("/path"));
		VERIFY_EQUAL(uri.query, U_("query"));
		VERIFY_EQUAL(uri.fragment, U_("fragment"));
	}

	{
		URI uri = ParseURI(U_("https://john.doe@www.example.com:123/forum/questions/?tag=networking&order=newest#top"));
		VERIFY_EQUAL(uri.scheme, U_("https"));
		VERIFY_EQUAL(uri.username, U_("john.doe"));
		VERIFY_EQUAL(uri.password, U_(""));
		VERIFY_EQUAL(uri.host, U_("www.example.com"));
		VERIFY_EQUAL(uri.port, U_("123"));
		VERIFY_EQUAL(uri.path, U_("/forum/questions/"));
		VERIFY_EQUAL(uri.query, U_("tag=networking&order=newest"));
		VERIFY_EQUAL(uri.fragment, U_("top"));
	}
	{
		URI uri = ParseURI(U_("ldap://[2001:db8::7]/c=GB?objectClass?one"));
		VERIFY_EQUAL(uri.scheme, U_("ldap"));
		VERIFY_EQUAL(uri.username, U_(""));
		VERIFY_EQUAL(uri.password, U_(""));
		VERIFY_EQUAL(uri.host, U_("[2001:db8::7]"));
		VERIFY_EQUAL(uri.port, U_(""));
		VERIFY_EQUAL(uri.path, U_("/c=GB"));
		VERIFY_EQUAL(uri.query, U_("objectClass?one"));
		VERIFY_EQUAL(uri.fragment, U_(""));
	}
	{
		URI uri = ParseURI(U_("mailto:John.Doe@example.com"));
		VERIFY_EQUAL(uri.scheme, U_("mailto"));
		VERIFY_EQUAL(uri.username, U_(""));
		VERIFY_EQUAL(uri.password, U_(""));
		VERIFY_EQUAL(uri.host, U_(""));
		VERIFY_EQUAL(uri.port, U_(""));
		VERIFY_EQUAL(uri.path, U_("John.Doe@example.com"));
		VERIFY_EQUAL(uri.query, U_(""));
		VERIFY_EQUAL(uri.fragment, U_(""));
	}
	{
		URI uri = ParseURI(U_("news:comp.infosystems.www.servers.unix"));
		VERIFY_EQUAL(uri.scheme, U_("news"));
		VERIFY_EQUAL(uri.username, U_(""));
		VERIFY_EQUAL(uri.password, U_(""));
		VERIFY_EQUAL(uri.host, U_(""));
		VERIFY_EQUAL(uri.port, U_(""));
		VERIFY_EQUAL(uri.path, U_("comp.infosystems.www.servers.unix"));
		VERIFY_EQUAL(uri.query, U_(""));
		VERIFY_EQUAL(uri.fragment, U_(""));
	}
	{
		URI uri = ParseURI(U_("tel:+1-816-555-1212"));
		VERIFY_EQUAL(uri.scheme, U_("tel"));
		VERIFY_EQUAL(uri.username, U_(""));
		VERIFY_EQUAL(uri.password, U_(""));
		VERIFY_EQUAL(uri.host, U_(""));
		VERIFY_EQUAL(uri.port, U_(""));
		VERIFY_EQUAL(uri.path, U_("+1-816-555-1212"));
		VERIFY_EQUAL(uri.query, U_(""));
		VERIFY_EQUAL(uri.fragment, U_(""));
	}
	{
		URI uri = ParseURI(U_("telnet://192.0.2.16:80/"));
		VERIFY_EQUAL(uri.scheme, U_("telnet"));
		VERIFY_EQUAL(uri.username, U_(""));
		VERIFY_EQUAL(uri.password, U_(""));
		VERIFY_EQUAL(uri.host, U_("192.0.2.16"));
		VERIFY_EQUAL(uri.port, U_("80"));
		VERIFY_EQUAL(uri.path, U_("/"));
		VERIFY_EQUAL(uri.query, U_(""));
		VERIFY_EQUAL(uri.fragment, U_(""));
	}
	{
		URI uri = ParseURI(U_("urn:oasis:names:specification:docbook:dtd:xml:4.1.2"));
		VERIFY_EQUAL(uri.scheme, U_("urn"));
		VERIFY_EQUAL(uri.username, U_(""));
		VERIFY_EQUAL(uri.password, U_(""));
		VERIFY_EQUAL(uri.host, U_(""));
		VERIFY_EQUAL(uri.port, U_(""));
		VERIFY_EQUAL(uri.path, U_("oasis:names:specification:docbook:dtd:xml:4.1.2"));
		VERIFY_EQUAL(uri.query, U_(""));
		VERIFY_EQUAL(uri.fragment, U_(""));
	}

	{
		HTTP::Request req;
		req.SetURI(ParseURI(U_("https://host/path?a1=a&a2=b")));
		VERIFY_EQUAL(req.protocol, HTTP::Protocol::HTTPS);
		VERIFY_EQUAL(req.host, U_("host"));
		VERIFY_EQUAL(req.path, U_("/path"));
		VERIFY_EQUAL(req.query.size(), 2u);
		if(req.query.size() == 2)
		{
			VERIFY_EQUAL(req.query[0], std::make_pair(U_("a1"), U_("a")));
			VERIFY_EQUAL(req.query[1], std::make_pair(U_("a2"), U_("b")));
		}
	}
	{
		HTTP::Request req;
		req.SetURI(ParseURI(U_("https://host/")));
		VERIFY_EQUAL(req.protocol, HTTP::Protocol::HTTPS);
		VERIFY_EQUAL(req.host, U_("host"));
		VERIFY_EQUAL(req.path, U_("/"));
	}

#endif // MODPLUG_TRACKER

	// https://github.com/kripken/emscripten/issues/4251
	#if MPT_OS_EMSCRIPTEN
		volatile int transpose = 32;
		volatile int finetune = 0;
		float exp = (transpose * 128.0f + finetune) * (1.0f / (12.0f * 128.0f)); 
		float f  = ::powf(2.0f,         exp);
		double d = ::pow (2.0 , (double)exp);
		VERIFY_EQUAL_EPS(d, 6.349605, 0.00001);
		VERIFY_EQUAL_EPS(f, 6.349605, 0.00001);
	#endif

}


static MPT_NOINLINE void TestRandom()
{

	#ifdef FLAKY_TESTS

		mpt::default_prng& prng = *s_PRNG;

		{
			std::vector<std::size_t> hist(256);
			for(std::size_t i = 0; i < 256*256; ++i)
			{
				uint8 value = mpt::random<uint8>(prng);
				hist[value] += 1;
			}
			for(std::size_t i = 0; i < 256; ++i)
			{
				VERIFY_EQUAL_QUIET_NONCONT(mpt::is_in_range(hist[i], 16u, 65520u), true);
			}
		}
		{
			std::vector<std::size_t> hist(256);
			for(std::size_t i = 0; i < 256*256; ++i)
			{
				int8 value = mpt::random<int8>(prng);
				hist[static_cast<int>(value) + 0x80] += 1;
			}
			for(std::size_t i = 0; i < 256; ++i)
			{
				VERIFY_EQUAL_QUIET_NONCONT(mpt::is_in_range(hist[i], 16u, 65520u), true);
			}
		}
		{
			std::vector<std::size_t> hist(256);
			for(std::size_t i = 0; i < 256*256; ++i)
			{
				uint8 value = mpt::random<uint8>(prng, 1);
				hist[value] += 1;
			}
			for(std::size_t i = 0; i < 256; ++i)
			{
				if(i < 2)
				{
					VERIFY_EQUAL_QUIET_NONCONT(mpt::is_in_range(hist[i], 16u, 65520u), true);
				} else
				{
					VERIFY_EQUAL_QUIET_NONCONT(hist[i], 0u);
				}
			}
		}
	#endif
}


static MPT_NOINLINE void TestCharsets()
{

	// Path splitting

#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS

	VERIFY_EQUAL(P_("").GetDrive(), P_(""));
	VERIFY_EQUAL(P_("").GetDir(), P_(""));
	VERIFY_EQUAL(P_("").GetPath(), P_(""));
	VERIFY_EQUAL(P_("").GetFileName(), P_(""));
	VERIFY_EQUAL(P_("").GetFileExt(), P_(""));
	VERIFY_EQUAL(P_("").GetFullFileName(), P_(""));

	VERIFY_EQUAL(P_("C:\\").GetDrive(), P_("C:"));
	VERIFY_EQUAL(P_("C:\\").GetDir(), P_("\\"));
	VERIFY_EQUAL(P_("C:\\").GetPath(), P_("C:\\"));
	VERIFY_EQUAL(P_("C:\\").GetFileName(), P_(""));
	VERIFY_EQUAL(P_("C:\\").GetFileExt(), P_(""));
	VERIFY_EQUAL(P_("C:\\").GetFullFileName(), P_(""));

	VERIFY_EQUAL(P_("\\directory\\").GetDrive(), P_(""));
	VERIFY_EQUAL(P_("\\directory\\").GetDir(), P_("\\directory\\"));
	VERIFY_EQUAL(P_("\\directory\\").GetPath(), P_("\\directory\\"));
	VERIFY_EQUAL(P_("\\directory\\").GetFileName(), P_(""));
	VERIFY_EQUAL(P_("\\directory\\").GetFileExt(), P_(""));
	VERIFY_EQUAL(P_("\\directory\\").GetFullFileName(), P_(""));

	VERIFY_EQUAL(P_("\\directory\\file.txt").GetDrive(), P_(""));
	VERIFY_EQUAL(P_("\\directory\\file.txt").GetDir(), P_("\\directory\\"));
	VERIFY_EQUAL(P_("\\directory\\file.txt").GetPath(), P_("\\directory\\"));
	VERIFY_EQUAL(P_("\\directory\\file.txt").GetFileName(), P_("file"));
	VERIFY_EQUAL(P_("\\directory\\file.txt").GetFileExt(), P_(".txt"));
	VERIFY_EQUAL(P_("\\directory\\file.txt").GetFullFileName(), P_("file.txt"));

	VERIFY_EQUAL(P_(".").GetDrive(), P_(""));
	VERIFY_EQUAL(P_(".").GetDir(), P_(""));
	VERIFY_EQUAL(P_(".").GetPath(), P_(""));
	VERIFY_EQUAL(P_(".").GetFileName(), P_("."));
	VERIFY_EQUAL(P_(".").GetFileExt(), P_(""));
	VERIFY_EQUAL(P_(".").GetFullFileName(), P_("."));

	VERIFY_EQUAL(P_("..").GetDrive(), P_(""));
	VERIFY_EQUAL(P_("..").GetDir(), P_(""));
	VERIFY_EQUAL(P_("..").GetPath(), P_(""));
	VERIFY_EQUAL(P_("..").GetFileName(), P_(".."));
	VERIFY_EQUAL(P_("..").GetFileExt(), P_(""));
	VERIFY_EQUAL(P_("..").GetFullFileName(), P_(".."));

	VERIFY_EQUAL(P_("dir\\.").GetDrive(), P_(""));
	VERIFY_EQUAL(P_("dir\\.").GetDir(), P_("dir\\"));
	VERIFY_EQUAL(P_("dir\\.").GetPath(), P_("dir\\"));
	VERIFY_EQUAL(P_("dir\\.").GetFileName(), P_("."));
	VERIFY_EQUAL(P_("dir\\.").GetFileExt(), P_(""));
	VERIFY_EQUAL(P_("dir\\.").GetFullFileName(), P_("."));

	VERIFY_EQUAL(P_("dir\\..").GetDrive(), P_(""));
	VERIFY_EQUAL(P_("dir\\..").GetDir(), P_("dir\\"));
	VERIFY_EQUAL(P_("dir\\..").GetPath(), P_("dir\\"));
	VERIFY_EQUAL(P_("dir\\..").GetFileName(), P_(".."));
	VERIFY_EQUAL(P_("dir\\..").GetFileExt(), P_(""));
	VERIFY_EQUAL(P_("dir\\..").GetFullFileName(), P_(".."));

	VERIFY_EQUAL(P_(".txt").GetDrive(), P_(""));
	VERIFY_EQUAL(P_(".txt").GetDir(), P_(""));
	VERIFY_EQUAL(P_(".txt").GetPath(), P_(""));
	VERIFY_EQUAL(P_(".txt").GetFileName(), P_(".txt"));
	VERIFY_EQUAL(P_(".txt").GetFileExt(), P_(""));
	VERIFY_EQUAL(P_(".txt").GetFullFileName(), P_(".txt"));

	VERIFY_EQUAL(P_("C:tmp.txt").GetDrive(), P_("C:"));
	VERIFY_EQUAL(P_("C:tmp.txt").GetDir(), P_(""));
	VERIFY_EQUAL(P_("C:tmp.txt").GetPath(), P_("C:"));
	VERIFY_EQUAL(P_("C:tmp.txt").GetFileName(), P_("tmp"));
	VERIFY_EQUAL(P_("C:tmp.txt").GetFileExt(), P_(".txt"));
	VERIFY_EQUAL(P_("C:tmp.txt").GetFullFileName(), P_("tmp.txt"));

	VERIFY_EQUAL(P_("C:tempdir\\tmp.txt").GetDrive(), P_("C:"));
	VERIFY_EQUAL(P_("C:tempdir\\tmp.txt").GetDir(), P_("tempdir\\"));
	VERIFY_EQUAL(P_("C:tempdir\\tmp.txt").GetPath(), P_("C:tempdir\\"));
	VERIFY_EQUAL(P_("C:tempdir\\tmp.txt").GetFileName(), P_("tmp"));
	VERIFY_EQUAL(P_("C:tempdir\\tmp.txt").GetFileExt(), P_(".txt"));
	VERIFY_EQUAL(P_("C:tempdir\\tmp.txt").GetFullFileName(), P_("tmp.txt"));

	VERIFY_EQUAL(P_("C:\\tempdir\\tmp.txt").GetDrive(), P_("C:"));
	VERIFY_EQUAL(P_("C:\\tempdir\\tmp.txt").GetDir(), P_("\\tempdir\\"));
	VERIFY_EQUAL(P_("C:\\tempdir\\tmp.txt").GetPath(), P_("C:\\tempdir\\"));
	VERIFY_EQUAL(P_("C:\\tempdir\\tmp.txt").GetFileName(), P_("tmp"));
	VERIFY_EQUAL(P_("C:\\tempdir\\tmp.txt").GetFileExt(), P_(".txt"));
	VERIFY_EQUAL(P_("C:\\tempdir\\tmp.txt").GetFullFileName(), P_("tmp.txt"));

	VERIFY_EQUAL(P_("C:\\tempdir\\tmp.foo.txt").GetFileName(), P_("tmp.foo"));
	VERIFY_EQUAL(P_("C:\\tempdir\\tmp.foo.txt").GetFileExt(), P_(".txt"));

	VERIFY_EQUAL(P_("\\\\server").GetDrive(), P_("\\\\server"));
	VERIFY_EQUAL(P_("\\\\server").GetDir(), P_(""));
	VERIFY_EQUAL(P_("\\\\server").GetPath(), P_("\\\\server"));
	VERIFY_EQUAL(P_("\\\\server").GetFileName(), P_(""));
	VERIFY_EQUAL(P_("\\\\server").GetFileExt(), P_(""));
	VERIFY_EQUAL(P_("\\\\server").GetFullFileName(), P_(""));

	VERIFY_EQUAL(P_("\\\\server\\").GetDrive(), P_("\\\\server\\"));
	VERIFY_EQUAL(P_("\\\\server\\").GetDir(), P_(""));
	VERIFY_EQUAL(P_("\\\\server\\").GetPath(), P_("\\\\server\\"));
	VERIFY_EQUAL(P_("\\\\server\\").GetFileName(), P_(""));
	VERIFY_EQUAL(P_("\\\\server\\").GetFileExt(), P_(""));
	VERIFY_EQUAL(P_("\\\\server\\").GetFullFileName(), P_(""));

	VERIFY_EQUAL(P_("\\\\server\\share").GetDrive(), P_("\\\\server\\share"));
	VERIFY_EQUAL(P_("\\\\server\\share").GetDir(), P_(""));
	VERIFY_EQUAL(P_("\\\\server\\share").GetPath(), P_("\\\\server\\share"));
	VERIFY_EQUAL(P_("\\\\server\\share").GetFileName(), P_(""));
	VERIFY_EQUAL(P_("\\\\server\\share").GetFileExt(), P_(""));
	VERIFY_EQUAL(P_("\\\\server\\share").GetFullFileName(), P_(""));

	VERIFY_EQUAL(P_("\\\\server\\share\\").GetDrive(), P_("\\\\server\\share"));
	VERIFY_EQUAL(P_("\\\\server\\share\\").GetDir(), P_("\\"));
	VERIFY_EQUAL(P_("\\\\server\\share\\").GetPath(), P_("\\\\server\\share\\"));
	VERIFY_EQUAL(P_("\\\\server\\share\\").GetFileName(), P_(""));
	VERIFY_EQUAL(P_("\\\\server\\share\\").GetFileExt(), P_(""));
	VERIFY_EQUAL(P_("\\\\server\\share\\").GetFullFileName(), P_(""));

	VERIFY_EQUAL(P_("\\\\server\\share\\dir1\\dir2\\name.foo.ext").GetDrive(), P_("\\\\server\\share"));
	VERIFY_EQUAL(P_("\\\\server\\share\\dir1\\dir2\\name.foo.ext").GetDir(), P_("\\dir1\\dir2\\"));
	VERIFY_EQUAL(P_("\\\\server\\share\\dir1\\dir2\\name.foo.ext").GetPath(), P_("\\\\server\\share\\dir1\\dir2\\"));
	VERIFY_EQUAL(P_("\\\\server\\share\\dir1\\dir2\\name.foo.ext").GetFileName(), P_("name.foo"));
	VERIFY_EQUAL(P_("\\\\server\\share\\dir1\\dir2\\name.foo.ext").GetFileExt(), P_(".ext"));
	VERIFY_EQUAL(P_("\\\\server\\share\\dir1\\dir2\\name.foo.ext").GetFullFileName(), P_("name.foo.ext"));

	VERIFY_EQUAL(P_("\\\\?\\C:\\tempdir\\dir.2\\tmp.foo.txt").GetDrive(), P_("C:"));
	VERIFY_EQUAL(P_("\\\\?\\C:\\tempdir\\dir.2\\tmp.foo.txt").GetDir(), P_("\\tempdir\\dir.2\\"));
	VERIFY_EQUAL(P_("\\\\?\\C:\\tempdir\\dir.2\\tmp.foo.txt").GetPath(), P_("C:\\tempdir\\dir.2\\"));
	VERIFY_EQUAL(P_("\\\\?\\C:\\tempdir\\dir.2\\tmp.foo.txt").GetFileName(), P_("tmp.foo"));
	VERIFY_EQUAL(P_("\\\\?\\C:\\tempdir\\dir.2\\tmp.foo.txt").GetFileExt(), P_(".txt"));
	VERIFY_EQUAL(P_("\\\\?\\C:\\tempdir\\dir.2\\tmp.foo.txt").GetFullFileName(), P_("tmp.foo.txt"));
	
	VERIFY_EQUAL(P_("\\\\?\\UNC\\server\\share\\dir1\\dir2\\name.foo.ext").GetDrive(), P_("\\\\server\\share"));
	VERIFY_EQUAL(P_("\\\\?\\UNC\\server\\share\\dir1\\dir2\\name.foo.ext").GetDir(), P_("\\dir1\\dir2\\"));
	VERIFY_EQUAL(P_("\\\\?\\UNC\\server\\share\\dir1\\dir2\\name.foo.ext").GetPath(), P_("\\\\server\\share\\dir1\\dir2\\"));
	VERIFY_EQUAL(P_("\\\\?\\UNC\\server\\share\\dir1\\dir2\\name.foo.ext").GetFileName(), P_("name.foo"));
	VERIFY_EQUAL(P_("\\\\?\\UNC\\server\\share\\dir1\\dir2\\name.foo.ext").GetFileExt(), P_(".ext"));
	VERIFY_EQUAL(P_("\\\\?\\UNC\\server\\share\\dir1\\dir2\\name.foo.ext").GetFullFileName(), P_("name.foo.ext"));
#endif



	// Path conversions
#ifdef MODPLUG_TRACKER
	const mpt::PathString exePath = P_("C:\\OpenMPT\\");
	VERIFY_EQUAL(P_("C:\\OpenMPT\\").AbsolutePathToRelative(exePath), P_(".\\"));
	VERIFY_EQUAL(P_("c:\\OpenMPT\\foo").AbsolutePathToRelative(exePath), P_(".\\foo"));
	VERIFY_EQUAL(P_("C:\\foo").AbsolutePathToRelative(exePath), P_("\\foo"));
	VERIFY_EQUAL(P_(".\\").RelativePathToAbsolute(exePath), P_("C:\\OpenMPT\\"));
	VERIFY_EQUAL(P_(".\\foo").RelativePathToAbsolute(exePath), P_("C:\\OpenMPT\\foo"));
	VERIFY_EQUAL(P_("\\foo").RelativePathToAbsolute(exePath), P_("C:\\foo"));
	VERIFY_EQUAL(P_("\\\\server\\path\\file").AbsolutePathToRelative(exePath), P_("\\\\server\\path\\file"));
	VERIFY_EQUAL(P_("\\\\server\\path\\file").RelativePathToAbsolute(exePath), P_("\\\\server\\path\\file"));

	VERIFY_EQUAL(P_("").Simplify(), P_(""));
	VERIFY_EQUAL(P_(" ").Simplify(), P_(" "));
	VERIFY_EQUAL(P_("foo\\bar").Simplify(), P_("foo\\bar"));
	VERIFY_EQUAL(P_(".\\foo\\bar").Simplify(), P_(".\\foo\\bar"));
	VERIFY_EQUAL(P_(".\\\\foo\\bar").Simplify(), P_(".\\foo\\bar"));
	VERIFY_EQUAL(P_("./\\foo\\bar").Simplify(), P_(".\\foo\\bar"));
	VERIFY_EQUAL(P_("\\foo\\bar").Simplify(), P_("\\foo\\bar"));
	VERIFY_EQUAL(P_("A:\\name_1\\.\\name_2\\..\\name_3\\").Simplify(), P_("A:\\name_1\\name_3"));
	VERIFY_EQUAL(P_("A:\\name_1\\..\\name_2\\./name_3").Simplify(), P_("A:\\name_2\\name_3"));
	VERIFY_EQUAL(P_("A:\\name_1\\.\\name_2\\.\\name_3\\..\\name_4\\..").Simplify(), P_("A:\\name_1\\name_2"));
	VERIFY_EQUAL(P_("A:foo\\\\bar").Simplify(), P_("A:\\foo\\bar"));
	VERIFY_EQUAL(P_("C:\\..").Simplify(), P_("C:\\"));
	VERIFY_EQUAL(P_("C:\\.").Simplify(), P_("C:\\"));
	VERIFY_EQUAL(P_("\\\\foo\\..\\.bar").Simplify(), P_("\\\\.bar"));
	VERIFY_EQUAL(P_("\\\\foo\\..\\..\\bar").Simplify(), P_("\\\\bar"));
#endif



#ifdef MODPLUG_TRACKER
#if MPT_COMPILER_MSVC

	// Tracker code currently relies on BROKEN MSVC behaviour with respect to %s
	// handling in printf functions when the target is a wide string.
	// Ensure that Microsoft KEEPS this BROKEN behavour (it conflicts with C99 and
	// C++11 demanded semantics.
	// Additionally, the tracker code makes use of %hs and %ls which are the only
	// way to unconditionally expect narrow or wide string parameters
	// respectively; %ls is standard conforming, %hs is a Microsoft extension.
	// We check all possible combinations and expect Microsoft-style behaviour
	// here.

	char src_char[256];
	wchar_t src_wchar_t[256];
	TCHAR src_TCHAR[256];

	char dst_char[256];
	wchar_t dst_wchar_t[256];
	TCHAR dst_TCHAR[256];

	MemsetZero(src_char);
	MemsetZero(src_wchar_t);
	MemsetZero(src_TCHAR);

	strcpy(src_char, "ab");
	wcscpy(src_wchar_t, L"ab");
	_tcscpy(src_TCHAR, _T("ab"));

#define MPT_TEST_PRINTF(dst_type, function, format, src_type) \
	do { \
		MemsetZero(dst_ ## dst_type); \
		function(dst_ ## dst_type, format, src_ ## src_type); \
		VERIFY_EQUAL(std::char_traits< dst_type >::compare(dst_ ## dst_type, src_ ## dst_type, std::char_traits< dst_type >::length( src_ ## dst_type ) + 1), 0); \
	} while(0) \
/**/

#define MPT_TEST_PRINTF_N(dst_type, function, format, src_type) \
	do { \
		MemsetZero(dst_ ## dst_type); \
		function(dst_ ## dst_type, 255, format, src_ ## src_type); \
		VERIFY_EQUAL(std::char_traits< dst_type >::compare(dst_ ## dst_type, src_ ## dst_type, std::char_traits< dst_type >::length( src_ ## dst_type ) + 1), 0); \
	} while(0) \
/**/

	// CRT narrow
	MPT_TEST_PRINTF(char, sprintf, "%s", char);
	MPT_TEST_PRINTF(char, sprintf, "%S", wchar_t);
	MPT_TEST_PRINTF(char, sprintf, "%hs", char);
	MPT_TEST_PRINTF(char, sprintf, "%hS", char);
	MPT_TEST_PRINTF(char, sprintf, "%ls", wchar_t);
	MPT_TEST_PRINTF(char, sprintf, "%lS", wchar_t);
	MPT_TEST_PRINTF(char, sprintf, "%ws", wchar_t);
	MPT_TEST_PRINTF(char, sprintf, "%wS", wchar_t);

	// CRT wide
	MPT_TEST_PRINTF_N(wchar_t, swprintf, L"%s", wchar_t);
	MPT_TEST_PRINTF_N(wchar_t, swprintf, L"%S", char);
	MPT_TEST_PRINTF_N(wchar_t, swprintf, L"%hs", char);
	MPT_TEST_PRINTF_N(wchar_t, swprintf, L"%hS", char);
	MPT_TEST_PRINTF_N(wchar_t, swprintf, L"%ls", wchar_t);
	MPT_TEST_PRINTF_N(wchar_t, swprintf, L"%lS", wchar_t);
	MPT_TEST_PRINTF_N(wchar_t, swprintf, L"%ws", wchar_t);
	MPT_TEST_PRINTF_N(wchar_t, swprintf, L"%wS", wchar_t);

	// WinAPI TCHAR
	MPT_TEST_PRINTF(TCHAR, wsprintf, _T("%s"), TCHAR);
	MPT_TEST_PRINTF(TCHAR, wsprintf, _T("%hs"), char);
	MPT_TEST_PRINTF(TCHAR, wsprintf, _T("%hS"), char);
	MPT_TEST_PRINTF(TCHAR, wsprintf, _T("%ls"), wchar_t);
	MPT_TEST_PRINTF(TCHAR, wsprintf, _T("%lS"), wchar_t);

	// WinAPI CHAR
	MPT_TEST_PRINTF(char, wsprintfA, "%s", char);
	MPT_TEST_PRINTF(char, wsprintfA, "%S", wchar_t);
	MPT_TEST_PRINTF(char, wsprintfA, "%hs", char);
	MPT_TEST_PRINTF(char, wsprintfA, "%hS", char);
	MPT_TEST_PRINTF(char, wsprintfA, "%ls", wchar_t);
	MPT_TEST_PRINTF(char, wsprintfA, "%lS", wchar_t);

	// WinAPI WCHAR
	MPT_TEST_PRINTF(wchar_t, wsprintfW, L"%s", wchar_t);
	MPT_TEST_PRINTF(wchar_t, wsprintfW, L"%S", char);
	MPT_TEST_PRINTF(wchar_t, wsprintfW, L"%hs", char);
	MPT_TEST_PRINTF(wchar_t, wsprintfW, L"%hS", char);
	MPT_TEST_PRINTF(wchar_t, wsprintfW, L"%ls", wchar_t);
	MPT_TEST_PRINTF(wchar_t, wsprintfW, L"%lS", wchar_t);

#undef MPT_TEST_PRINTF
#undef MPT_TEST_PRINTF_n

#endif
#endif

	VERIFY_EQUAL(mpt::CompareNoCaseAscii("f", "f", 6) == 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("f", "F", 6) == 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("F", "f", 6) == 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("f", "g", 6) < 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("h", "g", 6) > 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("fgh", "FgH", 6) == 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("012345678", "012345678", 9) == 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("", "012345678", 9) < 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("FgH", "", 6) > 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("FgH", "F", 6) > 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("FgH", "Fg", 6) > 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("FgH", "fg", 6) > 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("0123456789", "FgH", 0) == 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("FgH", "fgh", 1) == 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("FgH", "fgh", 2) == 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("FgH", "fgh", 3) == 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("FgH", "fghi", 3) == 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("FgH", "fghi", 4) < 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("FIH", "fghi", 1) == 0, true);
	VERIFY_EQUAL(mpt::CompareNoCaseAscii("FIH", "fghi", 2) > 0, true);

}


#ifdef MODPLUG_TRACKER

struct CustomSettingsTestType
{
	float x;
	float y;
	CustomSettingsTestType(float x_ = 0.0f, float y_ = 0.0f) : x(x_), y(y_) { }
};

} // namespace Test

template <>
inline Test::CustomSettingsTestType FromSettingValue(const SettingValue &val)
{
	MPT_ASSERT(val.GetTypeTag() == "myType");
	mpt::ustring xy = val.as<mpt::ustring>();
	if(xy.empty())
	{
		return Test::CustomSettingsTestType(0.0f, 0.0f);
	}
	std::size_t pos = xy.find(U_("|"));
	mpt::ustring x = xy.substr(0, pos);
	mpt::ustring y = xy.substr(pos + 1);
	return Test::CustomSettingsTestType(ConvertStrTo<float>(x), ConvertStrTo<float>(y));
}

template <>
inline SettingValue ToSettingValue(const Test::CustomSettingsTestType &val)
{
	return SettingValue(mpt::ufmt::val(val.x) + U_("|") + mpt::ufmt::val(val.y), "myType");
}

namespace Test {

#endif // MODPLUG_TRACKER


static MPT_NOINLINE void TestSettings()
{

#ifdef MODPLUG_TRACKER

	VERIFY_EQUAL(SettingPath(U_("a"),U_("b")) < SettingPath(U_("a"),U_("c")), true);
	VERIFY_EQUAL(!(SettingPath(U_("c"),U_("b")) < SettingPath(U_("a"),U_("c"))), true);

	{
		DefaultSettingsContainer conf;

		int32 foobar = conf.Read(U_("Test"), U_("bar"), 23);
		conf.Write(U_("Test"), U_("bar"), 64);
		conf.Write(U_("Test"), U_("bar"), 42);
		conf.Read(U_("Test"), U_("baz"), 4711);
		foobar = conf.Read(U_("Test"), U_("bar"), 28);
	}

	{
		DefaultSettingsContainer conf;

		int32 foobar = conf.Read(U_("Test"), U_("bar"), 28);
		VERIFY_EQUAL(foobar, 42);
		conf.Write(U_("Test"), U_("bar"), 43);
	}

	{
		DefaultSettingsContainer conf;

		int32 foobar = conf.Read(U_("Test"), U_("bar"), 123);
		VERIFY_EQUAL(foobar, 43);
		conf.Write(U_("Test"), U_("bar"), 88);
	}

	{
		DefaultSettingsContainer conf;

		Setting<int> foo(conf, U_("Test"), U_("bar"), 99);

		VERIFY_EQUAL(foo, 88);

		foo = 7;

	}

	{
		DefaultSettingsContainer conf;
		Setting<int> foo(conf, U_("Test"), U_("bar"), 99);
		VERIFY_EQUAL(foo, 7);
	}


	{
		DefaultSettingsContainer conf;
		conf.Read(U_("Test"), U_("struct"), std::string(""));
		conf.Write(U_("Test"), U_("struct"), std::string(""));
	}

	{
		DefaultSettingsContainer conf;
		CustomSettingsTestType dummy = conf.Read(U_("Test"), U_("struct"), CustomSettingsTestType(1.0f, 1.0f));
		dummy = CustomSettingsTestType(0.125f, 32.0f);
		conf.Write(U_("Test"), U_("struct"), dummy);
	}

	{
		DefaultSettingsContainer conf;
		Setting<CustomSettingsTestType> dummyVar(conf, U_("Test"), U_("struct"), CustomSettingsTestType(1.0f, 1.0f));
		CustomSettingsTestType dummy = dummyVar;
		VERIFY_EQUAL(dummy.x, 0.125f);
		VERIFY_EQUAL(dummy.y, 32.0f);
	}

#endif // MODPLUG_TRACKER

}


// Test MIDI Event generating / reading
static MPT_NOINLINE void TestMIDIEvents()
{
	uint32 midiEvent;

	midiEvent = MIDIEvents::CC(MIDIEvents::MIDICC_Balance_Coarse, 13, 40);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evControllerChange);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 13);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), MIDIEvents::MIDICC_Balance_Coarse);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 40);

	midiEvent = MIDIEvents::NoteOn(10, 50, 120);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evNoteOn);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 10);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 50);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 120);

	midiEvent = MIDIEvents::NoteOff(15, 127, 42);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evNoteOff);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 15);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 127);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 42);

	midiEvent = MIDIEvents::ProgramChange(1, 127);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evProgramChange);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 1);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 127);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 0);

	midiEvent = MIDIEvents::PitchBend(2, MIDIEvents::pitchBendCentre);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evPitchBend);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 2);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 0x00);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 0x40);

	midiEvent = MIDIEvents::System(MIDIEvents::sysStart);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evSystem);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), MIDIEvents::sysStart);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 0);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 0);
}


// Check if our test file was loaded correctly.
static void TestLoadXMFile(const CSoundFile &sndFile)
{
#ifdef MODPLUG_TRACKER
	const CModDoc *pModDoc = sndFile.GetpModDoc();
	VERIFY_EQUAL_NONCONT(pModDoc->IsChannelUnused(0), true);
	VERIFY_EQUAL_NONCONT(pModDoc->IsChannelUnused(1), false);
#endif // MODPLUG_TRACKER

	// Global Variables
	VERIFY_EQUAL_NONCONT(sndFile.GetTitle(), "Test Module");
	VERIFY_EQUAL_NONCONT(sndFile.m_songMessage.substr(0, 32), "OpenMPT Module Loader Test Suite");
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultTempo, TEMPO(139, 0));
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultSpeed, 5);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultGlobalVolume, 128);
	VERIFY_EQUAL_NONCONT(sndFile.m_nVSTiVolume, 42);
	VERIFY_EQUAL_NONCONT(sndFile.m_nSamplePreAmp, 23);
	VERIFY_EQUAL_NONCONT((sndFile.m_SongFlags & SONG_FILE_FLAGS), SONG_LINEARSLIDES | SONG_EXFILTERRANGE);
	VERIFY_EQUAL_NONCONT(sndFile.m_playBehaviour[MSF_COMPATIBLE_PLAY], true);
	VERIFY_EQUAL_NONCONT(sndFile.m_playBehaviour[kMIDICCBugEmulation], false);
	VERIFY_EQUAL_NONCONT(sndFile.m_playBehaviour[kMPTOldSwingBehaviour], false);
	VERIFY_EQUAL_NONCONT(sndFile.m_playBehaviour[kOldMIDIPitchBends], false);
	VERIFY_EQUAL_NONCONT(sndFile.GetMixLevels(), MixLevels::Compatible);
	VERIFY_EQUAL_NONCONT(sndFile.m_nTempoMode, TempoMode::Modern);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultRowsPerBeat, 6);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultRowsPerMeasure, 12);
	VERIFY_EQUAL_NONCONT(sndFile.m_dwCreatedWithVersion, MPT_V("1.19.02.05"));
	VERIFY_EQUAL_NONCONT(sndFile.Order().GetRestartPos(), 1);

	// Macros
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetParameteredMacroType(0), kSFxReso);
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetParameteredMacroType(1), kSFxDryWet);
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetFixedMacroType(), kZxxResoFltMode);

	// Channels
	VERIFY_EQUAL_NONCONT(sndFile.GetNumChannels(), 2);
	VERIFY_EQUAL_NONCONT((sndFile.ChnSettings[0].szName == "First Channel"), true);
#ifndef NO_PLUGINS
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].nMixPlugin, 0);
#endif // NO_PLUGINS

	VERIFY_EQUAL_NONCONT((sndFile.ChnSettings[1].szName == "Second Channel"), true);
#ifndef NO_PLUGINS
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].nMixPlugin, 1);
#endif // NO_PLUGINS

	// Samples
	VERIFY_EQUAL_NONCONT(sndFile.GetNumSamples(), 3);
	VERIFY_EQUAL_NONCONT((sndFile.m_szNames[1] == "Pulse Sample"), true);
	VERIFY_EQUAL_NONCONT((sndFile.m_szNames[2] == "Empty Sample"), true);
	VERIFY_EQUAL_NONCONT((sndFile.m_szNames[3] == "Unassigned Sample"), true);
#ifdef MODPLUG_TRACKER
	VERIFY_EQUAL_NONCONT(pModDoc->FindSampleParent(1), 1);
	VERIFY_EQUAL_NONCONT(pModDoc->FindSampleParent(2), 1);
	VERIFY_EQUAL_NONCONT(pModDoc->FindSampleParent(3), INSTRUMENTINDEX_INVALID);
#endif // MODPLUG_TRACKER
	const ModSample &sample = sndFile.GetSample(1);
	VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 1);
	VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 1);
	VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 1);
	VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 16);
	VERIFY_EQUAL_NONCONT(sample.nFineTune, 35);
	VERIFY_EQUAL_NONCONT(sample.RelativeTone, 1);
	VERIFY_EQUAL_NONCONT(sample.nVolume, 32 * 4);
	VERIFY_EQUAL_NONCONT(sample.nGlobalVol, 64);
	VERIFY_EQUAL_NONCONT(sample.nPan, 160);
	VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_PANNING | CHN_LOOP | CHN_PINGPONGLOOP);

	VERIFY_EQUAL_NONCONT(sample.nLoopStart, 1);
	VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 8);

	VERIFY_EQUAL_NONCONT(sample.nVibType, VIB_SQUARE);
	VERIFY_EQUAL_NONCONT(sample.nVibSweep, 3);
	VERIFY_EQUAL_NONCONT(sample.nVibRate, 4);
	VERIFY_EQUAL_NONCONT(sample.nVibDepth, 5);

	// Sample Data
	for(size_t i = 0; i < 6; i++)
	{
		VERIFY_EQUAL_NONCONT(sample.sample8()[i], 18);
	}
	for(size_t i = 6; i < 16; i++)
	{
		VERIFY_EQUAL_NONCONT(sample.sample8()[i], 0);
	}

	// Instruments
	VERIFY_EQUAL_NONCONT(sndFile.GetNumInstruments(), 1);
	const ModInstrument *pIns = sndFile.Instruments[1];
	VERIFY_EQUAL_NONCONT(pIns->nFadeOut, 1024);
	VERIFY_EQUAL_NONCONT(pIns->nPan, 128);
	VERIFY_EQUAL_NONCONT(pIns->dwFlags, InstrumentFlags(0));

	VERIFY_EQUAL_NONCONT(pIns->nPPS, 0);
	VERIFY_EQUAL_NONCONT(pIns->nPPC, NOTE_MIDDLEC - 1);

	VERIFY_EQUAL_NONCONT(pIns->nVolRampUp, 1200);
	VERIFY_EQUAL_NONCONT(pIns->resampling, SRCMODE_SINC8LP);

	VERIFY_EQUAL_NONCONT(pIns->IsCutoffEnabled(), false);
	VERIFY_EQUAL_NONCONT(pIns->GetCutoff(), 0);
	VERIFY_EQUAL_NONCONT(pIns->IsResonanceEnabled(), false);
	VERIFY_EQUAL_NONCONT(pIns->GetResonance(), 0);
	VERIFY_EQUAL_NONCONT(pIns->filterMode, FilterMode::Unchanged);

	VERIFY_EQUAL_NONCONT(pIns->nVolSwing, 0);
	VERIFY_EQUAL_NONCONT(pIns->nPanSwing, 0);
	VERIFY_EQUAL_NONCONT(pIns->nCutSwing, 0);
	VERIFY_EQUAL_NONCONT(pIns->nResSwing, 0);

	VERIFY_EQUAL_NONCONT(pIns->nNNA, NewNoteAction::NoteCut);
	VERIFY_EQUAL_NONCONT(pIns->nDCT, DuplicateCheckType::None);

	VERIFY_EQUAL_NONCONT(pIns->nMixPlug, 1);
	VERIFY_EQUAL_NONCONT(pIns->nMidiChannel, 16);
	VERIFY_EQUAL_NONCONT(pIns->nMidiProgram, 64);
	VERIFY_EQUAL_NONCONT(pIns->wMidiBank, 2);
	VERIFY_EQUAL_NONCONT(pIns->midiPWD, 8);

	VERIFY_EQUAL_NONCONT(pIns->pTuning, sndFile.GetDefaultTuning());

	VERIFY_EQUAL_NONCONT(pIns->pitchToTempoLock, TEMPO(0, 0));

	VERIFY_EQUAL_NONCONT(pIns->pluginVelocityHandling, PLUGIN_VELOCITYHANDLING_VOLUME);
	VERIFY_EQUAL_NONCONT(pIns->pluginVolumeHandling, PLUGIN_VOLUMEHANDLING_MIDI);

	for(size_t i = sndFile.GetModSpecifications().noteMin; i < sndFile.GetModSpecifications().noteMax; i++)
	{
		VERIFY_EQUAL_NONCONT(pIns->Keyboard[i], (i == NOTE_MIDDLEC - 1) ? 2 : 1);
	}

	VERIFY_EQUAL_NONCONT(pIns->VolEnv.dwFlags, ENV_ENABLED | ENV_SUSTAIN);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.size(), 3);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.nReleaseNode, ENV_RELEASE_NODE_UNSET);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv[2].tick, 96);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv[2].value, 0);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.nSustainStart, 1);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.nSustainEnd, 1);

	VERIFY_EQUAL_NONCONT(pIns->PanEnv.dwFlags, ENV_LOOP);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.size(), 12);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.nLoopStart, 9);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.nLoopEnd, 11);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.nReleaseNode, ENV_RELEASE_NODE_UNSET);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv[9].tick, 46);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv[9].value, 23);

	VERIFY_EQUAL_NONCONT(pIns->PitchEnv.dwFlags, EnvelopeFlags(0));
	VERIFY_EQUAL_NONCONT(pIns->PitchEnv.size(), 0);

	// Sequences
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetNumSequences(), 1);
	VERIFY_EQUAL_NONCONT(sndFile.Order()[0], 0);
	VERIFY_EQUAL_NONCONT(sndFile.Order()[1], 1);

	// Patterns
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.GetNumPatterns(), 2);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetName(), "First Pattern");
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetNumRows(), 64);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetNumChannels(), 2);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetOverrideSignature(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetRowsPerBeat(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetRowsPerMeasure(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.IsPatternEmpty(0), true);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetName(), "Second Pattern");
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetNumRows(), 32);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetNumChannels(), 2);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetOverrideSignature(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetRowsPerBeat(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetRowsPerMeasure(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.IsPatternEmpty(1), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->IsPcNote(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->note, NOTE_NONE);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->instr, 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->volcmd, VOLCMD_VIBRATOSPEED);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->vol, 15);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 0)->IsEmpty(), true);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->IsEmpty(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->IsPcNote(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->note, NOTE_MIDDLEC + 12);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->instr, 45);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->volcmd, VOLCMD_VOLSLIDEDOWN);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->vol, 5);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->command, CMD_PANNING8);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->param, 0xFF);

	// Test 4-Bit Panning conversion
	for(int i = 0; i < 16; i++)
	{
		VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(10 + i, 0)->vol, i * 4);
	}

	// Plugins
#ifndef NO_PLUGINS
	const SNDMIXPLUGIN &plug = sndFile.m_MixPlugins[0];
	VERIFY_EQUAL_NONCONT(plug.GetName(), U_("First Plugin"));
	VERIFY_EQUAL_NONCONT(plug.fDryRatio, 0.26f);
	VERIFY_EQUAL_NONCONT(plug.IsMasterEffect(), true);
	VERIFY_EQUAL_NONCONT(plug.GetGain(), 11);
#endif // NO_PLUGINS
}


// Check if our test file was loaded correctly.
static void TestLoadMPTMFile(const CSoundFile &sndFile)
{

	// Global Variables
	VERIFY_EQUAL_NONCONT(sndFile.GetTitle(), "Test Module_____________X");
	VERIFY_EQUAL_NONCONT(sndFile.m_songMessage.substr(0, 32), "OpenMPT Module Loader Test Suite");
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultTempo, TEMPO(139, 999));
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultSpeed, 5);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultGlobalVolume, 128);
	VERIFY_EQUAL_NONCONT(sndFile.m_nVSTiVolume, 42);
	VERIFY_EQUAL_NONCONT(sndFile.m_nSamplePreAmp, 23);
	VERIFY_EQUAL_NONCONT((sndFile.m_SongFlags & SONG_FILE_FLAGS), SONG_LINEARSLIDES | SONG_EXFILTERRANGE | SONG_ITCOMPATGXX | SONG_ITOLDEFFECTS);
	VERIFY_EQUAL_NONCONT(sndFile.m_playBehaviour[MSF_COMPATIBLE_PLAY], true);
	VERIFY_EQUAL_NONCONT(sndFile.m_playBehaviour[kMIDICCBugEmulation], false);
	VERIFY_EQUAL_NONCONT(sndFile.m_playBehaviour[kMPTOldSwingBehaviour], false);
	VERIFY_EQUAL_NONCONT(sndFile.m_playBehaviour[kOldMIDIPitchBends], false);
	VERIFY_EQUAL_NONCONT(sndFile.GetMixLevels(), MixLevels::Compatible);
	VERIFY_EQUAL_NONCONT(sndFile.m_nTempoMode, TempoMode::Modern);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultRowsPerBeat, 6);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultRowsPerMeasure, 12);
	VERIFY_EQUAL_NONCONT(sndFile.m_dwCreatedWithVersion, MPT_V("1.19.02.05"));
	VERIFY_EQUAL_NONCONT(sndFile.m_nResampling, SRCMODE_SINC8LP);
	VERIFY_EQUAL_NONCONT(sndFile.m_songArtist, U_("Tester"));
	VERIFY_EQUAL_NONCONT(sndFile.m_tempoSwing.size(), 6);
	VERIFY_EQUAL_NONCONT(sndFile.m_tempoSwing[0], 29360125);
	VERIFY_EQUAL_NONCONT(sndFile.m_tempoSwing[1], 4194305);
	VERIFY_EQUAL_NONCONT(sndFile.m_tempoSwing[2], 29360128);
	VERIFY_EQUAL_NONCONT(sndFile.m_tempoSwing[3], 4194305);
	VERIFY_EQUAL_NONCONT(sndFile.m_tempoSwing[4], 29360128);
	VERIFY_EQUAL_NONCONT(sndFile.m_tempoSwing[5], 4194305);

	// Edit history
	VERIFY_EQUAL_NONCONT(sndFile.GetFileHistory().size() > 15, true);
	const FileHistory &fh = sndFile.GetFileHistory().front();
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_year, 111);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_mon, 5);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_mday, 14);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_hour, 21);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_min, 8);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_sec, 32);
	VERIFY_EQUAL_NONCONT((uint32)((double)fh.openTime / HISTORY_TIMER_PRECISION), 31);

	// Macros
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetParameteredMacroType(0), kSFxReso);
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetParameteredMacroType(1), kSFxDryWet);
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetParameteredMacroType(2), kSFxPolyAT);
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetFixedMacroType(), kZxxResoFltMode);

	// Channels
	VERIFY_EQUAL_NONCONT(sndFile.GetNumChannels(), 70);
	VERIFY_EQUAL_NONCONT((sndFile.ChnSettings[0].szName == "First Channel"), true);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].nPan, 32);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].nVolume, 32);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].dwFlags, CHN_MUTE);
#ifndef NO_PLUGINS
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].nMixPlugin, 0);
#endif // NO_PLUGINS

	VERIFY_EQUAL_NONCONT((sndFile.ChnSettings[1].szName == "Second Channel"), true);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].nPan, 128);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].nVolume, 16);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].dwFlags, CHN_SURROUND);
#ifndef NO_PLUGINS
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].nMixPlugin, 1);
#endif // NO_PLUGINS

	VERIFY_EQUAL_NONCONT((sndFile.ChnSettings[69].szName == "Last Channel______X"), true);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[69].nPan, 256);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[69].nVolume, 7);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[69].dwFlags, ChannelFlags(0));
#ifndef NO_PLUGINS
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[69].nMixPlugin, 1);
#endif // NO_PLUGINS
	// Samples
	VERIFY_EQUAL_NONCONT(sndFile.GetNumSamples(), 4);
	{
		const ModSample &sample = sndFile.GetSample(1);
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 16);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_MPT), 9001);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 32 * 4);
		VERIFY_EQUAL_NONCONT(sample.nGlobalVol, 16);
		VERIFY_EQUAL_NONCONT(sample.nPan, 160);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_PANNING | CHN_LOOP | CHN_SUSTAINLOOP | CHN_PINGPONGSUSTAIN);

		VERIFY_EQUAL_NONCONT(sample.nLoopStart, 1);
		VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 8);
		VERIFY_EQUAL_NONCONT(sample.nSustainStart, 1);
		VERIFY_EQUAL_NONCONT(sample.nSustainEnd, 7);

		VERIFY_EQUAL_NONCONT(sample.nVibType, VIB_SQUARE);
		VERIFY_EQUAL_NONCONT(sample.nVibSweep, 3);
		VERIFY_EQUAL_NONCONT(sample.nVibRate, 4);
		VERIFY_EQUAL_NONCONT(sample.nVibDepth, 5);

		// Sample Data
		for(size_t i = 0; i < 6; i++)
		{
			VERIFY_EQUAL_NONCONT(sample.sample8()[i], 18);
		}
		for(size_t i = 6; i < 16; i++)
		{
			VERIFY_EQUAL_NONCONT(sample.sample8()[i], 0);
		}

		VERIFY_EQUAL_NONCONT(sample.cues[0], 2);
		VERIFY_EQUAL_NONCONT(sample.cues[8], 9);
	}

	{
		const ModSample &sample = sndFile.GetSample(2);
		VERIFY_EQUAL_NONCONT((sndFile.m_szNames[2] == "Stereo / 16-Bit"), true);
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 4);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 2);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 2);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 16 * 4);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_MPT), 16000);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_16BIT | CHN_STEREO | CHN_LOOP);

		// Sample Data (Stereo Interleaved)
		for(size_t i = 0; i < 7; i++)
		{
			VERIFY_EQUAL_NONCONT(sample.sample16()[4 + i], int16(-32768));
		}

		VERIFY_EQUAL_NONCONT(sample.cues[0], 3);
		VERIFY_EQUAL_NONCONT(sample.cues[8], 14);
	}

	// External sample
	{
		const ModSample &sample = sndFile.GetSample(4);
		VERIFY_EQUAL_NONCONT((sndFile.m_szNames[4] == "Overridden Name"), true);
		VERIFY_EQUAL_NONCONT((sample.filename == "External"), true);
#ifdef MPT_EXTERNAL_SAMPLES
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 64);

		VERIFY_EQUAL_NONCONT(sample.nLoopStart, 42);
		VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 55);
		VERIFY_EQUAL_NONCONT(sample.nSustainStart, 42);
		VERIFY_EQUAL_NONCONT(sample.nSustainEnd, 55);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_LOOP | CHN_SUSTAINLOOP | CHN_PINGPONGSUSTAIN | SMP_KEEPONDISK);
#endif // MPT_EXTERNAL_SAMPLES
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_MPT), 10101);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 26 * 4);
		VERIFY_EQUAL_NONCONT(sample.nGlobalVol, 26);
		VERIFY_EQUAL_NONCONT(sample.nPan, 26 * 4);

		VERIFY_EQUAL_NONCONT(sample.nVibType, VIB_SINE);
		VERIFY_EQUAL_NONCONT(sample.nVibSweep, 37);
		VERIFY_EQUAL_NONCONT(sample.nVibRate, 42);
		VERIFY_EQUAL_NONCONT(sample.nVibDepth, 23);

		// Sample Data
#ifdef MPT_EXTERNAL_SAMPLES
		for(size_t i = 0; i < 16; i++)
		{
			VERIFY_EQUAL_NONCONT(sample.sample8()[i], int8(45));
		}
#endif // MPT_EXTERNAL_SAMPLES

		VERIFY_EQUAL_NONCONT(sample.cues[0], 10);
		VERIFY_EQUAL_NONCONT(sample.cues[8], 50);
	}

	// Instruments
	VERIFY_EQUAL_NONCONT(sndFile.GetNumInstruments(), 2);
	for(INSTRUMENTINDEX ins = 1; ins <= 2; ins++)
	{
		const ModInstrument *pIns = sndFile.Instruments[ins];
		VERIFY_EQUAL_NONCONT(pIns->nGlobalVol, 32);
		VERIFY_EQUAL_NONCONT(pIns->nFadeOut, 1024);
		VERIFY_EQUAL_NONCONT(pIns->nPan, 64);
		VERIFY_EQUAL_NONCONT(pIns->dwFlags, INS_SETPANNING);

		VERIFY_EQUAL_NONCONT(pIns->nPPS, 8);
		VERIFY_EQUAL_NONCONT(pIns->nPPC, (NOTE_MIDDLEC - NOTE_MIN) + 6);	// F#5

		VERIFY_EQUAL_NONCONT(pIns->nVolRampUp, 1200);
		VERIFY_EQUAL_NONCONT(pIns->resampling, SRCMODE_SINC8LP);

		VERIFY_EQUAL_NONCONT(pIns->IsCutoffEnabled(), true);
		VERIFY_EQUAL_NONCONT(pIns->GetCutoff(), 0x32);
		VERIFY_EQUAL_NONCONT(pIns->IsResonanceEnabled(), true);
		VERIFY_EQUAL_NONCONT(pIns->GetResonance(), 0x64);
		VERIFY_EQUAL_NONCONT(pIns->filterMode, FilterMode::HighPass);

		VERIFY_EQUAL_NONCONT(pIns->nVolSwing, 0x30);
		VERIFY_EQUAL_NONCONT(pIns->nPanSwing, 0x18);
		VERIFY_EQUAL_NONCONT(pIns->nCutSwing, 0x0C);
		VERIFY_EQUAL_NONCONT(pIns->nResSwing, 0x3C);

		VERIFY_EQUAL_NONCONT(pIns->nNNA, NewNoteAction::Continue);
		VERIFY_EQUAL_NONCONT(pIns->nDCT, DuplicateCheckType::Note);
		VERIFY_EQUAL_NONCONT(pIns->nDNA, DuplicateNoteAction::NoteFade);

		VERIFY_EQUAL_NONCONT(pIns->nMixPlug, 1);
		VERIFY_EQUAL_NONCONT(pIns->nMidiChannel, 16);
		VERIFY_EQUAL_NONCONT(pIns->nMidiProgram, 64);
		VERIFY_EQUAL_NONCONT(pIns->wMidiBank, 2);
		VERIFY_EQUAL_NONCONT(pIns->midiPWD, ins);

		VERIFY_EQUAL_NONCONT(pIns->pTuning, sndFile.GetDefaultTuning());

		VERIFY_EQUAL_NONCONT(pIns->pitchToTempoLock, TEMPO(130, 2000));

		VERIFY_EQUAL_NONCONT(pIns->pluginVelocityHandling, PLUGIN_VELOCITYHANDLING_VOLUME);
		VERIFY_EQUAL_NONCONT(pIns->pluginVolumeHandling, PLUGIN_VOLUMEHANDLING_MIDI);

		for(size_t i = 0; i < NOTE_MAX; i++)
		{
			VERIFY_EQUAL_NONCONT(pIns->Keyboard[i], (i == NOTE_MIDDLEC - 1) ? 99 : 1);
			VERIFY_EQUAL_NONCONT(pIns->NoteMap[i], (i == NOTE_MIDDLEC - 1) ? (i + 13) : (i + 1));
		}

		VERIFY_EQUAL_NONCONT(pIns->VolEnv.dwFlags, ENV_ENABLED | ENV_CARRY);
		VERIFY_EQUAL_NONCONT(pIns->VolEnv.size(), 3);
		VERIFY_EQUAL_NONCONT(pIns->VolEnv.nReleaseNode, 1);
		VERIFY_EQUAL_NONCONT(pIns->VolEnv[2].tick, 96);
		VERIFY_EQUAL_NONCONT(pIns->VolEnv[2].value, 0);

		VERIFY_EQUAL_NONCONT(pIns->PanEnv.dwFlags, ENV_LOOP);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.size(), 76);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.nLoopStart, 22);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.nLoopEnd, 29);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.nReleaseNode, ENV_RELEASE_NODE_UNSET);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv[75].tick, 427);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv[75].value, 27);

		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.dwFlags, ENV_ENABLED | ENV_CARRY | ENV_SUSTAIN | ENV_FILTER);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.size(), 3);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.nSustainStart, 1);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.nSustainEnd, 2);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv[1].tick, 96);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv[1].value, 64);
	}
	// Sequences
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetNumSequences(), 2);

	VERIFY_EQUAL_NONCONT(sndFile.Order(0).GetLengthTailTrimmed(), 3);
	VERIFY_EQUAL_NONCONT(sndFile.Order(0).GetName(), U_("First Sequence"));
	VERIFY_EQUAL_NONCONT(sndFile.Order(0)[0], sndFile.Order.GetIgnoreIndex());
	VERIFY_EQUAL_NONCONT(sndFile.Order(0)[1], 0);
	VERIFY_EQUAL_NONCONT(sndFile.Order(0)[2], sndFile.Order.GetIgnoreIndex());
	VERIFY_EQUAL_NONCONT(sndFile.Order(0).GetRestartPos(), 1);

	VERIFY_EQUAL_NONCONT(sndFile.Order(1).GetLengthTailTrimmed(), 3);
	VERIFY_EQUAL_NONCONT(sndFile.Order(1).GetName(), U_("Second Sequence"));
	VERIFY_EQUAL_NONCONT(sndFile.Order(1)[0], 1);
	VERIFY_EQUAL_NONCONT(sndFile.Order(1)[1], 2);
	VERIFY_EQUAL_NONCONT(sndFile.Order(1)[2], 3);
	VERIFY_EQUAL_NONCONT(sndFile.Order(1).GetRestartPos(), 2);

	// Patterns
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.GetNumPatterns(), 2);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetName(), "First Pattern");
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetNumRows(), 70);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetNumChannels(), 70);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetOverrideSignature(), true);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetRowsPerBeat(), 5);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetRowsPerMeasure(), 10);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].HasTempoSwing(), true);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.IsPatternEmpty(0), true);

	{
		TempoSwing swing = sndFile.Patterns[0].GetTempoSwing();
		VERIFY_EQUAL_NONCONT(swing.size(), 5);
		VERIFY_EQUAL_NONCONT(swing[0], 16770149);
		VERIFY_EQUAL_NONCONT(swing[1], 16803696);
		VERIFY_EQUAL_NONCONT(swing[2], 16770157);
		VERIFY_EQUAL_NONCONT(swing[3], 29347774);
		VERIFY_EQUAL_NONCONT(swing[4], 4194304);
	}

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetName(), "Second Pattern");
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetNumRows(), 32);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetNumChannels(), 70);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetOverrideSignature(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetRowsPerBeat(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetRowsPerMeasure(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].HasTempoSwing(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.IsPatternEmpty(1), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->IsPcNote(), true);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->note, NOTE_PC);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->instr, 99);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->GetValueVolCol(), 1);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->GetValueEffectCol(), 200);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 0)->IsEmpty(), true);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->IsEmpty(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->IsPcNote(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->note, NOTE_MIDDLEC + 12);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->instr, 45);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->volcmd, VOLCMD_VOLSLIDEDOWN);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->vol, 5);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->command, CMD_PANNING8);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->param, 0xFF);

	// Plugins
#ifndef NO_PLUGINS
	const SNDMIXPLUGIN &plug = sndFile.m_MixPlugins[0];
	VERIFY_EQUAL_NONCONT(plug.GetName(), U_("First Plugin"));
	VERIFY_EQUAL_NONCONT(plug.fDryRatio, 0.26f);
	VERIFY_EQUAL_NONCONT(plug.IsMasterEffect(), true);
	VERIFY_EQUAL_NONCONT(plug.GetGain(), 11);
	VERIFY_EQUAL_NONCONT(plug.pMixPlugin != nullptr, true);
	if(plug.pMixPlugin)
	{
		VERIFY_EQUAL_NONCONT(plug.pMixPlugin->GetParameter(1), 0.5f);
		VERIFY_EQUAL_NONCONT(plug.pMixPlugin->IsInstrument(), false);
	}
#endif // NO_PLUGINS

#ifdef MODPLUG_TRACKER
	// MIDI Mapping
	VERIFY_EQUAL_NONCONT(sndFile.GetMIDIMapper().GetCount(), 1);
	const CMIDIMappingDirective &mapping = sndFile.GetMIDIMapper().GetDirective(0);
	VERIFY_EQUAL_NONCONT(mapping.GetAllowPatternEdit(), true);
	VERIFY_EQUAL_NONCONT(mapping.GetCaptureMIDI(), false);
	VERIFY_EQUAL_NONCONT(mapping.IsActive(), true);
	VERIFY_EQUAL_NONCONT(mapping.GetAnyChannel(), false);
	VERIFY_EQUAL_NONCONT(mapping.GetChannel(), 5);
	VERIFY_EQUAL_NONCONT(mapping.GetPlugIndex(), 1);
	VERIFY_EQUAL_NONCONT(mapping.GetParamIndex(), 0);
	VERIFY_EQUAL_NONCONT(mapping.GetEvent(), MIDIEvents::evControllerChange);
	VERIFY_EQUAL_NONCONT(mapping.GetController(), MIDIEvents::MIDICC_ModulationWheel_Coarse);

	// Channel colors
	const auto &chns = sndFile.ChnSettings;
	VERIFY_EQUAL_NONCONT(chns[0].color, RGB(255, 0, 0));
	VERIFY_EQUAL_NONCONT(chns[1].color, RGB(0, 255, 0));
	VERIFY_EQUAL_NONCONT(chns[2].color, RGB(0, 0, 255));
	VERIFY_EQUAL_NONCONT(chns[3].color, ModChannelSettings::INVALID_COLOR);
	VERIFY_EQUAL_NONCONT(chns[67].color, RGB(255, 0, 255));
	VERIFY_EQUAL_NONCONT(chns[68].color, RGB(255, 255, 0));
	VERIFY_EQUAL_NONCONT(chns[69].color, ModChannelSettings::INVALID_COLOR);
#endif

	VERIFY_EQUAL_NONCONT(sndFile.FrequencyToCutOff(sndFile.CutOffToFrequency(0)), 0);
	VERIFY_EQUAL_NONCONT(sndFile.FrequencyToCutOff(sndFile.CutOffToFrequency(80)), 80);
	VERIFY_EQUAL_NONCONT(sndFile.FrequencyToCutOff(sndFile.CutOffToFrequency(127)), 127);
}


// Check if our test file was loaded correctly.
static void TestLoadS3MFile(const CSoundFile &sndFile, bool resaved)
{

	// Global Variables
	VERIFY_EQUAL_NONCONT(sndFile.GetTitle(), "S3M_Test__________________X");
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultTempo, TEMPO(33, 0));
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultSpeed, 254);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultGlobalVolume, 32 * 4);
	VERIFY_EQUAL_NONCONT(sndFile.m_nVSTiVolume, 36);
	VERIFY_EQUAL_NONCONT(sndFile.m_nSamplePreAmp, 16);
	VERIFY_EQUAL_NONCONT((sndFile.m_SongFlags & SONG_FILE_FLAGS), SONG_FASTVOLSLIDES);
	VERIFY_EQUAL_NONCONT(sndFile.GetMixLevels(), MixLevels::Compatible);
	VERIFY_EQUAL_NONCONT(sndFile.m_nTempoMode, TempoMode::Classic);
	VERIFY_EQUAL_NONCONT(sndFile.m_dwLastSavedWithVersion, resaved ? Version::Current() : MPT_V("1.27.00.00"));

	// Channels
	VERIFY_EQUAL_NONCONT(sndFile.GetNumChannels(), 4);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].nPan, 0);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].dwFlags, ChannelFlags(0));

	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].nPan, 256);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].dwFlags, CHN_MUTE);

	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[2].nPan, 85);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[2].dwFlags, ChannelFlags(0));

	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[3].nPan, 171);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[3].dwFlags, CHN_MUTE);

	// Samples
	VERIFY_EQUAL_NONCONT(sndFile.GetNumSamples(), 4);
	{
		const ModSample &sample = sndFile.GetSample(1);
		VERIFY_EQUAL_NONCONT(sndFile.m_szNames[1], "Sample_1__________________X");
		VERIFY_EQUAL_NONCONT(sample.filename, "Filename_1_X");
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 60);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_S3M), 9001);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 32 * 4);
		VERIFY_EQUAL_NONCONT(sample.nGlobalVol, 64);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_LOOP);

		VERIFY_EQUAL_NONCONT(sample.nLoopStart, 16);
		VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 60);

		// Sample Data
		for(size_t i = 0; i < 30; i++)
		{
			VERIFY_EQUAL_NONCONT(sample.sample8()[i], 127);
		}
		for(size_t i = 31; i < 60; i++)
		{
			VERIFY_EQUAL_NONCONT(sample.sample8()[i], -128);
		}
	}

	{
		const ModSample &sample = sndFile.GetSample(2);
		VERIFY_EQUAL_NONCONT(sndFile.m_szNames[2], "Empty");
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_S3M), 16384);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 2 * 4);
	}

	{
		const ModSample &sample = sndFile.GetSample(3);
		VERIFY_EQUAL_NONCONT(sndFile.m_szNames[3], "Stereo / 16-Bit");
		VERIFY_EQUAL_NONCONT(sample.filename, "Filename_3_X");
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 4);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 2);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 2);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 64);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_S3M), 16000);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 0);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_LOOP | CHN_16BIT | CHN_STEREO);

		VERIFY_EQUAL_NONCONT(sample.nLoopStart, 0);
		VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 16);

		// Sample Data (Stereo Interleaved)
		for(size_t i = 0; i < 7; i++)
		{
			VERIFY_EQUAL_NONCONT(sample.sample16()[4 + i], int16(-32768));
		}
	}

	{
		const ModSample &sample = sndFile.GetSample(4);
		VERIFY_EQUAL_NONCONT(sndFile.m_szNames[4], "adlib");
		VERIFY_EQUAL_NONCONT((sample.filename == ""), true);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_S3M), 8363);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 58 * 4);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_ADLIB);
		VERIFY_EQUAL_NONCONT(sample.adlib, (OPLPatch{ { 0x00, 0x00, 0xC0, 0x00, 0xF0, 0xD2, 0x05, 0xB3, 0x01, 0x00, 0x00, 0x00 } }));
	}

	// Orders
	VERIFY_EQUAL_NONCONT(sndFile.Order().GetRestartPos(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Order().GetLengthTailTrimmed(), 5);
	VERIFY_EQUAL_NONCONT(sndFile.Order()[0], 0);
	VERIFY_EQUAL_NONCONT(sndFile.Order()[1], sndFile.Order.GetIgnoreIndex());
	VERIFY_EQUAL_NONCONT(sndFile.Order()[2], sndFile.Order.GetInvalidPatIndex());
	VERIFY_EQUAL_NONCONT(sndFile.Order()[3], 1);
	VERIFY_EQUAL_NONCONT(sndFile.Order()[4], 0);

	// Patterns
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.GetNumPatterns(), 2);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetNumRows(), 64);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetNumChannels(), 4);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetOverrideSignature(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(0, 0)->note, NOTE_MIN + 12);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(1, 0)->note, NOTE_MIN + 107);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(0, 1)->volcmd, VOLCMD_VOLUME);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(0, 1)->vol, 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(1, 1)->volcmd, VOLCMD_VOLUME);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(1, 1)->vol, 64);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(2, 1)->volcmd, VOLCMD_PANNING);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(2, 1)->vol, 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(3, 1)->volcmd, VOLCMD_PANNING);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(3, 1)->vol, 64);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(0, 3)->command, CMD_SPEED);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(0, 3)->param, 0x11);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetNumRows(), 64);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.IsPatternEmpty(1), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(63, 3)->param, 0x04);
}


// Check if our test file was loaded correctly.
static void TestLoadMODFile(CSoundFile &sndFile)
{
	// Global Variables
	VERIFY_EQUAL_NONCONT(sndFile.GetTitle(), "MOD_Test___________X");
	VERIFY_EQUAL_NONCONT((sndFile.m_SongFlags & SONG_FILE_FLAGS), SONG_PT_MODE | SONG_AMIGALIMITS | SONG_ISAMIGA);
	VERIFY_EQUAL_NONCONT(sndFile.GetMixLevels(), MixLevels::Compatible);
	VERIFY_EQUAL_NONCONT(sndFile.m_nTempoMode, TempoMode::Classic);
	VERIFY_EQUAL_NONCONT(sndFile.GetNumChannels(), 4);
	VERIFY_EQUAL_NONCONT(sndFile.m_playBehaviour[kMODOneShotLoops], true);
	VERIFY_EQUAL_NONCONT(sndFile.m_playBehaviour[kMODSampleSwap], true);
	VERIFY_EQUAL_NONCONT(sndFile.m_playBehaviour[kMODIgnorePanning], true);
	VERIFY_EQUAL_NONCONT(sndFile.m_playBehaviour[kMODVBlankTiming], false);

	// Test GetLength code, in particular with subsongs
	VERIFY_EQUAL_NONCONT(sndFile.GetLength(eNoAdjust, GetLengthTarget(0, 4)).back().targetReached, false);

	const auto allSubSongs = sndFile.GetLength(eNoAdjust, GetLengthTarget(true));
	VERIFY_EQUAL_NONCONT(allSubSongs.size(), 2);
	VERIFY_EQUAL_EPS(allSubSongs[0].duration, 2.04, 0.1);
	VERIFY_EQUAL_EPS(allSubSongs[1].duration, 118.84, 0.1);
	VERIFY_EQUAL_NONCONT(allSubSongs[0].lastOrder, 0);
	VERIFY_EQUAL_NONCONT(allSubSongs[0].lastRow, 1);
	VERIFY_EQUAL_NONCONT(allSubSongs[1].lastOrder, 2);
	VERIFY_EQUAL_NONCONT(allSubSongs[1].lastRow, 61);
	VERIFY_EQUAL_NONCONT(allSubSongs[1].startOrder, 2);
	VERIFY_EQUAL_NONCONT(allSubSongs[1].startRow, 0);

	// Samples
	VERIFY_EQUAL_NONCONT(sndFile.GetNumSamples(), 31);
	{
		const ModSample &sample = sndFile.GetSample(1);
		VERIFY_EQUAL_NONCONT(sndFile.m_szNames[1], "Sample_1_____________X");
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 1244);
		VERIFY_EQUAL_NONCONT(sample.nFineTune, 0x70);
		VERIFY_EQUAL_NONCONT(sample.RelativeTone, 0);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 256);
		VERIFY_EQUAL_NONCONT(sample.nGlobalVol, 64);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_LOOP);

		VERIFY_EQUAL_NONCONT(sample.nLoopStart, 0);
		VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 128);

		// Sample Data
		VERIFY_EQUAL_NONCONT(sample.sample8()[0], 0);
		VERIFY_EQUAL_NONCONT(sample.sample8()[1], 0);
		VERIFY_EQUAL_NONCONT(sample.sample8()[2], -29);
	}
	{
		const ModSample &sample = sndFile.GetSample(3);
		VERIFY_EQUAL_NONCONT(sndFile.m_szNames[3], "OpenMPT Module Loader");
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 0);
		VERIFY_EQUAL_NONCONT(sample.nFineTune, -0x80);
		VERIFY_EQUAL_NONCONT(sample.RelativeTone, 0);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 4);
		VERIFY_EQUAL_NONCONT(sample.nGlobalVol, 64);
	}

	// Orders
	VERIFY_EQUAL_NONCONT(sndFile.Order().GetRestartPos(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Order().GetLengthTailTrimmed(), 4);
	VERIFY_EQUAL_NONCONT(sndFile.Order()[0], 0);
	VERIFY_EQUAL_NONCONT(sndFile.Order()[1], 1);
	VERIFY_EQUAL_NONCONT(sndFile.Order()[2], 2);
	VERIFY_EQUAL_NONCONT(sndFile.Order()[3], 0);

	// Patterns
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.GetNumPatterns(), 3);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[2].GetNumRows(), 64);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[2].GetpModCommand(1, 0)->note, NOTE_MIDDLEC + 12);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[2].GetpModCommand(16, 3)->instr, 1);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[2].GetpModCommand(19, 3)->command, CMD_PANNING8);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[2].GetpModCommand(19, 3)->param, 0x28);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[2].GetpModCommand(20, 0)->command, CMD_TEMPO);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[2].GetpModCommand(20, 1)->command, CMD_SPEED);
}


#ifdef MODPLUG_TRACKER

static bool ShouldRunTests()
{
	mpt::PathString theFile = theApp.GetInstallPath();
	if(theFile.IsDirectory() && (theFile + P_("test")).IsDirectory())
	{
		if((theFile + P_("test\\test.mptm")).IsFile())
		{
			return true;
		}
	}
	return false;
}

static mpt::PathString GetTestFilenameBase()
{
	mpt::PathString theFile = theApp.GetInstallPath();
	theFile += P_("test/test.");
	return theFile;
}

static mpt::PathString GetTempFilenameBase()
{
	return GetTestFilenameBase();
}

typedef CModDoc *TSoundFileContainer;

static CSoundFile &GetSoundFile(TSoundFileContainer &sndFile)
{
	return sndFile->GetSoundFile();
}


static TSoundFileContainer CreateSoundFileContainer(const mpt::PathString &filename)
{
	CModDoc *pModDoc = static_cast<CModDoc *>(theApp.OpenDocumentFile(filename.ToCString(), FALSE));
	return pModDoc;
}

static void DestroySoundFileContainer(TSoundFileContainer &sndFile)
{
	sndFile->OnCloseDocument();
}

static void SaveTestFile(const TSoundFileContainer &sndFile, const mpt::PathString &filename)
{
	sndFile->DoSave(filename);
	// Saving the file puts it in the MRU list...
	theApp.RemoveMruItem(0);
}

const auto SaveIT = SaveTestFile;
const auto SaveXM = SaveTestFile;
const auto SaveS3M = SaveTestFile;
const auto SaveMOD = SaveTestFile;

#else // !MODPLUG_TRACKER

static bool ShouldRunTests()
{
	#if MPT_TEST_HAS_FILESYSTEM
		return true;
	#else
		return false;
	#endif
}

static mpt::PathString GetTestFilenameBase()
{
	return Test::GetPathPrefix() + P_("./test/test.");
}

static mpt::PathString GetTempFilenameBase()
{
	return P_("./test.");
}

typedef std::shared_ptr<CSoundFile> TSoundFileContainer;

static CSoundFile &GetSoundFile(TSoundFileContainer &sndFile)
{
	return *sndFile.get();
}

static TSoundFileContainer CreateSoundFileContainer(const mpt::PathString &filename)
{
	mpt::ifstream stream(filename, std::ios::binary);
	FileReader file = mpt::IO::make_FileCursor<mpt::PathString>(stream);
	std::shared_ptr<CSoundFile> pSndFile = std::make_shared<CSoundFile>();
	pSndFile->Create(file, CSoundFile::loadCompleteModule);
	return pSndFile;
}

static void DestroySoundFileContainer(TSoundFileContainer & /* sndFile */ )
{
	return;
}

#ifndef MODPLUG_NO_FILESAVE

static void SaveIT(const TSoundFileContainer &sndFile, const mpt::PathString &filename)
{
	mpt::ofstream f(filename, std::ios::binary);
	sndFile->SaveIT(f, filename, false);
}

static void SaveXM(const TSoundFileContainer &sndFile, const mpt::PathString &filename)
{
	mpt::ofstream f(filename, std::ios::binary);
	sndFile->SaveXM(f, false);
}

static void SaveS3M(const TSoundFileContainer &sndFile, const mpt::PathString &filename)
{
	mpt::ofstream f(filename, std::ios::binary);
	sndFile->SaveS3M(f);
}

static void SaveMOD(const TSoundFileContainer &sndFile, const mpt::PathString &filename)
{
	mpt::ofstream f(filename, std::ios::binary);
	sndFile->SaveMod(f);
}

#endif // !MODPLUG_NO_FILESAVE

#endif // MODPLUG_TRACKER



// Test file loading and saving
static MPT_NOINLINE void TestLoadSaveFile()
{
	if(!ShouldRunTests())
	{
		return;
	}

#ifdef MODPLUG_TRACKER
	bool saveMutedChannels = TrackerSettings::Instance().MiscSaveChannelMuteStatus;
	TrackerSettings::Instance().MiscSaveChannelMuteStatus = true;
#endif

	mpt::PathString filenameBaseSrc = GetTestFilenameBase();
	mpt::PathString filenameBase = GetTempFilenameBase();

	// Test MPTM file loading
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBaseSrc + P_("mptm"));

		TestLoadMPTMFile(GetSoundFile(sndFileContainer));

		#ifndef MODPLUG_NO_FILESAVE
			// Test file saving
			GetSoundFile(sndFileContainer).m_dwLastSavedWithVersion = Version::Current();
			SaveIT(sndFileContainer, filenameBase + P_("saved.mptm"));
		#endif

		DestroySoundFileContainer(sndFileContainer);
	}

	// Reload the saved file and test if everything is still working correctly.
	#ifndef MODPLUG_NO_FILESAVE
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBase + P_("saved.mptm"));

		TestLoadMPTMFile(GetSoundFile(sndFileContainer));

		DestroySoundFileContainer(sndFileContainer);

		RemoveFile(filenameBase + P_("saved.mptm"));
	}
	#endif

	// Test XM file loading
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBaseSrc + P_("xm"));

		TestLoadXMFile(GetSoundFile(sndFileContainer));

		// In OpenMPT 1.20 (up to revision 1459), there was a bug in the XM saver
		// that would create broken XMs if the sample map contained samples that
		// were only referenced below C-1 or above B-8 (such samples should not
		// be written). Let's insert a sample there and check if re-loading the
		// file still works.
		GetSoundFile(sndFileContainer).m_nSamples++;
		GetSoundFile(sndFileContainer).Instruments[1]->Keyboard[110] = GetSoundFile(sndFileContainer).GetNumSamples();

		#ifndef MODPLUG_NO_FILESAVE
			// Test file saving
			GetSoundFile(sndFileContainer).m_dwLastSavedWithVersion = Version::Current();
			SaveXM(sndFileContainer, filenameBase + P_("saved.xm"));
		#endif

		DestroySoundFileContainer(sndFileContainer);
	}

	// Reload the saved file and test if everything is still working correctly.
	#ifndef MODPLUG_NO_FILESAVE
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBase + P_("saved.xm"));

		TestLoadXMFile(GetSoundFile(sndFileContainer));

		DestroySoundFileContainer(sndFileContainer);

		RemoveFile(filenameBase + P_("saved.xm"));
	}
	#endif

	// Test S3M file loading
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBaseSrc + P_("s3m"));
		auto &sndFile = GetSoundFile(sndFileContainer);

		TestLoadS3MFile(sndFile, false);

		// Test GetLength code, in particular with subsongs
		sndFile.ChnSettings[1].dwFlags.reset(CHN_MUTE);
		
		VERIFY_EQUAL_EPS(sndFile.GetLength(eAdjustSamplePositions, GetLengthTarget(3, 1)).back().duration, 19.237, 0.01);
		VERIFY_EQUAL_NONCONT(sndFile.GetLength(eAdjustSamplePositions, GetLengthTarget(2, 0).StartPos(0, 1, 0)).back().targetReached, false);

		auto allSubSongs = sndFile.GetLength(eNoAdjust, GetLengthTarget(true));
		VERIFY_EQUAL_NONCONT(allSubSongs.size(), 3);
		double totalDuration = 0.0;
		for(const auto &subSong : allSubSongs)
		{
			totalDuration += subSong.duration;
		}
		VERIFY_EQUAL_EPS(totalDuration, 3674.38, 1.0);

		#ifndef MODPLUG_NO_FILESAVE
			// Test file saving
			sndFile.ChnSettings[1].dwFlags.set(CHN_MUTE);
			sndFile.m_dwLastSavedWithVersion = Version::Current();
			SaveS3M(sndFileContainer, filenameBase + P_("saved.s3m"));
		#endif

		DestroySoundFileContainer(sndFileContainer);
	}

	// Reload the saved file and test if everything is still working correctly.
	#ifndef MODPLUG_NO_FILESAVE
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBase + P_("saved.s3m"));

		TestLoadS3MFile(GetSoundFile(sndFileContainer), true);

		DestroySoundFileContainer(sndFileContainer);

		RemoveFile(filenameBase + P_("saved.s3m"));
	}
	#endif

	// Test MOD file loading
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBaseSrc + P_("mod"));
		auto &sndFile = GetSoundFile(sndFileContainer);

		TestLoadMODFile(sndFile);

#ifndef MODPLUG_NO_FILESAVE
		// Test file saving
		SaveMOD(sndFileContainer, filenameBase + P_("saved.mod"));
#endif

		DestroySoundFileContainer(sndFileContainer);
	}

	// Reload the saved file and test if everything is still working correctly.
#ifndef MODPLUG_NO_FILESAVE
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBase + P_("saved.mod"));
		TestLoadMODFile(GetSoundFile(sndFileContainer));
		DestroySoundFileContainer(sndFileContainer);
		RemoveFile(filenameBase + P_("saved.mod"));
	}
#endif

	// General file I/O tests
	{
		std::ostringstream f;
		size_t bytesWritten;
		mpt::IO::WriteVarInt(f, uint16(0), &bytesWritten);		VERIFY_EQUAL_NONCONT(bytesWritten, 1);
		mpt::IO::WriteVarInt(f, uint16(127), &bytesWritten);	VERIFY_EQUAL_NONCONT(bytesWritten, 1);
		mpt::IO::WriteVarInt(f, uint16(128), &bytesWritten);	VERIFY_EQUAL_NONCONT(bytesWritten, 2);
		mpt::IO::WriteVarInt(f, uint16(16383), &bytesWritten);	VERIFY_EQUAL_NONCONT(bytesWritten, 2);
		mpt::IO::WriteVarInt(f, uint16(16384), &bytesWritten);	VERIFY_EQUAL_NONCONT(bytesWritten, 3);
		mpt::IO::WriteVarInt(f, uint16(65535), &bytesWritten);	VERIFY_EQUAL_NONCONT(bytesWritten, 3);
		mpt::IO::WriteVarInt(f, uint64(0xFFFFFFFFFFFFFFFFull), &bytesWritten);	VERIFY_EQUAL_NONCONT(bytesWritten, 10);
		std::string data = f.str();
		FileReader file(mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(data)));
		uint64 v;
		file.ReadVarInt(v); VERIFY_EQUAL_NONCONT(v, 0);
		file.ReadVarInt(v); VERIFY_EQUAL_NONCONT(v, 127);
		file.ReadVarInt(v); VERIFY_EQUAL_NONCONT(v, 128);
		file.ReadVarInt(v); VERIFY_EQUAL_NONCONT(v, 16383);
		file.ReadVarInt(v); VERIFY_EQUAL_NONCONT(v, 16384);
		file.ReadVarInt(v); VERIFY_EQUAL_NONCONT(v, 65535);
		file.ReadVarInt(v); VERIFY_EQUAL_NONCONT(v, 0xFFFFFFFFFFFFFFFFull);
	}

#ifdef MODPLUG_TRACKER
	TrackerSettings::Instance().MiscSaveChannelMuteStatus = saveMutedChannels;
#endif
}


// Test various editing features
static MPT_NOINLINE void TestEditing()
{
#ifdef MODPLUG_TRACKER
	auto modDoc = static_cast<CModDoc *>(theApp.GetModDocTemplate()->CreateNewDocument());
	auto &sndFile = modDoc->GetSoundFile();
	sndFile.Create(FileReader(), CSoundFile::loadCompleteModule, modDoc);
	sndFile.m_nChannels = 4;
	sndFile.ChangeModTypeTo(MOD_TYPE_MPT);

	// Rearrange channels
	sndFile.Patterns.ResizeArray(2);
	sndFile.Patterns.Insert(0, 32);
	sndFile.Patterns.Insert(1, 48);
	sndFile.Patterns[1].SetName("Pattern");
	sndFile.Patterns[1].SetSignature(2, 4);
	TempoSwing swing;
	swing.resize(2);
	sndFile.Patterns[1].SetTempoSwing(swing);
	sndFile.Patterns[1].GetpModCommand(37, 0)->instr = 1;
	sndFile.Patterns[1].GetpModCommand(37, 1)->instr = 2;
	sndFile.Patterns[1].GetpModCommand(37, 2)->instr = 3;
	sndFile.Patterns[1].GetpModCommand(37, 3)->instr = 4;
	modDoc->ReArrangeChannels({ 3, 2, CHANNELINDEX_INVALID, 0 });
	modDoc->ReArrangeChannels({ 0, 1, 1, CHANNELINDEX_INVALID, 3 });
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetName(), "Pattern");
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetRowsPerBeat(), 2);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetRowsPerMeasure(), 4);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetTempoSwing(), swing);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(37, 0)->instr, 4);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(37, 1)->instr, 3);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(37, 2)->instr, 3);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(37, 3)->instr, 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(37, 4)->instr, 1);

	// Rearrange samples
	sndFile.m_nSamples = 2;
	sndFile.GetSample(1).filename = "1";
	sndFile.m_szNames[1] = "1";
	sndFile.GetSample(2).filename = "2";
	sndFile.m_szNames[2] = "2";
	sndFile.GetSample(2).nLength = 16;
	sndFile.GetSample(2).AllocateSample();
	modDoc->ReArrangeSamples({ 2, SAMPLEINDEX_INVALID, 1 });
	VERIFY_EQUAL_NONCONT(sndFile.GetSample(1).HasSampleData(), true);
	VERIFY_EQUAL_NONCONT(sndFile.GetSample(1).filename, "2");
	VERIFY_EQUAL_NONCONT(sndFile.m_szNames[1], "2");
	VERIFY_EQUAL_NONCONT(sndFile.GetSample(2).filename, "");
	VERIFY_EQUAL_NONCONT(sndFile.m_szNames[2], "");
	VERIFY_EQUAL_NONCONT(sndFile.GetSample(3).filename, "1");
	VERIFY_EQUAL_NONCONT(sndFile.m_szNames[3], "1");
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(37, 4)->instr, 3);

	// Convert / rearrange instruments
	modDoc->ConvertSamplesToInstruments();
	modDoc->ReArrangeInstruments({ INSTRUMENTINDEX_INVALID, 2, 1, 3 });
	VERIFY_EQUAL_NONCONT(sndFile.Instruments[1]->name, "");
	VERIFY_EQUAL_NONCONT(sndFile.Instruments[2]->name, "");
	VERIFY_EQUAL_NONCONT(sndFile.Instruments[3]->name, "2");
	VERIFY_EQUAL_NONCONT(sndFile.Instruments[4]->name, "1");
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(37, 4)->instr, 4);
	modDoc->ConvertInstrumentsToSamples();
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(37, 4)->instr, 3);

	modDoc->SetModified();
	VERIFY_EQUAL_NONCONT(modDoc->IsModified(), true);
	VERIFY_EQUAL_NONCONT(modDoc->ModifiedSinceLastAutosave(), true);
	VERIFY_EQUAL_NONCONT(modDoc->ModifiedSinceLastAutosave(), false);

	sndFile.Destroy();
	modDoc->OnCloseDocument();
#endif
}


static void RunITCompressionTest(const std::vector<int8> &sampleData, FlagSet<ChannelFlags> smpFormat, bool it215)
{

	ModSample smp;
	smp.uFlags = smpFormat;
	smp.pData.pSample = const_cast<int8 *>(sampleData.data());
	smp.nLength = mpt::saturate_cast<SmpLength>(sampleData.size() / smp.GetBytesPerSample());

	std::string data;

	{
		std::ostringstream f;
		ITCompression compression(smp, it215, &f);
		data = f.str();
	}

	{
		FileReader file(mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(data)));

		std::vector<int8> sampleDataNew(sampleData.size(), 0);
		smp.pData.pSample = sampleDataNew.data();

		ITDecompression decompression(file, smp, it215);
		VERIFY_EQUAL_NONCONT(memcmp(sampleData.data(), sampleDataNew.data(), sampleData.size()), 0);
	}
}


static MPT_NOINLINE void TestITCompression()
{
	// Test loading / saving of IT-compressed samples
	const int sampleDataSize = 65536;
	std::vector<int8> sampleData(sampleDataSize, 0);
	std::srand(0);
	for(int i = 0; i < sampleDataSize; i++)
	{
		sampleData[i] = mpt::random<int8>(*s_PRNG);
	}

	// Run each compression test with IT215 compression and without.
	for(int i = 0; i < 2; i++)
	{
		RunITCompressionTest(sampleData, ChannelFlags(0), i == 0);
		RunITCompressionTest(sampleData, CHN_16BIT, i == 0);
		RunITCompressionTest(sampleData, CHN_STEREO, i == 0);
		RunITCompressionTest(sampleData, CHN_16BIT | CHN_STEREO, i == 0);
	}
}



#if 0

static bool RatioEqual(CTuningBase::RATIOTYPE a, CTuningBase::RATIOTYPE b)
{
	if(a == CTuningBase::RATIOTYPE(0) && b == CTuningBase::RATIOTYPE(0))
	{
		return true;
	}
	if(a == CTuningBase::RATIOTYPE(0) || b == CTuningBase::RATIOTYPE(0))
	{
		return false;
	}
	return (std::fabs(CTuningBase::RATIOTYPE(1) - (a/b)) < CTuningBase::RATIOTYPE(0.0001));
}


static void CheckEqualTuningCollections(const CTuningCollection &a, const CTuningCollection &b)
{
	VERIFY_EQUAL(a.GetName(), b.GetName());
	VERIFY_EQUAL(a.GetNumTunings(), b.GetNumTunings());
	for(std::size_t tuning = 0; tuning < std::min(a.GetNumTunings(), b.GetNumTunings()); ++tuning)
	{
		VERIFY_EQUAL(a.GetTuning(tuning).GetName(), b.GetTuning(tuning).GetName());
		VERIFY_EQUAL(a.GetTuning(tuning).GetType(), b.GetTuning(tuning).GetType());
		VERIFY_EQUAL(a.GetTuning(tuning).GetGroupSize(), b.GetTuning(tuning).GetGroupSize());
		VERIFY_EQUAL(a.GetTuning(tuning).GetFineStepCount(), b.GetTuning(tuning).GetFineStepCount());
		VERIFY_EQUAL(RatioEqual(a.GetTuning(tuning).GetGroupRatio(), b.GetTuning(tuning).GetGroupRatio()), true);
		VERIFY_EQUAL(a.GetTuning(tuning).GetValidityRange(), b.GetTuning(tuning).GetValidityRange());
		for(ModCommand::NOTE note = NOTE_MIN; note <= NOTE_MAX; ++note)
		{
			VERIFY_EQUAL(a.GetTuning(tuning).GetNoteName(note - NOTE_MIDDLEC), b.GetTuning(tuning).GetNoteName(note - NOTE_MIDDLEC));
			VERIFY_EQUAL(RatioEqual(a.GetTuning(tuning).GetRatio(note - NOTE_MIDDLEC), b.GetTuning(tuning).GetRatio(note - NOTE_MIDDLEC)), true);
		}
	}
}

#endif



static double Rand01()
{
	return mpt::random(*s_PRNG, 0.0, 1.0);
}

template <class T>
T Rand(const T min, const T max)
{
	return mpt::saturate_round<T>(min + Rand01() * (max - min));
}

static void GenerateCommands(CPattern& pat, const double dProbPcs, const double dProbPc)
{
	const double dPcxProb = dProbPcs + dProbPc;
	for(auto &m : pat)
	{
		const double rand = Rand01();
		if(rand < dPcxProb)
		{
			if(rand < dProbPcs)
				m.note = NOTE_PCS;
			else
				m.note = NOTE_PC;

			m.instr = Rand<ModCommand::INSTR>(0, MAX_MIXPLUGINS);
			m.SetValueVolCol(Rand<uint16>(0, ModCommand::maxColumnValue));
			m.SetValueEffectCol(Rand<uint16>(0, ModCommand::maxColumnValue));
		}
		else
			m.Clear();
	}
}


// Test PC note serialization
static MPT_NOINLINE void TestPCnoteSerialization()
{
	FileReader file;
	std::unique_ptr<CSoundFile> pSndFile = std::make_unique<CSoundFile>();
	CSoundFile &sndFile = *pSndFile.get();
	sndFile.m_nType = MOD_TYPE_MPT;
	sndFile.Patterns.DestroyPatterns();
	sndFile.m_nChannels = ModSpecs::mptm.channelsMax;

	sndFile.Patterns.Insert(0, ModSpecs::mptm.patternRowsMin);
	sndFile.Patterns.Insert(1, 64);
	GenerateCommands(sndFile.Patterns[1], 0.3, 0.3);
	sndFile.Patterns.Insert(2, ModSpecs::mptm.patternRowsMax);
	GenerateCommands(sndFile.Patterns[2], 0.5, 0.5);

	// Copy pattern data for comparison.
	CPatternContainer patterns{ sndFile.Patterns };

	std::stringstream mem;
	WriteModPatterns(mem, sndFile.Patterns);

	VERIFY_EQUAL_NONCONT( mem.good(), true );

	// Clear patterns.
	sndFile.Patterns[0].ClearCommands();
	sndFile.Patterns[1].ClearCommands();
	sndFile.Patterns[2].ClearCommands();

	// Read data back.
	ReadModPatterns(mem, sndFile.Patterns);

	// Compare.
	VERIFY_EQUAL_NONCONT( sndFile.Patterns[0].GetNumRows(), ModSpecs::mptm.patternRowsMin);
	VERIFY_EQUAL_NONCONT( sndFile.Patterns[1].GetNumRows(), 64);
	VERIFY_EQUAL_NONCONT( sndFile.Patterns[2].GetNumRows(), ModSpecs::mptm.patternRowsMax);
	for(int i = 0; i < 3; i++)
	{
		VERIFY_EQUAL(sndFile.Patterns[i], patterns[i]);
	}
}


static inline std::size_t strnlen(const char *str, std::size_t n)
{
#if MPT_COMPILER_MSVC
	return ::strnlen(str, n);
#else
	if(n >= std::numeric_limits<std::size_t>::max())
	{
		return std::strlen(str);
	}
	for(std::size_t i = 0; i < n; ++i)
	{
		if(str[i] == '\0')
		{
			return i;
		}
	}
	return n;
#endif
}


// Test String I/O functionality
static MPT_NOINLINE void TestStringIO()
{
	char src0[4] = { '\0', 'X', ' ', 'X' };		// Weird empty buffer
	char src1[4] = { 'X', ' ', '\0', 'X' };		// Weird buffer (hello Impulse Tracker)
	char src2[4] = { 'X', 'Y', 'Z', ' ' };		// Full buffer, last character space
	char src3[4] = { 'X', 'Y', 'Z', '!' };		// Full buffer, last character non-space
	char src4[4] = { 'x', 'y', '\t', '\n' };	// Full buffer containing non-space whitespace
	char dst1[6];	// Destination buffer, larger than source buffer
	char dst2[3];	// Destination buffer, smaller than source buffer

#define ReadTest(mode, dst, src, expectedResult) \
	std::memset(dst, 0x7f, sizeof(dst)); \
	mpt::String::WriteAutoBuf(dst) = mpt::String::ReadBuf(mpt::String:: mode , src); \
	VERIFY_EQUAL_NONCONT(strncmp(dst, expectedResult, std::size(dst)), 0); /* Ensure that the strings are identical */ \
	for(size_t i = strlen(dst); i < std::size(dst); i++) \
		VERIFY_EQUAL_NONCONT(dst[i], '\0'); /* Ensure that rest of the buffer is completely nulled */ \
	/**/

#define WriteTest(mode, dst, src, expectedResult) \
	std::memset(dst, 0x7f, sizeof(dst)); \
	mpt::String::WriteBuf(mpt::String:: mode , dst) = mpt::String::ReadAutoBuf(src); \
	VERIFY_EQUAL_NONCONT(strncmp(dst, expectedResult, std::size(dst)), 0);  /* Ensure that the strings are identical */ \
	for(size_t i = Test::strnlen(dst, std::size(dst)); i < std::size(dst); i++) \
		VERIFY_EQUAL_NONCONT(dst[i], '\0'); /* Ensure that rest of the buffer is completely nulled */ \
	/**/

	// Check reading of null-terminated string into larger buffer
	ReadTest(nullTerminated, dst1, src0, "");
	ReadTest(nullTerminated, dst1, src1, "X ");
	ReadTest(nullTerminated, dst1, src2, "XYZ");
	ReadTest(nullTerminated, dst1, src3, "XYZ");
	ReadTest(nullTerminated, dst1, src4, "xy\t");

	// Check reading of string that should be null-terminated, but is maybe too long to still hold the null character.
	ReadTest(maybeNullTerminated, dst1, src0, "");
	ReadTest(maybeNullTerminated, dst1, src1, "X ");
	ReadTest(maybeNullTerminated, dst1, src2, "XYZ ");
	ReadTest(maybeNullTerminated, dst1, src3, "XYZ!");
	ReadTest(maybeNullTerminated, dst1, src4, "xy\t\n");

	// Check reading of space-padded strings with ignored last character
	ReadTest(spacePaddedNull, dst1, src0, " X");
	ReadTest(spacePaddedNull, dst1, src1, "X");
	ReadTest(spacePaddedNull, dst1, src2, "XYZ");
	ReadTest(spacePaddedNull, dst1, src3, "XYZ");
	ReadTest(spacePaddedNull, dst1, src4, "xy\t");

	// Check reading of space-padded strings
	ReadTest(spacePadded, dst1, src0, " X X");
	ReadTest(spacePadded, dst1, src1, "X  X");
	ReadTest(spacePadded, dst1, src2, "XYZ");
	ReadTest(spacePadded, dst1, src3, "XYZ!");
	ReadTest(spacePadded, dst1, src4, "xy\t\n");

	///////////////////////////////

	// Check reading of null-terminated string into smaller buffer
	ReadTest(nullTerminated, dst2, src0, "");
	ReadTest(nullTerminated, dst2, src1, "X ");
	ReadTest(nullTerminated, dst2, src2, "XY");
	ReadTest(nullTerminated, dst2, src3, "XY");
	ReadTest(nullTerminated, dst2, src4, "xy");

	// Check reading of string that should be null-terminated, but is maybe too long to still hold the null character.
	ReadTest(maybeNullTerminated, dst2, src0, "");
	ReadTest(maybeNullTerminated, dst2, src1, "X ");
	ReadTest(maybeNullTerminated, dst2, src2, "XY");
	ReadTest(maybeNullTerminated, dst2, src3, "XY");
	ReadTest(maybeNullTerminated, dst2, src4, "xy");

	// Check reading of space-padded strings with ignored last character
	ReadTest(spacePaddedNull, dst2, src0, " X");
	ReadTest(spacePaddedNull, dst2, src1, "X");
	ReadTest(spacePaddedNull, dst2, src2, "XY");
	ReadTest(spacePaddedNull, dst2, src3, "XY");
	ReadTest(spacePaddedNull, dst2, src4, "xy");

	// Check reading of space-padded strings
	ReadTest(spacePadded, dst2, src0, " X");
	ReadTest(spacePadded, dst2, src1, "X ");
	ReadTest(spacePadded, dst2, src2, "XY");
	ReadTest(spacePadded, dst2, src3, "XY");
	ReadTest(spacePadded, dst2, src4, "xy");

	///////////////////////////////

	// Check writing of null-terminated string into larger buffer
	WriteTest(nullTerminated, dst1, src0, "");
	WriteTest(nullTerminated, dst1, src1, "X ");
	WriteTest(nullTerminated, dst1, src2, "XYZ ");
	WriteTest(nullTerminated, dst1, src3, "XYZ!");

	// Check writing of string that should be null-terminated, but is maybe too long to still hold the null character.
	WriteTest(maybeNullTerminated, dst1, src0, "");
	WriteTest(maybeNullTerminated, dst1, src1, "X ");
	WriteTest(maybeNullTerminated, dst1, src2, "XYZ ");
	WriteTest(maybeNullTerminated, dst1, src3, "XYZ!");

	// Check writing of space-padded strings with last character set to null
	WriteTest(spacePaddedNull, dst1, src0, "     ");
	WriteTest(spacePaddedNull, dst1, src1, "X    ");
	WriteTest(spacePaddedNull, dst1, src2, "XYZ  ");
	WriteTest(spacePaddedNull, dst1, src3, "XYZ! ");

	// Check writing of space-padded strings
	WriteTest(spacePadded, dst1, src0, "      ");
	WriteTest(spacePadded, dst1, src1, "X     ");
	WriteTest(spacePadded, dst1, src2, "XYZ   ");
	WriteTest(spacePadded, dst1, src3, "XYZ!  ");

	///////////////////////////////

	// Check writing of null-terminated string into smaller buffer
	WriteTest(nullTerminated, dst2, src0, "");
	WriteTest(nullTerminated, dst2, src1, "X ");
	WriteTest(nullTerminated, dst2, src2, "XY");
	WriteTest(nullTerminated, dst2, src3, "XY");

	// Check writing of string that should be null-terminated, but is maybe too long to still hold the null character.
	WriteTest(maybeNullTerminated, dst2, src0, "");
	WriteTest(maybeNullTerminated, dst2, src1, "X ");
	WriteTest(maybeNullTerminated, dst2, src2, "XYZ");
	WriteTest(maybeNullTerminated, dst2, src3, "XYZ");

	// Check writing of space-padded strings with last character set to null
	WriteTest(spacePaddedNull, dst2, src0, "  ");
	WriteTest(spacePaddedNull, dst2, src1, "X ");
	WriteTest(spacePaddedNull, dst2, src2, "XY");
	WriteTest(spacePaddedNull, dst2, src3, "XY");

	// Check writing of space-padded strings
	WriteTest(spacePadded, dst2, src0, "   ");
	WriteTest(spacePadded, dst2, src1, "X  ");
	WriteTest(spacePadded, dst2, src2, "XYZ");
	WriteTest(spacePadded, dst2, src3, "XYZ");

#undef ReadTest
#undef WriteTest

	{

		std::string dststring;
		std::string src0string = std::string(src0, std::size(src0));
		std::string src1string = std::string(src1, std::size(src1));
		std::string src2string = std::string(src2, std::size(src2));
		std::string src3string = std::string(src3, std::size(src3));

#define ReadTest(mode, dst, src, expectedResult) \
	dst = mpt::String::ReadBuf(mpt::String:: mode , src); \
	VERIFY_EQUAL_NONCONT(dst, expectedResult); /* Ensure that the strings are identical */ \
	/**/

#define WriteTest(mode, dst, src, expectedResult) \
	std::memset(dst, 0x7f, sizeof(dst)); \
	mpt::String::WriteBuf(mpt::String:: mode , dst) = src; \
	VERIFY_EQUAL_NONCONT(strncmp(dst, expectedResult, std::size(dst)), 0);  /* Ensure that the strings are identical */ \
	for(size_t i = Test::strnlen(dst, std::size(dst)); i < std::size(dst); i++) \
		VERIFY_EQUAL_NONCONT(dst[i], '\0'); /* Ensure that rest of the buffer is completely nulled */ \
	/**/

		// Check reading of null-terminated string into std::string
		ReadTest(nullTerminated, dststring, src0, "");
		ReadTest(nullTerminated, dststring, src1, "X ");
		ReadTest(nullTerminated, dststring, src2, "XYZ");
		ReadTest(nullTerminated, dststring, src3, "XYZ");
		ReadTest(nullTerminated, dststring, src4, "xy\t");

		// Check reading of string that should be null-terminated, but is maybe too long to still hold the null character.
		ReadTest(maybeNullTerminated, dststring, src0, "");
		ReadTest(maybeNullTerminated, dststring, src1, "X ");
		ReadTest(maybeNullTerminated, dststring, src2, "XYZ ");
		ReadTest(maybeNullTerminated, dststring, src3, "XYZ!");
		ReadTest(maybeNullTerminated, dststring, src4, "xy\t\n");

		// Check reading of space-padded strings with ignored last character
		ReadTest(spacePaddedNull, dststring, src0, " X");
		ReadTest(spacePaddedNull, dststring, src1, "X");
		ReadTest(spacePaddedNull, dststring, src2, "XYZ");
		ReadTest(spacePaddedNull, dststring, src3, "XYZ");
		ReadTest(spacePaddedNull, dststring, src4, "xy\t");

		// Check reading of space-padded strings
		ReadTest(spacePadded, dststring, src0, " X X");
		ReadTest(spacePadded, dststring, src1, "X  X");
		ReadTest(spacePadded, dststring, src2, "XYZ");
		ReadTest(spacePadded, dststring, src3, "XYZ!");
		ReadTest(spacePadded, dststring, src4, "xy\t\n");

		///////////////////////////////

		// Check writing of null-terminated string into larger buffer
		WriteTest(nullTerminated, dst1, src0string, "");
		WriteTest(nullTerminated, dst1, src1string, "X ");
		WriteTest(nullTerminated, dst1, src2string, "XYZ ");
		WriteTest(nullTerminated, dst1, src3string, "XYZ!");

		// Check writing of string that should be null-terminated, but is maybe too long to still hold the null character.
		WriteTest(maybeNullTerminated, dst1, src0string, "");
		WriteTest(maybeNullTerminated, dst1, src1string, "X ");
		WriteTest(maybeNullTerminated, dst1, src2string, "XYZ ");
		WriteTest(maybeNullTerminated, dst1, src3string, "XYZ!");

		// Check writing of space-padded strings with last character set to null
		WriteTest(spacePaddedNull, dst1, src0string, "     ");
		WriteTest(spacePaddedNull, dst1, src1string, "X    ");
		WriteTest(spacePaddedNull, dst1, src2string, "XYZ  ");
		WriteTest(spacePaddedNull, dst1, src3string, "XYZ! ");

		// Check writing of space-padded strings
		WriteTest(spacePadded, dst1, src0string, "      ");
		WriteTest(spacePadded, dst1, src1string, "X     ");
		WriteTest(spacePadded, dst1, src2string, "XYZ   ");
		WriteTest(spacePadded, dst1, src3string, "XYZ!  ");

		///////////////////////////////

		// Check writing of null-terminated string into smaller buffer
		WriteTest(nullTerminated, dst2, src0string, "");
		WriteTest(nullTerminated, dst2, src1string, "X ");
		WriteTest(nullTerminated, dst2, src2string, "XY");
		WriteTest(nullTerminated, dst2, src3string, "XY");

		// Check writing of string that should be null-terminated, but is maybe too long to still hold the null character.
		WriteTest(maybeNullTerminated, dst2, src0string, "");
		WriteTest(maybeNullTerminated, dst2, src1string, "X ");
		WriteTest(maybeNullTerminated, dst2, src2string, "XYZ");
		WriteTest(maybeNullTerminated, dst2, src3string, "XYZ");

		// Check writing of space-padded strings with last character set to null
		WriteTest(spacePaddedNull, dst2, src0string, "  ");
		WriteTest(spacePaddedNull, dst2, src1string, "X ");
		WriteTest(spacePaddedNull, dst2, src2string, "XY");
		WriteTest(spacePaddedNull, dst2, src3string, "XY");

		// Check writing of space-padded strings
		WriteTest(spacePadded, dst2, src0string, "   ");
		WriteTest(spacePadded, dst2, src1string, "X  ");
		WriteTest(spacePadded, dst2, src2string, "XYZ");
		WriteTest(spacePadded, dst2, src3string, "XYZ");

		///////////////////////////////

#undef ReadTest
#undef WriteTest

	}

	{
	
		char s0[4] = {'\0', 'X', ' ', 'X' };
		char s2[4] = { 'X', ' ','\0', 'X' };
		char s4[4] = { 'X', 'Y', 'Z', ' ' };
	
		char d2[2] = {'\0','\0'};
		char d3[3] = {'\0','\0','\0'};
		char d4[4] = {'\0','\0','\0','\0'};
		char d5[5] = {'\0','\0','\0','\0','\0'};
	
		#define CopyTest(dst, src, expectedResult) \
			std::memset(dst, 0x7f, sizeof(dst)); \
			mpt::String::WriteAutoBuf(dst) = mpt::String::ReadAutoBuf(src); \
			VERIFY_EQUAL_NONCONT(strncmp(dst, expectedResult, std::size(dst)), 0); /* Ensure that the strings are identical */ \
			for(size_t i = strlen(dst); i < std::size(dst); i++) \
				VERIFY_EQUAL_NONCONT(dst[i], '\0'); /* Ensure that rest of the buffer is completely nulled */ \
			/**/

		CopyTest(d2, s0, "");
		CopyTest(d2, s2, "X");
		CopyTest(d2, s4, "X");
		CopyTest(d3, s0, "");
		CopyTest(d3, s2, "X ");
		CopyTest(d3, s4, "XY");
		CopyTest(d4, s0, "");
		CopyTest(d4, s2, "X ");
		CopyTest(d4, s4, "XYZ");
		CopyTest(d5, s0, "");
		CopyTest(d5, s2, "X ");
		CopyTest(d5, s4, "XYZ ");

		#undef CopyTest

		#define CopyTestN(dst, src, len, expectedResult) \
			std::memset(dst, 0x7f, sizeof(dst)); \
			mpt::String::WriteAutoBuf(dst) = mpt::String::ReadAutoBuf(src, std::min(std::size(src), static_cast<std::size_t>(len))); \
			VERIFY_EQUAL_NONCONT(strncmp(dst, expectedResult, std::size(dst)), 0); /* Ensure that the strings are identical */ \
			for(size_t i = strlen(dst); i < std::size(dst); i++) \
				VERIFY_EQUAL_NONCONT(dst[i], '\0'); /* Ensure that rest of the buffer is completely nulled */ \
			/**/

		CopyTestN(d2, s0, 1, "");
		CopyTestN(d2, s2, 1, "X");
		CopyTestN(d2, s4, 1, "X");
		CopyTestN(d3, s0, 1, "");
		CopyTestN(d3, s2, 1, "X");
		CopyTestN(d3, s4, 1, "X");
		CopyTestN(d4, s0, 1, "");
		CopyTestN(d4, s2, 1, "X");
		CopyTestN(d4, s4, 1, "X");
		CopyTestN(d5, s0, 1, "");
		CopyTestN(d5, s2, 1, "X");
		CopyTestN(d5, s4, 1, "X");

		CopyTestN(d2, s0, 2, "");
		CopyTestN(d2, s2, 2, "X");
		CopyTestN(d2, s4, 2, "X");
		CopyTestN(d3, s0, 2, "");
		CopyTestN(d3, s2, 2, "X ");
		CopyTestN(d3, s4, 2, "XY");
		CopyTestN(d4, s0, 2, "");
		CopyTestN(d4, s2, 2, "X ");
		CopyTestN(d4, s4, 2, "XY");
		CopyTestN(d5, s0, 2, "");
		CopyTestN(d5, s2, 2, "X ");
		CopyTestN(d5, s4, 2, "XY");

		CopyTestN(d2, s0, 3, "");
		CopyTestN(d2, s2, 3, "X");
		CopyTestN(d2, s4, 3, "X");
		CopyTestN(d3, s0, 3, "");
		CopyTestN(d3, s2, 3, "X ");
		CopyTestN(d3, s4, 3, "XY");
		CopyTestN(d4, s0, 3, "");
		CopyTestN(d4, s2, 3, "X ");
		CopyTestN(d4, s4, 3, "XYZ");
		CopyTestN(d5, s0, 3, "");
		CopyTestN(d5, s2, 3, "X ");
		CopyTestN(d5, s4, 3, "XYZ");

		CopyTestN(d2, s0, 4, "");
		CopyTestN(d2, s2, 4, "X");
		CopyTestN(d2, s4, 4, "X");
		CopyTestN(d3, s0, 4, "");
		CopyTestN(d3, s2, 4, "X ");
		CopyTestN(d3, s4, 4, "XY");
		CopyTestN(d4, s0, 4, "");
		CopyTestN(d4, s2, 4, "X ");
		CopyTestN(d4, s4, 4, "XYZ");
		CopyTestN(d5, s0, 4, "");
		CopyTestN(d5, s2, 4, "X ");
		CopyTestN(d5, s4, 4, "XYZ ");

		CopyTestN(d2, s0, 5, "");
		CopyTestN(d2, s2, 5, "X");
		CopyTestN(d2, s4, 5, "X");
		CopyTestN(d3, s0, 5, "");
		CopyTestN(d3, s2, 5, "X ");
		CopyTestN(d3, s4, 5, "XY");
		CopyTestN(d4, s0, 5, "");
		CopyTestN(d4, s2, 5, "X ");
		CopyTestN(d4, s4, 5, "XYZ");
		CopyTestN(d5, s0, 5, "");
		CopyTestN(d5, s2, 5, "X ");
		CopyTestN(d5, s4, 5, "XYZ ");

		#undef CopyTest

	}

}


static MPT_NOINLINE void TestSampleConversion()
{
	std::vector<uint8> sourceBufContainer(65536 * 4);
	std::vector<uint8> targetBufContainer(65536 * 6);

	uint8 *sourceBuf = &(sourceBufContainer[0]);
	void *targetBuf = &(targetBufContainer[0]);

	// Signed 8-Bit Integer PCM
	// Unsigned 8-Bit Integer PCM
	// Delta 8-Bit Integer PCM
	{
		uint8 *source8 = sourceBuf;
		for(size_t i = 0; i < 256; i++)
		{
			source8[i] = static_cast<uint8>(i);
		}

		int8 *signed8 = static_cast<int8 *>(targetBuf);
		uint8 *unsigned8 = static_cast<uint8 *>(targetBuf) + 256;
		int8 *delta8 = static_cast<int8 *>(targetBuf) + 512;
		int8 delta = 0;
		CopySample<SC::DecodeInt8>(signed8, 256, 1, mpt::byte_cast<const std::byte *>(source8), 256, 1);
		CopySample<SC::DecodeUint8>(reinterpret_cast<int8 *>(unsigned8), 256, 1, mpt::byte_cast<const std::byte *>(source8), 256, 1);
		CopySample<SC::DecodeInt8Delta>(delta8, 256, 1, mpt::byte_cast<const std::byte *>(source8), 256, 1);

		for(size_t i = 0; i < 256; i++)
		{
			delta += static_cast<int8>(i);
			VERIFY_EQUAL_QUIET_NONCONT(signed8[i], static_cast<int8>(i));
			VERIFY_EQUAL_QUIET_NONCONT(unsigned8[i], static_cast<uint8>(i + 0x80u));
			VERIFY_EQUAL_QUIET_NONCONT(delta8[i], static_cast<int8>(delta));
		}
	}

	// Signed 16-Bit Integer PCM
	// Unsigned 16-Bit Integer PCM
	// Delta 16-Bit Integer PCM
	{
		// Little Endian

		uint8 *source16 = sourceBuf;
		for(size_t i = 0; i < 65536; i++)
		{
			source16[i * 2 + 0] = static_cast<uint8>(i & 0xFF);
			source16[i * 2 + 1] = static_cast<uint8>(i >> 8);
		}

		int16 *signed16 = static_cast<int16 *>(targetBuf);
		uint16 *unsigned16 = static_cast<uint16 *>(targetBuf) + 65536;
		int16 *delta16 = static_cast<int16 *>(targetBuf) + 65536 * 2;
		int16 delta = 0;
		CopySample<SC::DecodeInt16<0, littleEndian16> >(signed16, 65536, 1, mpt::byte_cast<const std::byte *>(source16), 65536 * 2, 1);
		CopySample<SC::DecodeInt16<0x8000u, littleEndian16> >(reinterpret_cast<int16*>(unsigned16), 65536, 1, mpt::byte_cast<const std::byte *>(source16), 65536 * 2, 1);
		CopySample<SC::DecodeInt16Delta<littleEndian16> >(delta16, 65536, 1, mpt::byte_cast<const std::byte *>(source16), 65536 * 2, 1);

		for(size_t i = 0; i < 65536; i++)
		{
			delta += static_cast<int16>(i);
			VERIFY_EQUAL_QUIET_NONCONT(signed16[i], static_cast<int16>(i));
			VERIFY_EQUAL_QUIET_NONCONT(unsigned16[i], static_cast<uint16>(i + 0x8000u));
			VERIFY_EQUAL_QUIET_NONCONT(delta16[i], static_cast<int16>(delta));
		}

		// Big Endian

		for(size_t i = 0; i < 65536; i++)
		{
			source16[i * 2 + 0] = static_cast<uint8>(i >> 8);
			source16[i * 2 + 1] = static_cast<uint8>(i & 0xFF);
		}

		CopySample<SC::DecodeInt16<0, bigEndian16> >(signed16, 65536, 1, mpt::byte_cast<const std::byte *>(source16), 65536 * 2, 1);
		CopySample<SC::DecodeInt16<0x8000u, bigEndian16> >(reinterpret_cast<int16*>(unsigned16), 65536, 1, mpt::byte_cast<const std::byte *>(source16), 65536 * 2, 1);
		CopySample<SC::DecodeInt16Delta<bigEndian16> >(delta16, 65536, 1, mpt::byte_cast<const std::byte *>(source16), 65536 * 2, 1);

		delta = 0;
		for(size_t i = 0; i < 65536; i++)
		{
			delta += static_cast<int16>(i);
			VERIFY_EQUAL_QUIET_NONCONT(signed16[i], static_cast<int16>(i));
			VERIFY_EQUAL_QUIET_NONCONT(unsigned16[i], static_cast<uint16>(i + 0x8000u));
			VERIFY_EQUAL_QUIET_NONCONT(delta16[i], static_cast<int16>(delta));
		}

	}

	// Signed 24-Bit Integer PCM
	{
		uint8 *source24 = sourceBuf;
		for(size_t i = 0; i < 65536; i++)
		{
			source24[i * 3 + 0] = 0;
			source24[i * 3 + 1] = static_cast<uint8>(i & 0xFF);
			source24[i * 3 + 2] = static_cast<uint8>(i >> 8);
		}

		int16 *truncated16 = static_cast<int16 *>(targetBuf);
		ModSample sample;
		sample.Initialize();
		sample.nLength = 65536;
		sample.uFlags.set(CHN_16BIT);
		sample.pData.pSample = (static_cast<int16 *>(targetBuf) + 65536);
		CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, littleEndian24> > >(sample, mpt::byte_cast<const std::byte *>(source24), 3*65536);
		CopySample<SC::ConversionChain<SC::ConvertShift<int16, int32, 16>, SC::DecodeInt24<0, littleEndian24> > >(truncated16, 65536, 1, mpt::byte_cast<const std::byte *>(source24), 65536 * 3, 1);

		for(size_t i = 0; i < 65536; i++)
		{
			VERIFY_EQUAL_QUIET_NONCONT(sample.sample16()[i], static_cast<int16>(i));
			VERIFY_EQUAL_QUIET_NONCONT(truncated16[i], static_cast<int16>(i));
		}
	}

	// Float 32-Bit
	{
		uint8 *source32 = sourceBuf;
		for(size_t i = 0; i < 65536; i++)
		{
			IEEE754binary32BE floatbits = IEEE754binary32BE((static_cast<float>(i) / 65536.0f) - 0.5f);
			source32[i * 4 + 0] = mpt::byte_cast<uint8>(floatbits.GetByte(0));
			source32[i * 4 + 1] = mpt::byte_cast<uint8>(floatbits.GetByte(1));
			source32[i * 4 + 2] = mpt::byte_cast<uint8>(floatbits.GetByte(2));
			source32[i * 4 + 3] = mpt::byte_cast<uint8>(floatbits.GetByte(3));
		}

		int16 *truncated16 = static_cast<int16 *>(targetBuf);
		ModSample sample;
		sample.Initialize();
		sample.nLength = 65536;
		sample.uFlags.set(CHN_16BIT);
		sample.pData.pSample = static_cast<int16 *>(targetBuf) + 65536;
		CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, float32>, SC::DecodeFloat32<bigEndian32> > >(sample, mpt::byte_cast<const std::byte *>(source32), 4*65536);
		CopySample<SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeFloat32<bigEndian32> > >(truncated16, 65536, 1, mpt::byte_cast<const std::byte *>(source32), 65536 * 4, 1);

		for(size_t i = 0; i < 65536; i++)
		{
			VERIFY_EQUAL_QUIET_NONCONT(sample.sample16()[i], static_cast<int16>(i - 0x8000u));
			VERIFY_EQUAL_QUIET_NONCONT(std::abs(truncated16[i] - static_cast<int16>((i - 0x8000u) / 2)) <= 1, true);
		}
	}

	// ALaw
	{
		for(unsigned int i = 0; i < 256; ++i)
		{
			std::byte in = mpt::byte_cast<std::byte>(static_cast<uint8>(i));
			std::byte out = SC::EncodeALaw{}(SC::DecodeInt16ALaw{}(&in));
			VERIFY_EQUAL_NONCONT(in, out);
		}
		VERIFY_EQUAL_NONCONT(SC::EncodeALaw{}(-32768), SC::EncodeALaw{}(-32256));
		VERIFY_EQUAL_NONCONT(SC::EncodeALaw{}(-32767), SC::EncodeALaw{}(-32256));
		VERIFY_EQUAL_NONCONT(SC::EncodeALaw{}(-1), SC::EncodeALaw{}(-8));
		VERIFY_EQUAL_NONCONT(SC::EncodeALaw{}(0), SC::EncodeALaw{}(8));
		VERIFY_EQUAL_NONCONT(SC::EncodeALaw{}(1), SC::EncodeALaw{}(8));
		VERIFY_EQUAL_NONCONT(SC::EncodeALaw{}(32766), SC::EncodeALaw{}(32256));
		VERIFY_EQUAL_NONCONT(SC::EncodeALaw{}(32767), SC::EncodeALaw{}(32256));
#if 0
		// compare with reference impl
		for (int i = -32768; i <= 32767; ++i)
		{
			VERIFY_EQUAL_NONCONT(SC::EncodeALaw{}(i), mpt::byte_cast<std::byte>(alaw_encode(i)));
		}
#endif
	}

	// uLaw
	{
		for(unsigned int i = 0; i < 256; ++i)
		{
			std::byte in = mpt::byte_cast<std::byte>(static_cast<uint8>(i));
			std::byte out = SC::EncodeuLaw{}(SC::DecodeInt16uLaw{}(&in));
			VERIFY_EQUAL_NONCONT(in, out);
		}
#if 0
		// compare with reference impl
		/*
		bool lastMatch = true;
		*/
		for(int i = -32768; i <= 32767; ++i)
		{
			/*
			uint8 mine = mpt::byte_cast<uint8>(SC::EncodeuLaw{}(i));
			uint8 ref = ulaw_encode(i);
			if(lastMatch)
			{
				if(mine == ref)
				{
					VERIFY_EQUAL_NONCONT(mine, ref);
					lastMatch = true;
				} else
				{
					VERIFY_EQUAL_NONCONT(std::abs(static_cast<int>(mine) - static_cast<int>(ref)) <= 1, true);
					lastMatch = false;
				}
			} else
			{
				VERIFY_EQUAL_NONCONT(mine, ref);
				lastMatch = true;
			}
			*/
			//MPT_LOG_GLOBAL(LogNotification, "test", MPT_UFORMAT("{} {} {}")(i, ulaw_encode(i), mpt::byte_cast<uint8>(SC::EncodeuLaw{}(i))));
			VERIFY_EQUAL_NONCONT(SC::EncodeuLaw{}(i), mpt::byte_cast<std::byte>(ulaw_encode(i)));
		}
#endif
	}

	// Range checks
	{
		int8 oneSample = 1;
		char *signed8 = reinterpret_cast<char *>(targetBuf);
		memset(signed8, 0, 4);
		CopySample<SC::DecodeInt8>(reinterpret_cast<int8*>(targetBuf), 4, 1, reinterpret_cast<const std::byte*>(&oneSample), sizeof(oneSample), 1);
		VERIFY_EQUAL_NONCONT(signed8[0], 1);
		VERIFY_EQUAL_NONCONT(signed8[1], 0);
		VERIFY_EQUAL_NONCONT(signed8[2], 0);
		VERIFY_EQUAL_NONCONT(signed8[3], 0);
	}

	// Dither
	{
		std::vector<MixSampleInt> buffer(64);
		DithersOpenMPT dithers(mpt::global_random_device(), 2 /* DitherModPlug */ , 2);
		for(std::size_t i = 0; i < 64; ++i)
		{
			std::visit(
				[&](auto &dither)
				{
					buffer[i] = dither.template process<16>(0, buffer[i]);
				},
				dithers.Variant()
			);
		}
		std::vector<MixSampleInt> expected = {
		    727,
		    -557,
		    -552,
		    -727,
		    439,
		    405,
		    703,
		    -337,
		    235,
		    -776,
		    -458,
		    905,
		    -110,
		    158,
		    374,
		    -362,
		    283,
		    306,
		    710,
		    304,
		    -608,
		    536,
		    -501,
		    -593,
		    -349,
		    812,
		    916,
		    53,
		    -953,
		    881,
		    -236,
		    -20,
		    -623,
		    -895,
		    -302,
		    -415,
		    899,
		    -948,
		    -766,
		    -186,
		    -390,
		    -169,
		    253,
		    -622,
		    -769,
		    -1001,
		    1019,
		    787,
		    -239,
		    718,
		    -423,
		    988,
		    -91,
		    763,
		    -933,
		    -510,
		    484,
		    794,
		    -340,
		    552,
		    866,
		    -608,
		    35,
		    395};
		for(std::size_t i = 0; i < 64; ++i)
		{
			VERIFY_EQUAL_QUIET_NONCONT(buffer[i], expected[i]);
		}
	}
}


} // namespace Test

OPENMPT_NAMESPACE_END

#else //Case: ENABLE_TESTS is not defined.

OPENMPT_NAMESPACE_BEGIN

namespace Test {

void DoTests()
{
	return;
}

} // namespace Test

OPENMPT_NAMESPACE_END

#endif
