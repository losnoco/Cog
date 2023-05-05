/*
 * mptFileTemporary.h
 * ------------------
 * Purpose:
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/namespace.hpp"

#include "mptPathString.h"


OPENMPT_NAMESPACE_BEGIN



namespace mpt
{



#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS



// Returns a new unique absolute path.
class TemporaryPathname
{
private:
	mpt::PathString m_Path;
public:
	TemporaryPathname(const mpt::PathString &fileNameExtension = P_("tmp"));
public:
	mpt::PathString GetPathname() const
	{
		return m_Path;
	}
};



// Scoped temporary file guard. Deletes the file when going out of scope.
// The file itself is not created automatically.
class TempFileGuard
{
private:
	const mpt::PathString filename;
public:
	TempFileGuard(const mpt::TemporaryPathname &pathname = mpt::TemporaryPathname{});
	mpt::PathString GetFilename() const;
	~TempFileGuard();
};


// Scoped temporary directory guard. Deletes the directory when going out of scope.
// The directory itself is created automatically.
class TempDirGuard
{
private:
	mpt::PathString dirname;
public:
	TempDirGuard(const mpt::TemporaryPathname &pathname = mpt::TemporaryPathname{});
	mpt::PathString GetDirname() const;
	~TempDirGuard();
};



#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS



} // namespace mpt



OPENMPT_NAMESPACE_END
