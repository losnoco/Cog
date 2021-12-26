/*
 * FileReader.cpp
 * --------------
 * Purpose: A basic class for transparent reading of memory-based files.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "FileReader.h"

#if defined(MPT_ENABLE_TEMPFILE) && MPT_OS_WINDOWS
#include <windows.h>
#include "mptFileIO.h"
#endif // MPT_ENABLE_TEMPFILE && MPT_OS_WINDOWS


OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_ENABLE_TEMPFILE) && MPT_OS_WINDOWS


OnDiskFileWrapper::OnDiskFileWrapper(FileReader &file, const mpt::PathString &fileNameExtension)
	: m_IsTempFile(false)
{
	try
	{
		file.Rewind();
		if(file.GetFileName().empty())
		{
			const mpt::PathString tempName = mpt::CreateTempFileName(P_("OpenMPT"), fileNameExtension);

#if MPT_OS_WINDOWS && MPT_OS_WINDOWS_WINRT
#if (_WIN32_WINNT < 0x0602)
#define MPT_ONDISKFILEWRAPPER_NO_CREATEFILE
#endif
#endif

#ifdef MPT_ONDISKFILEWRAPPER_NO_CREATEFILE

			mpt::ofstream f(tempName, std::ios::binary);
			if(!f)
			{
				throw std::runtime_error("");
			}
			while(!file.EndOfFile())
			{
				FileReader::PinnedRawDataView view = file.ReadPinnedRawDataView(mpt::IO::BUFFERSIZE_NORMAL);
				std::size_t towrite = view.size();
				std::size_t written = 0;
				do
				{
					std::size_t chunkSize = mpt::saturate_cast<std::size_t>(towrite);
					bool chunkOk = false;
					chunkOk = mpt::IO::WriteRaw(f, mpt::const_byte_span(view.data() + written, chunkSize));
					if(!chunkOk)
					{
						throw std::runtime_error("");
					}
					towrite -= chunkSize;
					written += chunkSize;
				} while(towrite > 0);
			}
			f.close();

#else // !MPT_ONDISKFILEWRAPPER_NO_CREATEFILE

			HANDLE hFile = NULL;
			#if MPT_OS_WINDOWS_WINRT
				hFile = CreateFile2(tempName.AsNative().c_str(), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, NULL);
			#else
				hFile = CreateFile(tempName.AsNative().c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
			#endif
			if(hFile == NULL || hFile == INVALID_HANDLE_VALUE)
			{
				throw std::runtime_error("");
			}
			while(!file.EndOfFile())
			{
				FileReader::PinnedRawDataView view = file.ReadPinnedRawDataView(mpt::IO::BUFFERSIZE_NORMAL);
				std::size_t towrite = view.size();
				std::size_t written = 0;
				do
				{
					DWORD chunkSize = mpt::saturate_cast<DWORD>(towrite);
					DWORD chunkDone = 0;
					WriteFile(hFile, view.data() + written, chunkSize, &chunkDone, NULL);
					if(chunkDone != chunkSize)
					{
						CloseHandle(hFile);
						hFile = NULL;
						throw std::runtime_error("");
					}
					towrite -= chunkDone;
					written += chunkDone;
				} while(towrite > 0);
			}
			CloseHandle(hFile);
			hFile = NULL;

#endif // MPT_ONDISKFILEWRAPPER_NO_CREATEFILE

			m_Filename = tempName;
			m_IsTempFile = true;
		} else
		{
			m_Filename = file.GetFileName();
		}
	} catch (const std::runtime_error &)
	{
		m_IsTempFile = false;
		m_Filename = mpt::PathString();
	}
}


OnDiskFileWrapper::~OnDiskFileWrapper()
{
	if(m_IsTempFile)
	{
		DeleteFile(m_Filename.AsNative().c_str());
		m_IsTempFile = false;
	}
	m_Filename = mpt::PathString();
}


bool OnDiskFileWrapper::IsValid() const
{
	return !m_Filename.empty();
}


mpt::PathString OnDiskFileWrapper::GetFilename() const
{
	return m_Filename;
}


#else


MPT_MSVC_WORKAROUND_LNK4221(FileReader)


#endif // MPT_ENABLE_TEMPFILE && MPT_OS_WINDOWS


OPENMPT_NAMESPACE_END
