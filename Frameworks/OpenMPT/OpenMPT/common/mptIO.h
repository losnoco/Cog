/*
 * mptIO.h
 * -------
 * Purpose: Basic functions for reading/writing binary and endian safe data to/from files/streams.
 * Notes  : Some useful functions for reading and writing are still missing.
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"


#include "../common/Endianness.h"
#include <algorithm>
#include <array>
#include <iosfwd>
#include <limits>
#include <cstring>


OPENMPT_NAMESPACE_BEGIN


namespace mpt {

namespace IO {

typedef int64 Offset;

static constexpr std::size_t BUFFERSIZE_TINY   =    1 * 1024; // on stack usage
static constexpr std::size_t BUFFERSIZE_SMALL  =    4 * 1024; // on heap
static constexpr std::size_t BUFFERSIZE_NORMAL =   64 * 1024; // FILE I/O
static constexpr std::size_t BUFFERSIZE_LARGE  = 1024 * 1024;



// Returns true iff 'off' fits into 'Toff'.
template < typename Toff >
inline bool OffsetFits(IO::Offset off)
{
	return (static_cast<IO::Offset>(mpt::saturate_cast<Toff>(off)) == off);
}



bool IsValid(std::ostream & f);
bool IsValid(std::istream & f);
bool IsValid(std::iostream & f);
bool IsReadSeekable(std::istream& f);
bool IsWriteSeekable(std::ostream& f);
IO::Offset TellRead(std::istream & f);
IO::Offset TellWrite(std::ostream & f);
bool SeekBegin(std::ostream & f);
bool SeekBegin(std::istream & f);
bool SeekBegin(std::iostream & f);
bool SeekEnd(std::ostream & f);
bool SeekEnd(std::istream & f);
bool SeekEnd(std::iostream & f);
bool SeekAbsolute(std::ostream & f, IO::Offset pos);
bool SeekAbsolute(std::istream & f, IO::Offset pos);
bool SeekAbsolute(std::iostream & f, IO::Offset pos);
bool SeekRelative(std::ostream & f, IO::Offset off);
bool SeekRelative(std::istream & f, IO::Offset off);
bool SeekRelative(std::iostream & f, IO::Offset off);
IO::Offset ReadRawImpl(std::istream & f, mpt::byte_span data);
bool WriteRawImpl(std::ostream & f, mpt::const_byte_span data);
bool IsEof(std::istream & f);
bool Flush(std::ostream & f);



template <typename Tfile> class WriteBuffer;

template <typename Tfile> bool IsValid(WriteBuffer<Tfile> & f) { return IsValid(f.file()); }
template <typename Tfile> bool IsReadSeekable(WriteBuffer<Tfile> & f) { return IsReadSeekable(f.file()); }
template <typename Tfile> bool IsWriteSeekable(WriteBuffer<Tfile> & f) { return IsWriteSeekable(f.file()); }
template <typename Tfile> IO::Offset TellRead(WriteBuffer<Tfile> & f) { f.FlushLocal(); return TellRead(f.file()); }
template <typename Tfile> IO::Offset TellWrite(WriteBuffer<Tfile> & f) { return TellWrite(f.file()) + f.GetCurrentSize(); }
template <typename Tfile> bool SeekBegin(WriteBuffer<Tfile> & f) { f.FlushLocal(); return SeekBegin(f.file()); }
template <typename Tfile> bool SeekEnd(WriteBuffer<Tfile> & f) { f.FlushLocal(); return SeekEnd(f.file()); }
template <typename Tfile> bool SeekAbsolute(WriteBuffer<Tfile> & f, IO::Offset pos) { f.FlushLocal(); return SeekAbsolute(f.file(), pos); }
template <typename Tfile> bool SeekRelative(WriteBuffer<Tfile> & f, IO::Offset off) { f.FlushLocal(); return SeekRelative(f.file(), off); }
template <typename Tfile> IO::Offset ReadRawImpl(WriteBuffer<Tfile> & f, mpt::byte_span data) { f.FlushLocal(); return ReadRawImpl(f.file(), data); }
template <typename Tfile> bool WriteRawImpl(WriteBuffer<Tfile> & f, mpt::const_byte_span data) { return f.Write(data); }
template <typename Tfile> bool IsEof(WriteBuffer<Tfile> & f) { f.FlushLocal(); return IsEof(f.file()); }
template <typename Tfile> bool Flush(WriteBuffer<Tfile> & f) { f.FlushLocal(); return Flush(f.file()); }





template <typename Tbyte> bool IsValid(std::pair<mpt::span<Tbyte>, IO::Offset> & f)
{
	return (f.second >= 0);
}
template <typename Tbyte> IO::Offset TellRead(std::pair<mpt::span<Tbyte>, IO::Offset> & f)
{
	return f.second;
}
template <typename Tbyte> IO::Offset TellWrite(std::pair<mpt::span<Tbyte>, IO::Offset> & f)
{
	return f.second;
}
template <typename Tbyte> bool SeekBegin(std::pair<mpt::span<Tbyte>, IO::Offset> & f)
{
	f.second = 0;
	return true;
}
template <typename Tbyte> bool SeekEnd(std::pair<mpt::span<Tbyte>, IO::Offset> & f)
{
	f.second = f.first.size();
	return true;
}
template <typename Tbyte> bool SeekAbsolute(std::pair<mpt::span<Tbyte>, IO::Offset> & f, IO::Offset pos)
{
	f.second = pos;
	return true;
}
template <typename Tbyte> bool SeekRelative(std::pair<mpt::span<Tbyte>, IO::Offset> & f, IO::Offset off)
{
	if(f.second < 0)
	{
		return false;
	}
	f.second += off;
	return true;
}
template <typename Tbyte> IO::Offset ReadRawImpl(std::pair<mpt::span<Tbyte>, IO::Offset> & f, mpt::byte_span data)
{
	if(f.second < 0)
	{
		return 0;
	}
	if(f.second >= static_cast<IO::Offset>(f.first.size()))
	{
		return 0;
	}
	std::size_t num = mpt::saturate_cast<std::size_t>(std::min(static_cast<IO::Offset>(f.first.size()) - f.second, static_cast<IO::Offset>(data.size())));
	std::copy(mpt::byte_cast<const std::byte*>(f.first.data() + f.second), mpt::byte_cast<const std::byte*>(f.first.data() + f.second + num), data.data());
	f.second += num;
	return num;
}
template <typename Tbyte> bool WriteRawImpl(std::pair<mpt::span<Tbyte>, IO::Offset> & f, mpt::const_byte_span data)
{
	if(f.second < 0)
	{
		return false;
	}
	if(f.second > static_cast<IO::Offset>(f.first.size()))
	{
		return false;
	}
	std::size_t num = mpt::saturate_cast<std::size_t>(std::min(static_cast<IO::Offset>(f.first.size()) - f.second, static_cast<IO::Offset>(data.size())));
	if(num != data.size())
	{
		return false;
	}
	std::copy(data.data(), data.data() + num, mpt::byte_cast<std::byte*>(f.first.data() + f.second));
	f.second += num;
	return true;
}
template <typename Tbyte> bool IsEof(std::pair<mpt::span<Tbyte>, IO::Offset> & f)
{
	return (f.second >= static_cast<IO::Offset>(f.first.size()));
}
template <typename Tbyte> bool Flush(std::pair<mpt::span<Tbyte>, IO::Offset> & f)
{
	MPT_UNREFERENCED_PARAMETER(f);
	return true;
}



template <typename Tbyte, typename Tfile>
inline IO::Offset ReadRaw(Tfile & f, Tbyte * data, std::size_t size)
{
	return IO::ReadRawImpl(f, mpt::as_span(mpt::byte_cast<std::byte*>(data), size));
}

template <typename Tbyte, typename Tfile>
inline IO::Offset ReadRaw(Tfile & f, mpt::span<Tbyte> data)
{
	return IO::ReadRawImpl(f, mpt::byte_cast<mpt::byte_span>(data));
}

template <typename Tbyte, typename Tfile>
inline bool WriteRaw(Tfile & f, const Tbyte * data, std::size_t size)
{
	return IO::WriteRawImpl(f, mpt::as_span(mpt::byte_cast<const std::byte*>(data), size));
}

template <typename Tbyte, typename Tfile>
inline bool WriteRaw(Tfile & f, mpt::span<Tbyte> data)
{
	return IO::WriteRawImpl(f, mpt::byte_cast<mpt::const_byte_span>(data));
}

template <typename Tbinary, typename Tfile>
inline bool Read(Tfile & f, Tbinary & v)
{
	return IO::ReadRaw(f, mpt::as_raw_memory(v)) == mpt::saturate_cast<mpt::IO::Offset>(mpt::as_raw_memory(v).size());
}

template <typename Tbinary, typename Tfile>
inline bool Write(Tfile & f, const Tbinary & v)
{
	return IO::WriteRaw(f, mpt::as_raw_memory(v));
}

template <typename Tbinary, typename Tfile>
inline bool Write(Tfile & f, const std::vector<Tbinary> & v)
{
	static_assert(mpt::is_binary_safe<Tbinary>::value);
	return IO::WriteRaw(f, mpt::as_raw_memory(v));
}

template <typename T, typename Tfile>
inline bool WritePartial(Tfile & f, const T & v, size_t size = sizeof(T))
{
	MPT_ASSERT(size <= sizeof(T));
	return IO::WriteRaw(f, mpt::as_span(mpt::as_raw_memory(v).data(), size));
}

template <typename Tfile>
inline bool ReadByte(Tfile & f, std::byte & v)
{
	bool result = false;
	std::byte byte = mpt::as_byte(0);
	const IO::Offset readResult = IO::ReadRaw(f, &byte, sizeof(std::byte));
	if(readResult < 0)
	{
		result = false;
	} else
	{
		result = (static_cast<uint64>(readResult) == sizeof(std::byte));
	}
	v = byte;
	return result;
}

template <typename T, typename Tfile>
inline bool ReadBinaryTruncatedLE(Tfile & f, T & v, std::size_t size)
{
	bool result = false;
	static_assert(std::numeric_limits<T>::is_integer);
	uint8 bytes[sizeof(T)];
	std::memset(bytes, 0, sizeof(T));
	const IO::Offset readResult = IO::ReadRaw(f, bytes, std::min(size, sizeof(T)));
	if(readResult < 0)
	{
		result = false;
	} else
	{
		result = (static_cast<uint64>(readResult) == std::min(size, sizeof(T)));
	}
	typename mpt::make_le<T>::type val;
	std::memcpy(&val, bytes, sizeof(T));
	v = val;
	return result;
}

template <typename T, typename Tfile>
inline bool ReadIntLE(Tfile & f, T & v)
{
	bool result = false;
	static_assert(std::numeric_limits<T>::is_integer);
	uint8 bytes[sizeof(T)];
	std::memset(bytes, 0, sizeof(T));
	const IO::Offset readResult = IO::ReadRaw(f, bytes, sizeof(T));
	if(readResult < 0)
	{
		result = false;
	} else
	{
		result = (static_cast<uint64>(readResult) == sizeof(T));
	}
	typename mpt::make_le<T>::type val;
	std::memcpy(&val, bytes, sizeof(T));
	v = val;
	return result;
}

template <typename T, typename Tfile>
inline bool ReadIntBE(Tfile & f, T & v)
{
	bool result = false;
	static_assert(std::numeric_limits<T>::is_integer);
	uint8 bytes[sizeof(T)];
	std::memset(bytes, 0, sizeof(T));
	const IO::Offset readResult = IO::ReadRaw(f, bytes, sizeof(T));
	if(readResult < 0)
	{
		result = false;
	} else
	{
		result = (static_cast<uint64>(readResult) == sizeof(T));
	}
	typename mpt::make_be<T>::type val;
	std::memcpy(&val, bytes, sizeof(T));
	v = val;
	return result;
}

template <typename Tfile>
inline bool ReadAdaptiveInt16LE(Tfile & f, uint16 & v)
{
	bool result = true;
	uint8 byte = 0;
	std::size_t additionalBytes = 0;
	v = 0;
	byte = 0;
	if(!IO::ReadIntLE<uint8>(f, byte)) result = false;
	additionalBytes = (byte & 0x01);
	v = byte >> 1;
	for(std::size_t i = 0; i < additionalBytes; ++i)
	{
		byte = 0;
		if(!IO::ReadIntLE<uint8>(f, byte)) result = false;
		v |= (static_cast<uint16>(byte) << (((i+1)*8) - 1));
	}
	return result;
}

template <typename Tfile>
inline bool ReadAdaptiveInt32LE(Tfile & f, uint32 & v)
{
	bool result = true;
	uint8 byte = 0;
	std::size_t additionalBytes = 0;
	v = 0;
	byte = 0;
	if(!IO::ReadIntLE<uint8>(f, byte)) result = false;
	additionalBytes = (byte & 0x03);
	v = byte >> 2;
	for(std::size_t i = 0; i < additionalBytes; ++i)
	{
		byte = 0;
		if(!IO::ReadIntLE<uint8>(f, byte)) result = false;
		v |= (static_cast<uint32>(byte) << (((i+1)*8) - 2));
	}
	return result;
}

template <typename Tfile>
inline bool ReadAdaptiveInt64LE(Tfile & f, uint64 & v)
{
	bool result = true;
	uint8 byte = 0;
	std::size_t additionalBytes = 0;
	v = 0;
	byte = 0;
	if(!IO::ReadIntLE<uint8>(f, byte)) result = false;
	additionalBytes = (1 << (byte & 0x03)) - 1;
	v = byte >> 2;
	for(std::size_t i = 0; i < additionalBytes; ++i)
	{
		byte = 0;
		if(!IO::ReadIntLE<uint8>(f, byte)) result = false;
		v |= (static_cast<uint64>(byte) << (((i+1)*8) - 2));
	}
	return result;
}

template <typename Tsize, typename Tfile>
inline bool ReadSizedStringLE(Tfile & f, std::string & str, Tsize maxSize = std::numeric_limits<Tsize>::max())
{
	static_assert(std::numeric_limits<Tsize>::is_integer);
	str.clear();
	Tsize size = 0;
	if(!mpt::IO::ReadIntLE(f, size))
	{
		return false;
	}
	if(size > maxSize)
	{
		return false;
	}
	for(Tsize i = 0; i != size; ++i)
	{
		char c = '\0';
		if(!mpt::IO::ReadIntLE(f, c))
		{
			return false;
		}
		str.push_back(c);
	}
	return true;
}


template <typename T, typename Tfile>
inline bool WriteIntLE(Tfile & f, const T v)
{
	static_assert(std::numeric_limits<T>::is_integer);
	return IO::Write(f, mpt::as_le(v));
}

template <typename T, typename Tfile>
inline bool WriteIntBE(Tfile & f, const T v)
{
	static_assert(std::numeric_limits<T>::is_integer);
	return IO::Write(f, mpt::as_be(v));
}

template <typename Tfile>
inline bool WriteAdaptiveInt16LE(Tfile & f, const uint16 v, std::size_t fixedSize = 0)
{
	std::size_t minSize = fixedSize;
	std::size_t maxSize = fixedSize;
	MPT_ASSERT(minSize == 0 || minSize == 1 || minSize == 2);
	MPT_ASSERT(maxSize == 0 || maxSize == 1 || maxSize == 2);
	MPT_ASSERT(maxSize == 0 || maxSize >= minSize);
	if(maxSize == 0)
	{
		maxSize = 2;
	}
	if(v < 0x80 && minSize <= 1 && 1 <= maxSize)
	{
		return IO::WriteIntLE<uint8>(f, static_cast<uint8>(v << 1) | 0x00);
	} else if(v < 0x8000 && minSize <= 2 && 2 <= maxSize)
	{
		return IO::WriteIntLE<uint16>(f, static_cast<uint16>(v << 1) | 0x01);
	} else
	{
		MPT_ASSERT_NOTREACHED();
		return false;
	}
}

template <typename Tfile>
inline bool WriteAdaptiveInt32LE(Tfile & f, const uint32 v, std::size_t fixedSize = 0)
{
	std::size_t minSize = fixedSize;
	std::size_t maxSize = fixedSize;
	MPT_ASSERT(minSize == 0 || minSize == 1 || minSize == 2 || minSize == 3 || minSize == 4);
	MPT_ASSERT(maxSize == 0 || maxSize == 1 || maxSize == 2 || maxSize == 3 || maxSize == 4);
	MPT_ASSERT(maxSize == 0 || maxSize >= minSize);
	if(maxSize == 0)
	{
		maxSize = 4;
	}
	if(v < 0x40 && minSize <= 1 && 1 <= maxSize)
	{
		return IO::WriteIntLE<uint8>(f, static_cast<uint8>(v << 2) | 0x00);
	} else if(v < 0x4000 && minSize <= 2 && 2 <= maxSize)
	{
		return IO::WriteIntLE<uint16>(f, static_cast<uint16>(v << 2) | 0x01);
	} else if(v < 0x400000 && minSize <= 3 && 3 <= maxSize)
	{
		uint32 value = static_cast<uint32>(v << 2) | 0x02;
		std::byte bytes[3];
		bytes[0] = static_cast<std::byte>(value >>  0);
		bytes[1] = static_cast<std::byte>(value >>  8);
		bytes[2] = static_cast<std::byte>(value >> 16);
		return IO::WriteRaw(f, bytes, 3);
	} else if(v < 0x40000000 && minSize <= 4 && 4 <= maxSize)
	{
		return IO::WriteIntLE<uint32>(f, static_cast<uint32>(v << 2) | 0x03);
	} else
	{
		MPT_ASSERT_NOTREACHED();
		return false;
	}
}

template <typename Tfile>
inline bool WriteAdaptiveInt64LE(Tfile & f, const uint64 v, std::size_t fixedSize = 0)
{
	std::size_t minSize = fixedSize;
	std::size_t maxSize = fixedSize;
	MPT_ASSERT(minSize == 0 || minSize == 1 || minSize == 2 || minSize == 4 || minSize == 8);
	MPT_ASSERT(maxSize == 0 || maxSize == 1 || maxSize == 2 || maxSize == 4 || maxSize == 8);
	MPT_ASSERT(maxSize == 0 || maxSize >= minSize);
	if(maxSize == 0)
	{
		maxSize = 8;
	}
	if(v < 0x40 && minSize <= 1 && 1 <= maxSize)
	{
		return IO::WriteIntLE<uint8>(f, static_cast<uint8>(v << 2) | 0x00);
	} else if(v < 0x4000 && minSize <= 2 && 2 <= maxSize)
	{
		return IO::WriteIntLE<uint16>(f, static_cast<uint16>(v << 2) | 0x01);
	} else if(v < 0x40000000 && minSize <= 4 && 4 <= maxSize)
	{
		return IO::WriteIntLE<uint32>(f, static_cast<uint32>(v << 2) | 0x02);
	} else if(v < 0x4000000000000000ull && minSize <= 8 && 8 <= maxSize)
	{
		return IO::WriteIntLE<uint64>(f, static_cast<uint64>(v << 2) | 0x03);
	} else
	{
		MPT_ASSERT_NOTREACHED();
		return false;
	}
}

// Write a variable-length integer, as found in MIDI files. The number of written bytes is placed in the bytesWritten parameter.
template <typename Tfile, typename T>
bool WriteVarInt(Tfile & f, const T v, size_t *bytesWritten = nullptr)
{
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(!std::numeric_limits<T>::is_signed);
	std::byte out[(sizeof(T) * 8 + 6) / 7];
	size_t numBytes = 0;
	for(uint32 n = (sizeof(T) * 8) / 7; n > 0; n--)
	{
		if(v >= (static_cast<T>(1) << (n * 7u)))
		{
			out[numBytes++] = static_cast<std::byte>(((v >> (n * 7u)) & 0x7F) | 0x80);
		}
	}
	out[numBytes++] = static_cast<std::byte>(v & 0x7F);
	MPT_ASSERT(numBytes <= std::size(out));
	if(bytesWritten != nullptr) *bytesWritten = numBytes;
	return mpt::IO::WriteRaw(f, out, numBytes);
}

template <typename Tsize, typename Tfile>
inline bool WriteSizedStringLE(Tfile & f, const std::string & str)
{
	static_assert(std::numeric_limits<Tsize>::is_integer);
	if(str.size() > std::numeric_limits<Tsize>::max())
	{
		return false;
	}
	Tsize size = static_cast<Tsize>(str.size());
	if(!mpt::IO::WriteIntLE(f, size))
	{
		return false;
	}
	if(!mpt::IO::WriteRaw(f, str.data(), str.size()))
	{
		return false;
	}
	return true;
}

template <typename Tfile>
inline bool WriteText(Tfile &f, const std::string &s)
{
	return mpt::IO::WriteRaw(f, s.data(), s.size());
}

template <typename Tfile>
inline bool WriteTextCRLF(Tfile &f)
{
	return mpt::IO::WriteText(f, "\r\n");
}

template <typename Tfile>
inline bool WriteTextLF(Tfile &f)
{
	return mpt::IO::WriteText(f, "\n");
}

template <typename Tfile>
inline bool WriteTextCRLF(Tfile &f, const std::string &s)
{
	return mpt::IO::WriteText(f, s) && mpt::IO::WriteTextCRLF(f);
}

template <typename Tfile>
inline bool WriteTextLF(Tfile &f, const std::string &s)
{
	return mpt::IO::WriteText(f, s) && mpt::IO::WriteTextLF(f);
}



// WriteBuffer class that avoids calling to the underlying file writing
// functions for every operation, which would involve rather slow un-inlinable
// virtual calls in the iostream and FILE* cases. It is the users responabiliy
// to call HasWriteError() to check for writeback errors at this buffering
// level.

template <typename Tfile>
class WriteBuffer
{
private:
	mpt::byte_span buffer;
	std::size_t size = 0;
	Tfile & f;
	bool writeError = false;
public:
	WriteBuffer(const WriteBuffer &) = delete;
	WriteBuffer & operator=(const WriteBuffer &) = delete;
public:
	inline WriteBuffer(Tfile & f_, mpt::byte_span buffer_)
		: buffer(buffer_)
		, f(f_)
	{
	}
	inline ~WriteBuffer() noexcept(false)
	{
		if(!writeError)
		{
			FlushLocal();
		}
	}
public:
	inline Tfile & file() const
	{
		if(IsDirty())
		{
			FlushLocal();
		}
		return f;
	}
public:
	inline bool HasWriteError() const
	{
		return writeError;
	}
	inline void ClearError()
	{
		writeError = false;
	}
	inline bool IsDirty() const
	{
		return size > 0;
	}
	inline bool IsClean() const
	{
		return size == 0;
	}
	inline bool IsFull() const
	{
		return size == buffer.size();
	}
	inline std::size_t GetCurrentSize() const
	{
		return size;
	}
	inline bool Write(mpt::const_byte_span data)
	{
		bool result = true;
		for(std::size_t i = 0; i < data.size(); ++i)
		{
			buffer[size] = data[i];
			size++;
			if(IsFull())
			{
				FlushLocal();
			}
		}
		return result;
	}
	inline void FlushLocal()
	{
		if(IsClean())
		{
			return;
		}
		try
		{
			if(!mpt::IO::WriteRaw(f, mpt::as_span(buffer.data(), size)))
			{
				writeError = true;
			}
		} catch (const std::exception &)
		{
			writeError = true;
			throw;
		}
		size = 0;
	}
};



} // namespace IO


} // namespace mpt



class IFileDataContainer {
public:
	typedef std::size_t off_t;
protected:
	IFileDataContainer() = default;
public:
	IFileDataContainer(const IFileDataContainer&) = default;
	IFileDataContainer & operator=(const IFileDataContainer&) = default;
	virtual ~IFileDataContainer() = default;
public:
	virtual bool IsValid() const = 0;
	virtual bool HasFastGetLength() const = 0;
	virtual bool HasPinnedView() const = 0;
	virtual const std::byte *GetRawData() const = 0;
	virtual off_t GetLength() const = 0;
	virtual off_t Read(std::byte *dst, off_t pos, off_t count) const = 0;

	virtual off_t Read(off_t pos, mpt::byte_span dst) const
	{
		return Read(dst.data(), pos, dst.size());
	}

	virtual bool CanRead(off_t pos, off_t length) const
	{
		off_t dataLength = GetLength();
		if((pos == dataLength) && (length == 0))
		{
			return true;
		}
		if(pos >= dataLength)
		{
			return false;
		}
		return length <= dataLength - pos;
	}

	virtual off_t GetReadableLength(off_t pos, off_t length) const
	{
		off_t dataLength = GetLength();
		if(pos >= dataLength)
		{
			return 0;
		}
		return std::min(length, dataLength - pos);
	}
};


class FileDataContainerDummy : public IFileDataContainer {
public:
	FileDataContainerDummy() { }
public:
	bool IsValid() const override
	{
		return false;
	}

	bool HasFastGetLength() const override
	{
		return true;
	}

	bool HasPinnedView() const override
	{
		return true;
	}

	const std::byte *GetRawData() const override
	{
		return nullptr;
	}

	off_t GetLength() const override
	{
		return 0;
	}
	off_t Read(std::byte * /*dst*/, off_t /*pos*/, off_t /*count*/) const override
	{
		return 0;
	}
};


class FileDataContainerWindow : public IFileDataContainer
{
private:
	std::shared_ptr<const IFileDataContainer> data;
	const off_t dataOffset;
	const off_t dataLength;
public:
	FileDataContainerWindow(std::shared_ptr<const IFileDataContainer> src, off_t off, off_t len) : data(src), dataOffset(off), dataLength(len) { }

	bool IsValid() const override
	{
		return data->IsValid();
	}
	bool HasFastGetLength() const override
	{
		return data->HasFastGetLength();
	}
	bool HasPinnedView() const override
	{
		return data->HasPinnedView();
	}
	const std::byte *GetRawData() const override
	{
		return data->GetRawData() + dataOffset;
	}
	off_t GetLength() const override
	{
		return dataLength;
	}
	off_t Read(std::byte *dst, off_t pos, off_t count) const override
	{
		if(pos >= dataLength)
		{
			return 0;
		}
		return data->Read(dst, dataOffset + pos, std::min(count, dataLength - pos));
	}
	bool CanRead(off_t pos, off_t length) const override
	{
		if((pos == dataLength) && (length == 0))
		{
			return true;
		}
		if(pos >= dataLength)
		{
			return false;
		}
		return (length <= dataLength - pos);
	}
	off_t GetReadableLength(off_t pos, off_t length) const override
	{
		if(pos >= dataLength)
		{
			return 0;
		}
		return std::min(length, dataLength - pos);
	}
};


class FileDataContainerSeekable : public IFileDataContainer {

private:

	off_t streamLength;

	mutable bool cached;
	mutable std::vector<std::byte> cache;

private:

	mutable bool m_Buffered;
	enum : std::size_t {
		CHUNK_SIZE = mpt::IO::BUFFERSIZE_SMALL,
		BUFFER_SIZE = mpt::IO::BUFFERSIZE_NORMAL
	};
	enum : std::size_t {
		NUM_CHUNKS = BUFFER_SIZE / CHUNK_SIZE
	};
	struct chunk_info {
		off_t ChunkOffset = 0;
		off_t ChunkLength = 0;
		bool ChunkValid = false;
	};
	mutable std::vector<std::byte> m_Buffer;
	mpt::byte_span chunk_data(std::size_t chunkIndex) const
	{
		return mpt::byte_span(m_Buffer.data() + (chunkIndex * CHUNK_SIZE), CHUNK_SIZE);
	}
	mutable std::array<chunk_info, NUM_CHUNKS> m_ChunkInfo;
	mutable std::array<std::size_t, NUM_CHUNKS> m_ChunkIndexLRU;

	std::size_t InternalFillPageAndReturnIndex(off_t pos) const;

protected:

	FileDataContainerSeekable(off_t length, bool buffered);

private:
	
	void CacheStream() const;

public:

	bool IsValid() const override;
	bool HasFastGetLength() const override;
	bool HasPinnedView() const override;
	const std::byte *GetRawData() const override;
	off_t GetLength() const override;
	off_t Read(std::byte *dst, off_t pos, off_t count) const override;

private:

	off_t InternalReadBuffered(std::byte* dst, off_t pos, off_t count) const;

	virtual off_t InternalRead(std::byte *dst, off_t pos, off_t count) const = 0;

};


class FileDataContainerStdStreamSeekable : public FileDataContainerSeekable {

private:

	std::istream *stream;

public:

	FileDataContainerStdStreamSeekable(std::istream *s);

	static bool IsSeekable(std::istream *stream);
	static off_t GetLength(std::istream *stream);

private:

	off_t InternalRead(std::byte *dst, off_t pos, off_t count) const override;

};


class FileDataContainerUnseekable : public IFileDataContainer {

private:

	mutable std::vector<std::byte> cache;
	mutable std::size_t cachesize;
	mutable bool streamFullyCached;

protected:

	FileDataContainerUnseekable();

private:

	enum : std::size_t {
		QUANTUM_SIZE = mpt::IO::BUFFERSIZE_SMALL,
		BUFFER_SIZE = mpt::IO::BUFFERSIZE_NORMAL
	};

	void EnsureCacheBuffer(std::size_t requiredbuffersize) const;
	void CacheStream() const;
	void CacheStreamUpTo(off_t pos, off_t length) const;

private:

	void ReadCached(std::byte *dst, off_t pos, off_t count) const;

public:

	bool IsValid() const override;
	bool HasFastGetLength() const override;
	bool HasPinnedView() const override;
	const std::byte *GetRawData() const override;
	off_t GetLength() const override;
	off_t Read(std::byte *dst, off_t pos, off_t count) const override;
	bool CanRead(off_t pos, off_t length) const override;
	off_t GetReadableLength(off_t pos, off_t length) const override;

private:

	virtual bool InternalEof() const = 0;
	virtual off_t InternalRead(std::byte *dst, off_t count) const = 0;

};


class FileDataContainerStdStream : public FileDataContainerUnseekable {

private:

	std::istream *stream;

public:

	FileDataContainerStdStream(std::istream *s);

private:

	bool InternalEof() const override;
	off_t InternalRead(std::byte *dst, off_t count) const override;

};


#if defined(MPT_FILEREADER_CALLBACK_STREAM)


struct CallbackStream
{
	enum : int {
		SeekSet = 0,
		SeekCur = 1,
		SeekEnd = 2
	};
	void *stream;
	std::size_t (*read)( void * stream, void * dst, std::size_t bytes );
	int (*seek)( void * stream, int64 offset, int whence );
	int64 (*tell)( void * stream );
};


class FileDataContainerCallbackStreamSeekable : public FileDataContainerSeekable
{
private:
	CallbackStream stream;
public:
	static bool IsSeekable(CallbackStream stream);
	static off_t GetLength(CallbackStream stream);
	FileDataContainerCallbackStreamSeekable(CallbackStream s);
private:
	off_t InternalRead(std::byte *dst, off_t pos, off_t count) const override;
};


class FileDataContainerCallbackStream : public FileDataContainerUnseekable
{
private:
	CallbackStream stream;
	mutable bool eof_reached;
public:
	FileDataContainerCallbackStream(CallbackStream s);
private:
	bool InternalEof() const override;
	off_t InternalRead(std::byte *dst, off_t count) const override;
};


#endif // MPT_FILEREADER_CALLBACK_STREAM


class FileDataContainerMemory
	: public IFileDataContainer
{

private:

	const std::byte *streamData;	// Pointer to memory-mapped file
	off_t streamLength;		// Size of memory-mapped file in bytes

public:
	FileDataContainerMemory() : streamData(nullptr), streamLength(0) { }
	FileDataContainerMemory(mpt::const_byte_span data) : streamData(data.data()), streamLength(data.size()) { }

public:

	bool IsValid() const override
	{
		return streamData != nullptr;
	}

	bool HasFastGetLength() const override
	{
		return true;
	}

	bool HasPinnedView() const override
	{
		return true;
	}

	const std::byte *GetRawData() const override
	{
		return streamData;
	}

	off_t GetLength() const override
	{
		return streamLength;
	}

	off_t Read(std::byte *dst, off_t pos, off_t count) const override
	{
		if(pos >= streamLength)
		{
			return 0;
		}
		off_t avail = std::min(streamLength - pos, count);
		std::copy(streamData + pos, streamData + pos + avail, dst);
		return avail;
	}

	off_t Read(off_t pos, mpt::byte_span dst) const override
	{
		return Read(dst.data(), pos, dst.size());
	}

	bool CanRead(off_t pos, off_t length) const override
	{
		if((pos == streamLength) && (length == 0))
		{
			return true;
		}
		if(pos >= streamLength)
		{
			return false;
		}
		return (length <= streamLength - pos);
	}

	off_t GetReadableLength(off_t pos, off_t length) const override
	{
		if(pos >= streamLength)
		{
			return 0;
		}
		return std::min(length, streamLength - pos);
	}

};



OPENMPT_NAMESPACE_END
