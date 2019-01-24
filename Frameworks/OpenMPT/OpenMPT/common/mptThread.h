/*
 * mptThread.h
 * -----------
 * Purpose: Helper class for running threads, with a more or less platform-independent interface.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "BuildSettings.h"

#if defined(MPT_ENABLE_THREAD)

#include <thread>

#if defined(MODPLUG_TRACKER)
#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER

#endif // MPT_ENABLE_THREAD


OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_ENABLE_THREAD)

namespace mpt
{


#if defined(MODPLUG_TRACKER)

#if MPT_OS_WINDOWS && (MPT_COMPILER_MSVC || MPT_COMPILER_CLANG)

enum ThreadPriority
{
	ThreadPriorityLowest  = THREAD_PRIORITY_LOWEST,
	ThreadPriorityLower   = THREAD_PRIORITY_BELOW_NORMAL,
	ThreadPriorityNormal  = THREAD_PRIORITY_NORMAL,
	ThreadPriorityHigh    = THREAD_PRIORITY_ABOVE_NORMAL,
	ThreadPriorityHighest = THREAD_PRIORITY_HIGHEST
};

inline void SetThreadPriority(std::thread &t, mpt::ThreadPriority priority)
{
	::SetThreadPriority(t.native_handle(), priority);
}

inline void SetCurrentThreadPriority(mpt::ThreadPriority priority)
{
	::SetThreadPriority(GetCurrentThread(), priority);
}

#else // !MPT_OS_WINDOWS

enum ThreadPriority
{
	ThreadPriorityLowest  = -2,
	ThreadPriorityLower   = -1,
	ThreadPriorityNormal  =  0,
	ThreadPriorityHigh    =  1,
	ThreadPriorityHighest =  2
};

inline void SetThreadPriority(std::thread & /*t*/ , mpt::ThreadPriority /*priority*/ )
{
	// nothing
}

inline void SetCurrentThreadPriority(mpt::ThreadPriority /*priority*/ )
{
	// nothing
}

#endif // MPT_OS_WINDOWS && (MPT_COMPILER_MSVC || MPT_COMPILER_CLANG)

#endif // MODPLUG_TRACKER



#if defined(MODPLUG_TRACKER)

#if MPT_OS_WINDOWS

// Default WinAPI thread
class UnmanagedThread
{
protected:
	HANDLE threadHandle;

public:

	operator HANDLE& () { return threadHandle; }
	operator bool () const { return threadHandle != nullptr; }

	UnmanagedThread() : threadHandle(nullptr) { }
	UnmanagedThread(LPTHREAD_START_ROUTINE function, void *userData = nullptr)
	{
		DWORD dummy = 0;	// For Win9x
		threadHandle = CreateThread(NULL, 0, function, userData, 0, &dummy);
	}
};

// Thread that operates on a member function
template<typename T, void (T::*Fun)()>
class UnmanagedThreadMember : public mpt::UnmanagedThread
{
protected:
	static DWORD WINAPI wrapperFunc(LPVOID param)
	{
		(static_cast<T *>(param)->*Fun)();
		return 0;
	}

public:

	UnmanagedThreadMember(T *instance) : mpt::UnmanagedThread(wrapperFunc, instance) { }
};

inline void SetThreadPriority(mpt::UnmanagedThread &t, mpt::ThreadPriority priority)
{
	::SetThreadPriority(t, priority);
}

#endif // MPT_OS_WINDOWS

#endif // MODPLUG_TRACKER



}	// namespace mpt

#endif // MPT_ENABLE_THREAD

OPENMPT_NAMESPACE_END
