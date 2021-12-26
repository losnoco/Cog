/*
 * mptOSException.h
 * ----------------
 * Purpose: platform-specific exception/signal handling
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "mptBaseMacros.h"
#include "mptBaseTypes.h"

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


namespace SEH
{


struct Code
{
private:
	DWORD m_Code;
public:
	constexpr Code(DWORD code) noexcept
		: m_Code(code)
	{
		return;
	}
public:
	constexpr DWORD code() const noexcept
	{
		return m_Code;
	}
};


template <typename Tfn, typename Tfilter, typename Thandler>
auto TryFilterHandleThrow(const Tfn &fn, const Tfilter &filter, const Thandler &handler) -> decltype(fn())
{
	static_assert(std::is_trivially_copy_assignable<decltype(fn())>::value);
	static_assert(std::is_trivially_copy_constructible<decltype(fn())>::value);
	static_assert(std::is_trivially_move_assignable<decltype(fn())>::value);
	static_assert(std::is_trivially_move_constructible<decltype(fn())>::value);
	DWORD code = 0;
	__try
	{
		return fn();
	} __except(filter(GetExceptionCode(), GetExceptionInformation()))
	{
		code = GetExceptionCode();
	}
	throw Windows::SEH::Code(code);
}


template <typename Tfn, typename Tfilter, typename Thandler>
void TryFilterHandleVoid(const Tfn &fn, const Tfilter &filter, const Thandler &handler)
{
	static_assert(std::is_same<decltype(fn()), void>::value || std::is_trivially_copy_assignable<decltype(fn())>::value);
	static_assert(std::is_same<decltype(fn()), void>::value || std::is_trivially_copy_constructible<decltype(fn())>::value);
	static_assert(std::is_same<decltype(fn()), void>::value || std::is_trivially_move_assignable<decltype(fn())>::value);
	static_assert(std::is_same<decltype(fn()), void>::value || std::is_trivially_move_constructible<decltype(fn())>::value);
	__try
	{
		fn();
		return;
	} __except(filter(GetExceptionCode(), GetExceptionInformation()))
	{
		DWORD code = GetExceptionCode();
		handler(code);
	}
	return;
}


template <typename Tfn, typename Tfilter, typename Thandler>
auto TryFilterHandleDefault(const Tfn &fn, const Tfilter &filter, const Thandler &handler, decltype(fn()) def = decltype(fn()){}) -> decltype(fn())
{
	static_assert(std::is_trivially_copy_assignable<decltype(fn())>::value);
	static_assert(std::is_trivially_copy_constructible<decltype(fn())>::value);
	static_assert(std::is_trivially_move_assignable<decltype(fn())>::value);
	static_assert(std::is_trivially_move_constructible<decltype(fn())>::value);
	auto result = def;
	__try
	{
		result = fn();
	} __except(filter(GetExceptionCode(), GetExceptionInformation()))
	{
		DWORD code = GetExceptionCode();
		result = handler(code);
	}
	return result;
}


template <typename Tfn>
auto TryReturnOrThrow(const Tfn &fn) -> decltype(fn())
{
	return TryFilterHandleThrow(
		fn,
		[](auto code, auto eptr)
		{
			MPT_UNREFERENCED_PARAMETER(code);
			MPT_UNREFERENCED_PARAMETER(eptr);
			return EXCEPTION_EXECUTE_HANDLER;
		},
		[](auto code)
		{
			throw Windows::SEH::Code(code);
		});
}


template <typename Tfn>
DWORD TryOrError(const Tfn &fn)
{
	DWORD result = DWORD{0};
	TryFilterHandleVoid(
		fn,
		[](auto code, auto eptr)
		{
			MPT_UNREFERENCED_PARAMETER(code);
			MPT_UNREFERENCED_PARAMETER(eptr);
			return EXCEPTION_EXECUTE_HANDLER;
		},
		[&result](auto code)
		{
			result = code;
		});
	return result;
}


template <typename Tfn>
auto TryReturnOrDefault(const Tfn &fn, decltype(fn()) def = decltype(fn()){}) -> decltype(fn())
{
	return TryFilterHandleDefault(
		fn,
		[](auto code, auto eptr)
		{
			MPT_UNREFERENCED_PARAMETER(code);
			MPT_UNREFERENCED_PARAMETER(eptr);
			return EXCEPTION_EXECUTE_HANDLER;
		},
		[def](auto code)
		{
			MPT_UNREFERENCED_PARAMETER(code);
			return def;
		});
}


} // namspace SEH


}  // namespace Windows


#endif // MPT_OS_WINDOWS


#endif // MODPLUG_TRACKER


OPENMPT_NAMESPACE_END
