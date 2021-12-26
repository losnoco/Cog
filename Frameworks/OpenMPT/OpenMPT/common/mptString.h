/*
 * mptString.h
 * ----------
 * Purpose: Small string-related utilities, number and message formatting.
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/alloc.hpp"
#include "mpt/base/span.hpp"
#include "mpt/string/types.hpp"
#include "mpt/string/utility.hpp"

#include "mptBaseTypes.h"

#include <algorithm>
#include <limits>
#include <string>
#include <string_view>

#include <cstring>



OPENMPT_NAMESPACE_BEGIN


namespace mpt
{



namespace String
{


template <typename Tstring, typename Tstring2, typename Tstring3>
inline Tstring Replace(Tstring str, const Tstring2 &oldStr, const Tstring3 &newStr)
{
	return mpt::replace(str, oldStr, newStr);
}


} // namespace String


enum class Charset {

	UTF8,

	ASCII, // strictly 7-bit ASCII

	ISO8859_1,
	ISO8859_15,

	CP850,
	CP437,
	CP437AMS,
	CP437AMS2,

	Windows1252,

#if defined(MPT_ENABLE_CHARSET_LOCALE)
	Locale, // CP_ACP on windows, current C locale otherwise
#endif // MPT_ENABLE_CHARSET_LOCALE

};



// source code / preprocessor (i.e. # token)
inline constexpr Charset CharsetSource = Charset::ASCII;

// debug log files
inline constexpr Charset CharsetLogfile = Charset::UTF8;

// std::clog / std::cout / std::cerr
#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS && defined(MPT_ENABLE_CHARSET_LOCALE)
inline constexpr Charset CharsetStdIO = Charset::Locale;
#else
inline constexpr Charset CharsetStdIO = Charset::UTF8;
#endif

// getenv
#if defined(MPT_ENABLE_CHARSET_LOCALE)
inline constexpr Charset CharsetEnvironment = Charset::Locale;
#else
inline constexpr Charset CharsetEnvironment = Charset::UTF8;
#endif

// std::exception::what()
#if defined(MPT_ENABLE_CHARSET_LOCALE)
inline constexpr Charset CharsetException = Charset::Locale;
#else
inline constexpr Charset CharsetException = Charset::UTF8;
#endif



// Checks if the std::string represents an UTF8 string.
// This is currently implemented as converting to std::wstring and back assuming UTF8 both ways,
// and comparing the result to the original string.
// Caveats:
//  - can give false negatives because of possible unicode normalization during conversion
//  - can give false positives if the 8bit encoding contains high-ascii only in valid utf8 groups
//  - slow because of double conversion
bool IsUTF8(const std::string &str);



#if MPT_WSTRING_CONVERT
// Convert to a wide character string.
// The wide encoding is UTF-16 or UTF-32, based on sizeof(wchar_t).
// If str does not contain any invalid characters, this conversion is lossless.
// Invalid source bytes will be replaced by some replacement character or string.
inline std::wstring ToWide(const std::wstring &str) { return str; }
inline std::wstring ToWide(const wchar_t * str) { return (str ? std::wstring(str) : std::wstring()); }
std::wstring ToWide(Charset from, const std::string &str);
inline std::wstring ToWide(Charset from, const char * str) { return ToWide(from, str ? std::string(str) : std::string()); }
#if defined(MPT_ENABLE_CHARSET_LOCALE)
std::wstring ToWide(const mpt::lstring &str);
#endif // MPT_ENABLE_CHARSET_LOCALE
#endif

// Convert to a string encoded in the 'to'-specified character set.
// If str does not contain any invalid characters,
// this conversion will be lossless iff, and only iff,
// 'to' is UTF8.
// Invalid source bytes or characters that are not representable in the
// destination charset will be replaced by some replacement character or string.
#if MPT_WSTRING_CONVERT
std::string ToCharset(Charset to, const std::wstring &str);
inline std::string ToCharset(Charset to, const wchar_t * str) { return ToCharset(to, str ? std::wstring(str) : std::wstring()); }
#endif
std::string ToCharset(Charset to, Charset from, const std::string &str);
inline std::string ToCharset(Charset to, Charset from, const char * str) { return ToCharset(to, from, str ? std::string(str) : std::string()); }
#if defined(MPT_ENABLE_CHARSET_LOCALE)
std::string ToCharset(Charset to, const mpt::lstring &str);
#endif // MPT_ENABLE_CHARSET_LOCALE

#if defined(MPT_ENABLE_CHARSET_LOCALE)
#if MPT_WSTRING_CONVERT
mpt::lstring ToLocale(const std::wstring &str);
inline mpt::lstring ToLocale(const wchar_t * str) { return ToLocale(str ? std::wstring(str): std::wstring()); }
#endif
mpt::lstring ToLocale(Charset from, const std::string &str);
inline mpt::lstring ToLocale(Charset from, const char * str) { return ToLocale(from, str ? std::string(str): std::string()); }
inline mpt::lstring ToLocale(const mpt::lstring &str) { return str; }
#endif // MPT_ENABLE_CHARSET_LOCALE

#if MPT_OS_WINDOWS
#if MPT_WSTRING_CONVERT
mpt::winstring ToWin(const std::wstring &str);
inline mpt::winstring ToWin(const wchar_t * str) { return ToWin(str ? std::wstring(str): std::wstring()); }
#endif
mpt::winstring ToWin(Charset from, const std::string &str);
inline mpt::winstring ToWin(Charset from, const char * str) { return ToWin(from, str ? std::string(str): std::string()); }
#if defined(MPT_ENABLE_CHARSET_LOCALE)
mpt::winstring ToWin(const mpt::lstring &str);
#endif // MPT_ENABLE_CHARSET_LOCALE
#endif // MPT_OS_WINDOWS


#if defined(MPT_WITH_MFC)
#if !(MPT_WSTRING_CONVERT)
#error "MFC depends on MPT_WSTRING_CONVERT)"
#endif

// Convert to a MFC CString. The CString encoding depends on UNICODE.
// This should also be used when converting to TCHAR strings.
// If UNICODE is defined, this is a completely lossless operation.
inline CString ToCString(const CString &str) { return str; }
CString ToCString(const std::wstring &str);
inline CString ToCString(const wchar_t * str) { return ToCString(str ? std::wstring(str) : std::wstring()); }
CString ToCString(Charset from, const std::string &str);
inline CString ToCString(Charset from, const char * str) { return ToCString(from, str ? std::string(str) : std::string()); }
#if defined(MPT_ENABLE_CHARSET_LOCALE)
CString ToCString(const mpt::lstring &str);
mpt::lstring ToLocale(const CString &str);
#endif // MPT_ENABLE_CHARSET_LOCALE
#if MPT_OS_WINDOWS
mpt::winstring ToWin(const CString &str);
#endif // MPT_OS_WINDOWS

// Convert from a MFC CString. The CString encoding depends on UNICODE.
// This should also be used when converting from TCHAR strings.
// If UNICODE is defined, this is a completely lossless operation.
std::wstring ToWide(const CString &str);
std::string ToCharset(Charset to, const CString &str);

#endif // MPT_WITH_MFC



#define UC_(x)           MPT_UCHAR(x)
#define UL_(x)           MPT_ULITERAL(x)
#define U_(x)            MPT_USTRING(x)



#if MPT_USTRING_MODE_WIDE
#if !(MPT_WSTRING_CONVERT)
#error "MPT_USTRING_MODE_WIDE depends on MPT_WSTRING_CONVERT)"
#endif
inline mpt::ustring ToUnicode(const std::wstring &str) { return str; }
inline mpt::ustring ToUnicode(const wchar_t * str) { return (str ? std::wstring(str) : std::wstring()); }
inline mpt::ustring ToUnicode(Charset from, const std::string &str) { return ToWide(from, str); }
inline mpt::ustring ToUnicode(Charset from, const char * str) { return ToUnicode(from, str ? std::string(str) : std::string()); }
#if defined(MPT_ENABLE_CHARSET_LOCALE)
inline mpt::ustring ToUnicode(const mpt::lstring &str) { return ToWide(str); }
#endif // MPT_ENABLE_CHARSET_LOCALE
#if defined(MPT_WITH_MFC)
inline mpt::ustring ToUnicode(const CString &str) { return ToWide(str); }
#endif // MFC
#else // !MPT_USTRING_MODE_WIDE
inline mpt::ustring ToUnicode(const mpt::ustring &str) { return str; }
#if MPT_WSTRING_CONVERT
mpt::ustring ToUnicode(const std::wstring &str);
inline mpt::ustring ToUnicode(const wchar_t * str) { return ToUnicode(str ? std::wstring(str) : std::wstring()); }
#endif
mpt::ustring ToUnicode(Charset from, const std::string &str);
inline mpt::ustring ToUnicode(Charset from, const char * str) { return ToUnicode(from, str ? std::string(str) : std::string()); }
#if defined(MPT_ENABLE_CHARSET_LOCALE)
mpt::ustring ToUnicode(const mpt::lstring &str);
#endif // MPT_ENABLE_CHARSET_LOCALE
#if defined(MPT_WITH_MFC)
mpt::ustring ToUnicode(const CString &str);
#endif // MPT_WITH_MFC
#endif // MPT_USTRING_MODE_WIDE

#if MPT_USTRING_MODE_WIDE
#if !(MPT_WSTRING_CONVERT)
#error "MPT_USTRING_MODE_WIDE depends on MPT_WSTRING_CONVERT)"
#endif
// nothing, std::wstring overloads will catch all stuff
#else // !MPT_USTRING_MODE_WIDE
#if MPT_WSTRING_CONVERT
std::wstring ToWide(const mpt::ustring &str);
#endif
std::string ToCharset(Charset to, const mpt::ustring &str);
#if defined(MPT_ENABLE_CHARSET_LOCALE)
mpt::lstring ToLocale(const mpt::ustring &str);
#endif // MPT_ENABLE_CHARSET_LOCALE
#if MPT_OS_WINDOWS
mpt::winstring ToWin(const mpt::ustring &str);
#endif // MPT_OS_WINDOWS
#if defined(MPT_WITH_MFC)
CString ToCString(const mpt::ustring &str);
#endif // MPT_WITH_MFC
#endif // MPT_USTRING_MODE_WIDE





// The MPT_UTF8 allows specifying UTF8 char arrays.
// The resulting type is mpt::ustring and the construction might require runtime translation,
// i.e. it is NOT generally available at compile time.
// Use explicit UTF8 encoding,
// i.e. U+00FC (LATIN SMALL LETTER U WITH DIAERESIS) would be written as "\xC3\xBC".
#define MPT_UTF8(x) mpt::ToUnicode(mpt::Charset::UTF8, x)





mpt::ustring ToUnicode(uint16 codepage, mpt::Charset fallback, const std::string &str);





char ToLowerCaseAscii(char c);
char ToUpperCaseAscii(char c);
std::string ToLowerCaseAscii(std::string s);
std::string ToUpperCaseAscii(std::string s);

int CompareNoCaseAscii(const char *a, const char *b, std::size_t n);
int CompareNoCaseAscii(std::string_view a, std::string_view b);
int CompareNoCaseAscii(const std::string &a, const std::string &b);


#if defined(MODPLUG_TRACKER)

mpt::ustring ToLowerCase(const mpt::ustring &s);
mpt::ustring ToUpperCase(const mpt::ustring &s);

#endif // MODPLUG_TRACKER





} // namespace mpt





// The AnyString types are meant to be used as function argument types only,
// and only during the transition phase to all-unicode strings in the whole codebase.
// Using an AnyString type as function argument avoids the need to overload a function for all the
// different string types that we currently have.
// Warning: These types will silently do charset conversions. Only use them when this can be tolerated.

// BasicAnyString is convertable to mpt::ustring and constructable from any string at all.
template <mpt::Charset charset = mpt::Charset::UTF8, bool tryUTF8 = true>
class BasicAnyString : public mpt::ustring
{

private:
	
	static mpt::ustring From8bit(const std::string &str)
	{
		if constexpr(charset == mpt::Charset::UTF8)
		{
			return mpt::ToUnicode(mpt::Charset::UTF8, str);
		} else
		{
			// auto utf8 detection
			if constexpr(tryUTF8)
			{
				if(mpt::IsUTF8(str))
				{
					return mpt::ToUnicode(mpt::Charset::UTF8, str);
				} else
				{
					return mpt::ToUnicode(charset, str);
				}
			} else
			{
				return mpt::ToUnicode(charset, str);
			}
		}
	}

public:

	// 8 bit
	BasicAnyString(const char *str) : mpt::ustring(From8bit(str ? str : std::string())) { }
	BasicAnyString(const std::string str) : mpt::ustring(From8bit(str)) { }

	// locale
#if defined(MPT_ENABLE_CHARSET_LOCALE)
	BasicAnyString(const mpt::lstring str) : mpt::ustring(mpt::ToUnicode(str)) { }
#endif // MPT_ENABLE_CHARSET_LOCALE

	// unicode
	BasicAnyString(const mpt::ustring &str) : mpt::ustring(str) { }
	BasicAnyString(mpt::ustring &&str) : mpt::ustring(std::move(str)) { }
#if MPT_USTRING_MODE_UTF8 && MPT_WSTRING_CONVERT
	BasicAnyString(const std::wstring &str) : mpt::ustring(mpt::ToUnicode(str)) { }
#endif
#if MPT_WSTRING_CONVERT
	BasicAnyString(const wchar_t *str) : mpt::ustring(str ? mpt::ToUnicode(str) : mpt::ustring()) { }
#endif

	// mfc
#if defined(MPT_WITH_MFC)
	BasicAnyString(const CString &str) : mpt::ustring(mpt::ToUnicode(str)) { }
#endif // MPT_WITH_MFC

	// fallback for custom string types
	template <typename Tstring> BasicAnyString(const Tstring &str) : mpt::ustring(mpt::ToUnicode(str)) { }
	template <typename Tstring> BasicAnyString(Tstring &&str) : mpt::ustring(mpt::ToUnicode(std::forward<Tstring>(str))) { }

};

// AnyUnicodeString is convertable to mpt::ustring and constructable from any unicode string,
class AnyUnicodeString : public mpt::ustring
{

public:

	// locale
#if defined(MPT_ENABLE_CHARSET_LOCALE)
	AnyUnicodeString(const mpt::lstring &str) : mpt::ustring(mpt::ToUnicode(str)) { }
#endif // MPT_ENABLE_CHARSET_LOCALE

	// unicode
	AnyUnicodeString(const mpt::ustring &str) : mpt::ustring(str) { }
	AnyUnicodeString(mpt::ustring &&str) : mpt::ustring(std::move(str)) { }
#if MPT_USTRING_MODE_UTF8 && MPT_WSTRING_CONVERT
	AnyUnicodeString(const std::wstring &str) : mpt::ustring(mpt::ToUnicode(str)) { }
#endif
#if MPT_WSTRING_CONVERT
	AnyUnicodeString(const wchar_t *str) : mpt::ustring(str ? mpt::ToUnicode(str) : mpt::ustring()) { }
#endif

	// mfc
#if defined(MPT_WITH_MFC)
	AnyUnicodeString(const CString &str) : mpt::ustring(mpt::ToUnicode(str)) { }
#endif // MPT_WITH_MFC

	// fallback for custom string types
	template <typename Tstring> AnyUnicodeString(const Tstring &str) : mpt::ustring(mpt::ToUnicode(str)) { }
	template <typename Tstring> AnyUnicodeString(Tstring &&str) : mpt::ustring(mpt::ToUnicode(std::forward<Tstring>(str))) { }

};

// AnyString
// Try to do the smartest auto-magic we can do.
#if defined(MPT_ENABLE_CHARSET_LOCALE)
using AnyString = BasicAnyString<mpt::Charset::Locale, true>;
#elif MPT_OS_WINDOWS
using AnyString = BasicAnyString<mpt::Charset::Windows1252, true>;
#else
using AnyString = BasicAnyString<mpt::Charset::ISO8859_1, true>;
#endif

// AnyStringLocale
// char-based strings are assumed to be in locale encoding.
#if defined(MPT_ENABLE_CHARSET_LOCALE)
using AnyStringLocale = BasicAnyString<mpt::Charset::Locale, false>;
#else
using AnyStringLocale = BasicAnyString<mpt::Charset::UTF8, false>;
#endif

// AnyStringUTF8orLocale
// char-based strings are tried in UTF8 first, if this fails, locale is used.
#if defined(MPT_ENABLE_CHARSET_LOCALE)
using AnyStringUTF8orLocale = BasicAnyString<mpt::Charset::Locale, true>;
#else
using AnyStringUTF8orLocale = BasicAnyString<mpt::Charset::UTF8, false>;
#endif

// AnyStringUTF8
// char-based strings are assumed to be in UTF8.
using AnyStringUTF8 = BasicAnyString<mpt::Charset::UTF8, false>;



OPENMPT_NAMESPACE_END
