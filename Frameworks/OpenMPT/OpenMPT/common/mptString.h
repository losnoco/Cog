/*
 * mptString.h
 * ----------
 * Purpose: Small string-related utilities, number and message formatting.
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "mptAlloc.h"
#include "mptBaseTypes.h"
#include "mptSpan.h"

#include <algorithm>
#include <limits>
#include <string>
#include <string_view>

#include <cstring>


OPENMPT_NAMESPACE_BEGIN


namespace mpt
{



template <typename T> inline span<T> as_span(std::basic_string<T> & str) { return span<T>(&(str[0]), str.length()); }

template <typename T> inline span<const T> as_span(const std::basic_string<T> & str) { return span<const T>(&(str[0]), str.length()); }



template <typename T> inline std::vector<typename std::remove_const<T>::type> make_vector(const std::basic_string<T> & str) { return std::vector<typename std::remove_const<T>::type>(str.begin(), str.end()); }



// string_traits abstract the API of underlying string classes, in particular they allow adopting to CString without having to specialize for CString explicitly 

template <typename Tstring>
struct string_traits
{

	using string_type = Tstring;
	using size_type = typename string_type::size_type;
	using char_type = typename string_type::value_type;

	static inline std::size_t length(const string_type &str) { return str.length(); }

	static inline void reserve(string_type &str, std::size_t size) { str.reserve(size); }

	static inline string_type& append(string_type &str, const string_type &a) { return str.append(a); }
	static inline string_type& append(string_type &str, string_type &&a) { return str.append(std::move(a)); }
	static inline string_type& append(string_type &str, std::size_t count, char_type c) { return str.append(count, c); }

	static inline string_type pad(string_type str, std::size_t left, std::size_t right)
	{
		str.insert(str.begin(), left, char_type(' '));
		str.insert(str.end(), right, char_type(' '));
		return str;
	}

};

#if defined(MPT_WITH_MFC)
template <>
struct string_traits<CString>
{

	using string_type = CString;
	using size_type = int;
	using char_type = typename CString::XCHAR;

	static inline size_type length(const string_type &str) { return str.GetLength(); }

	static inline void reserve(string_type &str, size_type size) { str.Preallocate(size); }

	static inline string_type& append(string_type &str, const string_type &a) { str += a; return str; }
	static inline string_type& append(string_type &str, size_type count, char_type c) { while(count--) str.AppendChar(c); return str; }

	static inline string_type pad(const string_type &str, size_type left, size_type right)
	{
		CString tmp;
		while(left--) tmp.AppendChar(char_type(' '));
		tmp += str;
		while(right--) tmp.AppendChar(char_type(' '));
		return tmp;
	}

};
#endif // MPT_WITH_MFC



namespace String
{


template <typename Tstring> struct Traits {
	static MPT_FORCEINLINE const char * GetDefaultWhitespace() noexcept { return " \n\r\t"; }
	static MPT_FORCEINLINE bool IsLineEnding(char c) noexcept { return c == '\r' || c == '\n'; }
};

template <> struct Traits<std::string> {
	static MPT_FORCEINLINE const char * GetDefaultWhitespace() noexcept { return " \n\r\t"; }
	static MPT_FORCEINLINE bool IsLineEnding(char c) noexcept { return c == '\r' || c == '\n'; }
};

#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
template <> struct Traits<std::wstring> {
	static MPT_FORCEINLINE const wchar_t * GetDefaultWhitespace() noexcept { return L" \n\r\t"; }
	static MPT_FORCEINLINE bool IsLineEnding(wchar_t c) noexcept { return c == L'\r' || c == L'\n'; }
};
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR


// Remove whitespace at start of string
template <typename Tstring>
inline Tstring LTrim(Tstring str, const Tstring &whitespace = Tstring(mpt::String::Traits<Tstring>::GetDefaultWhitespace()))
{
	typename Tstring::size_type pos = str.find_first_not_of(whitespace);
	if(pos != Tstring::npos)
	{
		str.erase(str.begin(), str.begin() + pos);
	} else if(pos == Tstring::npos && str.length() > 0 && str.find_last_of(whitespace) == str.length() - 1)
	{
		return Tstring();
	}
	return str;
}


// Remove whitespace at end of string
template <typename Tstring>
inline Tstring RTrim(Tstring str, const Tstring &whitespace = Tstring(mpt::String::Traits<Tstring>::GetDefaultWhitespace()))
{
	typename Tstring::size_type pos = str.find_last_not_of(whitespace);
	if(pos != Tstring::npos)
	{
		str.erase(str.begin() + pos + 1, str.end());
	} else if(pos == Tstring::npos && str.length() > 0 && str.find_first_of(whitespace) == 0)
	{
		return Tstring();
	}
	return str;
}


// Remove whitespace at start and end of string
template <typename Tstring>
inline Tstring Trim(Tstring str, const Tstring &whitespace = Tstring(mpt::String::Traits<Tstring>::GetDefaultWhitespace()))
{
	return RTrim(LTrim(str, whitespace), whitespace);
}


template <typename Tstring, typename Tstring2, typename Tstring3>
inline Tstring Replace(Tstring str, const Tstring2 &oldStr_, const Tstring3 &newStr_)
{
	std::size_t pos = 0;
	const Tstring oldStr = oldStr_;
	const Tstring newStr = newStr_;
	while((pos = str.find(oldStr, pos)) != Tstring::npos)
	{
		str.replace(pos, oldStr.length(), newStr);
		pos += newStr.length();
	}
	return str;
}


} // namespace String


static inline std::string truncate(std::string str, std::size_t maxLen)
{
	if(str.length() > maxLen)
	{
		str.resize(maxLen);
	}
	return str;
}


enum class Charset {

	UTF8,

	ASCII, // strictly 7-bit ASCII

	ISO8859_1,
	ISO8859_15,

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

// std::exception::what()
#if defined(MPT_ENABLE_CHARSET_LOCALE)
inline constexpr Charset CharsetException = Charset::Locale;
#else
inline constexpr Charset CharsetException = Charset::UTF8;
#endif

// Locale in tracker builds, UTF8 in non-locale-aware libopenmpt builds.
#if defined(MPT_ENABLE_CHARSET_LOCALE)
inline constexpr Charset CharsetLocaleOrUTF8 = Charset::Locale;
#else
inline constexpr Charset CharsetLocaleOrUTF8 = Charset::UTF8;
#endif



// Checks if the std::string represents an UTF8 string.
// This is currently implemented as converting to std::wstring and back assuming UTF8 both ways,
// and comparing the result to the original string.
// Caveats:
//  - can give false negatives because of possible unicode normalization during conversion
//  - can give false positives if the 8bit encoding contains high-ascii only in valid utf8 groups
//  - slow because of double conversion
bool IsUTF8(const std::string &str);


#define MPT_CHAR_TYPE    char
#define MPT_CHAR(x)      x
#define MPT_LITERAL(x)   x
#define MPT_STRING(x)    std::string( x )

#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
#define MPT_WCHAR_TYPE   wchar_t
#define MPT_WCHAR(x)     L ## x
#define MPT_WLITERAL(x)  L ## x
#define MPT_WSTRING(x)   std::wstring( L ## x )
#else // MPT_COMPILER_QUIRK_NO_WCHAR
#define MPT_WCHAR_TYPE   char32_t
#define MPT_WCHAR(x)     U ## x
#define MPT_WLITERAL(x)  U ## x
#define MPT_WSTRING(x)   std::u32string( U ## x )
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR


template <mpt::Charset charset_tag>
struct charset_char_traits : std::char_traits<char> {
	static mpt::Charset charset() { return charset_tag; }
};
#define MPT_ENCODED_STRING_TYPE(charset) std::basic_string< char, mpt::charset_char_traits< charset > >


#if defined(MPT_ENABLE_CHARSET_LOCALE)

using lstring = MPT_ENCODED_STRING_TYPE(mpt::Charset::Locale);

#endif // MPT_ENABLE_CHARSET_LOCALE

#if MPT_OS_WINDOWS

template <typename Tchar> struct windows_char_traits { };
template <> struct windows_char_traits<char>  { using string_type = mpt::lstring; };
template <> struct windows_char_traits<wchar_t> { using string_type = std::wstring; };

#ifdef UNICODE
using tstring = windows_char_traits<wchar_t>::string_type;
#else
using tstring = windows_char_traits<char>::string_type;
#endif

using winstring = mpt::tstring;

#endif // MPT_OS_WINDOWS


#if MPT_ENABLE_U8STRING

#if MPT_CXX_AT_LEAST(20)

using u8string = std::u8string;

#define MPT_U8CHAR_TYPE  char8_t
#define MPT_U8CHAR(x)    u8 ## x
#define MPT_U8LITERAL(x) u8 ## x
#define MPT_U8STRING(x)  std::u8string( u8 ## x )

#else // !C++20

using u8string = MPT_ENCODED_STRING_TYPE(mpt::Charset::UTF8);

#define MPT_U8CHAR_TYPE  char
#define MPT_U8CHAR(x)    x
#define MPT_U8LITERAL(x) x
#define MPT_U8STRING(x)  mpt::u8string( x )

// mpt::u8string is a moderately type-safe string that is meant to contain
// UTF-8 encoded char bytes.
//
// mpt::u8string is not implicitely convertible to/from std::string, but
// it is convertible to/from C strings the same way as std::string is.
//
// The implementation of mpt::u8string is a compromise of compatibilty
// with implementation-defined STL details, efficiency, source code size,
// executable bloat, type-safety  and simplicity.
//
// mpt::u8string is not meant to be used directly though.
// mpt::u8string is meant as an alternative implementaion to std::wstring
// for implementing the unicode string type mpt::ustring.

#endif // C++20

#endif // MPT_ENABLE_U8STRING


#if MPT_WSTRING_CONVERT
// Convert to a wide character string.
// The wide encoding is UTF-16 or UTF-32, based on sizeof(wchar_t).
// If str does not contain any invalid characters, this conversion is lossless.
// Invalid source bytes will be replaced by some replacement character or string.
static inline std::wstring ToWide(const std::wstring &str) { return str; }
static inline std::wstring ToWide(const wchar_t * str) { return (str ? std::wstring(str) : std::wstring()); }
std::wstring ToWide(Charset from, const std::string &str);
static inline std::wstring ToWide(Charset from, const char * str) { return ToWide(from, str ? std::string(str) : std::string()); }
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
static inline std::string ToCharset(Charset to, const wchar_t * str) { return ToCharset(to, str ? std::wstring(str) : std::wstring()); }
#endif
std::string ToCharset(Charset to, Charset from, const std::string &str);
static inline std::string ToCharset(Charset to, Charset from, const char * str) { return ToCharset(to, from, str ? std::string(str) : std::string()); }
#if defined(MPT_ENABLE_CHARSET_LOCALE)
std::string ToCharset(Charset to, const mpt::lstring &str);
#endif // MPT_ENABLE_CHARSET_LOCALE

#if defined(MPT_ENABLE_CHARSET_LOCALE)
#if MPT_WSTRING_CONVERT
mpt::lstring ToLocale(const std::wstring &str);
static inline mpt::lstring ToLocale(const wchar_t * str) { return ToLocale(str ? std::wstring(str): std::wstring()); }
#endif
mpt::lstring ToLocale(Charset from, const std::string &str);
static inline mpt::lstring ToLocale(Charset from, const char * str) { return ToLocale(from, str ? std::string(str): std::string()); }
static inline mpt::lstring ToLocale(const mpt::lstring &str) { return str; }
#endif // MPT_ENABLE_CHARSET_LOCALE

#if MPT_OS_WINDOWS
#if MPT_WSTRING_CONVERT
mpt::winstring ToWin(const std::wstring &str);
static inline mpt::winstring ToWin(const wchar_t * str) { return ToWin(str ? std::wstring(str): std::wstring()); }
#endif
mpt::winstring ToWin(Charset from, const std::string &str);
static inline mpt::winstring ToWin(Charset from, const char * str) { return ToWin(from, str ? std::string(str): std::string()); }
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
static inline CString ToCString(const CString &str) { return str; }
CString ToCString(const std::wstring &str);
static inline CString ToCString(const wchar_t * str) { return ToCString(str ? std::wstring(str) : std::wstring()); }
CString ToCString(Charset from, const std::string &str);
static inline CString ToCString(Charset from, const char * str) { return ToCString(from, str ? std::string(str) : std::string()); }
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


// mpt::ustring
//
// mpt::ustring is a string type that can hold unicode strings.
// It is implemented as a std::basic_string either based on wchar_t (i.e. the
//  same as std::wstring) or a custom-defined char_traits class that is derived
//  from std::char_traits<char>.
// The selection of the underlying implementation is done at compile-time.
// MPT_UCHAR, MPT_ULITERAL and MPT_USTRING are macros that ease construction
//  of ustring char literals, ustring char array literals and ustring objects
//  from ustring char literals that work consistently in both modes.
//  Note that these are not supported for non-ASCII characters appearing in
//  the macro argument.
// Also note that, as both UTF8 and UTF16 (it is less of an issue for UTF32)
//  are variable-length encodings and mpt::ustring is implemented as a
//  std::basic_string, all member functions that require individual character
//  access will not work consistently or even at all in a meaningful way.
//  This in particular affects operator[], at(), find() and substr().
//  The code makes no effort in preventing these or generating warnings when
//  these are used on mpt::ustring objects. However, compiling in the
//  respectively other mpt::ustring mode will catch most of these anyway.

#if MPT_USTRING_MODE_WIDE
#if MPT_USTRING_MODE_UTF8
#error "MPT_USTRING_MODE_WIDE and MPT_USTRING_MODE_UTF8 are mutually exclusive."
#endif

using ustring = std::wstring;
using uchar = wchar_t;
#define MPT_UCHAR(x)     L ## x
#define MPT_ULITERAL(x)  L ## x
#define MPT_USTRING(x)   std::wstring( L ## x )

#endif // MPT_USTRING_MODE_WIDE

#if MPT_USTRING_MODE_UTF8
#if MPT_USTRING_MODE_WIDE
#error "MPT_USTRING_MODE_WIDE and MPT_USTRING_MODE_UTF8 are mutually exclusive."
#endif

using ustring = mpt::u8string;
using uchar = MPT_U8CHAR_TYPE;
#define MPT_UCHAR(x)     MPT_U8CHAR( x )
#define MPT_ULITERAL(x)  MPT_U8LITERAL( x )
#define MPT_USTRING(x)   MPT_U8STRING( x )

#endif // MPT_USTRING_MODE_UTF8

#define UC_(x)           MPT_UCHAR(x)
#define UL_(x)           MPT_ULITERAL(x)
#define U_(x)            MPT_USTRING(x)

#if MPT_USTRING_MODE_WIDE
#if !(MPT_WSTRING_CONVERT)
#error "MPT_USTRING_MODE_WIDE depends on MPT_WSTRING_CONVERT)"
#endif
static inline mpt::ustring ToUnicode(const std::wstring &str) { return str; }
static inline mpt::ustring ToUnicode(const wchar_t * str) { return (str ? std::wstring(str) : std::wstring()); }
static inline mpt::ustring ToUnicode(Charset from, const std::string &str) { return ToWide(from, str); }
static inline mpt::ustring ToUnicode(Charset from, const char * str) { return ToUnicode(from, str ? std::string(str) : std::string()); }
#if defined(MPT_ENABLE_CHARSET_LOCALE)
static inline mpt::ustring ToUnicode(const mpt::lstring &str) { return ToWide(str); }
#endif // MPT_ENABLE_CHARSET_LOCALE
#if defined(MPT_WITH_MFC)
static inline mpt::ustring ToUnicode(const CString &str) { return ToWide(str); }
#endif // MFC
#else // !MPT_USTRING_MODE_WIDE
static inline mpt::ustring ToUnicode(const mpt::ustring &str) { return str; }
#if MPT_WSTRING_CONVERT
mpt::ustring ToUnicode(const std::wstring &str);
static inline mpt::ustring ToUnicode(const wchar_t * str) { return ToUnicode(str ? std::wstring(str) : std::wstring()); }
#endif
mpt::ustring ToUnicode(Charset from, const std::string &str);
static inline mpt::ustring ToUnicode(Charset from, const char * str) { return ToUnicode(from, str ? std::string(str) : std::string()); }
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
#define MPT_UTF8(x) mpt::ToUnicode(mpt::Charset::UTF8, x )





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
