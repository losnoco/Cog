/*
 * mptFileIO.cpp
 * -------------
 * Purpose: File I/O wrappers
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptFileIO.h"

#ifdef MODPLUG_TRACKER
#if MPT_OS_WINDOWS
#include <windows.h>
#include <WinIoCtl.h>
#include <io.h>
#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER


OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_ENABLE_FILEIO)


#if !defined(MPT_BUILD_SILENCE_LIBOPENMPT_CONFIGURATION_WARNINGS)

#if defined(MPT_COMPILER_QUIRK_WINDOWS_FSTREAM_NO_WCHAR)
#if MPT_GCC_BEFORE(9,1,0)
MPT_WARNING("Warning: MinGW with GCC earlier than 9.1 detected. Standard library does neither provide std::fstream wchar_t overloads nor std::filesystem with wchar_t support. Unicode filename support is thus unavailable.")
#endif // MPT_GCC_AT_LEAST(9,1,0)
#endif // MPT_COMPILER_QUIRK_WINDOWS_FSTREAM_NO_WCHAR

#endif // !MPT_BUILD_SILENCE_LIBOPENMPT_CONFIGURATION_WARNINGS


#ifdef MODPLUG_TRACKER

#if MPT_OS_WINDOWS

bool SetFilesystemCompression(HANDLE hFile)
{
	if(hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	USHORT format = COMPRESSION_FORMAT_DEFAULT;
	DWORD dummy = 0;
	BOOL result = DeviceIoControl(hFile, FSCTL_SET_COMPRESSION, (LPVOID)&format, sizeof(format), NULL, 0, &dummy /*required*/ , NULL);
	return result != FALSE;
}

bool SetFilesystemCompression(int fd)
{
	if(fd < 0)
	{
		return false;
	}
	uintptr_t fhandle = _get_osfhandle(fd);
	HANDLE hFile = (HANDLE)fhandle;
	if(hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	return SetFilesystemCompression(hFile);
}

bool SetFilesystemCompression(const mpt::PathString &filename)
{
	DWORD attributes = GetFileAttributes(mpt::support_long_path(filename.AsNative()).c_str());
	if(attributes == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}
	if(attributes & FILE_ATTRIBUTE_COMPRESSED)
	{
		return true;
	}
	HANDLE hFile = CreateFile(mpt::support_long_path(filename.AsNative()).c_str(), GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	bool result = SetFilesystemCompression(hFile);
	CloseHandle(hFile);
	return result;
}

#endif // MPT_OS_WINDOWS

#endif // MODPLUG_TRACKER


#endif // MPT_ENABLE_FILEIO


OPENMPT_NAMESPACE_END
