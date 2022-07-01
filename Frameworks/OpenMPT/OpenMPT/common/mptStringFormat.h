/*
 * mptStringFormat.h
 * -----------------
 * Purpose: Convert other types to strings.
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/pointer.hpp"
#include "mpt/format/message.hpp"
#include "mpt/format/simple_spec.hpp"
#include "mpt/string/types.hpp"

#include <stdexcept>

#include "mptString.h"

#include "openmpt/base/FlagSet.hpp"


OPENMPT_NAMESPACE_BEGIN



// The following section demands a rationale.
//  1. mpt::afmt::val(), mpt::wfmt::val() and mpt::ufmt::val() mimic the semantics of c++11 std::to_string() and std::to_wstring().
//     There is an important difference though. The c++11 versions are specified in terms of sprintf formatting which in turn
//     depends on the current C locale. This renders these functions unusable in a library context because the current
//     C locale is set by the library-using application and could be anything. There is no way a library can get reliable semantics
//     out of these functions. It is thus better to just avoid them.
//     ToAString() and ToWString() are based on iostream internally, but the the locale of the stream is forced to std::locale::classic(),
//     which results in "C" ASCII locale behavior.
//  2. The full suite of printf-like or iostream like number formatting is generally not required. Instead, a sane subset functionality
//     is provided here.
//     When formatting integers, it is recommended to use mpt::afmt::dec or mpt::afmt::hex. Appending a template argument '<n>' sets the width,
//     the same way as '%nd' would do. Appending a '0' to the function name causes zero-filling as print-like '%0nd' would do. Spelling 'HEX'
//     in upper-case generates upper-case hex digits. If these are not known at compile-time, a more verbose FormatValA(int, format) can be
//     used.
//  3. mpt::format(format)(...) provides simplified and type-safe message and localization string formatting.
//     The only specifier allowed is '{}' enclosing a number n. It references to n-th parameter after the format string (1-based).
//     This mimics the behaviour of QString::arg() in QT4/5 or MFC AfxFormatString2(). C printf-like functions offer similar functionality
//     with a '%n$TYPE' syntax. In .NET, the syntax is '{n}'. This is useful to support localization strings that can change the parameter
//     ordering.
//  4. Every function is available for std::string, std::wstring and mpt::ustring. std::string makes no assumption about the encoding, which
//     basically means, it should work for any 7-bit or 8-bit encoding, including for example ASCII, UTF8 or the current locale encoding.
//     std::string         std::wstring          mpt::ustring                    mpt::tsrtring                        CString
//     mpt::afmt           mpt::wfmt             mpt::ufmt                       mpt::tfmt                            mpt::cfmt
//     MPT_AFORMAT("{}")   MPT_WFORMAT("{}")     MPT_UFORMAT("{}")               MPT_TFORMAT("{}")                    MPT_CFORMAT("{}")
//  5. All functionality here delegates real work outside of the header file so that <sstream> and <locale> do not need to be included when
//     using this functionality.
//     Advantages:
//      - Avoids binary code bloat when too much of iostream operator << gets inlined at every usage site.
//      - Faster compile times because <sstream> and <locale> (2 very complex headers) are not included everywhere.
//     Disadvantages:
//      - Slightly more c++ code is required for delegating work.
//      - As the header does not use iostreams, custom types need to overload mpt::UString instead of iostream operator << to allow for custom type
//        formatting.
//      - std::string, std::wstring and mpt::ustring are returned from somewhat deep cascades of helper functions. Where possible, code is
//        written in such a way that return-value-optimization (RVO) or named-return-value-optimization (NRVO) should be able to eliminate
//        almost all these copies. This should not be a problem for any decent modern compiler (and even less so for a c++11 compiler where
//        move-semantics will kick in if RVO/NRVO fails).

namespace mpt
{

// ToUString() converts various built-in types to a well-defined, locale-independent string representation.
// This is also used as a type-tunnel pattern for mpt::format.
// Custom types that need to be converted to strings are encouraged to overload ToUString().

// fallback to member function ToUString()
#if MPT_USTRING_MODE_UTF8
template <typename T> [[deprecated]] auto ToAString(const T & x) -> decltype(mpt::ToCharset(mpt::Charset::UTF8, x.ToUString())) { return mpt::ToCharset(mpt::Charset::UTF8, x.ToUString()); } // unknown encoding
#else
#if defined(MPT_ENABLE_CHARSET_LOCALE)
template <typename T> [[deprecated]] auto ToAString(const T & x) -> decltype(mpt::ToCharset(mpt::Charset::Locale, x.ToUString())) { return mpt::ToCharset(mpt::Charset::Locale, x.ToUString()); } // unknown encoding
#else // !MPT_ENABLE_CHARSET_LOCALE
template <typename T> [[deprecated]] auto ToAString(const T & x) -> decltype(mpt::ToCharset(mpt::Charset::UTF8, x.ToUString())) { return mpt::ToCharset(mpt::Charset::UTF8, x.ToUString()); } // unknown encoding
#endif // MPT_ENABLE_CHARSET_LOCALE
#endif

inline std::string ToAString(const std::string & x) { return x; }
inline std::string ToAString(const char * const & x) { return x; }
std::string ToAString(const char &x) = delete; // deprecated to catch potential API mis-use, use std::string(1, x) instead
#if MPT_WSTRING_FORMAT
std::string ToAString(const std::wstring & x) = delete; // Unknown encoding.
std::string ToAString(const wchar_t * const & x) = delete; // Unknown encoding.
std::string ToAString(const wchar_t &x ) = delete; // deprecated to catch potential API mis-use, use std::wstring(1, x) instead
#endif
#if MPT_USTRING_MODE_UTF8
std::string ToAString(const mpt::ustring & x) = delete; // Unknown encoding.
#endif
#if defined(MPT_WITH_MFC)
std::string ToAString(const CString & x) = delete; // unknown encoding
#endif // MPT_WITH_MFC
std::string ToAString(const bool & x);
std::string ToAString(const signed char & x);
std::string ToAString(const unsigned char & x);
std::string ToAString(const signed short & x);
std::string ToAString(const unsigned short & x);
std::string ToAString(const signed int & x);
std::string ToAString(const unsigned int & x);
std::string ToAString(const signed long & x);
std::string ToAString(const unsigned long & x);
std::string ToAString(const signed long long & x);
std::string ToAString(const unsigned long long & x);
std::string ToAString(const float & x);
std::string ToAString(const double & x);
std::string ToAString(const long double & x);

// fallback to member function ToUString()
template <typename T> auto ToUString(const T & x) -> decltype(x.ToUString()) { return x.ToUString(); }

inline mpt::ustring ToUString(const mpt::ustring & x) { return x; }
mpt::ustring ToUString(const std::string & x) = delete; // Unknown encoding.
mpt::ustring ToUString(const char * const & x) = delete; // Unknown encoding. Note that this also applies to TCHAR in !UNICODE builds as the type is indistinguishable from char. Wrap with CString or FromTcharStr in this case.
mpt::ustring ToUString(const char & x) = delete; // deprecated to catch potential API mis-use, use std::string(1, x) instead
#if MPT_WSTRING_FORMAT
#if MPT_USTRING_MODE_UTF8
mpt::ustring ToUString(const std::wstring & x);
#endif
mpt::ustring ToUString(const wchar_t * const & x);
mpt::ustring ToUString(const wchar_t & x) = delete; // deprecated to catch potential API mis-use, use std::wstring(1, x) instead
#endif
#if defined(MPT_WITH_MFC)
mpt::ustring ToUString(const CString & x);
#endif // MPT_WITH_MFC
mpt::ustring ToUString(const bool & x);
mpt::ustring ToUString(const signed char & x);
mpt::ustring ToUString(const unsigned char & x);
mpt::ustring ToUString(const signed short & x);
mpt::ustring ToUString(const unsigned short & x);
mpt::ustring ToUString(const signed int & x);
mpt::ustring ToUString(const unsigned int & x);
mpt::ustring ToUString(const signed long & x);
mpt::ustring ToUString(const unsigned long & x);
mpt::ustring ToUString(const signed long long & x);
mpt::ustring ToUString(const unsigned long long & x);
mpt::ustring ToUString(const float & x);
mpt::ustring ToUString(const double & x);
mpt::ustring ToUString(const long double & x);

#if MPT_WSTRING_FORMAT
std::wstring ToWString(const std::string & x) = delete; // Unknown encoding.
std::wstring ToWString(const char * const & x) = delete; // Unknown encoding. Note that this also applies to TCHAR in !UNICODE builds as the type is indistinguishable from char. Wrap with CString or FromTcharStr in this case.
std::wstring ToWString(const char & x) = delete; // deprecated to catch potential API mis-use, use std::string(1, x) instead
inline std::wstring ToWString(const std::wstring & x) { return x; }
inline std::wstring ToWString(const wchar_t * const & x) { return x; }
std::wstring ToWString(const wchar_t & x) = delete; // deprecated to catch potential API mis-use, use std::wstring(1, x) instead
#if MPT_USTRING_MODE_UTF8
std::wstring ToWString(const mpt::ustring & x);
#endif
#if defined(MPT_WITH_MFC)
std::wstring ToWString(const CString & x);
#endif // MPT_WITH_MFC
std::wstring ToWString(const bool & x);
std::wstring ToWString(const signed char & x);
std::wstring ToWString(const unsigned char & x);
std::wstring ToWString(const signed short & x);
std::wstring ToWString(const unsigned short & x);
std::wstring ToWString(const signed int & x);
std::wstring ToWString(const unsigned int & x);
std::wstring ToWString(const signed long & x);
std::wstring ToWString(const unsigned long & x);
std::wstring ToWString(const signed long long & x);
std::wstring ToWString(const unsigned long long & x);
std::wstring ToWString(const float & x);
std::wstring ToWString(const double & x);
std::wstring ToWString(const long double & x);
// fallback to member function ToUString()
template <typename T> auto ToWString(const T & x) -> decltype(mpt::ToWide(x.ToUString())) { return mpt::ToWide(x.ToUString()); }
#endif

#if defined(MPT_ENABLE_CHARSET_LOCALE)
template <typename T> struct ToLocaleHelper { mpt::lstring operator () (const T & v) { return mpt::ToLocale(ToUString(v)); } };
template <> struct ToLocaleHelper<mpt::lstring> { mpt::lstring operator () (const mpt::lstring & v) { return v; } };
#endif // MPT_ENABLE_CHARSET_LOCALE

#if defined(MPT_WITH_MFC)
template <typename T> struct ToCStringHelper { CString operator () (const T & v) { return mpt::ToCString(ToUString(v)); } };
template <> struct ToCStringHelper<CString> { CString operator () (const CString & v) { return v; } };
#endif // MPT_WITH_MFC

template <typename Tstring> struct ToStringTFunctor {};
template <> struct ToStringTFunctor<std::string> { template <typename T> inline std::string operator() (const T & x) { return ToAString(x); } };
template <> struct ToStringTFunctor<mpt::ustring> { template <typename T> inline mpt::ustring operator() (const T & x) { return ToUString(x); } };
#if MPT_WSTRING_FORMAT && MPT_USTRING_MODE_UTF8
template <> struct ToStringTFunctor<std::wstring> { template <typename T> inline std::wstring operator() (const T & x) { return ToWString(x); } };
#endif
#if defined(MPT_ENABLE_CHARSET_LOCALE)
template <> struct ToStringTFunctor<mpt::lstring> { template <typename T> inline mpt::lstring operator() (const T & x) { return mpt::ToLocaleHelper<T>()(x); } };
#endif // MPT_ENABLE_CHARSET_LOCALE
#if defined(MPT_WITH_MFC)
template <> struct ToStringTFunctor<CString> { template <typename T> inline CString operator() (const T & x) { return mpt::ToCStringHelper<T>()(x); } };
#endif // MPT_WITH_MFC

template<typename Tstring, typename T> inline Tstring ToStringT(const T & x) { return ToStringTFunctor<Tstring>()(x); }


struct ToStringFormatter {
	template <typename Tstring, typename T>
	static inline Tstring format(const T& value) {
		return ToStringTFunctor<Tstring>()(value);

	}
};


using FormatSpec = mpt::format_simple_spec;

using FormatFlags = mpt::format_simple_flags;

using fmt_base = mpt::format_simple_base;


std::string FormatValA(const char & x, const FormatSpec & f) = delete; // deprecated to catch potential API mis-use, use std::string(1, x) instead
#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
std::string FormatValA(const wchar_t & x, const FormatSpec & f) = delete; // deprecated to catch potential API mis-use, use std::wstring(1, x) instead
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR
std::string FormatValA(const bool & x, const FormatSpec & f);
std::string FormatValA(const signed char & x, const FormatSpec & f);
std::string FormatValA(const unsigned char & x, const FormatSpec & f);
std::string FormatValA(const signed short & x, const FormatSpec & f);
std::string FormatValA(const unsigned short & x, const FormatSpec & f);
std::string FormatValA(const signed int & x, const FormatSpec & f);
std::string FormatValA(const unsigned int & x, const FormatSpec & f);
std::string FormatValA(const signed long & x, const FormatSpec & f);
std::string FormatValA(const unsigned long & x, const FormatSpec & f);
std::string FormatValA(const signed long long & x, const FormatSpec & f);
std::string FormatValA(const unsigned long long & x, const FormatSpec & f);
std::string FormatValA(const float & x, const FormatSpec & f);
std::string FormatValA(const double & x, const FormatSpec & f);
std::string FormatValA(const long double & x, const FormatSpec & f);

mpt::ustring FormatValU(const char & x, const FormatSpec & f) = delete; // deprecated to catch potential API mis-use, use std::string(1, x) instead
#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
mpt::ustring FormatValU(const wchar_t & x, const FormatSpec & f) = delete; // deprecated to catch potential API mis-use, use std::wstring(1, x) instead
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR
mpt::ustring FormatValU(const bool & x, const FormatSpec & f);
mpt::ustring FormatValU(const signed char & x, const FormatSpec & f);
mpt::ustring FormatValU(const unsigned char & x, const FormatSpec & f);
mpt::ustring FormatValU(const signed short & x, const FormatSpec & f);
mpt::ustring FormatValU(const unsigned short & x, const FormatSpec & f);
mpt::ustring FormatValU(const signed int & x, const FormatSpec & f);
mpt::ustring FormatValU(const unsigned int & x, const FormatSpec & f);
mpt::ustring FormatValU(const signed long & x, const FormatSpec & f);
mpt::ustring FormatValU(const unsigned long & x, const FormatSpec & f);
mpt::ustring FormatValU(const signed long long & x, const FormatSpec & f);
mpt::ustring FormatValU(const unsigned long long & x, const FormatSpec & f);
mpt::ustring FormatValU(const float & x, const FormatSpec & f);
mpt::ustring FormatValU(const double & x, const FormatSpec & f);
mpt::ustring FormatValU(const long double & x, const FormatSpec & f);

#if MPT_WSTRING_FORMAT
std::wstring FormatValW(const char & x, const FormatSpec & f) = delete; // deprecated to catch potential API mis-use, use std::string(1, x) instead
#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
std::wstring FormatValW(const wchar_t & x, const FormatSpec & f) = delete; // deprecated to catch potential API mis-use, use std::wstring(1, x) instead
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR
std::wstring FormatValW(const bool & x, const FormatSpec & f);
std::wstring FormatValW(const signed char & x, const FormatSpec & f);
std::wstring FormatValW(const unsigned char & x, const FormatSpec & f);
std::wstring FormatValW(const signed short & x, const FormatSpec & f);
std::wstring FormatValW(const unsigned short & x, const FormatSpec & f);
std::wstring FormatValW(const signed int & x, const FormatSpec & f);
std::wstring FormatValW(const unsigned int & x, const FormatSpec & f);
std::wstring FormatValW(const signed long & x, const FormatSpec & f);
std::wstring FormatValW(const unsigned long & x, const FormatSpec & f);
std::wstring FormatValW(const signed long long & x, const FormatSpec & f);
std::wstring FormatValW(const unsigned long long & x, const FormatSpec & f);
std::wstring FormatValW(const float & x, const FormatSpec & f);
std::wstring FormatValW(const double & x, const FormatSpec & f);
std::wstring FormatValW(const long double & x, const FormatSpec & f);
#endif

template <typename Tstring> struct FormatValTFunctor {};
template <> struct FormatValTFunctor<std::string> { template <typename T> inline std::string operator() (const T & x, const FormatSpec & f) { return FormatValA(x, f); } };
template <> struct FormatValTFunctor<mpt::ustring> { template <typename T> inline mpt::ustring operator() (const T & x, const FormatSpec & f) { return FormatValU(x, f); } };
#if MPT_USTRING_MODE_UTF8 && MPT_WSTRING_FORMAT
template <> struct FormatValTFunctor<std::wstring> { template <typename T> inline std::wstring operator() (const T & x, const FormatSpec & f) { return FormatValW(x, f); } };
#endif
#if defined(MPT_ENABLE_CHARSET_LOCALE)
template <> struct FormatValTFunctor<mpt::lstring> { template <typename T> inline mpt::lstring operator() (const T & x, const FormatSpec & f) { return mpt::ToLocale(mpt::Charset::Locale, FormatValA(x, f)); } };
#endif // MPT_ENABLE_CHARSET_LOCALE
#if defined(MPT_WITH_MFC)
#ifdef UNICODE
template <> struct FormatValTFunctor<CString> { template <typename T> inline CString operator() (const T & x, const FormatSpec & f) { return mpt::ToCString(FormatValW(x, f)); } };
#else // !UNICODE
template <> struct FormatValTFunctor<CString> { template <typename T> inline CString operator() (const T & x, const FormatSpec & f) { return mpt::ToCString(mpt::Charset::Locale, FormatValA(x, f)); } };
#endif // UNICODE
#endif // MPT_WITH_MFC


template <typename Tstring>
struct fmtT : fmt_base
{

template<typename T>
static inline Tstring val(const T& x)
{
	return ToStringTFunctor<Tstring>()(x);
}

template<typename T>
static inline Tstring fmt(const T& x, const FormatSpec& f)
{
	return FormatValTFunctor<Tstring>()(x, f);
}

template<typename T>
static inline Tstring dec(const T& x)
{
	static_assert(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, FormatSpec().BaseDec().FillOff());
}
template<int width, typename T>
static inline Tstring dec0(const T& x)
{
	static_assert(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, FormatSpec().BaseDec().FillNul().Width(width));
}

template<typename T>
static inline Tstring dec(unsigned int g, char s, const T& x)
{
	static_assert(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, FormatSpec().BaseDec().FillOff().Group(g).GroupSep(s));
}
template<int width, typename T>
static inline Tstring dec0(unsigned int g, char s, const T& x)
{
	static_assert(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, FormatSpec().BaseDec().FillNul().Width(width).Group(g).GroupSep(s));
}

template<typename T>
static inline Tstring hex(const T& x)
{
	static_assert(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, FormatSpec().BaseHex().CaseLow().FillOff());
}
template<typename T>
static inline Tstring HEX(const T& x)
{
	static_assert(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, FormatSpec().BaseHex().CaseUpp().FillOff());
}
template<int width, typename T>
static inline Tstring hex0(const T& x)
{
	static_assert(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, FormatSpec().BaseHex().CaseLow().FillNul().Width(width));
}
template<int width, typename T>
static inline Tstring HEX0(const T& x)
{
	static_assert(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, FormatSpec().BaseHex().CaseUpp().FillNul().Width(width));
}

template<typename T>
static inline Tstring hex(unsigned int g, char s, const T& x)
{
	static_assert(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, FormatSpec().BaseHex().CaseLow().FillOff().Group(g).GroupSep(s));
}
template<typename T>
static inline Tstring HEX(unsigned int g, char s, const T& x)
{
	static_assert(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, FormatSpec().BaseHex().CaseUpp().FillOff().Group(g).GroupSep(s));
}
template<int width, typename T>
static inline Tstring hex0(unsigned int g, char s, const T& x)
{
	static_assert(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, FormatSpec().BaseHex().CaseLow().FillNul().Width(width).Group(g).GroupSep(s));
}
template<int width, typename T>
static inline Tstring HEX0(unsigned int g, char s, const T& x)
{
	static_assert(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, FormatSpec().BaseHex().CaseUpp().FillNul().Width(width).Group(g).GroupSep(s));
}

template<typename T>
static inline Tstring flt(const T& x, int precision = -1)
{
	static_assert(std::is_floating_point<T>::value);
	return FormatValTFunctor<Tstring>()(x, FormatSpec().NotaNrm().FillOff().Precision(precision));
}
template<typename T>
static inline Tstring fix(const T& x, int precision = -1)
{
	static_assert(std::is_floating_point<T>::value);
	return FormatValTFunctor<Tstring>()(x, FormatSpec().NotaFix().FillOff().Precision(precision));
}
template<typename T>
static inline Tstring sci(const T& x, int precision = -1)
{
	static_assert(std::is_floating_point<T>::value);
	return FormatValTFunctor<Tstring>()(x, FormatSpec().NotaSci().FillOff().Precision(precision));
}

template<typename T>
static inline Tstring ptr(const T& x)
{
	static_assert(std::is_pointer<T>::value || std::is_same<T, std::uintptr_t>::value || std::is_same<T, std::intptr_t>::value, "");
	return hex0<mpt::pointer_size * 2>(mpt::pointer_cast<const std::uintptr_t>(x));
}
template<typename T>
static inline Tstring PTR(const T& x)
{
	static_assert(std::is_pointer<T>::value || std::is_same<T, std::uintptr_t>::value || std::is_same<T, std::intptr_t>::value, "");
	return HEX0<mpt::pointer_size * 2>(mpt::pointer_cast<const std::uintptr_t>(x));
}

static inline Tstring pad_left(std::size_t width_, const Tstring &str)
{
	typedef mpt::string_traits<Tstring> traits;
	typename traits::size_type width = static_cast<typename traits::size_type>(width_);
	return traits::pad(str, width, 0);
}
static inline Tstring pad_right(std::size_t width_, const Tstring &str)
{
	typedef mpt::string_traits<Tstring> traits;
	typename traits::size_type width = static_cast<typename traits::size_type>(width_);
	return traits::pad(str, 0, width);
}
static inline Tstring left(std::size_t width_, const Tstring &str)
{
	typedef mpt::string_traits<Tstring> traits;
	typename traits::size_type width = static_cast<typename traits::size_type>(width_);
	return (traits::length(str) < width) ? traits::pad(str, 0, width - traits::length(str)) : str;
}
static inline Tstring right(std::size_t width_, const Tstring &str)
{
	typedef mpt::string_traits<Tstring> traits;
	typename traits::size_type width = static_cast<typename traits::size_type>(width_);
	return (traits::length(str) < width) ? traits::pad(str, width - traits::length(str), 0) : str;
}
static inline Tstring center(std::size_t width_, const Tstring &str)
{
	typedef mpt::string_traits<Tstring> traits;
	typename traits::size_type width = static_cast<typename traits::size_type>(width_);
	return (traits::length(str) < width) ? traits::pad(str, (width - traits::length(str)) / 2, (width - traits::length(str) + 1) / 2) : str;
}

}; // struct fmtT


typedef fmtT<std::string> afmt;
#if MPT_WSTRING_FORMAT
typedef fmtT<std::wstring> wfmt;
#endif
#if MPT_USTRING_MODE_WIDE
typedef fmtT<std::wstring> ufmt;
#else
typedef fmtT<mpt::ustring> ufmt;
#endif
#if defined(MPT_ENABLE_CHARSET_LOCALE)
typedef fmtT<mpt::lstring> lfmt;
#endif // MPT_ENABLE_CHARSET_LOCALE
#if MPT_OS_WINDOWS
typedef fmtT<mpt::tstring> tfmt;
#endif
#if defined(MPT_WITH_MFC)
typedef fmtT<CString> cfmt;
#endif // MPT_WITH_MFC


#define MPT_AFORMAT(f) mpt::format_message<mpt::ToStringFormatter, mpt::parse_format_string_argument_count(f)>(f)

#if MPT_WSTRING_FORMAT
#define MPT_WFORMAT(f) mpt::format_message_typed<mpt::ToStringFormatter, mpt::parse_format_string_argument_count( L ## f ), std::wstring>( L ## f )
#endif

#define MPT_UFORMAT(f) mpt::format_message_typed<mpt::ToStringFormatter, mpt::parse_format_string_argument_count(MPT_ULITERAL(f)), mpt::ustring>(MPT_ULITERAL(f))

#if defined(MPT_ENABLE_CHARSET_LOCALE)
#define MPT_LFORMAT(f) mpt::format_message_typed<mpt::ToStringFormatter, mpt::parse_format_string_argument_count(f), mpt::lstring>(f)
#endif // MPT_ENABLE_CHARSET_LOCALE

#if MPT_OS_WINDOWS
#define MPT_TFORMAT(f) mpt::format_message_typed<mpt::ToStringFormatter, mpt::parse_format_string_argument_count(TEXT(f)), mpt::tstring>(TEXT(f))
#endif

#if defined(MPT_WITH_MFC)
#define MPT_CFORMAT(f) mpt::format_message_typed<mpt::ToStringFormatter, mpt::parse_format_string_argument_count(TEXT(f)), CString>(TEXT(f))
#endif // MPT_WITH_MFC


} // namespace mpt



namespace mpt { namespace String {

// Combine a vector of values into a string, separated with the given separator.
// No escaping is performed.
template<typename T>
mpt::ustring Combine(const std::vector<T> &vals, const mpt::ustring &sep=U_(","))
{
	mpt::ustring str;
	for(std::size_t i = 0; i < vals.size(); ++i)
	{
		if(i > 0)
		{
			str += sep;
		}
		str += mpt::ufmt::val(vals[i]);
	}
	return str;
}
template<typename T>
std::string Combine(const std::vector<T> &vals, const std::string &sep=std::string(","))
{
	std::string str;
	for(std::size_t i = 0; i < vals.size(); ++i)
	{
		if(i > 0)
		{
			str += sep;
		}
		str += mpt::afmt::val(vals[i]);
	}
	return str;
}

} } // namespace mpt::String



template <typename enum_t, typename store_t>
mpt::ustring ToUString(FlagSet<enum_t, store_t> flagset)
{
	mpt::ustring str(flagset.size_bits(), UC_('0'));

	for(std::size_t x = 0; x < flagset.size_bits(); ++x)
	{
		str[flagset.size_bits() - x - 1] = (flagset.value().as_bits() & (static_cast<typename FlagSet<enum_t>::store_type>(1) << x) ? UC_('1') : UC_('0'));
	}

	return str;
}




OPENMPT_NAMESPACE_END
