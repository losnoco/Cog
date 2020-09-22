/*
 * FileReader.h
 * ------------
 * Purpose: A basic class for transparent reading of memory-based files.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"


#include "mptStringBuffer.h"
#include "misc_util.h"
#include "Endianness.h"
#include "mptIO.h"
#include <algorithm>
#include <limits>
#include <vector>
#include <cstring>

#include "FileReaderFwd.h"


OPENMPT_NAMESPACE_BEGIN


// change to show warnings for functions which trigger pre-caching the whole file for unseekable streams
//#define FILEREADER_DEPRECATED [[deprecated]]
#define FILEREADER_DEPRECATED


class FileReaderTraitsMemory
{

public:

	using off_t = FileDataContainerMemory::off_t;

	using data_type = FileDataContainerMemory;
	using ref_data_type = const FileDataContainerMemory &;
	using shared_data_type = const FileDataContainerMemory &;
	using value_data_type = FileDataContainerMemory;

	static shared_data_type get_shared(const data_type & data) { return data; }
	static ref_data_type get_ref(const data_type & data) { return data; }

	static value_data_type make_data() { return mpt::const_byte_span(); }
	static value_data_type make_data(mpt::const_byte_span data) { return data; }

	static value_data_type make_chunk(shared_data_type data, off_t position, off_t size)
	{
		return mpt::as_span(data.GetRawData() + position, size);
	}

};

class FileReaderTraitsStdStream
{

public:

	using off_t = IFileDataContainer::off_t;

	using data_type = std::shared_ptr<const IFileDataContainer>;
	using ref_data_type = const IFileDataContainer &;
	using shared_data_type = std::shared_ptr<const IFileDataContainer>;
	using value_data_type = std::shared_ptr<const IFileDataContainer>;

	static shared_data_type get_shared(const data_type & data) { return data; }
	static ref_data_type get_ref(const data_type & data) { return *data; }

	static value_data_type make_data() { return std::make_shared<FileDataContainerDummy>(); }
	static value_data_type make_data(mpt::const_byte_span data) { return std::make_shared<FileDataContainerMemory>(data); }

	static value_data_type make_chunk(shared_data_type data, off_t position, off_t size)
	{
		return std::static_pointer_cast<IFileDataContainer>(std::make_shared<FileDataContainerWindow>(data, position, size));
	}

};

using FileReaderTraitsDefault = FileReaderTraitsStdStream;

namespace mpt
{
namespace FileReader
{

	// Read a "T" object from the stream.
	// If not enough bytes can be read, false is returned.
	// If successful, the file cursor is advanced by the size of "T".
	template <typename T, typename TFileCursor>
	bool Read(TFileCursor &f, T &target)
	{
		// cppcheck false-positive
		// cppcheck-suppress uninitvar
		mpt::byte_span dst = mpt::as_raw_memory(target);
		if(dst.size() != f.GetRaw(dst))
		{
			return false;
		}
		f.Skip(dst.size());
		return true;
	}

	// Read some kind of integer in little-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	template <typename T, typename TFileCursor>
	T ReadIntLE(TFileCursor &f)
	{
		static_assert(std::numeric_limits<T>::is_integer == true, "Target type is a not an integer");
		typename mpt::make_le<T>::type target;
		if(Read(f, target))
		{
			return target;
		} else
		{
			return 0;
		}
	}

	// Read some kind of integer in big-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	template <typename T, typename TFileCursor>
	T ReadIntBE(TFileCursor &f)
	{
		static_assert(std::numeric_limits<T>::is_integer == true, "Target type is a not an integer");
		typename mpt::make_be<T>::type target;
		if(Read(f, target))
		{
			return target;
		} else
		{
			return 0;
		}
	}

	// Read a integer in little-endian format which has some of its higher bytes not stored in file.
	// If successful, the file cursor is advanced by the given size.
	template <typename T, typename TFileCursor>
	T ReadTruncatedIntLE(TFileCursor &f, typename TFileCursor::off_t size)
	{
		static_assert(std::numeric_limits<T>::is_integer == true, "Target type is a not an integer");
		MPT_ASSERT(sizeof(T) >= size);
		if(size == 0)
		{
			return 0;
		}
		if(!f.CanRead(size))
		{
			return 0;
		}
		uint8 buf[sizeof(T)];
		bool negative = false;
		for(std::size_t i = 0; i < sizeof(T); ++i)
		{
			uint8 byte = 0;
			if(i < size)
			{
				Read(f, byte);
				negative = std::numeric_limits<T>::is_signed && ((byte & 0x80) != 0x00);
			} else
			{
				// sign or zero extend
				byte = negative ? 0xff : 0x00;
			}
			buf[i] = byte;
		}
		typename mpt::make_le<T>::type target;
		std::memcpy(&target, buf, sizeof(T));
		return target;
	}

	// Read a supplied-size little endian integer to a fixed size variable.
	// The data is properly sign-extended when fewer bytes are stored.
	// If more bytes are stored, higher order bytes are silently ignored.
	// If successful, the file cursor is advanced by the given size.
	template <typename T, typename TFileCursor>
	T ReadSizedIntLE(TFileCursor &f, typename TFileCursor::off_t size)
	{
		static_assert(std::numeric_limits<T>::is_integer == true, "Target type is a not an integer");
		if(size == 0)
		{
			return 0;
		}
		if(!f.CanRead(size))
		{
			return 0;
		}
		if(size < sizeof(T))
		{
			return ReadTruncatedIntLE<T>(f, size);
		}
		T retval = ReadIntLE<T>(f);
		f.Skip(size - sizeof(T));
		return retval;
	}

	// Read unsigned 32-Bit integer in little-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	template <typename TFileCursor>
	uint32 ReadUint32LE(TFileCursor &f)
	{
		return ReadIntLE<uint32>(f);
	}

	// Read unsigned 32-Bit integer in big-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	template <typename TFileCursor>
	uint32 ReadUint32BE(TFileCursor &f)
	{
		return ReadIntBE<uint32>(f);
	}

	// Read signed 32-Bit integer in little-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	template <typename TFileCursor>
	int32 ReadInt32LE(TFileCursor &f)
	{
		return ReadIntLE<int32>(f);
	}

	// Read signed 32-Bit integer in big-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	template <typename TFileCursor>
	int32 ReadInt32BE(TFileCursor &f)
	{
		return ReadIntBE<int32>(f);
	}

	// Read unsigned 16-Bit integer in little-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	template <typename TFileCursor>
	uint16 ReadUint16LE(TFileCursor &f)
	{
		return ReadIntLE<uint16>(f);
	}

	// Read unsigned 16-Bit integer in big-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	template <typename TFileCursor>
	uint16 ReadUint16BE(TFileCursor &f)
	{
		return ReadIntBE<uint16>(f);
	}

	// Read signed 16-Bit integer in little-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	template <typename TFileCursor>
	int16 ReadInt16LE(TFileCursor &f)
	{
		return ReadIntLE<int16>(f);
	}

	// Read signed 16-Bit integer in big-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	template <typename TFileCursor>
	int16 ReadInt16BE(TFileCursor &f)
	{
		return ReadIntBE<int16>(f);
	}

	// Read a single 8bit character.
	// If successful, the file cursor is advanced by the size of the integer.
	template <typename TFileCursor>
	char ReadChar(TFileCursor &f)
	{
		char target;
		if(Read(f, target))
		{
			return target;
		} else
		{
			return 0;
		}
	}

	// Read unsigned 8-Bit integer.
	// If successful, the file cursor is advanced by the size of the integer.
	template <typename TFileCursor>
	uint8 ReadUint8(TFileCursor &f)
	{
		uint8 target;
		if(Read(f, target))
		{
			return target;
		} else
		{
			return 0;
		}
	}

	// Read signed 8-Bit integer. If successful, the file cursor is advanced by the size of the integer.
	template <typename TFileCursor>
	int8 ReadInt8(TFileCursor &f)
	{
		int8 target;
		if(Read(f, target))
		{
			return target;
		} else
		{
			return 0;
		}
	}

	// Read 32-Bit float in little-endian format.
	// If successful, the file cursor is advanced by the size of the float.
	template <typename TFileCursor>
	float ReadFloatLE(TFileCursor &f)
	{
		IEEE754binary32LE target;
		if(Read(f, target))
		{
			return target;
		} else
		{
			return 0.0f;
		}
	}

	// Read 32-Bit float in big-endian format.
	// If successful, the file cursor is advanced by the size of the float.
	template <typename TFileCursor>
	float ReadFloatBE(TFileCursor &f)
	{
		IEEE754binary32BE target;
		if(Read(f, target))
		{
			return target;
		} else
		{
			return 0.0f;
		}
	}

	// Read 64-Bit float in little-endian format.
	// If successful, the file cursor is advanced by the size of the float.
	template <typename TFileCursor>
	double ReadDoubleLE(TFileCursor &f)
	{
		IEEE754binary64LE target;
		if(Read(f, target))
		{
			return target;
		} else
		{
			return 0.0;
		}
	}

	// Read 64-Bit float in big-endian format.
	// If successful, the file cursor is advanced by the size of the float.
	template <typename TFileCursor>
	double ReadDoubleBE(TFileCursor &f)
	{
		IEEE754binary64BE target;
		if(Read(f, target))
		{
			return target;
		} else
		{
			return 0.0;
		}
	}

	// Read a struct.
	// If successful, the file cursor is advanced by the size of the struct. Otherwise, the target is zeroed.
	template <typename T, typename TFileCursor>
	bool ReadStruct(TFileCursor &f, T &target)
	{
		static_assert(mpt::is_binary_safe<T>::value);
		if(Read(f, target))
		{
			return true;
		} else
		{
			Clear(target);
			return false;
		}
	}

	// Allow to read a struct partially (if there's less memory available than the struct's size, fill it up with zeros).
	// The file cursor is advanced by "partialSize" bytes.
	template <typename T, typename TFileCursor>
	typename TFileCursor::off_t ReadStructPartial(TFileCursor &f, T &target, typename TFileCursor::off_t partialSize = sizeof(T))
	{
		static_assert(mpt::is_binary_safe<T>::value);
		typename TFileCursor::off_t copyBytes = std::min(partialSize, sizeof(T));
		if(!f.CanRead(copyBytes))
		{
			copyBytes = f.BytesLeft();
		}
		f.GetRaw(mpt::as_raw_memory(target).data(), copyBytes);
		std::memset(mpt::as_raw_memory(target).data() + copyBytes, 0, sizeof(target) - copyBytes);
		f.Skip(partialSize);
		return copyBytes;
	}

	// Read a string of length srcSize into fixed-length char array destBuffer using a given read mode.
	// The file cursor is advanced by "srcSize" bytes.
	// Returns true if at least one byte could be read or 0 bytes were requested.
	template<mpt::String::ReadWriteMode mode, size_t destSize, typename TFileCursor>
	bool ReadString(TFileCursor &f, char (&destBuffer)[destSize], const typename TFileCursor::off_t srcSize)
	{
		typename TFileCursor::PinnedRawDataView source = f.ReadPinnedRawDataView(srcSize); // Make sure the string is cached properly.
		typename TFileCursor::off_t realSrcSize = source.size();	// In case fewer bytes are available
		mpt::String::WriteAutoBuf(destBuffer) = mpt::String::ReadBuf(mode, mpt::byte_cast<const char*>(source.data()), realSrcSize);
		return (realSrcSize > 0 || srcSize == 0);
	}

	// Read a string of length srcSize into a std::string dest using a given read mode.
	// The file cursor is advanced by "srcSize" bytes.
	// Returns true if at least one character could be read or 0 characters were requested.
	template<mpt::String::ReadWriteMode mode, typename TFileCursor>
	bool ReadString(TFileCursor &f, std::string &dest, const typename TFileCursor::off_t srcSize)
	{
		dest.clear();
		typename TFileCursor::PinnedRawDataView source = f.ReadPinnedRawDataView(srcSize);	// Make sure the string is cached properly.
		typename TFileCursor::off_t realSrcSize = source.size();	// In case fewer bytes are available
		dest = mpt::String::ReadBuf(mode, mpt::byte_cast<const char*>(source.data()), realSrcSize);
		return (realSrcSize > 0 || srcSize == 0);
	}

	// Read a string of length srcSize into a mpt::charbuf dest using a given read mode.
	// The file cursor is advanced by "srcSize" bytes.
	// Returns true if at least one character could be read or 0 characters were requested.
	template<mpt::String::ReadWriteMode mode, std::size_t len, typename TFileCursor>
	bool ReadString(TFileCursor &f, mpt::charbuf<len> &dest, const typename TFileCursor::off_t srcSize)
	{
		typename TFileCursor::PinnedRawDataView source = f.ReadPinnedRawDataView(srcSize);	// Make sure the string is cached properly.
		typename TFileCursor::off_t realSrcSize = source.size();	// In case fewer bytes are available
		dest = mpt::String::ReadBuf(mode, mpt::byte_cast<const char*>(source.data()), realSrcSize);
		return (realSrcSize > 0 || srcSize == 0);
	}

	// Read a charset encoded string of length srcSize into a mpt::ustring dest using a given read mode.
	// The file cursor is advanced by "srcSize" bytes.
	// Returns true if at least one character could be read or 0 characters were requested.
	template<mpt::String::ReadWriteMode mode, typename TFileCursor>
	bool ReadString(TFileCursor &f, mpt::ustring &dest, mpt::Charset charset, const typename TFileCursor::off_t srcSize)
	{
		dest.clear();
		typename TFileCursor::PinnedRawDataView source = f.ReadPinnedRawDataView(srcSize);	// Make sure the string is cached properly.
		typename TFileCursor::off_t realSrcSize = source.size();	// In case fewer bytes are available
		dest = mpt::ToUnicode(charset, mpt::String::ReadBuf(mode, mpt::byte_cast<const char*>(source.data()), realSrcSize));
		return (realSrcSize > 0 || srcSize == 0);
	}

	// Read a string with a preprended length field of type Tsize (must be a packed<*,*> type) into a std::string dest using a given read mode.
	// The file cursor is advanced by the string length.
	// Returns true if the size field could be read and at least one character could be read or 0 characters were requested.
	template<typename Tsize, mpt::String::ReadWriteMode mode, size_t destSize, typename TFileCursor>
	bool ReadSizedString(TFileCursor &f, char (&destBuffer)[destSize], const typename TFileCursor::off_t maxLength = std::numeric_limits<typename TFileCursor::off_t>::max())
	{
		packed<typename Tsize::base_type, typename Tsize::endian_type> srcSize;	// Enforce usage of a packed type by ensuring that the passed type has the required typedefs
		if(!Read(f, srcSize))
			return false;
		return ReadString<mode>(f, destBuffer, std::min(static_cast<typename TFileCursor::off_t>(srcSize), maxLength));
	}

	// Read a string with a preprended length field of type Tsize (must be a packed<*,*> type) into a std::string dest using a given read mode.
	// The file cursor is advanced by the string length.
	// Returns true if the size field could be read and at least one character could be read or 0 characters were requested.
	template<typename Tsize, mpt::String::ReadWriteMode mode, typename TFileCursor>
	bool ReadSizedString(TFileCursor &f, std::string &dest, const typename TFileCursor::off_t maxLength = std::numeric_limits<typename TFileCursor::off_t>::max())
	{
		packed<typename Tsize::base_type, typename Tsize::endian_type> srcSize;	// Enforce usage of a packed type by ensuring that the passed type has the required typedefs
		if(!Read(f, srcSize))
			return false;
		return ReadString<mode>(f, dest, std::min(static_cast<typename TFileCursor::off_t>(srcSize), maxLength));
	}

	// Read a string with a preprended length field of type Tsize (must be a packed<*,*> type) into a mpt::charbuf dest using a given read mode.
	// The file cursor is advanced by the string length.
	// Returns true if the size field could be read and at least one character could be read or 0 characters were requested.
	template<typename Tsize, mpt::String::ReadWriteMode mode, std::size_t len, typename TFileCursor>
	bool ReadSizedString(TFileCursor &f, mpt::charbuf<len> &dest, const typename TFileCursor::off_t maxLength = std::numeric_limits<typename TFileCursor::off_t>::max())
	{
		packed<typename Tsize::base_type, typename Tsize::endian_type> srcSize;	// Enforce usage of a packed type by ensuring that the passed type has the required typedefs
		if(!Read(f, srcSize))
			return false;
		return ReadString<mode>(f, dest, std::min(static_cast<typename TFileCursor::off_t>(srcSize), maxLength));
	}

	// Read a null-terminated string into a std::string
	template <typename TFileCursor>
	bool ReadNullString(TFileCursor &f, std::string &dest, const typename TFileCursor::off_t maxLength = std::numeric_limits<typename TFileCursor::off_t>::max())
	{
		dest.clear();
		if(!f.CanRead(1))
			return false;
		try
		{
			char buffer[64];
			typename TFileCursor::off_t avail = 0;
			while((avail = std::min(f.GetRaw(buffer, std::size(buffer)), maxLength - dest.length())) != 0)
			{
				auto end = std::find(buffer, buffer + avail, '\0');
				dest.insert(dest.end(), buffer, end);
				f.Skip(end - buffer);
				if(end < buffer + avail)
				{
					// Found null char
					f.Skip(1);
					break;
				}
			}
		} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
		{
			MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e);
		}
		return dest.length() != 0;
	}

	// Read a string up to the next line terminator into a std::string
	template <typename TFileCursor>
	bool ReadLine(TFileCursor &f, std::string &dest, const typename TFileCursor::off_t maxLength = std::numeric_limits<typename TFileCursor::off_t>::max())
	{
		dest.clear();
		if(!f.CanRead(1))
			return false;
		try
		{
			char buffer[64];
			char c = '\0';
			typename TFileCursor::off_t avail = 0;
			while((avail = std::min(f.GetRaw(buffer, std::size(buffer)), maxLength - dest.length())) != 0)
			{
				auto end = std::find_if(buffer, buffer + avail, mpt::String::Traits<std::string>::IsLineEnding);
				dest.insert(dest.end(), buffer, end);
				f.Skip(end - buffer);
				if(end < buffer + avail)
				{
					// Found line ending
					f.Skip(1);
					// Handle CRLF line ending
					if(*end == '\r')
					{
						if(Read(f, c) && c != '\n')
							f.SkipBack(1);
					}
					break;
				}
			}
		} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
		{
			MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e);
		}
		return true;
	}

	// Read an array of binary-safe T values.
	// If successful, the file cursor is advanced by the size of the array.
	// Otherwise, the target is zeroed.
	template<typename T, std::size_t destSize, typename TFileCursor>
	bool ReadArray(TFileCursor &f, T (&destArray)[destSize])
	{
		static_assert(mpt::is_binary_safe<T>::value);
		if(f.CanRead(sizeof(destArray)))
		{
			for(auto &element : destArray)
			{
				Read(f, element);
			}
			return true;
		} else
		{
			Clear(destArray);
			return false;
		}
	}

	// Read an array of binary-safe T values.
	// If successful, the file cursor is advanced by the size of the array.
	// Otherwise, the target is zeroed.
	template<typename T, std::size_t destSize, typename TFileCursor>
	bool ReadArray(TFileCursor &f, std::array<T, destSize> &destArray)
	{
		static_assert(mpt::is_binary_safe<T>::value);
		if(f.CanRead(sizeof(destArray)))
		{
			for(auto &element : destArray)
			{
				Read(f, element);
			}
			return true;
		} else
		{
			destArray.fill(T());
			return false;
		}
	}

	// Read destSize elements of binary-safe type T into a vector.
	// If successful, the file cursor is advanced by the size of the vector.
	// Otherwise, the vector is resized to destSize, but possibly existing contents are not cleared.
	template<typename T, typename TFileCursor>
	bool ReadVector(TFileCursor &f, std::vector<T> &destVector, size_t destSize)
	{
		static_assert(mpt::is_binary_safe<T>::value);
		destVector.resize(destSize);
		if(f.CanRead(sizeof(T) * destSize))
		{
			for(auto &element : destVector)
			{
				Read(f, element);
			}
			return true;
		} else
		{
			return false;
		}
	}

	template <typename T, std::size_t destSize, typename TFileCursor>
	std::array<T, destSize> ReadArray(TFileCursor &f)
	{
		std::array<T, destSize> destArray;
		ReadArray(f, destArray);
		return destArray;
	}

	// Compare a magic string with the current stream position.
	// Returns true if they are identical and advances the file cursor by the the length of the "magic" string.
	// Returns false if the string could not be found. The file cursor is not advanced in this case.
	template <typename TFileCursor>
	bool ReadMagic(TFileCursor &f, const char *const magic, typename TFileCursor::off_t magicLength)
	{
		std::byte buffer[16] = { std::byte(0) };
		typename TFileCursor::off_t bytesRead = 0;
		typename TFileCursor::off_t bytesRemain = magicLength;
		while(bytesRemain)
		{
			typename TFileCursor::off_t numBytes = std::min(static_cast<typename TFileCursor::off_t>(sizeof(buffer)), bytesRemain);
			if(f.GetRawWithOffset(bytesRead, buffer, numBytes) != numBytes)
				return false;
			if(memcmp(buffer, magic + bytesRead, numBytes))
				return false;
			bytesRemain -= numBytes;
			bytesRead += numBytes;
		}
		f.Skip(magicLength);
		return true;
	}
	template<size_t N, typename TFileCursor>
	bool ReadMagic(TFileCursor &f, const char (&magic)[N])
	{
		MPT_ASSERT(magic[N - 1] == '\0');
		for(std::size_t i = 0; i < N - 1; ++i)
		{
			MPT_ASSERT(magic[i] != '\0');
		}
		return ReadMagic(f, static_cast<const char*>(magic), static_cast<typename TFileCursor::off_t>(N - 1));
	}

	// Read variable-length unsigned integer (as found in MIDI files).
	// If successful, the file cursor is advanced by the size of the integer and true is returned.
	// False is returned if not enough bytes were left to finish reading of the integer or if an overflow happened (source doesn't fit into target integer).
	// In case of an overflow, the target is also set to the maximum value supported by its data type.
	template<typename T, typename TFileCursor>
	bool ReadVarInt(TFileCursor &f, T &target)
	{
		static_assert(std::numeric_limits<T>::is_integer == true
			&& std::numeric_limits<T>::is_signed == false,
			"Target type is not an unsigned integer");

		if(f.NoBytesLeft())
		{
			target = 0;
			return false;
		}

		std::byte bytes[16];	// More than enough for any valid VarInt
		typename TFileCursor::off_t avail = f.GetRaw(bytes, sizeof(bytes));
		typename TFileCursor::off_t readPos = 1;
		
		size_t writtenBits = 0;
		uint8 b = mpt::byte_cast<uint8>(bytes[0]);
		target = (b & 0x7F);

		// Count actual bits used in most significant byte (i.e. this one)
		for(size_t bit = 0; bit < 7; bit++)
		{
			if((b & (1u << bit)) != 0)
			{
				writtenBits = bit + 1;
			}
		}

		while(readPos < avail && (b & 0x80) != 0)
		{
			b = mpt::byte_cast<uint8>(bytes[readPos++]);
			target <<= 7;
			target |= (b & 0x7F);
			writtenBits += 7;
			if(readPos == avail)
			{
				f.Skip(readPos);
				avail = f.GetRaw(bytes, sizeof(bytes));
				readPos = 0;
			}
		}
		f.Skip(readPos);

		if(writtenBits > sizeof(target) * 8u)
		{
			// Overflow
			target = Util::MaxValueOfType<T>(target);
			return false;
		} else if((b & 0x80) != 0)
		{
			// Reached EOF
			return false;
		}
		return true;
	}

} // namespace FileReader
} // namespace mpt

namespace FR = mpt::FileReader;

namespace detail {

template <typename Ttraits>
class FileReader
{

private:

	using traits_type = Ttraits;
	
public:

	using off_t = typename traits_type::off_t;

	using data_type        = typename traits_type::data_type;
	using ref_data_type    = typename traits_type::ref_data_type;
	using shared_data_type = typename traits_type::shared_data_type;
	using value_data_type  = typename traits_type::value_data_type;

protected:

	shared_data_type SharedDataContainer() const { return traits_type::get_shared(m_data); }
	ref_data_type DataContainer() const { return traits_type::get_ref(m_data); }

	static value_data_type DataInitializer() { return traits_type::make_data(); }
	static value_data_type DataInitializer(mpt::const_byte_span data) { return traits_type::make_data(data); }

	static value_data_type CreateChunkImpl(shared_data_type data, off_t position, off_t size) { return traits_type::make_chunk(data, position, size); }

private:

	data_type m_data;

	off_t streamPos;		// Cursor location in the file

	const mpt::PathString *fileName;  // Filename that corresponds to this FileReader. It is only set if this FileReader represents the whole contents of fileName. May be nullptr. Lifetime is managed outside of FileReader.

public:

	// Initialize invalid file reader object.
	FileReader() : m_data(DataInitializer()), streamPos(0), fileName(nullptr) { }

	// Initialize file reader object with pointer to data and data length.
	template <typename Tbyte> FileReader(mpt::span<Tbyte> bytedata, const mpt::PathString *filename = nullptr) : m_data(DataInitializer(mpt::byte_cast<mpt::const_byte_span>(bytedata))), streamPos(0), fileName(filename) { }

	// Initialize file reader object based on an existing file reader object window.
	explicit FileReader(value_data_type other, const mpt::PathString *filename = nullptr) : m_data(other), streamPos(0), fileName(filename) { }

public:

	mpt::PathString GetFileName() const
	{
		if(!fileName)
		{
			return mpt::PathString();
		}
		return *fileName;
	}

	// Returns true if the object points to a valid (non-empty) stream.
	operator bool() const
	{
		return IsValid();
	}

	// Returns true if the object points to a valid (non-empty) stream.
	bool IsValid() const
	{
		return DataContainer().IsValid();
	}

	// Reset cursor to first byte in file.
	void Rewind()
	{
		streamPos = 0;
	}

	// Seek to a position in the mapped file.
	// Returns false if position is invalid.
	bool Seek(off_t position)
	{
		if(position <= streamPos)
		{
			streamPos = position;
			return true;
		}
		if(position <= DataContainer().GetLength())
		{
			streamPos = position;
			return true;
		} else
		{
			return false;
		}
	}

	// Increases position by skipBytes.
	// Returns true if skipBytes could be skipped or false if the file end was reached earlier.
	bool Skip(off_t skipBytes)
	{
		if(CanRead(skipBytes))
		{
			streamPos += skipBytes;
			return true;
		} else
		{
			streamPos = DataContainer().GetLength();
			return false;
		}
	}

	// Decreases position by skipBytes.
	// Returns true if skipBytes could be skipped or false if the file start was reached earlier.
	bool SkipBack(off_t skipBytes)
	{
		if(streamPos >= skipBytes)
		{
			streamPos -= skipBytes;
			return true;
		} else
		{
			streamPos = 0;
			return false;
		}
	}

	// Returns cursor position in the mapped file.
	off_t GetPosition() const
	{
		return streamPos;
	}

	// Return true IFF seeking and GetLength() is fast.
	// In particular, it returns false for unseekable stream where GetLength()
	// requires pre-caching.
	bool HasFastGetLength() const
	{
		return DataContainer().HasFastGetLength();
	}

	// Returns size of the mapped file in bytes.
	FILEREADER_DEPRECATED off_t GetLength() const
	{
		// deprecated because in case of an unseekable std::istream, this triggers caching of the whole file
		return DataContainer().GetLength();
	}

	// Return byte count between cursor position and end of file, i.e. how many bytes can still be read.
	FILEREADER_DEPRECATED off_t BytesLeft() const
	{
		// deprecated because in case of an unseekable std::istream, this triggers caching of the whole file
		return DataContainer().GetLength() - streamPos;
	}

	bool EndOfFile() const
	{
		return !CanRead(1);
	}

	bool NoBytesLeft() const
	{
		return !CanRead(1);
	}

	// Check if "amount" bytes can be read from the current position in the stream.
	bool CanRead(off_t amount) const
	{
		return DataContainer().CanRead(streamPos, amount);
	}

	// Check if file size is at least size, without potentially caching the whole file to query the exact file length.
	bool LengthIsAtLeast(off_t size) const
	{
		return DataContainer().CanRead(0, size);
	}

	// Check if file size is exactly size, without potentially caching the whole file if it is larger.
	bool LengthIs(off_t size) const
	{
		return DataContainer().CanRead(0, size) && !DataContainer().CanRead(size, 1);
	}

protected:

	FileReader CreateChunk(off_t position, off_t length) const
	{
		off_t readableLength = DataContainer().GetReadableLength(position, length);
		if(readableLength == 0)
		{
			return FileReader();
		}
		return FileReader(CreateChunkImpl(SharedDataContainer(), position, std::min(length, DataContainer().GetLength() - position)));
	}

public:

	// Create a new FileReader object for parsing a sub chunk at a given position with a given length.
	// The file cursor is not modified.
	FileReader GetChunkAt(off_t position, off_t length) const
	{
		return CreateChunk(position, length);
	}

	// Create a new FileReader object for parsing a sub chunk at the current position with a given length.
	// The file cursor is not advanced.
	FileReader GetChunk(off_t length)
	{
		return CreateChunk(streamPos, length);
	}
	// Create a new FileReader object for parsing a sub chunk at the current position with a given length.
	// The file cursor is advanced by "length" bytes.
	FileReader ReadChunk(off_t length)
	{
		off_t position = streamPos;
		Skip(length);
		return CreateChunk(position, length);
	}

	class PinnedRawDataView
	{
	private:
		std::size_t size_;
		const std::byte *pinnedData;
		std::vector<std::byte> cache;
	private:
		void Init(const FileReader &file, std::size_t size)
		{
			size_ = 0;
			pinnedData = nullptr;
			if(!file.CanRead(size))
			{
				size = file.BytesLeft();
			}
			size_ = size;
			if(file.DataContainer().HasPinnedView())
			{
				pinnedData = file.DataContainer().GetRawData() + file.GetPosition();
			} else
			{
				cache.resize(size_);
				if(!cache.empty())
				{
					// cppcheck false-positive
					// cppcheck-suppress containerOutOfBounds
					file.GetRaw(&(cache[0]), size);
				}
			}
		}
	public:
		PinnedRawDataView()
			: size_(0)
			, pinnedData(nullptr)
		{
		}
		PinnedRawDataView(const FileReader &file)
		{
			Init(file, file.BytesLeft());
		}
		PinnedRawDataView(const FileReader &file, std::size_t size)
		{
			Init(file, size);
		}
		PinnedRawDataView(FileReader &file, bool advance)
		{
			Init(file, file.BytesLeft());
			if(advance)
			{
				file.Skip(size_);
			}
		}
		PinnedRawDataView(FileReader &file, std::size_t size, bool advance)
		{
			Init(file, size);
			if(advance)
			{
				file.Skip(size_);
			}
		}
	public:
		mpt::const_byte_span GetSpan() const
		{
			if(pinnedData)
			{
				return mpt::as_span(pinnedData, size_);
			} else if(!cache.empty())
			{
				return mpt::as_span(cache);
			} else
			{
				return mpt::const_byte_span();
			}
		}
		mpt::const_byte_span span() const { return GetSpan(); }
		void invalidate() { size_ = 0; pinnedData = nullptr; cache = std::vector<std::byte>(); }
		const std::byte *data() const { return span().data(); }
		std::size_t size() const { return size_; }
		mpt::const_byte_span::pointer begin() const { return span().data(); }
		mpt::const_byte_span::pointer end() const { return span().data() + span().size(); }
		mpt::const_byte_span::const_pointer cbegin() const { return span().data(); }
		mpt::const_byte_span::const_pointer cend() const { return span().data() + span().size(); }
	};

	// Returns a pinned view into the remaining raw data from cursor position.
	PinnedRawDataView GetPinnedRawDataView() const
	{
		return PinnedRawDataView(*this);
	}
	// Returns a pinned view into the remeining raw data from cursor position, clamped at size.
	PinnedRawDataView GetPinnedRawDataView(std::size_t size) const
	{
		return PinnedRawDataView(*this, size);
	}

	// Returns a pinned view into the remeining raw data from cursor position.
	// File cursor is advaned by the size of the returned pinned view.
	PinnedRawDataView ReadPinnedRawDataView()
	{
		return PinnedRawDataView(*this, true);
	}
	// Returns a pinned view into the remeining raw data from cursor position, clamped at size.
	// File cursor is advaned by the size of the returned pinned view.
	PinnedRawDataView ReadPinnedRawDataView(std::size_t size)
	{
		return PinnedRawDataView(*this, size, true);
	}

	// Returns raw stream data at cursor position.
	// Should only be used if absolutely necessary, for example for sample reading, or when used with a small chunk of the file retrieved by ReadChunk().
	// Use GetPinnedRawDataView(size) whenever possible.
	FILEREADER_DEPRECATED const std::byte *GetRawData() const
	{
		// deprecated because in case of an unseekable std::istream, this triggers caching of the whole file
		return DataContainer().GetRawData() + streamPos;
	}
	template <typename T>
	FILEREADER_DEPRECATED const T *GetRawData() const
	{
		// deprecated because in case of an unseekable std::istream, this triggers caching of the whole file
		return mpt::byte_cast<const T*>(DataContainer().GetRawData() + streamPos);
	}

	template <typename T>
	std::size_t GetRawWithOffset(std::size_t offset, T *dst, std::size_t count) const
	{
		return static_cast<std::size_t>(DataContainer().Read(mpt::byte_cast<std::byte*>(dst), streamPos + offset, count));
	}
	std::size_t GetRawWithOffset(std::size_t offset, mpt::byte_span dst) const
	{
		return static_cast<std::size_t>(DataContainer().Read(streamPos + offset, dst));
	}

	template <typename T>
	std::size_t GetRaw(T *dst, std::size_t count) const
	{
		return static_cast<std::size_t>(DataContainer().Read(mpt::byte_cast<std::byte*>(dst), streamPos, count));
	}
	std::size_t GetRaw(mpt::byte_span dst) const
	{
		return static_cast<std::size_t>(DataContainer().Read(streamPos, dst));
	}

	template <typename T>
	std::size_t ReadRaw(T *dst, std::size_t count)
	{
		std::size_t result = static_cast<std::size_t>(DataContainer().Read(mpt::byte_cast<std::byte*>(dst), streamPos, count));
		streamPos += result;
		return result;
	}
	std::size_t ReadRaw(mpt::byte_span dst)
	{
		std::size_t result = static_cast<std::size_t>(DataContainer().Read(streamPos, dst));
		streamPos += result;
		return result;
	}
	
	std::vector<std::byte> GetRawDataAsByteVector() const
	{
		PinnedRawDataView view = GetPinnedRawDataView();
		return mpt::make_vector(view.span());
	}
	std::vector<std::byte> ReadRawDataAsByteVector()
	{
		PinnedRawDataView view = ReadPinnedRawDataView();
		return mpt::make_vector(view.span());
	}
	std::vector<std::byte> GetRawDataAsByteVector(std::size_t size) const
	{
		PinnedRawDataView view = GetPinnedRawDataView(size);
		return mpt::make_vector(view.span());
	}
	std::vector<std::byte> ReadRawDataAsByteVector(std::size_t size)
	{
		PinnedRawDataView view = ReadPinnedRawDataView(size);
		return mpt::make_vector(view.span());
	}

	std::string GetRawDataAsString() const
	{
		PinnedRawDataView view = GetPinnedRawDataView();
		mpt::span<const char> data = mpt::byte_cast<mpt::span<const char>>(view.span());
		return std::string(data.begin(), data.end());
	}
	std::string ReadRawDataAsString()
	{
		PinnedRawDataView view = ReadPinnedRawDataView();
		mpt::span<const char> data = mpt::byte_cast<mpt::span<const char>>(view.span());
		return std::string(data.begin(), data.end());
	}
	std::string GetRawDataAsString(std::size_t size) const
	{
		PinnedRawDataView view = GetPinnedRawDataView(size);
		mpt::span<const char> data = mpt::byte_cast<mpt::span<const char>>(view.span());
		return std::string(data.begin(), data.end());
	}
	std::string ReadRawDataAsString(std::size_t size)
	{
		PinnedRawDataView view = ReadPinnedRawDataView(size);
		mpt::span<const char> data = mpt::byte_cast<mpt::span<const char>>(view.span());
		return std::string(data.begin(), data.end());
	}

	template <typename T>
	bool Read(T &target)
	{
		return mpt::FileReader::Read(*this, target);
	}

	template <typename T>
	T ReadIntLE()
	{
		return mpt::FileReader::ReadIntLE<T>(*this);
	}

	template <typename T>
	T ReadIntBE()
	{
		return mpt::FileReader::ReadIntLE<T>(*this);
	}

	template <typename T>
	T ReadTruncatedIntLE(off_t size)
	{
		return mpt::FileReader::ReadTruncatedIntLE<T>(*this, size);
	}

	template <typename T>
	T ReadSizedIntLE(off_t size)
	{
		return mpt::FileReader::ReadSizedIntLE<T>(*this, size);
	}

	uint32 ReadUint32LE()
	{
		return mpt::FileReader::ReadUint32LE(*this);
	}

	uint32 ReadUint32BE()
	{
		return mpt::FileReader::ReadUint32BE(*this);
	}

	int32 ReadInt32LE()
	{
		return mpt::FileReader::ReadInt32LE(*this);
	}

	int32 ReadInt32BE()
	{
		return mpt::FileReader::ReadInt32BE(*this);
	}

	uint16 ReadUint16LE()
	{
		return mpt::FileReader::ReadUint16LE(*this);
	}

	uint16 ReadUint16BE()
	{
		return mpt::FileReader::ReadUint16BE(*this);
	}

	int16 ReadInt16LE()
	{
		return mpt::FileReader::ReadInt16LE(*this);
	}

	int16 ReadInt16BE()
	{
		return mpt::FileReader::ReadInt16BE(*this);
	}

	char ReadChar()
	{
		return mpt::FileReader::ReadChar(*this);
	}

	uint8 ReadUint8()
	{
		return mpt::FileReader::ReadUint8(*this);
	}

	int8 ReadInt8()
	{
		return mpt::FileReader::ReadInt8(*this);
	}

	float ReadFloatLE()
	{
		return mpt::FileReader::ReadFloatLE(*this);
	}

	float ReadFloatBE()
	{
		return mpt::FileReader::ReadFloatBE(*this);
	}

	double ReadDoubleLE()
	{
		return mpt::FileReader::ReadDoubleLE(*this);
	}

	double ReadDoubleBE()
	{
		return mpt::FileReader::ReadDoubleBE(*this);
	}

	template <typename T>
	bool ReadStruct(T &target)
	{
		return mpt::FileReader::ReadStruct(*this, target);
	}

	template <typename T>
	size_t ReadStructPartial(T &target, size_t partialSize = sizeof(T))
	{
		return mpt::FileReader::ReadStructPartial(*this, target, partialSize);
	}

	template<mpt::String::ReadWriteMode mode, size_t destSize>
	bool ReadString(char (&destBuffer)[destSize], const off_t srcSize)
	{
		return mpt::FileReader::ReadString<mode>(*this, destBuffer, srcSize);
	}

	template<mpt::String::ReadWriteMode mode>
	bool ReadString(std::string &dest, const off_t srcSize)
	{
		return mpt::FileReader::ReadString<mode>(*this, dest, srcSize);
	}

	template<mpt::String::ReadWriteMode mode, std::size_t len>
	bool ReadString(mpt::charbuf<len> &dest, const off_t srcSize)
	{
		return mpt::FileReader::ReadString<mode>(*this, dest, srcSize);
	}

	template<mpt::String::ReadWriteMode mode>
	bool ReadString(mpt::ustring &dest, mpt::Charset charset, const off_t srcSize)
	{
		return mpt::FileReader::ReadString<mode>(*this, dest, charset, srcSize);
	}

	template<typename Tsize, mpt::String::ReadWriteMode mode, size_t destSize>
	bool ReadSizedString(char (&destBuffer)[destSize], const off_t maxLength = std::numeric_limits<off_t>::max())
	{
		return mpt::FileReader::ReadSizedString<Tsize, mode>(*this, destBuffer, maxLength);
	}

	template<typename Tsize, mpt::String::ReadWriteMode mode>
	bool ReadSizedString(std::string &dest, const off_t maxLength = std::numeric_limits<off_t>::max())
	{
		return mpt::FileReader::ReadSizedString<Tsize, mode>(*this, dest, maxLength);
	}

	template<typename Tsize, mpt::String::ReadWriteMode mode, std::size_t len>
	bool ReadSizedString(mpt::charbuf<len> &dest, const off_t maxLength = std::numeric_limits<off_t>::max())
	{
		return mpt::FileReader::ReadSizedString<Tsize, mode, len>(*this, dest, maxLength);
	}

	bool ReadNullString(std::string &dest, const off_t maxLength = std::numeric_limits<off_t>::max())
	{
		return mpt::FileReader::ReadNullString(*this, dest, maxLength);
	}

	bool ReadLine(std::string &dest, const off_t maxLength = std::numeric_limits<off_t>::max())
	{
		return mpt::FileReader::ReadLine(*this, dest, maxLength);
	}

	template<typename T, std::size_t destSize>
	bool ReadArray(T (&destArray)[destSize])
	{
		return mpt::FileReader::ReadArray(*this, destArray);
	}

	template<typename T, std::size_t destSize>
	bool ReadArray(std::array<T, destSize> &destArray)
	{
		return mpt::FileReader::ReadArray(*this, destArray);
	}

	template <typename T, std::size_t destSize>
	std::array<T, destSize> ReadArray()
	{
		return mpt::FileReader::ReadArray<T, destSize>(*this);
	}

	template<typename T>
	bool ReadVector(std::vector<T> &destVector, size_t destSize)
	{
		return mpt::FileReader::ReadVector(*this, destVector, destSize);
	}

	template<size_t N>
	bool ReadMagic(const char (&magic)[N])
	{
		return mpt::FileReader::ReadMagic(*this, magic);
	}

	bool ReadMagic(const char *const magic, off_t magicLength)
	{
		return mpt::FileReader::ReadMagic(*this, magic, magicLength);
	}

	template<typename T>
	bool ReadVarInt(T &target)
	{
		return mpt::FileReader::ReadVarInt(*this, target);
	}

};

} // namespace detail

using FileReader = detail::FileReader<FileReaderTraitsDefault>;

using MemoryFileReader = detail::FileReader<FileReaderTraitsMemory>;


// Initialize file reader object with pointer to data and data length.
template <typename Tbyte> static inline FileReader make_FileReader(mpt::span<Tbyte> bytedata, const mpt::PathString *filename = nullptr)
{
	return FileReader(mpt::byte_cast<mpt::const_byte_span>(bytedata), filename);
}

#if defined(MPT_FILEREADER_CALLBACK_STREAM)

// Initialize file reader object with a CallbackStream.
static inline FileReader make_FileReader(CallbackStream s, const mpt::PathString *filename = nullptr)
{
	return FileReader(
				FileDataContainerCallbackStreamSeekable::IsSeekable(s) ?
					std::static_pointer_cast<IFileDataContainer>(std::make_shared<FileDataContainerCallbackStreamSeekable>(s))
				:
					std::static_pointer_cast<IFileDataContainer>(std::make_shared<FileDataContainerCallbackStream>(s))
			, filename
		);
}
#endif // MPT_FILEREADER_CALLBACK_STREAM
	
// Initialize file reader object with a std::istream.
static inline FileReader make_FileReader(std::istream *s, const mpt::PathString *filename = nullptr)
{
	return FileReader(
				FileDataContainerStdStreamSeekable::IsSeekable(s) ?
					std::static_pointer_cast<IFileDataContainer>(std::make_shared<FileDataContainerStdStreamSeekable>(s))
				:
					std::static_pointer_cast<IFileDataContainer>(std::make_shared<FileDataContainerStdStream>(s))
			, filename
		);
}


#if defined(MPT_ENABLE_FILEIO)
// templated in order to reduce header inter-dependencies
template <typename TInputFile>
FileReader GetFileReader(TInputFile &file)
{
	if(!file.IsValid())
	{
		return FileReader();
	}
	if(file.IsCached())
	{
		return make_FileReader(file.GetCache(), &file.GetFilenameRef());
	} else
	{
		return make_FileReader(file.GetStream(), &file.GetFilenameRef());
	}
}
#endif // MPT_ENABLE_FILEIO


#if defined(MPT_ENABLE_TEMPFILE) && MPT_OS_WINDOWS

class OnDiskFileWrapper
{

private:

	mpt::PathString m_Filename;
	bool m_IsTempFile;

public:

	OnDiskFileWrapper(FileReader &file, const mpt::PathString &fileNameExtension = P_("tmp"));

	~OnDiskFileWrapper();

public:

	bool IsValid() const;

	mpt::PathString GetFilename() const;

}; // class OnDiskFileWrapper

#endif // MPT_ENABLE_TEMPFILE && MPT_OS_WINDOWS


OPENMPT_NAMESPACE_END
