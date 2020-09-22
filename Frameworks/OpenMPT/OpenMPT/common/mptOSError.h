/*
 * mptOSError.h
 * ------------
 * Purpose: OS-specific error message handling.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"



#include "mptBaseMacros.h"
#include "mptException.h"
#include "mptString.h"
#include "mptStringFormat.h"

#if defined(MODPLUG_TRACKER)
#if MPT_OS_WINDOWS
#include <stdexcept>
#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER

#if defined(MODPLUG_TRACKER)
#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER



OPENMPT_NAMESPACE_BEGIN


#if defined(MODPLUG_TRACKER)
#if MPT_OS_WINDOWS

namespace Windows
{


inline mpt::ustring GetErrorMessage(DWORD errorCode, HANDLE hModule = NULL)
{
	mpt::ustring message;
	void *lpMsgBuf = nullptr;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | (hModule ? FORMAT_MESSAGE_FROM_HMODULE : 0) | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		hModule,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);
	if(!lpMsgBuf)
	{
		DWORD e = GetLastError();
		if((e == ERROR_NOT_ENOUGH_MEMORY) || (e == ERROR_OUTOFMEMORY))
		{
			MPT_EXCEPTION_THROW_OUT_OF_MEMORY();
		}
		return {};
	}
	try
	{
		message = mpt::ToUnicode(mpt::winstring((LPTSTR)lpMsgBuf));
	} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
	{
		LocalFree(lpMsgBuf);
		MPT_EXCEPTION_RETHROW_OUT_OF_MEMORY(e);
	}
	LocalFree(lpMsgBuf);
	return message;
}


class Error
	: public std::runtime_error
{
public:
	Error(DWORD errorCode, HANDLE hModule = NULL)
		: std::runtime_error(mpt::ToCharset(mpt::CharsetException, mpt::format(U_("Windows Error: 0x%1: %2"))(mpt::ufmt::hex0<8>(errorCode), GetErrorMessage(errorCode, hModule))))
	{
		return;
	}
};


} // namespace Windows

#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER



OPENMPT_NAMESPACE_END
