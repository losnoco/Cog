/*
 * mptFileIO.h
 * -----------
 * Purpose: A wrapper around std::fstream, enforcing usage of mpt::PathString.
 * Notes  : You should only ever use these wrappers instead of plain std::fstream classes.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "openmpt/all/BuildSettings.hpp"

#if defined(MPT_ENABLE_FILEIO)

#include "mpt/base/detect_libcxx.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/io_file/fstream.hpp"
#include "mpt/io_file_read/inputfile_filecursor.hpp"

#include "../common/mptString.h"
#include "../common/mptPathString.h"
#include "../common/FileReaderFwd.h"

#include <utility>

#ifdef MODPLUG_TRACKER
#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER

#endif // MPT_ENABLE_FILEIO


OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_ENABLE_FILEIO)


// Sets the NTFS compression attribute on the file or directory.
// Requires read and write permissions for already opened files.
// Returns true if the attribute has been set.
// In almost all cases, the return value should be ignored because most filesystems other than NTFS do not support compression.
#ifdef MODPLUG_TRACKER
#if MPT_OS_WINDOWS
bool SetFilesystemCompression(HANDLE hFile);
bool SetFilesystemCompression(int fd);
bool SetFilesystemCompression(const mpt::PathString &filename);
#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER


namespace mpt
{

using fstream = mpt::IO::fstream;
using ifstream = mpt::IO::ifstream;
using ofstream = mpt::IO::ofstream;

} // namespace mpt


template <typename Targ1>
inline FileCursor GetFileReader(Targ1 &&arg1)
{
	return mpt::IO::make_FileCursor<mpt::PathString>(std::forward<Targ1>(arg1));
}


template <typename Targ1, typename Targ2>
inline FileCursor GetFileReader(Targ1 &&arg1, Targ2 &&arg2)
{
	return mpt::IO::make_FileCursor<mpt::PathString>(std::forward<Targ1>(arg1), std::forward<Targ2>(arg2));
}


#endif // MPT_ENABLE_FILEIO


OPENMPT_NAMESPACE_END

