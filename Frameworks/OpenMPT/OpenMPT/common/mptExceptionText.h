/*
 * mptExceptionText.h
 * ------------------
 * Purpose: Guess encoding of exception string
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"



#include "mptException.h"
#include "mptString.h"

#include <exception>



OPENMPT_NAMESPACE_BEGIN



namespace mpt
{

template <typename T> T get_exception_text_impl(const std::exception & e)
{
	if(e.what() && (std::strlen(e.what()) > 0))
	{
		return T(e.what());
	} else if(typeid(e).name() && (std::strlen(typeid(e).name()) > 0))
	{
		return T(typeid(e).name());
	} else
	{
		return T("unknown exception");
	}
}

template <typename T> inline T get_exception_text(const std::exception & e)
{
	return mpt::get_exception_text_impl<T>(e);
}
template <> inline std::string get_exception_text<std::string>(const std::exception & e)
{
	return mpt::get_exception_text_impl<std::string>(e);
}
#if defined(MPT_ENABLE_CHARSET_LOCALE)
template <> inline mpt::lstring get_exception_text<mpt::lstring>(const std::exception & e)
{
	return mpt::ToLocale(mpt::CharsetException, mpt::get_exception_text_impl<std::string>(e));
}
#endif
#if MPT_WSTRING_FORMAT
template <> inline std::wstring get_exception_text<std::wstring>(const std::exception & e)
{
	return mpt::ToWide(mpt::CharsetException, mpt::get_exception_text_impl<std::string>(e));
}
#endif
#if MPT_USTRING_MODE_UTF8
template <> inline mpt::ustring get_exception_text<mpt::ustring>(const std::exception & e)
{
	return mpt::ToUnicode(mpt::CharsetException, mpt::get_exception_text_impl<std::string>(e));
}
#endif

} // namespace mpt



OPENMPT_NAMESPACE_END
