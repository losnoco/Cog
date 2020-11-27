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
#include <WinIoCtl.h>
#include <io.h>
#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER

#if defined(MPT_ENABLE_FILEIO)
#if MPT_COMPILER_MSVC
#include <tchar.h>
#endif // MPT_COMPILER_MSVC
#endif // MPT_ENABLE_FILEIO


OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_ENABLE_FILEIO)



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
	DWORD attributes = GetFileAttributes(filename.AsNativePrefixed().c_str());
	if(attributes == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}
	if(attributes & FILE_ATTRIBUTE_COMPRESSED)
	{
		return true;
	}
	HANDLE hFile = CreateFile(filename.AsNativePrefixed().c_str(), GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
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



#ifdef MODPLUG_TRACKER

namespace mpt {

#if MPT_COMPILER_MSVC

mpt::tstring SafeOutputFile::convert_mode(std::ios_base::openmode mode, FlushMode flushMode)
{
	mpt::tstring fopen_mode;
	switch(mode & ~(std::ios_base::ate | std::ios_base::binary))
	{
	case std::ios_base::in:
		fopen_mode = _T("r");
		break;
	case std::ios_base::out:
		[[fallthrough]];
	case std::ios_base::out | std::ios_base::trunc:
		fopen_mode = _T("w");
		break;
	case std::ios_base::app:
		[[fallthrough]];
	case std::ios_base::out | std::ios_base::app:
		fopen_mode = _T("a");
		break;
	case std::ios_base::out | std::ios_base::in:
		fopen_mode = _T("r+");
		break;
	case std::ios_base::out | std::ios_base::in | std::ios_base::trunc:
		fopen_mode = _T("w+");
		break;
	case std::ios_base::out | std::ios_base::in | std::ios_base::app:
		[[fallthrough]];
	case std::ios_base::in | std::ios_base::app:
		fopen_mode = _T("a+");
		break;
	}
	if(fopen_mode.empty())
	{
		return fopen_mode;
	}
	if(mode & std::ios_base::binary)
	{
		fopen_mode += _T("b");
	}
	if(flushMode == FlushMode::Full)
	{
		fopen_mode += _T("c");  // force commit on fflush (MSVC specific)
	}
	return fopen_mode;
}

FILE * SafeOutputFile::internal_fopen(const mpt::PathString &filename, std::ios_base::openmode mode, FlushMode flushMode)
{
	mpt::tstring fopen_mode = convert_mode(mode, flushMode);
	if(fopen_mode.empty())
	{
		return nullptr;
	}
	FILE *f =
#ifdef UNICODE
		_wfopen(filename.AsNativePrefixed().c_str(), fopen_mode.c_str())
#else
		fopen(filename.AsNativePrefixed().c_str(), fopen_mode.c_str())
#endif
		;
	if(!f)
	{
		return nullptr;
	}
	if(mode & std::ios_base::ate)
	{
		if(fseek(f, 0, SEEK_END) != 0)
		{
			fclose(f);
			f = nullptr;
			return nullptr;
		}
	}
	m_f = f;
	return f;
}

#endif // MPT_COMPILER_MSVC

// cppcheck-suppress exceptThrowInDestructor
SafeOutputFile::~SafeOutputFile() noexcept(false)
{
	const bool mayThrow = (std::uncaught_exceptions() == 0);
	if(!stream())
	{
		#if MPT_COMPILER_MSVC
			if(m_f)
			{
				fclose(m_f);
			}
		#endif // MPT_COMPILER_MSVC
		return;
	}
	if(!stream().rdbuf())
	{
		#if MPT_COMPILER_MSVC
			if(m_f)
			{
				fclose(m_f);
			}
		#endif // MPT_COMPILER_MSVC
		return;
	}
#if MPT_COMPILER_MSVC
	if(!m_f)
	{
		return;
	}
#endif // MPT_COMPILER_MSVC
	bool errorOnFlush = false;
	if(m_FlushMode != FlushMode::None)
	{
		try
		{
			if(stream().rdbuf()->pubsync() != 0)
			{
				errorOnFlush = true;
			}
		} catch(const std::exception &)
		{
			errorOnFlush = true;
#if MPT_COMPILER_MSVC
			if(m_FlushMode != FlushMode::None)
			{
				if(fflush(m_f) != 0)
				{
					errorOnFlush = true;
				}
			}
			if(fclose(m_f) != 0)
			{
				errorOnFlush = true;
			}
#endif // MPT_COMPILER_MSVC
			if(mayThrow)
			{
				// ignore errorOnFlush here, and re-throw the earlier exception
				// cppcheck-suppress exceptThrowInDestructor
				throw;
			}
		}
	}
#if MPT_COMPILER_MSVC
	if(m_FlushMode != FlushMode::None)
	{
		if(fflush(m_f) != 0)
		{
			errorOnFlush = true;
		}
	}
	if(fclose(m_f) != 0)
	{
		errorOnFlush = true;
	}
#endif // MPT_COMPILER_MSVC
	if(mayThrow && errorOnFlush && (stream().exceptions() & (std::ios::badbit | std::ios::failbit)))
	{
		// cppcheck-suppress exceptThrowInDestructor
		throw std::ios_base::failure(std::string("Error flushing file buffers."));
	}
}

} // namespace mpt

#endif // MODPLUG_TRACKER



#ifdef MODPLUG_TRACKER

namespace mpt {

LazyFileRef & LazyFileRef::operator = (const std::vector<std::byte> &data)
{
	mpt::ofstream file(m_Filename, std::ios::binary);
	file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
	mpt::IO::WriteRaw(file, mpt::as_span(data));
	mpt::IO::Flush(file);
	return *this;
}

LazyFileRef & LazyFileRef::operator = (const std::vector<char> &data)
{
	mpt::ofstream file(m_Filename, std::ios::binary);
	file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
	mpt::IO::WriteRaw(file, mpt::as_span(data));
	mpt::IO::Flush(file);
	return *this;
}

LazyFileRef & LazyFileRef::operator = (const std::string &data)
{
	mpt::ofstream file(m_Filename, std::ios::binary);
	file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
	mpt::IO::WriteRaw(file, mpt::as_span(data));
	mpt::IO::Flush(file);
	return *this;
}

LazyFileRef::operator std::vector<std::byte> () const
{
	mpt::ifstream file(m_Filename, std::ios::binary);
	if(!mpt::IO::IsValid(file))
	{
		return std::vector<std::byte>();
	}
	file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
	mpt::IO::SeekEnd(file);
	std::vector<std::byte> buf(mpt::saturate_cast<std::size_t>(mpt::IO::TellRead(file)));
	mpt::IO::SeekBegin(file);
	mpt::IO::ReadRaw(file, mpt::as_span(buf));
	return buf;
}

LazyFileRef::operator std::vector<char> () const
{
	mpt::ifstream file(m_Filename, std::ios::binary);
	if(!mpt::IO::IsValid(file))
	{
		return std::vector<char>();
	}
	file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
	mpt::IO::SeekEnd(file);
	std::vector<char> buf(mpt::saturate_cast<std::size_t>(mpt::IO::TellRead(file)));
	mpt::IO::SeekBegin(file);
	mpt::IO::ReadRaw(file, mpt::as_span(buf));
	return buf;
}

LazyFileRef::operator std::string () const
{
	mpt::ifstream file(m_Filename, std::ios::binary);
	if(!mpt::IO::IsValid(file))
	{
		return std::string();
	}
	file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
	mpt::IO::SeekEnd(file);
	std::vector<char> buf(mpt::saturate_cast<std::size_t>(mpt::IO::TellRead(file)));
	mpt::IO::SeekBegin(file);
	mpt::IO::ReadRaw(file, mpt::as_span(buf));
	return std::string(buf.begin(), buf.end());
}

} // namespace mpt

#endif // MODPLUG_TRACKER


bool InputFile::DefaultToLargeAddressSpaceUsage()
{
	return false;
}


InputFile::InputFile()
	: m_IsCached(false)
{
	return;
}

InputFile::InputFile(const mpt::PathString &filename, bool allowWholeFileCaching)
	: m_IsCached(false)
{
	Open(filename, allowWholeFileCaching);
}

InputFile::~InputFile()
{
	return;
}


bool InputFile::Open(const mpt::PathString &filename, bool allowWholeFileCaching)
{
	m_IsCached = false;
	m_Cache.resize(0);
	m_Cache.shrink_to_fit();
	m_Filename = filename;
	m_File.open(m_Filename, std::ios::binary | std::ios::in);
	if(allowWholeFileCaching)
	{
		if(mpt::IO::IsReadSeekable(m_File))
		{
			if(!mpt::IO::SeekEnd(m_File))
			{
				m_File.close();
				return false;
			}
			mpt::IO::Offset filesize = mpt::IO::TellRead(m_File);
			if(!mpt::IO::SeekBegin(m_File))
			{
				m_File.close();
				return false;
			}
			if(Util::TypeCanHoldValue<std::size_t>(filesize))
			{
				std::size_t buffersize = mpt::saturate_cast<std::size_t>(filesize);
				m_Cache.resize(buffersize);
				if(mpt::IO::ReadRaw(m_File, mpt::as_span(m_Cache)) != filesize)
				{
					m_File.close();
					return false;
				}
				if(!mpt::IO::SeekBegin(m_File))
				{
					m_File.close();
					return false;
				}
				m_IsCached = true;
				return true;
			}
		}
	}
	return m_File.good();
}


bool InputFile::IsValid() const
{
	return m_File.good();
}


bool InputFile::IsCached() const
{
	return m_IsCached;
}


const mpt::PathString& InputFile::GetFilenameRef() const
{
	return m_Filename;
}


std::istream* InputFile::GetStream()
{
	MPT_ASSERT(!m_IsCached);
	return &m_File;
}


mpt::const_byte_span InputFile::GetCache()
{
	MPT_ASSERT(m_IsCached);
	return mpt::as_span(m_Cache);
}


#else // !MPT_ENABLE_FILEIO

MPT_MSVC_WORKAROUND_LNK4221(mptFileIO)

#endif // MPT_ENABLE_FILEIO


OPENMPT_NAMESPACE_END
