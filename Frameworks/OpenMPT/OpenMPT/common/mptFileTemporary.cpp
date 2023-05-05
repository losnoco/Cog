/*
 * mptFileTemporary.cpp
 * --------------------
 * Purpose:
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "mptFileTemporary.h"

#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS
#include "mpt/fs/common_directories.hpp"
#include "mpt/fs/fs.hpp"
#include "mpt/io_file_unique/unique_basename.hpp"
#include "mpt/io_file_unique/unique_tempfilename.hpp"
#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS
#include "mpt/string_transcode/transcode.hpp"
#include "mpt/uuid/uuid.hpp"

#include "mptRandom.h"

#if MPT_OS_WINDOWS
#include <windows.h>
#endif



OPENMPT_NAMESPACE_BEGIN



#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS



namespace mpt
{



TemporaryPathname::TemporaryPathname(const mpt::PathString &fileNameExtension)
{
	mpt::PathString prefix;
#if defined(LIBOPENMPT_BUILD)
	prefix = P_("libopenmpt");
#else
	prefix = P_("OpenMPT");
#endif
	m_Path = mpt::PathString::FromNative(mpt::IO::unique_tempfilename{mpt::IO::unique_basename{prefix, mpt::UUID::GenerateLocalUseOnly(mpt::global_prng())}, fileNameExtension});
}



TempFileGuard::TempFileGuard(const mpt::TemporaryPathname &pathname)
	: filename(pathname.GetPathname())
{
	return;
}

mpt::PathString TempFileGuard::GetFilename() const
{
	return filename;
}

TempFileGuard::~TempFileGuard()
{
	if(!filename.empty())
	{
		DeleteFile(filename.AsNative().c_str());
	}
}


TempDirGuard::TempDirGuard(const mpt::TemporaryPathname &pathname)
	: dirname(pathname.GetPathname().WithTrailingSlash())
{
	if(dirname.empty())
	{
		return;
	}
	if(::CreateDirectory(dirname.AsNative().c_str(), NULL) == 0)
	{ // fail
		dirname = mpt::PathString();
	}
}

mpt::PathString TempDirGuard::GetDirname() const
{
	return dirname;
}

TempDirGuard::~TempDirGuard()
{
	if(!dirname.empty())
	{
		mpt::native_fs{}.delete_tree(dirname);
	}
}



} // namespace mpt



#else
MPT_MSVC_WORKAROUND_LNK4221(mptFileTemporary)
#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS



OPENMPT_NAMESPACE_END
