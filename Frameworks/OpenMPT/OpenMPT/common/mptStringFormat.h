/*
 * mptStringFormat.h
 * -----------------
 * Purpose: Convert other types to strings.
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"


#include "mptString.h"


OPENMPT_NAMESPACE_BEGIN



// The following section demands a rationale.
//  1. mpt::fmt::val(), mpt::wfmt::val() and mpt::ufmt::val() mimic the semantics of c++11 std::to_string() and std::to_wstring().
//     There is an important difference though. The c++11 versions are specified in terms of sprintf formatting which in turn
//     depends on the current C locale. This renders these functions unusable in a library context because the current
//     C locale is set by the library-using application and could be anything. There is no way a library can get reliable semantics
//     out of these functions. It is thus better to just avoid them.
//     ToString() and ToWString() are based on iostream internally, but the the locale of the stream is forced to std::locale::classic(),
//     which results in "C" ASCII locale behavior.
//  2. The full suite of printf-like or iostream like number formatting is generally not required. Instead, a sane subset functionality
//     is provided here.
//     For convenience, mpt::fmt::f(const char *, float) allows formatting a single floating point value with a
//     standard printf-like format string. This itself relies on iostream with classic() locale internally and is thus current locale
//     agnostic.
//     When formatting integers, it is recommended to use mpt::fmt::dec or mpt::fmt::hex. Appending a template argument '<n>' sets the width,
//     the same way as '%nd' would do. Appending a '0' to the function name causes zero-filling as print-like '%0nd' would do. Spelling 'HEX'
//     in upper-case generates upper-case hex digits. If these are not known at compile-time, a more verbose FormatVal(int, format) can be
//     used.
//  3. mpt::format(format)(...) provides simplified and type-safe message and localization string formatting.
//     The only specifier allowed is '%' followed by a single digit n. It references to n-th parameter after the format string (1-based).
//     This mimics the behaviour of QString::arg() in QT4/5 or MFC AfxFormatString2(). C printf-like functions offer similar functionality
//     with a '%n$TYPE' syntax. In .NET, the syntax is '{n}'. This is useful to support localization strings that can change the parameter
//     ordering.
//  4. Every function is available for std::string, std::wstring and mpt::ustring. std::string makes no assumption about the encoding, which
//     basically means, it should work for any 7-bit or 8-bit encoding, including for example ASCII, UTF8 or the current locale encoding.
//     std::string         std::wstring          mpt::ustring                    mpt::tsrtring                        CString
//     mpt::fmt            mpt::wfmt             mpt::ufmt                       mpt::tfmt                            mpt::cfmt
//     mpt::format("%1")   mpt::wformat(L"%1")   mpt::uformat(MPT_ULITERAL(%1)   mpt::tformat(_T("%1"))               mpt::cformat(_T("%1"))
//     mpt::format("%1")   mpt::format(L"%1")    mpt::format(MPT_USTRING(%1))    mpt::format(mpt::tstring(_T("%1"))   mpt::format(CString(_T("%1"))
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
template <typename T> auto ToString(const T & x) -> decltype(mpt::ToCharset(mpt::Charset::UTF8, x.ToUString())) { return mpt::ToCharset(mpt::Charset::UTF8, x.ToUString()); }
#else
template <typename T> auto ToString(const T & x) -> decltype(mpt::ToCharset(mpt::CharsetLocaleOrUTF8, x.ToUString())) { return mpt::ToCharset(mpt::CharsetLocaleOrUTF8, x.ToUString()); }
#endif

static inline std::string ToString(const std::string & x) { return x; }
static inline std::string ToString(const char * const & x) { return x; }
std::string ToString(const char &x) = delete; // deprecated to catch potential API mis-use, use std::string(1, x) instead
#if MPT_WSTRING_FORMAT
std::string ToString(const std::wstring & x) = delete; // Unknown encoding.
std::string ToString(const wchar_t * const & x) = delete; // Unknown encoding.
std::string ToString(const wchar_t &x ) = delete; // deprecated to catch potential API mis-use, use std::wstring(1, x) instead
#endif
#if MPT_USTRING_MODE_UTF8
std::string ToString(const mpt::ustring & x) = delete; // Unknown encoding.
#endif
#if defined(MPT_WITH_MFC)
std::string ToString(const CString & x) = delete; // unknown encoding
#endif // MPT_WITH_MFC
std::string ToString(const bool & x);
std::string ToString(const signed char & x);
std::string ToString(const unsigned char & x);
std::string ToString(const signed short & x);
std::string ToString(const unsigned short & x);
std::string ToString(const signed int & x);
std::string ToString(const unsigned int & x);
std::string ToString(const signed long & x);
std::string ToString(const unsigned long & x);
std::string ToString(const signed long long & x);
std::string ToString(const unsigned long long & x);
std::string ToString(const float & x);
std::string ToString(const double & x);
std::string ToString(const long double & x);

// fallback to member function ToUString()
template <typename T> auto ToUString(const T & x) -> decltype(x.ToUString()) { return x.ToUString(); }

static inline mpt::ustring ToUString(const mpt::ustring & x) { return x; }
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
static inline std::wstring ToWString(const std::wstring & x) { return x; }
static inline std::wstring ToWString(const wchar_t * const & x) { return x; }
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
template <> struct ToStringTFunctor<std::string> { template <typename T> inline std::string operator() (const T & x) { return ToString(x); } };
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



struct fmt_base
{

enum FormatFlagsEnum
{
	BaseDec = 0x0001, // base 10 (integers only)
	BaseHex = 0x0002, // base 16 (integers only)
	CaseLow = 0x0010, // lower case hex digits
	CaseUpp = 0x0020, // upper case hex digits
	FillOff = 0x0100, // do not fill up width
	FillNul = 0x0400, // fill up width with zeros
	NotaNrm = 0x1000, // float: normal/default notation
	NotaFix = 0x2000, // float: fixed point notation
	NotaSci = 0x4000, // float: scientific notation
};

}; // struct fmt_base

typedef unsigned int FormatFlags;

static_assert(sizeof(FormatFlags) >= sizeof(fmt_base::FormatFlagsEnum));

class FormatSpec;

std::string FormatVal(const char & x, const FormatSpec & f) = delete; // deprecated to catch potential API mis-use, use std::string(1, x) instead
#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
std::string FormatVal(const wchar_t & x, const FormatSpec & f) = delete; // deprecated to catch potential API mis-use, use std::wstring(1, x) instead
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR
std::string FormatVal(const bool & x, const FormatSpec & f);
std::string FormatVal(const signed char & x, const FormatSpec & f);
std::string FormatVal(const unsigned char & x, const FormatSpec & f);
std::string FormatVal(const signed short & x, const FormatSpec & f);
std::string FormatVal(const unsigned short & x, const FormatSpec & f);
std::string FormatVal(const signed int & x, const FormatSpec & f);
std::string FormatVal(const unsigned int & x, const FormatSpec & f);
std::string FormatVal(const signed long & x, const FormatSpec & f);
std::string FormatVal(const unsigned long & x, const FormatSpec & f);
std::string FormatVal(const signed long long & x, const FormatSpec & f);
std::string FormatVal(const unsigned long long & x, const FormatSpec & f);
std::string FormatVal(const float & x, const FormatSpec & f);
std::string FormatVal(const double & x, const FormatSpec & f);
std::string FormatVal(const long double & x, const FormatSpec & f);

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
template <> struct FormatValTFunctor<std::string> { template <typename T> inline std::string operator() (const T & x, const FormatSpec & f) { return FormatVal(x, f); } };
template <> struct FormatValTFunctor<mpt::ustring> { template <typename T> inline mpt::ustring operator() (const T & x, const FormatSpec & f) { return FormatValU(x, f); } };
#if MPT_USTRING_MODE_UTF8 && MPT_WSTRING_FORMAT
template <> struct FormatValTFunctor<std::wstring> { template <typename T> inline std::wstring operator() (const T & x, const FormatSpec & f) { return FormatValW(x, f); } };
#endif
#if defined(MPT_ENABLE_CHARSET_LOCALE)
template <> struct FormatValTFunctor<mpt::lstring> { template <typename T> inline mpt::lstring operator() (const T & x, const FormatSpec & f) { return mpt::ToLocale(mpt::Charset::Locale, FormatVal(x, f)); } };
#endif // MPT_ENABLE_CHARSET_LOCALE
#if defined(MPT_WITH_MFC )
#ifdef UNICODE
template <> struct FormatValTFunctor<CString> { template <typename T> inline CString operator() (const T & x, const FormatSpec & f) { return mpt::ToCString(FormatValW(x, f)); } };
#else // !UNICODE
template <> struct FormatValTFunctor<CString> { template <typename T> inline CString operator() (const T & x, const FormatSpec & f) { return mpt::ToCString(mpt::Charset::Locale, FormatVal(x, f)); } };
#endif // UNICODE
#endif // MPT_WITH_MFC


class FormatSpec
{
private:
	FormatFlags flags;
	std::size_t width;
	int precision;
	unsigned int group;
	char group_sep;
public:
	MPT_CONSTEXPR11_FUN FormatSpec() noexcept : flags(0), width(0), precision(-1), group(0), group_sep(',') {}
	MPT_CONSTEXPR11_FUN FormatFlags GetFlags() const noexcept { return flags; }
	MPT_CONSTEXPR11_FUN std::size_t GetWidth() const noexcept { return width; }
	MPT_CONSTEXPR11_FUN int GetPrecision() const noexcept { return precision; }
	MPT_CONSTEXPR11_FUN unsigned int GetGroup() const noexcept { return group; }
	MPT_CONSTEXPR11_FUN char GetGroupSep() const noexcept { return group_sep; }
	MPT_CONSTEXPR14_FUN FormatSpec & SetFlags(FormatFlags f) noexcept { flags = f; return *this; }
	MPT_CONSTEXPR14_FUN FormatSpec & SetWidth(std::size_t w) noexcept { width = w; return *this; }
	MPT_CONSTEXPR14_FUN FormatSpec & SetPrecision(int p) noexcept { precision = p; return *this; }
	MPT_CONSTEXPR14_FUN FormatSpec & SetGroup(unsigned int g) noexcept { group = g; return *this; }
	MPT_CONSTEXPR14_FUN FormatSpec & SetGroupSep(char s) noexcept { group_sep = s; return *this; }
public:
	MPT_CONSTEXPR14_FUN FormatSpec & BaseDec() noexcept { flags &= ~(fmt_base::BaseDec|fmt_base::BaseHex); flags |= fmt_base::BaseDec; return *this; }
	MPT_CONSTEXPR14_FUN FormatSpec & BaseHex() noexcept { flags &= ~(fmt_base::BaseDec|fmt_base::BaseHex); flags |= fmt_base::BaseHex; return *this; }
	MPT_CONSTEXPR14_FUN FormatSpec & CaseLow() noexcept { flags &= ~(fmt_base::CaseLow|fmt_base::CaseUpp); flags |= fmt_base::CaseLow; return *this; }
	MPT_CONSTEXPR14_FUN FormatSpec & CaseUpp() noexcept { flags &= ~(fmt_base::CaseLow|fmt_base::CaseUpp); flags |= fmt_base::CaseUpp; return *this; }
	MPT_CONSTEXPR14_FUN FormatSpec & FillOff() noexcept { flags &= ~(fmt_base::FillOff|fmt_base::FillNul); flags |= fmt_base::FillOff; return *this; }
	MPT_CONSTEXPR14_FUN FormatSpec & FillNul() noexcept { flags &= ~(fmt_base::FillOff|fmt_base::FillNul); flags |= fmt_base::FillNul; return *this; }
	MPT_CONSTEXPR14_FUN FormatSpec & NotaNrm() noexcept { flags &= ~(fmt_base::NotaNrm|fmt_base::NotaFix|fmt_base::NotaSci); flags |= fmt_base::NotaNrm; return *this; }
	MPT_CONSTEXPR14_FUN FormatSpec & NotaFix() noexcept { flags &= ~(fmt_base::NotaNrm|fmt_base::NotaFix|fmt_base::NotaSci); flags |= fmt_base::NotaFix; return *this; }
	MPT_CONSTEXPR14_FUN FormatSpec & NotaSci() noexcept { flags &= ~(fmt_base::NotaNrm|fmt_base::NotaFix|fmt_base::NotaSci); flags |= fmt_base::NotaSci; return *this; }
	MPT_CONSTEXPR14_FUN FormatSpec & Width(std::size_t w) noexcept { width = w; return *this; }
	MPT_CONSTEXPR14_FUN FormatSpec & Prec(int p) noexcept { precision = p; return *this; }
	MPT_CONSTEXPR14_FUN FormatSpec & Group(unsigned int g) noexcept { group = g; return *this; }
	MPT_CONSTEXPR14_FUN FormatSpec & GroupSep(char s) noexcept { group_sep = s; return *this; }
public:
	MPT_CONSTEXPR14_FUN FormatSpec & Dec() noexcept { return BaseDec(); }
	MPT_CONSTEXPR14_FUN FormatSpec & Hex() noexcept { return BaseHex(); }
	MPT_CONSTEXPR14_FUN FormatSpec & Low() noexcept { return CaseLow(); }
	MPT_CONSTEXPR14_FUN FormatSpec & Upp() noexcept { return CaseUpp(); }
	MPT_CONSTEXPR14_FUN FormatSpec & Off() noexcept { return FillOff(); }
	MPT_CONSTEXPR14_FUN FormatSpec & Nul() noexcept { return FillNul(); }
	MPT_CONSTEXPR14_FUN FormatSpec & Nrm() noexcept { return NotaNrm(); }
	MPT_CONSTEXPR14_FUN FormatSpec & Fix() noexcept { return NotaFix(); }
	MPT_CONSTEXPR14_FUN FormatSpec & Sci() noexcept { return NotaSci(); }
public:
	MPT_CONSTEXPR14_FUN FormatSpec & Decimal() noexcept { return BaseDec(); }
	MPT_CONSTEXPR14_FUN FormatSpec & Hexadecimal() noexcept { return BaseHex(); }
	MPT_CONSTEXPR14_FUN FormatSpec & Lower() noexcept { return CaseLow(); }
	MPT_CONSTEXPR14_FUN FormatSpec & Upper() noexcept { return CaseUpp(); }
	MPT_CONSTEXPR14_FUN FormatSpec & FillNone() noexcept { return FillOff(); }
	MPT_CONSTEXPR14_FUN FormatSpec & FillZero() noexcept { return FillNul(); }
	MPT_CONSTEXPR14_FUN FormatSpec & FloatNormal() noexcept { return NotaNrm(); }
	MPT_CONSTEXPR14_FUN FormatSpec & FloatFixed() noexcept { return NotaFix(); }
	MPT_CONSTEXPR14_FUN FormatSpec & FloatScientific() noexcept { return NotaSci(); }
	MPT_CONSTEXPR14_FUN FormatSpec & Precision(int p) noexcept { return Prec(p); }
};

template <typename Tdst, typename Tsrc>
struct pointer_cast_helper
{
	Tdst operator()(const Tsrc & src) const { return src; }
};
template <typename Tdst, typename Tptr>
struct pointer_cast_helper<Tdst, const Tptr*>
{
	Tdst operator()(const Tptr * const & src) const { return reinterpret_cast<const Tdst>(src); }
};
template <typename Tdst, typename Tptr>
struct pointer_cast_helper<Tdst, Tptr*>
{
	Tdst operator()(const Tptr * const & src) const { return reinterpret_cast<const Tdst>(src); }
};


template <typename Tdst, typename Tsrc>
Tdst pointer_cast(const Tsrc & src)
{
	return pointer_cast_helper<Tdst, Tsrc>()(src);
}

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
	return hex0<mpt::pointer_size * 2>(pointer_cast<const std::uintptr_t>(x));
}
template<typename T>
static inline Tstring PTR(const T& x)
{
	static_assert(std::is_pointer<T>::value || std::is_same<T, std::uintptr_t>::value || std::is_same<T, std::intptr_t>::value, "");
	return HEX0<mpt::pointer_size * 2>(pointer_cast<const std::uintptr_t>(x));
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


typedef fmtT<std::string> fmt;
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

} // namespace mpt

namespace mpt {

namespace String {

namespace detail
{

template <typename T> struct to_string_type { };
template <> struct to_string_type<std::string    > { typedef std::string  type; };
template <> struct to_string_type<char           > { typedef std::string  type; };
template <> struct to_string_type<char *         > { typedef std::string  type; };
template <> struct to_string_type<const char     > { typedef std::string  type; };
template <> struct to_string_type<const char *   > { typedef std::string  type; };
#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
template <> struct to_string_type<std::wstring   > { typedef std::wstring type; };
template <> struct to_string_type<wchar_t        > { typedef std::wstring type; };
template <> struct to_string_type<wchar_t *      > { typedef std::wstring type; };
template <> struct to_string_type<const wchar_t  > { typedef std::wstring type; };
template <> struct to_string_type<const wchar_t *> { typedef std::wstring type; };
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR
#if MPT_USTRING_MODE_UTF8
template <> struct to_string_type<mpt::ustring   > { typedef mpt::ustring type; };
#endif
#if defined(MPT_ENABLE_CHARSET_LOCALE)
template <> struct to_string_type<mpt::lstring   > { typedef mpt::lstring type; };
#endif // MPT_ENABLE_CHARSET_LOCALE
#if defined(MPT_WITH_MFC)
template <> struct to_string_type<CString        > { typedef CString      type; };
#endif // MPT_WITH_MFC
template <typename T, std::size_t N> struct to_string_type<T [N]> { typedef typename to_string_type<T>::type type; };

} // namespace detail

} // namespace String

template<typename Tformat>
class message_formatter
{

public:

	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;

private:

	Tstring format;

private:

	MPT_NOINLINE Tstring do_format(mpt::span<const Tstring> vals) const
	{
		typedef typename mpt::string_traits<Tstring> traits;
		Tstring result;
		const typename traits::size_type len = traits::length(format);
		traits::reserve(result, len);
		for(typename traits::size_type pos = 0; pos != len; ++pos)
		{
			typename traits::char_type c = format[pos];
			if(pos + 1 != len && c == typename traits::char_type('%'))
			{
				pos++;
				c = format[pos];
				if(typename traits::char_type('1') <= c && c <= typename traits::char_type('9'))
				{
					const std::size_t n = c - typename traits::char_type('0') - 1;
					if(n < std::size(vals))
					{
						traits::append(result, vals[n]);
					}
					continue;
				} else if(c != typename traits::char_type('%'))
				{
					traits::append(result, 1, typename traits::char_type('%'));
				}
			}
			traits::append(result, 1, c);
		}
		return result;
	}

public:

	message_formatter(Tstring format_)
		: format(std::move(format_))
	{
	}

public:

	template<typename ...Ts>
	Tstring operator() (const Ts&... xs) const
	{
		const std::array<Tstring, sizeof...(xs)> vals{{ToStringTFunctor<Tstring>()(xs)...}};
		return do_format(mpt::as_span(vals));
	}

}; // struct message_formatter<Tformat>

template<typename Tformat>
message_formatter<typename mpt::String::detail::to_string_type<Tformat>::type> format(Tformat format)
{
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return message_formatter<Tstring>(Tstring(std::move(format)));
}

#if MPT_WSTRING_FORMAT
static inline message_formatter<std::wstring> wformat(std::wstring format)
{
	return message_formatter<std::wstring>(std::move(format));
}
#endif

static inline message_formatter<mpt::ustring> uformat(mpt::ustring format)
{
	return message_formatter<mpt::ustring>(std::move(format));
}

#if defined(MPT_ENABLE_CHARSET_LOCALE)
static inline message_formatter<mpt::lstring> lformat(mpt::lstring format)
{
	return message_formatter<mpt::lstring>(std::move(format));
}
#endif // MPT_ENABLE_CHARSET_LOCALE

#if MPT_OS_WINDOWS
static inline message_formatter<mpt::tstring> tformat(mpt::tstring format)
{
	return message_formatter<mpt::tstring>(std::move(format));
}
#endif

#if defined(MPT_WITH_MFC)
static inline message_formatter<CString> cformat(CString format)
{
	return message_formatter<CString>(std::move(format));
}
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
		str += mpt::fmt::val(vals[i]);
	}
	return str;
}

} } // namespace mpt::String



OPENMPT_NAMESPACE_END
