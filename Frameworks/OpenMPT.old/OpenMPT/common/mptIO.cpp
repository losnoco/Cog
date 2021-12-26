/*
 * mptIO.cpp
 * ---------
 * Purpose: Basic functions for reading/writing binary and endian safe data to/from files/streams.
 * Notes  : Some useful functions for reading and writing are still missing.
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "mptIO.h"

#include <ios>
#include <istream>
#include <ostream>
#include <sstream>


OPENMPT_NAMESPACE_BEGIN


namespace mpt {

namespace IO {


//static_assert(sizeof(std::streamoff) == 8); // Assert 64bit file support.
bool IsValid(std::ostream & f) { return !f.fail(); }
bool IsValid(std::istream & f) { return !f.fail(); }
bool IsValid(std::iostream & f) { return !f.fail(); }
bool IsReadSeekable(std::istream & f)
{
	f.clear();
	std::streampos oldpos = f.tellg();
	if(f.fail() || oldpos == std::streampos(-1))
	{
		f.clear();
		return false;
	}
	f.seekg(0, std::ios::beg);
	if(f.fail())
	{
		f.clear();
		f.seekg(oldpos);
		f.clear();
		return false;
	}
	f.seekg(0, std::ios::end);
	if(f.fail())
	{
		f.clear();
		f.seekg(oldpos);
		f.clear();
		return false;
	}
	std::streampos length = f.tellg();
	if(f.fail() || length == std::streampos(-1))
	{
		f.clear();
		f.seekg(oldpos);
		f.clear();
		return false;
	}
	f.seekg(oldpos);
	f.clear();
	return true;
}
bool IsWriteSeekable(std::ostream & f)
{
	f.clear();
	std::streampos oldpos = f.tellp();
	if(f.fail() || oldpos == std::streampos(-1))
	{
		f.clear();
		return false;
	}
	f.seekp(0, std::ios::beg);
	if(f.fail())
	{
		f.clear();
		f.seekp(oldpos);
		f.clear();
		return false;
	}
	f.seekp(0, std::ios::end);
	if(f.fail())
	{
		f.clear();
		f.seekp(oldpos);
		f.clear();
		return false;
	}
	std::streampos length = f.tellp();
	if(f.fail() || length == std::streampos(-1))
	{
		f.clear();
		f.seekp(oldpos);
		f.clear();
		return false;
	}
	f.seekp(oldpos);
	f.clear();
	return true;
}
IO::Offset TellRead(std::istream & f)
{
	return f.tellg();
}
IO::Offset TellWrite(std::ostream & f)
{
	return f.tellp();
}
bool SeekBegin(std::ostream & f)
{
	f.seekp(0); return !f.fail();
}
bool SeekBegin(std::istream & f)
{
	f.seekg(0); return !f.fail();
}
bool SeekBegin(std::iostream & f)
{
	f.seekg(0); f.seekp(0); return !f.fail();
}
bool SeekEnd(std::ostream & f)
{
	f.seekp(0, std::ios::end); return !f.fail();
}
bool SeekEnd(std::istream & f)
{
	f.seekg(0, std::ios::end); return !f.fail();
}
bool SeekEnd(std::iostream & f)
{
	f.seekg(0, std::ios::end); f.seekp(0, std::ios::end); return !f.fail();
}
bool SeekAbsolute(std::ostream & f, IO::Offset pos)
{
	if(!OffsetFits<std::streamoff>(pos)) { return false; }
	f.seekp(static_cast<std::streamoff>(pos), std::ios::beg); return !f.fail();
}
bool SeekAbsolute(std::istream & f, IO::Offset pos)
{
	if(!OffsetFits<std::streamoff>(pos)) { return false; }
	f.seekg(static_cast<std::streamoff>(pos), std::ios::beg); return !f.fail();
}
bool SeekAbsolute(std::iostream & f, IO::Offset pos)
{
	if(!OffsetFits<std::streamoff>(pos)) { return false; }
	f.seekg(static_cast<std::streamoff>(pos), std::ios::beg); f.seekp(static_cast<std::streamoff>(pos), std::ios::beg); return !f.fail();
}
bool SeekRelative(std::ostream & f, IO::Offset off)
{
	if(!OffsetFits<std::streamoff>(off)) { return false; }
	f.seekp(static_cast<std::streamoff>(off), std::ios::cur); return !f.fail();
}
bool SeekRelative(std::istream & f, IO::Offset off)
{
	if(!OffsetFits<std::streamoff>(off)) { return false; }
	f.seekg(static_cast<std::streamoff>(off), std::ios::cur); return !f.fail();
}
bool SeekRelative(std::iostream & f, IO::Offset off)
{
	if(!OffsetFits<std::streamoff>(off)) { return false; }
	f.seekg(static_cast<std::streamoff>(off), std::ios::cur); f.seekp(static_cast<std::streamoff>(off), std::ios::cur); return !f.fail();
}
IO::Offset ReadRawImpl(std::istream & f, mpt::byte_span data) { f.read(mpt::byte_cast<char *>(data.data()), data.size()); return f.gcount(); }
bool WriteRawImpl(std::ostream & f, mpt::const_byte_span data) { f.write(mpt::byte_cast<const char *>(data.data()), data.size()); return !f.fail(); }
bool IsEof(std::istream & f) { return f.eof(); }
bool Flush(std::ostream & f) { f.flush(); return !f.fail(); }



} // namespace IO

} // namespace mpt



FileDataContainerSeekable::FileDataContainerSeekable(off_t streamLength, bool buffered)
	: streamLength(streamLength)
	, cached(false)
	, m_Buffered(buffered)
	, m_Buffer(m_Buffered ? static_cast<off_t>(BUFFER_SIZE) : 0)
{
	if(m_Buffered)
	{
		for(std::size_t chunkIndex = 0; chunkIndex < NUM_CHUNKS; ++chunkIndex)
		{
			m_ChunkIndexLRU[chunkIndex] = chunkIndex;
		}
	}
}

void FileDataContainerSeekable::CacheStream() const
{
	if(cached)
	{
		return;
	}
	if(m_Buffered)
	{
		m_Buffered = false;
		for (std::size_t chunkIndex = 0; chunkIndex < NUM_CHUNKS; ++chunkIndex)
		{
			m_ChunkInfo[chunkIndex].ChunkValid = false;
		}
		m_Buffer.resize(0);
		m_Buffer.shrink_to_fit();
	}
	cache.resize(streamLength);
	InternalRead(cache.data(), 0, streamLength);
	cached = true;
}

std::size_t FileDataContainerSeekable::InternalFillPageAndReturnIndex(off_t pos) const
{
	pos = Util::AlignDown(pos, static_cast<off_t>(CHUNK_SIZE));
	for(std::size_t chunkLRUIndex = 0; chunkLRUIndex < NUM_CHUNKS; ++chunkLRUIndex)
	{
		std::size_t chunkIndex = m_ChunkIndexLRU[chunkLRUIndex];
		if(m_ChunkInfo[chunkIndex].ChunkValid && (m_ChunkInfo[chunkIndex].ChunkOffset == pos))
		{
			std::size_t chunk = std::move(m_ChunkIndexLRU[chunkLRUIndex]);
			std::move_backward(m_ChunkIndexLRU.begin(), m_ChunkIndexLRU.begin() + chunkLRUIndex, m_ChunkIndexLRU.begin() + (chunkLRUIndex + 1));
			m_ChunkIndexLRU[0] = std::move(chunk);
			return chunkIndex;
		}
	}
	{
		std::size_t chunk = std::move(m_ChunkIndexLRU[NUM_CHUNKS - 1]);
		std::move_backward(m_ChunkIndexLRU.begin(), m_ChunkIndexLRU.begin() + (NUM_CHUNKS - 1), m_ChunkIndexLRU.begin() + NUM_CHUNKS);
		m_ChunkIndexLRU[0] = std::move(chunk);
	}
	std::size_t chunkIndex = m_ChunkIndexLRU[0];
	chunk_info& chunk = m_ChunkInfo[chunkIndex];
	chunk.ChunkOffset = pos;
	chunk.ChunkLength = InternalRead(chunk_data(chunkIndex).data(), pos, CHUNK_SIZE);
	chunk.ChunkValid = true;
	return chunkIndex;
}

bool FileDataContainerSeekable::IsValid() const
{
	return true;
}

bool FileDataContainerSeekable::HasFastGetLength() const
{
	return true;
}

bool FileDataContainerSeekable::HasPinnedView() const
{
	return cached;
}

const std::byte *FileDataContainerSeekable::GetRawData() const
{
	CacheStream();
	return cache.data();
}

IFileDataContainer::off_t FileDataContainerSeekable::GetLength() const
{
	return streamLength;
}

IFileDataContainer::off_t FileDataContainerSeekable::Read(std::byte *dst, IFileDataContainer::off_t pos, IFileDataContainer::off_t count) const
{
	if(cached)
	{
		IFileDataContainer::off_t cache_avail = std::min(IFileDataContainer::off_t(cache.size()) - pos, count);
		std::copy(cache.begin() + pos, cache.begin() + pos + cache_avail, dst);
		return cache_avail;
	} else
	{
		return InternalReadBuffered(dst, pos, count);
	}
}

IFileDataContainer::off_t FileDataContainerSeekable::InternalReadBuffered(std::byte* dst, off_t pos, off_t count) const
{
	if(!m_Buffered)
	{
		return InternalRead(dst, pos, count);
	}
	off_t totalRead = 0;
	while (count > 0)
	{
		std::size_t chunkIndex = InternalFillPageAndReturnIndex(pos);
		off_t pageSkip = pos - m_ChunkInfo[chunkIndex].ChunkOffset;
		off_t chunkWanted = std::min(static_cast<off_t>(CHUNK_SIZE) - pageSkip, count);
		off_t chunkGot = (m_ChunkInfo[chunkIndex].ChunkLength > pageSkip) ? (m_ChunkInfo[chunkIndex].ChunkLength - pageSkip) : 0;
		off_t chunk = std::min(chunkWanted, chunkGot);
		std::copy(chunk_data(chunkIndex).data() + pageSkip, chunk_data(chunkIndex).data() + pageSkip + chunk, dst);
		pos += chunk;
		dst += chunk;
		totalRead += chunk;
		count -= chunk;
		if (chunkWanted > chunk)
		{
			return totalRead;
		}
	}
	return totalRead;
}



bool FileDataContainerStdStreamSeekable::IsSeekable(std::istream *stream)
{
	return mpt::IO::IsReadSeekable(*stream);
}

IFileDataContainer::off_t FileDataContainerStdStreamSeekable::GetLength(std::istream *stream)
{
	stream->clear();
	std::streampos oldpos = stream->tellg();
	stream->seekg(0, std::ios::end);
	std::streampos length = stream->tellg();
	stream->seekg(oldpos);
	return mpt::saturate_cast<IFileDataContainer::off_t>(static_cast<int64>(length));
}

FileDataContainerStdStreamSeekable::FileDataContainerStdStreamSeekable(std::istream *s)
	: FileDataContainerSeekable(GetLength(s), true)
	, stream(s)
{
	return;
}

IFileDataContainer::off_t FileDataContainerStdStreamSeekable::InternalRead(std::byte *dst, off_t pos, off_t count) const
{
	stream->clear(); // tellg needs eof and fail bits unset
	std::streampos currentpos = stream->tellg();
	if(currentpos == std::streampos(-1) || static_cast<int64>(pos) != currentpos)
	{ // inefficient istream implementations might invalidate their buffer when seeking, even when seeking to the current position
		stream->seekg(pos);
	}
	stream->read(mpt::byte_cast<char*>(dst), count);
	return static_cast<IFileDataContainer::off_t>(stream->gcount());
}


FileDataContainerUnseekable::FileDataContainerUnseekable()
	: cachesize(0), streamFullyCached(false)
{
	return;
}

void FileDataContainerUnseekable::EnsureCacheBuffer(std::size_t requiredbuffersize) const
{
	if(cache.size() >= cachesize + requiredbuffersize)
	{
		return;
	}
	if(cache.size() == 0)
	{
		cache.resize(Util::AlignUp<std::size_t>(cachesize + requiredbuffersize, BUFFER_SIZE));
	} else if(Util::ExponentialGrow(cache.size()) < cachesize + requiredbuffersize)
	{
		cache.resize(Util::AlignUp<std::size_t>(cachesize + requiredbuffersize, BUFFER_SIZE));
	} else
	{
		cache.resize(Util::ExponentialGrow(cache.size()));
	}
}

void FileDataContainerUnseekable::CacheStream() const
{
	if(streamFullyCached)
	{
		return;
	}
	while(!InternalEof())
	{
		EnsureCacheBuffer(BUFFER_SIZE);
		std::size_t readcount = InternalRead(&cache[cachesize], BUFFER_SIZE);
		cachesize += readcount;
	}
	streamFullyCached = true;
}

void FileDataContainerUnseekable::CacheStreamUpTo(off_t pos, off_t length) const
{
	if(streamFullyCached)
	{
		return;
	}
	if(length > std::numeric_limits<off_t>::max() - pos)
	{
		length = std::numeric_limits<off_t>::max() - pos;
	}
	std::size_t target = mpt::saturate_cast<std::size_t>(pos + length);
	if(target <= cachesize)
	{
		return;
	}
	std::size_t alignedpos = Util::AlignUp<std::size_t>(target, QUANTUM_SIZE);
	std::size_t needcount = alignedpos - cachesize;
	EnsureCacheBuffer(needcount);
	std::size_t readcount = InternalRead(&cache[cachesize], alignedpos - cachesize);
	cachesize += readcount;
	if(!InternalEof())
	{
		// can read further
		return;
	}
	streamFullyCached = true;
}

void FileDataContainerUnseekable::ReadCached(std::byte *dst, IFileDataContainer::off_t pos, IFileDataContainer::off_t count) const
{
	std::copy(cache.begin() + pos, cache.begin() + pos + count, dst);
}

bool FileDataContainerUnseekable::IsValid() const
{
	return true;
}

bool FileDataContainerUnseekable::HasFastGetLength() const
{
	return false;
}

bool FileDataContainerUnseekable::HasPinnedView() const
{
	return true; // we have the cache which is required for seeking anyway
}

const std::byte *FileDataContainerUnseekable::GetRawData() const
{
	CacheStream();
	return cache.data();
}

IFileDataContainer::off_t FileDataContainerUnseekable::GetLength() const
{
	CacheStream();
	return cachesize;
}

IFileDataContainer::off_t FileDataContainerUnseekable::Read(std::byte *dst, IFileDataContainer::off_t pos, IFileDataContainer::off_t count) const
{
	CacheStreamUpTo(pos, count);
	if(pos >= IFileDataContainer::off_t(cachesize))
	{
		return 0;
	}
	IFileDataContainer::off_t cache_avail = std::min(IFileDataContainer::off_t(cachesize) - pos, count);
	ReadCached(dst, pos, cache_avail);
	return cache_avail;
}

bool FileDataContainerUnseekable::CanRead(IFileDataContainer::off_t pos, IFileDataContainer::off_t length) const
{
	CacheStreamUpTo(pos, length);
	if((pos == IFileDataContainer::off_t(cachesize)) && (length == 0))
	{
		return true;
	}
	if(pos >= IFileDataContainer::off_t(cachesize))
	{
		return false;
	}
	return length <= IFileDataContainer::off_t(cachesize) - pos;
}

IFileDataContainer::off_t FileDataContainerUnseekable::GetReadableLength(IFileDataContainer::off_t pos, IFileDataContainer::off_t length) const
{
	CacheStreamUpTo(pos, length);
	if(pos >= cachesize)
	{
		return 0;
	}
	return std::min(static_cast<IFileDataContainer::off_t>(cachesize) - pos, length);
}



FileDataContainerStdStream::FileDataContainerStdStream(std::istream *s)
	: stream(s)
{
	return;
}

bool FileDataContainerStdStream::InternalEof() const
{
	if(*stream)
	{
		return false;
	} else
	{
		return true;
	}
}

IFileDataContainer::off_t FileDataContainerStdStream::InternalRead(std::byte *dst, off_t count) const
{
	stream->read(mpt::byte_cast<char*>(dst), count);
	return static_cast<std::size_t>(stream->gcount());
}



#if defined(MPT_FILEREADER_CALLBACK_STREAM)


bool FileDataContainerCallbackStreamSeekable::IsSeekable(CallbackStream stream)
{
	if(!stream.stream)
	{
		return false;
	}
	if(!stream.seek)
	{
		return false;
	}
	if(!stream.tell)
	{
		return false;
	}
	int64 oldpos = stream.tell(stream.stream);
	if(oldpos < 0)
	{
		return false;
	}
	if(stream.seek(stream.stream, 0, CallbackStream::SeekSet) < 0)
	{
		stream.seek(stream.stream, oldpos, CallbackStream::SeekSet);
		return false;
	}
	if(stream.seek(stream.stream, 0, CallbackStream::SeekEnd) < 0)
	{
		stream.seek(stream.stream, oldpos, CallbackStream::SeekSet);
		return false;
	}
	int64 length = stream.tell(stream.stream);
	if(length < 0)
	{
		stream.seek(stream.stream, oldpos, CallbackStream::SeekSet);
		return false;
	}
	stream.seek(stream.stream, oldpos, CallbackStream::SeekSet);
	return true;
}

IFileDataContainer::off_t FileDataContainerCallbackStreamSeekable::GetLength(CallbackStream stream)
{
	if(!stream.stream)
	{
		return 0;
	}
	if(!stream.seek)
	{
		return false;
	}
	if(!stream.tell)
	{
		return false;
	}
	int64 oldpos = stream.tell(stream.stream);
	if(oldpos < 0)
	{
		return 0;
	}
	if(stream.seek(stream.stream, 0, CallbackStream::SeekSet) < 0)
	{
		stream.seek(stream.stream, oldpos, CallbackStream::SeekSet);
		return 0;
	}
	if(stream.seek(stream.stream, 0, CallbackStream::SeekEnd) < 0)
	{
		stream.seek(stream.stream, oldpos, CallbackStream::SeekSet);
		return 0;
	}
	int64 length = stream.tell(stream.stream);
	if(length < 0)
	{
		stream.seek(stream.stream, oldpos, CallbackStream::SeekSet);
		return 0;
	}
	stream.seek(stream.stream, oldpos, CallbackStream::SeekSet);
	return mpt::saturate_cast<IFileDataContainer::off_t>(length);
}

FileDataContainerCallbackStreamSeekable::FileDataContainerCallbackStreamSeekable(CallbackStream s)
	: FileDataContainerSeekable(GetLength(s), false)
	, stream(s)
{
	return;
}

IFileDataContainer::off_t FileDataContainerCallbackStreamSeekable::InternalRead(std::byte *dst, off_t pos, off_t count) const
{
	if(!stream.read)
	{
		return 0;
	}
	if(stream.seek(stream.stream, pos, CallbackStream::SeekSet) < 0)
	{
		return 0;
	}
	int64 totalread = 0;
	while(count > 0)
	{
		int64 readcount = stream.read(stream.stream, dst, count);
		if(readcount <= 0)
		{
			break;
		}
		dst += static_cast<std::size_t>(readcount);
		count -= static_cast<IFileDataContainer::off_t>(readcount);
		totalread += readcount;
	}
	return static_cast<IFileDataContainer::off_t>(totalread);
}



FileDataContainerCallbackStream::FileDataContainerCallbackStream(CallbackStream s)
	: FileDataContainerUnseekable()
	, stream(s)
	, eof_reached(false)
{
	return;
}

bool FileDataContainerCallbackStream::InternalEof() const
{
	return eof_reached;
}

IFileDataContainer::off_t FileDataContainerCallbackStream::InternalRead(std::byte *dst, off_t count) const
{
	if(eof_reached)
	{
		return 0;
	}
	if(!stream.read)
	{
		eof_reached = true;
		return 0;
	}
	int64 totalread = 0;
	while(count > 0)
	{
		int64 readcount = stream.read(stream.stream, dst, count);
		if(readcount <= 0)
		{
			eof_reached = true;
			break;
		}
		dst += static_cast<std::size_t>(readcount);
		count -= static_cast<IFileDataContainer::off_t>(readcount);
		totalread += readcount;
	}
	return static_cast<IFileDataContainer::off_t>(totalread);
}


#endif // MPT_FILEREADER_CALLBACK_STREAM



OPENMPT_NAMESPACE_END
