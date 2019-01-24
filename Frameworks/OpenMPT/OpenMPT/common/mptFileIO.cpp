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
		MPT_FALLTHROUGH;
	case std::ios_base::out | std::ios_base::trunc:
		fopen_mode = _T("w");
		break;
	case std::ios_base::app:
		MPT_FALLTHROUGH;
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
		MPT_FALLTHROUGH;
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
	if(!stream())
	{
		return;
	}
	if(!stream().rdbuf())
	{
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
			// ignore errorOnFlush here, and re-throw the earlier exception
			throw;
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
	if(errorOnFlush && (stream().exceptions() & (std::ios::badbit | std::ios::failbit)))
	{
		throw std::ios_base::failure(std::string("Error flushing file buffers."));
	}
}

} // namespace mpt

#endif // MODPLUG_TRACKER



#ifdef MODPLUG_TRACKER

namespace mpt {

LazyFileRef & LazyFileRef::operator = (const std::vector<mpt::byte> &data)
{
	mpt::ofstream file(m_Filename, std::ios::binary);
	file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
	mpt::IO::WriteRaw(file, data.data(), data.size());
	mpt::IO::Flush(file);
	return *this;
}

LazyFileRef & LazyFileRef::operator = (const std::vector<char> &data)
{
	mpt::ofstream file(m_Filename, std::ios::binary);
	file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
	mpt::IO::WriteRaw(file, data.data(), data.size());
	mpt::IO::Flush(file);
	return *this;
}

LazyFileRef & LazyFileRef::operator = (const std::string &data)
{
	mpt::ofstream file(m_Filename, std::ios::binary);
	file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
	mpt::IO::WriteRaw(file, data.data(), data.size());
	mpt::IO::Flush(file);
	return *this;
}

LazyFileRef::operator std::vector<mpt::byte> () const
{
	mpt::ifstream file(m_Filename, std::ios::binary);
	if(!mpt::IO::IsValid(file))
	{
		return std::vector<mpt::byte>();
	}
	file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
	mpt::IO::SeekEnd(file);
	std::vector<mpt::byte> buf(mpt::saturate_cast<std::size_t>(mpt::IO::TellRead(file)));
	mpt::IO::SeekBegin(file);
	mpt::IO::ReadRaw(file, buf.data(), buf.size());
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
	mpt::IO::ReadRaw(file, buf.data(), buf.size());
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
	mpt::IO::ReadRaw(file, buf.data(), buf.size());
	return std::string(buf.begin(), buf.end());
}

} // namespace mpt

#endif // MODPLUG_TRACKER



#ifdef MODPLUG_TRACKER

#if MPT_OS_WINDOWS

CMappedFile::~CMappedFile()
{
	Close();
}


bool CMappedFile::Open(const mpt::PathString &filename)
{
	m_hFile = CreateFile(
		filename.AsNativePrefixed().c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if(m_hFile == INVALID_HANDLE_VALUE)
	{
		m_hFile = nullptr;
		return false;
	}
	m_FileName = filename;
	return true;
}


void CMappedFile::Close()
{
	m_FileName = mpt::PathString();
	// Unlock file
	if(m_hFMap)
	{
		if(m_pData)
		{
			UnmapViewOfFile(m_pData);
			m_pData = nullptr;
		}
		CloseHandle(m_hFMap);
		m_hFMap = nullptr;
	} else if(m_pData)
	{
		free(m_pData);
		m_pData = nullptr;
	}

	// Close file handle
	if(m_hFile)
	{
		CloseHandle(m_hFile);
		m_hFile = nullptr;
	}
}


size_t CMappedFile::GetLength()
{
	LARGE_INTEGER size;
	if(GetFileSizeEx(m_hFile, &size) == FALSE)
	{
		return 0;
	}
	return mpt::saturate_cast<size_t>(size.QuadPart);
}


const mpt::byte *CMappedFile::Lock()
{
	size_t length = GetLength();
	if(!length) return nullptr;

	void *lpStream;

	HANDLE hmf = CreateFileMapping(
		m_hFile,
		NULL,
		PAGE_READONLY,
		0, 0,
		NULL);

	// Try memory-mapping first
	if(hmf)
	{
		lpStream = MapViewOfFile(
			hmf,
			FILE_MAP_READ,
			0, 0,
			length);
		if(lpStream)
		{
			m_hFMap = hmf;
			m_pData = lpStream;
			return mpt::void_cast<const mpt::byte*>(lpStream);
		}
		CloseHandle(hmf);
		hmf = nullptr;
	}

	// Fallback if memory-mapping fails for some weird reason
	if((lpStream = malloc(length)) == nullptr) return nullptr;
	memset(lpStream, 0, length);
	size_t bytesToRead = length;
	size_t bytesRead = 0;
	while(bytesToRead > 0)
	{
		DWORD chunkToRead = mpt::saturate_cast<DWORD>(length);
		DWORD chunkRead = 0;
		if(ReadFile(m_hFile, mpt::void_cast<mpt::byte*>(lpStream) + bytesRead, chunkToRead, &chunkRead, NULL) == FALSE)
		{
			// error
			free(lpStream);
			return nullptr;
		}
		bytesRead += chunkRead;
		bytesToRead -= chunkRead;
	}
	m_pData = lpStream;
	return mpt::void_cast<const mpt::byte*>(lpStream);
}

#endif // MPT_OS_WINDOWS

#endif // MODPLUG_TRACKER



InputFile::InputFile()
{
	return;
}

InputFile::InputFile(const mpt::PathString &filename)
	: m_Filename(filename)
{
#if defined(MPT_FILEREADER_STD_ISTREAM)
	m_File.open(m_Filename, std::ios::binary | std::ios::in);
#else
	m_File.Open(m_Filename);
#endif
}

InputFile::~InputFile()
{
	return;
}


bool InputFile::Open(const mpt::PathString &filename)
{
	m_Filename = filename;
#if defined(MPT_FILEREADER_STD_ISTREAM)
	m_File.open(m_Filename, std::ios::binary | std::ios::in);
	return m_File.good();
#else
	return m_File.Open(m_Filename);
#endif
}


bool InputFile::IsValid() const
{
#if defined(MPT_FILEREADER_STD_ISTREAM)
	return m_File.good();
#else
	return m_File.IsOpen();
#endif
}

#if defined(MPT_FILEREADER_STD_ISTREAM)

InputFile::ContentsRef InputFile::Get()
{
	InputFile::ContentsRef result;
	result.first = &m_File;
	result.second = m_File.good() ? &m_Filename : nullptr;
	return result;
}

#else

InputFile::ContentsRef InputFile::Get()
{
	InputFile::ContentsRef result;
	result.first.data = nullptr;
	result.first.size = 0;
	result.second = nullptr;
	if(!m_File.IsOpen())
	{
		return result;
	}
	result.first.data = m_File.Lock();
	result.first.size = m_File.GetLength();
	result.second = &m_Filename;
	return result;
}

#endif

#else // !MPT_ENABLE_FILEIO

MPT_MSVC_WORKAROUND_LNK4221(mptFileIO)

#endif // MPT_ENABLE_FILEIO


OPENMPT_NAMESPACE_END
