/*
 * mptStringFormat.cpp
 * -------------------
 * Purpose: Convert other types to strings.
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "mptStringFormat.h"

#include "mpt/format/default_floatingpoint.hpp"
#include "mpt/format/default_integer.hpp"
#include "mpt/format/helpers.hpp"
#include "mpt/format/simple_floatingpoint.hpp"
#include "mpt/format/simple_integer.hpp"


OPENMPT_NAMESPACE_BEGIN


namespace mpt
{



std::string ToAString(const bool & x) { return mpt::format_value_default<std::string>(x); }
std::string ToAString(const signed char & x) { return mpt::format_value_default<std::string>(x); }
std::string ToAString(const unsigned char & x) { return mpt::format_value_default<std::string>(x); }
std::string ToAString(const signed short & x) { return mpt::format_value_default<std::string>(x); }
std::string ToAString(const unsigned short & x) { return mpt::format_value_default<std::string>(x); }
std::string ToAString(const signed int & x) { return mpt::format_value_default<std::string>(x); }
std::string ToAString(const unsigned int & x) { return mpt::format_value_default<std::string>(x); }
std::string ToAString(const signed long & x) { return mpt::format_value_default<std::string>(x); }
std::string ToAString(const unsigned long & x) { return mpt::format_value_default<std::string>(x); }
std::string ToAString(const signed long long & x) { return mpt::format_value_default<std::string>(x); }
std::string ToAString(const unsigned long long & x) { return mpt::format_value_default<std::string>(x); }
std::string ToAString(const float & x) { return mpt::format_value_default<std::string>(x); }
std::string ToAString(const double & x) { return mpt::format_value_default<std::string>(x); }
std::string ToAString(const long double & x) { return mpt::format_value_default<std::string>(x); }

#if MPT_WSTRING_FORMAT
#if MPT_USTRING_MODE_UTF8
mpt::ustring ToUString(const std::wstring & x) { return mpt::ToUnicode(x); }
#endif
mpt::ustring ToUString(const wchar_t * const & x) { return mpt::ToUnicode(x); }
#endif
#if defined(MPT_WITH_MFC)
mpt::ustring ToUString(const CString & x)  { return mpt::ToUnicode(x); }
#endif // MPT_WITH_MFC
#if MPT_USTRING_MODE_WIDE
mpt::ustring ToUString(const bool & x) { return mpt::format_value_default<std::wstring>(x); }
mpt::ustring ToUString(const signed char & x) { return mpt::format_value_default<std::wstring>(x); }
mpt::ustring ToUString(const unsigned char & x) { return mpt::format_value_default<std::wstring>(x); }
mpt::ustring ToUString(const signed short & x) { return mpt::format_value_default<std::wstring>(x); }
mpt::ustring ToUString(const unsigned short & x) { return mpt::format_value_default<std::wstring>(x); }
mpt::ustring ToUString(const signed int & x) { return mpt::format_value_default<std::wstring>(x); }
mpt::ustring ToUString(const unsigned int & x) { return mpt::format_value_default<std::wstring>(x); }
mpt::ustring ToUString(const signed long & x) { return mpt::format_value_default<std::wstring>(x); }
mpt::ustring ToUString(const unsigned long & x) { return mpt::format_value_default<std::wstring>(x); }
mpt::ustring ToUString(const signed long long & x) { return mpt::format_value_default<std::wstring>(x); }
mpt::ustring ToUString(const unsigned long long & x) { return mpt::format_value_default<std::wstring>(x); }
mpt::ustring ToUString(const float & x) { return mpt::format_value_default<std::wstring>(x); }
mpt::ustring ToUString(const double & x) { return mpt::format_value_default<std::wstring>(x); }
mpt::ustring ToUString(const long double & x) { return mpt::format_value_default<std::wstring>(x); }
#endif
#if MPT_USTRING_MODE_UTF8
mpt::ustring ToUString(const bool & x) { return mpt::format_value_default<mpt::ustring>(x); }
mpt::ustring ToUString(const signed char & x) { return mpt::format_value_default<mpt::ustring>(x); }
mpt::ustring ToUString(const unsigned char & x) { return mpt::format_value_default<mpt::ustring>(x); }
mpt::ustring ToUString(const signed short & x) { return mpt::format_value_default<mpt::ustring>(x); }
mpt::ustring ToUString(const unsigned short & x) { return mpt::format_value_default<mpt::ustring>(x); }
mpt::ustring ToUString(const signed int & x) { return mpt::format_value_default<mpt::ustring>(x); }
mpt::ustring ToUString(const unsigned int & x) { return mpt::format_value_default<mpt::ustring>(x); }
mpt::ustring ToUString(const signed long & x) { return mpt::format_value_default<mpt::ustring>(x); }
mpt::ustring ToUString(const unsigned long & x) { return mpt::format_value_default<mpt::ustring>(x); }
mpt::ustring ToUString(const signed long long & x) { return mpt::format_value_default<mpt::ustring>(x); }
mpt::ustring ToUString(const unsigned long long & x) { return mpt::format_value_default<mpt::ustring>(x); }
mpt::ustring ToUString(const float & x) { return mpt::format_value_default<mpt::ustring>(x); }
mpt::ustring ToUString(const double & x) { return mpt::format_value_default<mpt::ustring>(x); }
mpt::ustring ToUString(const long double & x) { return mpt::format_value_default<mpt::ustring>(x); }
#endif

#if MPT_WSTRING_FORMAT
#if MPT_USTRING_MODE_UTF8
std::wstring ToWString(const mpt::ustring & x) { return mpt::ToWide(x); }
#endif
#if defined(MPT_WITH_MFC)
std::wstring ToWString(const CString & x) { return mpt::ToWide(x); }
#endif // MPT_WITH_MFC
std::wstring ToWString(const bool & x) { return mpt::format_value_default<std::wstring>(x); }
std::wstring ToWString(const signed char & x) { return mpt::format_value_default<std::wstring>(x); }
std::wstring ToWString(const unsigned char & x) { return mpt::format_value_default<std::wstring>(x); }
std::wstring ToWString(const signed short & x) { return mpt::format_value_default<std::wstring>(x); }
std::wstring ToWString(const unsigned short & x) { return mpt::format_value_default<std::wstring>(x); }
std::wstring ToWString(const signed int & x) { return mpt::format_value_default<std::wstring>(x); }
std::wstring ToWString(const unsigned int & x) { return mpt::format_value_default<std::wstring>(x); }
std::wstring ToWString(const signed long & x) { return mpt::format_value_default<std::wstring>(x); }
std::wstring ToWString(const unsigned long & x) { return mpt::format_value_default<std::wstring>(x); }
std::wstring ToWString(const signed long long & x) { return mpt::format_value_default<std::wstring>(x); }
std::wstring ToWString(const unsigned long long & x) { return mpt::format_value_default<std::wstring>(x); }
std::wstring ToWString(const float & x) { return mpt::format_value_default<std::wstring>(x); }
std::wstring ToWString(const double & x) { return mpt::format_value_default<std::wstring>(x); }
std::wstring ToWString(const long double & x) { return mpt::format_value_default<std::wstring>(x); }
#endif



std::string FormatValA(const bool & x, const FormatSpec & f) { return mpt::format_simple<std::string>(x, f); }
std::string FormatValA(const signed char & x, const FormatSpec & f) { return mpt::format_simple<std::string>(x, f); }
std::string FormatValA(const unsigned char & x, const FormatSpec & f) { return mpt::format_simple<std::string>(x, f); }
std::string FormatValA(const signed short & x, const FormatSpec & f) { return mpt::format_simple<std::string>(x, f); }
std::string FormatValA(const unsigned short & x, const FormatSpec & f) { return mpt::format_simple<std::string>(x, f); }
std::string FormatValA(const signed int & x, const FormatSpec & f) { return mpt::format_simple<std::string>(x, f); }
std::string FormatValA(const unsigned int & x, const FormatSpec & f) { return mpt::format_simple<std::string>(x, f); }
std::string FormatValA(const signed long & x, const FormatSpec & f) { return mpt::format_simple<std::string>(x, f); }
std::string FormatValA(const unsigned long & x, const FormatSpec & f) { return mpt::format_simple<std::string>(x, f); }
std::string FormatValA(const signed long long & x, const FormatSpec & f) { return mpt::format_simple<std::string>(x, f); }
std::string FormatValA(const unsigned long long & x, const FormatSpec & f) { return mpt::format_simple<std::string>(x, f); }
std::string FormatValA(const float & x, const FormatSpec & f) { return mpt::format_simple<std::string>(x, f); }
std::string FormatValA(const double & x, const FormatSpec & f) { return mpt::format_simple<std::string>(x, f); }
std::string FormatValA(const long double & x, const FormatSpec & f) { return mpt::format_simple<std::string>(x, f); }

#if MPT_USTRING_MODE_WIDE
mpt::ustring FormatValU(const bool & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
mpt::ustring FormatValU(const signed char & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
mpt::ustring FormatValU(const unsigned char & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
mpt::ustring FormatValU(const signed short & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
mpt::ustring FormatValU(const unsigned short & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
mpt::ustring FormatValU(const signed int & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
mpt::ustring FormatValU(const unsigned int & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
mpt::ustring FormatValU(const signed long & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
mpt::ustring FormatValU(const unsigned long & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
mpt::ustring FormatValU(const signed long long & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
mpt::ustring FormatValU(const unsigned long long & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
mpt::ustring FormatValU(const float & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
mpt::ustring FormatValU(const double & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
mpt::ustring FormatValU(const long double & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
#endif
#if MPT_USTRING_MODE_UTF8
mpt::ustring FormatValU(const bool & x, const FormatSpec & f) { return mpt::format_simple<mpt::ustring>(x, f); }
mpt::ustring FormatValU(const signed char & x, const FormatSpec & f) { return mpt::format_simple<mpt::ustring>(x, f); }
mpt::ustring FormatValU(const unsigned char & x, const FormatSpec & f) { return mpt::format_simple<mpt::ustring>(x, f); }
mpt::ustring FormatValU(const signed short & x, const FormatSpec & f) { return mpt::format_simple<mpt::ustring>(x, f); }
mpt::ustring FormatValU(const unsigned short & x, const FormatSpec & f) { return mpt::format_simple<mpt::ustring>(x, f); }
mpt::ustring FormatValU(const signed int & x, const FormatSpec & f) { return mpt::format_simple<mpt::ustring>(x, f); }
mpt::ustring FormatValU(const unsigned int & x, const FormatSpec & f) { return mpt::format_simple<mpt::ustring>(x, f); }
mpt::ustring FormatValU(const signed long & x, const FormatSpec & f) { return mpt::format_simple<mpt::ustring>(x, f); }
mpt::ustring FormatValU(const unsigned long & x, const FormatSpec & f) { return mpt::format_simple<mpt::ustring>(x, f); }
mpt::ustring FormatValU(const signed long long & x, const FormatSpec & f) { return mpt::format_simple<mpt::ustring>(x, f); }
mpt::ustring FormatValU(const unsigned long long & x, const FormatSpec & f) { return mpt::format_simple<mpt::ustring>(x, f); }
mpt::ustring FormatValU(const float & x, const FormatSpec & f) { return mpt::format_simple<mpt::ustring>(x, f); }
mpt::ustring FormatValU(const double & x, const FormatSpec & f) { return mpt::format_simple<mpt::ustring>(x, f); }
mpt::ustring FormatValU(const long double & x, const FormatSpec & f) { return mpt::format_simple<mpt::ustring>(x, f); }
#endif

#if MPT_WSTRING_FORMAT
std::wstring FormatValW(const bool & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
std::wstring FormatValW(const signed char & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
std::wstring FormatValW(const unsigned char & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
std::wstring FormatValW(const signed short & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
std::wstring FormatValW(const unsigned short & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
std::wstring FormatValW(const signed int & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
std::wstring FormatValW(const unsigned int & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
std::wstring FormatValW(const signed long & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
std::wstring FormatValW(const unsigned long & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
std::wstring FormatValW(const signed long long & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
std::wstring FormatValW(const unsigned long long & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
std::wstring FormatValW(const float & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
std::wstring FormatValW(const double & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
std::wstring FormatValW(const long double & x, const FormatSpec & f) { return mpt::format_simple<std::wstring>(x, f); }
#endif


} // namespace mpt


OPENMPT_NAMESPACE_END
