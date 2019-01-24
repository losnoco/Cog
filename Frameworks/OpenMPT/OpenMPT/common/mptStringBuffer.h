/*
 * mptStringBuffer.h
 * -----------------
 * Purpose: Various functions for "fixing" char array strings for writing to or
 *          reading from module files, or for securing char arrays in general.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "mptString.h"

#include <algorithm>
#include <string>
#include <vector>



OPENMPT_NAMESPACE_BEGIN



namespace mpt
{


namespace String
{


	enum ReadWriteMode
	{
		// Reading / Writing: Standard null-terminated string handling.
		nullTerminated,
		// Reading: Source string is not guaranteed to be null-terminated (if it fills the whole char array).
		// Writing: Destination string is not guaranteed to be null-terminated (if it fills the whole char array).
		maybeNullTerminated,
		// Reading: String may contain null characters anywhere. They should be treated as spaces.
		// Writing: A space-padded string is written.
		spacePadded,
		// Reading: String may contain null characters anywhere. The last character is ignored (it is supposed to be 0).
		// Writing: A space-padded string with a trailing null is written.
		spacePaddedNull
	};
	
	namespace detail
	{

	std::string ReadStringBuffer(String::ReadWriteMode mode, const char *srcBuffer, std::size_t srcSize);

	void WriteStringBuffer(String::ReadWriteMode mode, char *destBuffer, const std::size_t destSize, const char *srcBuffer, const std::size_t srcSize);

	} // namespace detail


} // namespace String



template <typename Tstring, typename Tchar>
class StringBufRefImpl
{
private:
	Tchar * buf;
	std::size_t size;
public:
	// cppcheck false-positive
	// cppcheck-suppress uninitMemberVar
	explicit StringBufRefImpl(Tchar * buf, std::size_t size)
		: buf(buf)
		, size(size)
	{
		MPT_STATIC_ASSERT(sizeof(Tchar) == sizeof(typename Tstring::value_type));
		MPT_ASSERT(size > 0);
	}
	StringBufRefImpl(const StringBufRefImpl &) = delete;
	StringBufRefImpl(StringBufRefImpl &&) = default;
	StringBufRefImpl & operator = (const StringBufRefImpl &) = delete;
	StringBufRefImpl & operator = (StringBufRefImpl &&) = delete;
	operator Tstring () const
	{
		std::size_t len = std::find(buf, buf + size, Tchar('\0')) - buf; // terminate at \0
		return Tstring(buf, buf + len);
	}
	StringBufRefImpl & operator = (const Tstring & str)
	{
		std::fill(buf, buf + size, Tchar('\0'));
		std::copy(str.data(), str.data() + std::min(str.length(), size - 1), buf);
		std::fill(buf + std::min(str.length(), size - 1), buf + size, Tchar('\0'));
		return *this;
	}
};

template <typename Tstring, typename Tchar>
class StringBufRefImpl<Tstring, const Tchar>
{
private:
	const Tchar * buf;
	std::size_t size;
public:
	// cppcheck false-positive
	// cppcheck-suppress uninitMemberVar
	explicit StringBufRefImpl(const Tchar * buf, std::size_t size)
		: buf(buf)
		, size(size)
	{
		MPT_STATIC_ASSERT(sizeof(Tchar) == sizeof(typename Tstring::value_type));
		MPT_ASSERT(size > 0);
	}
	StringBufRefImpl(const StringBufRefImpl &) = delete;
	StringBufRefImpl(StringBufRefImpl &&) = default;
	StringBufRefImpl & operator = (const StringBufRefImpl &) = delete;
	StringBufRefImpl & operator = (StringBufRefImpl &&) = delete;
	operator Tstring () const
	{
		std::size_t len = std::find(buf, buf + size, Tchar('\0')) - buf; // terminate at \0
		return Tstring(buf, buf + len);
	}
};

namespace String {
template <typename Tstring, typename Tchar, std::size_t size>
inline StringBufRefImpl<Tstring, typename std::add_const<Tchar>::type> ReadTypedBuf(Tchar (&buf)[size])
{
	return StringBufRefImpl<Tstring, typename std::add_const<Tchar>::type>(buf, size);
}
template <typename Tstring, typename Tchar>
inline StringBufRefImpl<Tstring, typename std::add_const<Tchar>::type> ReadTypedBuf(Tchar * buf, std::size_t size)
{
	return StringBufRefImpl<Tstring, typename std::add_const<Tchar>::type>(buf, size);
}
template <typename Tstring, typename Tchar, std::size_t size>
inline StringBufRefImpl<Tstring, Tchar> WriteTypedBuf(Tchar (&buf)[size])
{
	return StringBufRefImpl<Tstring, Tchar>(buf, size);
}
template <typename Tstring, typename Tchar>
inline StringBufRefImpl<Tstring, Tchar> WriteTypedBuf(Tchar * buf, std::size_t size)
{
	return StringBufRefImpl<Tstring, Tchar>(buf, size);
}
} // namespace String

namespace String {
template <typename Tchar, std::size_t size>
inline StringBufRefImpl<typename std::basic_string<typename std::remove_const<Tchar>::type>, typename std::add_const<Tchar>::type> ReadAutoBuf(Tchar (&buf)[size])
{
	return StringBufRefImpl<typename std::basic_string<typename std::remove_const<Tchar>::type>, typename std::add_const<Tchar>::type>(buf, size);
}
template <typename Tchar>
inline StringBufRefImpl<typename std::basic_string<typename std::remove_const<Tchar>::type>, typename std::add_const<Tchar>::type> ReadAutoBuf(Tchar * buf, std::size_t size)
{
	return StringBufRefImpl<typename std::basic_string<typename std::remove_const<Tchar>::type>, typename std::add_const<Tchar>::type>(buf, size);
}
template <typename Tchar, std::size_t size>
inline StringBufRefImpl<typename std::basic_string<typename std::remove_const<Tchar>::type>, Tchar> WriteAutoBuf(Tchar (&buf)[size])
{
	return StringBufRefImpl<typename std::basic_string<typename std::remove_const<Tchar>::type>, Tchar>(buf, size);
}
template <typename Tchar>
inline StringBufRefImpl<typename std::basic_string<typename std::remove_const<Tchar>::type>, Tchar> WriteAutoBuf(Tchar * buf, std::size_t size)
{
	return StringBufRefImpl<typename std::basic_string<typename std::remove_const<Tchar>::type>, Tchar>(buf, size);
}
} // namespace String

template <typename Tchar>
class StringModeBufRefImpl
{
private:
	Tchar * buf;
	std::size_t size;
	String::ReadWriteMode mode;
public:
	// cppcheck false-positive
	// cppcheck-suppress uninitMemberVar
	StringModeBufRefImpl(Tchar * buf, std::size_t size, String::ReadWriteMode mode)
		: buf(buf)
		, size(size)
		, mode(mode)
	{
		MPT_STATIC_ASSERT(sizeof(Tchar) == 1);
		MPT_ASSERT(size > 0);
	}
	StringModeBufRefImpl(const StringModeBufRefImpl &) = delete;
	StringModeBufRefImpl(StringModeBufRefImpl &&) = default;
	StringModeBufRefImpl & operator = (const StringModeBufRefImpl &) = delete;
	StringModeBufRefImpl & operator = (StringModeBufRefImpl &&) = delete;
	operator std::string () const
	{
		return String::detail::ReadStringBuffer(mode, buf, size);
	}
	StringModeBufRefImpl & operator = (const std::string & str)
	{
		String::detail::WriteStringBuffer(mode, buf, size, str.data(), str.size());
		return *this;
	}
};

template <typename Tchar>
class StringModeBufRefImpl<const Tchar>
{
private:
	const Tchar * buf;
	std::size_t size;
	String::ReadWriteMode mode;
public:
	// cppcheck false-positive
	// cppcheck-suppress uninitMemberVar
	StringModeBufRefImpl(const Tchar * buf, std::size_t size, String::ReadWriteMode mode)
		: buf(buf)
		, size(size)
		, mode(mode)
	{
		MPT_STATIC_ASSERT(sizeof(Tchar) == 1);
		MPT_ASSERT(size > 0);
	}
	StringModeBufRefImpl(const StringModeBufRefImpl &) = delete;
	StringModeBufRefImpl(StringModeBufRefImpl &&) = default;
	StringModeBufRefImpl & operator = (const StringModeBufRefImpl &) = delete;
	StringModeBufRefImpl & operator = (StringModeBufRefImpl &&) = delete;
	operator std::string () const
	{
		return String::detail::ReadStringBuffer(mode, buf, size);
	}
};

namespace String {
template <typename Tchar, std::size_t size>
inline StringModeBufRefImpl<typename std::add_const<Tchar>::type> ReadBuf(String::ReadWriteMode mode, Tchar (&buf)[size])
{
	return StringModeBufRefImpl<typename std::add_const<Tchar>::type>(buf, size, mode);
}
template <typename Tchar>
inline StringModeBufRefImpl<typename std::add_const<Tchar>::type> ReadBuf(String::ReadWriteMode mode, Tchar * buf, std::size_t size)
{
	return StringModeBufRefImpl<typename std::add_const<Tchar>::type>(buf, size, mode);
}
template <typename Tchar, std::size_t size>
inline StringModeBufRefImpl<Tchar> WriteBuf(String::ReadWriteMode mode, Tchar (&buf)[size])
{
	return StringModeBufRefImpl<Tchar>(buf, size, mode);
}
template <typename Tchar>
inline StringModeBufRefImpl<Tchar> WriteBuf(String::ReadWriteMode mode, Tchar * buf, std::size_t size)
{
	return StringModeBufRefImpl<Tchar>(buf, size, mode);
}
} // namespace String


#ifdef MODPLUG_TRACKER

#if MPT_OS_WINDOWS

namespace String {
template <typename Tchar, std::size_t size>
inline StringBufRefImpl<typename mpt::windows_char_traits<typename std::remove_const<Tchar>::type>::string_type, typename std::add_const<Tchar>::type> ReadWinBuf(Tchar (&buf)[size])
{
	return StringBufRefImpl<typename mpt::windows_char_traits<typename std::remove_const<Tchar>::type>::string_type, typename std::add_const<Tchar>::type>(buf, size);
}
template <typename Tchar>
inline StringBufRefImpl<typename mpt::windows_char_traits<typename std::remove_const<Tchar>::type>::string_type, typename std::add_const<Tchar>::type> ReadWinBuf(Tchar * buf, std::size_t size)
{
	return StringBufRefImpl<typename mpt::windows_char_traits<typename std::remove_const<Tchar>::type>::string_type, typename std::add_const<Tchar>::type>(buf, size);
}
template <typename Tchar, std::size_t size>
inline StringBufRefImpl<typename mpt::windows_char_traits<typename std::remove_const<Tchar>::type>::string_type, Tchar> WriteWinBuf(Tchar (&buf)[size])
{
	return StringBufRefImpl<typename mpt::windows_char_traits<typename std::remove_const<Tchar>::type>::string_type, Tchar>(buf, size);
}
template <typename Tchar>
inline StringBufRefImpl<typename mpt::windows_char_traits<typename std::remove_const<Tchar>::type>::string_type, Tchar> WriteWinBuf(Tchar * buf, std::size_t size)
{
	return StringBufRefImpl<typename mpt::windows_char_traits<typename std::remove_const<Tchar>::type>::string_type, Tchar>(buf, size);
}
} // namespace String

#if defined(_MFC_VER)

template <typename Tchar>
class CStringBufRefImpl
{
private:
	Tchar * buf;
	std::size_t size;
public:
	// cppcheck false-positive
	// cppcheck-suppress uninitMemberVar
	explicit CStringBufRefImpl(Tchar * buf, std::size_t size)
		: buf(buf)
		, size(size)
	{
		MPT_ASSERT(size > 0);
	}
	CStringBufRefImpl(const CStringBufRefImpl &) = delete;
	CStringBufRefImpl(CStringBufRefImpl &&) = default;
	CStringBufRefImpl & operator = (const CStringBufRefImpl &) = delete;
	CStringBufRefImpl & operator = (CStringBufRefImpl &&) = delete;
	operator CString () const
	{
		std::size_t len = std::find(buf, buf + size, Tchar('\0')) - buf; // terminate at \0
		return CString(buf, mpt::saturate_cast<int>(len));
	}
	CStringBufRefImpl & operator = (const CString & str)
	{
		std::fill(buf, buf + size, Tchar('\0'));
		std::copy(str.GetString(), str.GetString() + std::min(static_cast<std::size_t>(str.GetLength()), size - 1), buf);
		buf[size - 1] = Tchar('\0');
		return *this;
	}
};

template <typename Tchar>
class CStringBufRefImpl<const Tchar>
{
private:
	const Tchar * buf;
	std::size_t size;
public:
	// cppcheck false-positive
	// cppcheck-suppress uninitMemberVar
	explicit CStringBufRefImpl(const Tchar * buf, std::size_t size)
		: buf(buf)
		, size(size)
	{
		MPT_ASSERT(size > 0);
	}
	CStringBufRefImpl(const CStringBufRefImpl &) = delete;
	CStringBufRefImpl(CStringBufRefImpl &&) = default;
	CStringBufRefImpl & operator = (const CStringBufRefImpl &) = delete;
	CStringBufRefImpl & operator = (CStringBufRefImpl &&) = delete;
	operator CString () const
	{
		std::size_t len = std::find(buf, buf + size, Tchar('\0')) - buf; // terminate at \0
		return CString(buf, mpt::saturate_cast<int>(len));
	}
};

namespace String {
template <typename Tchar, std::size_t size>
inline CStringBufRefImpl<typename std::add_const<Tchar>::type> ReadCStringBuf(Tchar (&buf)[size])
{
	return CStringBufRefImpl<typename std::add_const<Tchar>::type>(buf, size);
}
template <typename Tchar>
inline CStringBufRefImpl<typename std::add_const<Tchar>::type> ReadCStringBuf(Tchar * buf, std::size_t size)
{
	return CStringBufRefImpl<typename std::add_const<Tchar>::type>(buf, size);
}
template <typename Tchar, std::size_t size>
inline CStringBufRefImpl<Tchar> WriteCStringBuf(Tchar (&buf)[size])
{
	return CStringBufRefImpl<Tchar>(buf, size);
}
template <typename Tchar>
inline CStringBufRefImpl<Tchar> WriteCStringBuf(Tchar * buf, std::size_t size)
{
	return CStringBufRefImpl<Tchar>(buf, size);
}
} // namespace String

#endif // _MFC_VER

#endif // MPT_OS_WINDOWS

#endif // MODPLUG_TRACKER





namespace String
{


#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant
#endif // MPT_COMPILER_MSVC


	// Sets last character to null in given char array.
	// Size of the array must be known at compile time.
	template <size_t size>
	void SetNullTerminator(char (&buffer)[size])
	{
		STATIC_ASSERT(size > 0);
		buffer[size - 1] = 0;
	}

	inline void SetNullTerminator(char *buffer, size_t size)
	{
		MPT_ASSERT(size > 0);
		buffer[size - 1] = 0;
	}

	template <size_t size>
	void SetNullTerminator(wchar_t (&buffer)[size])
	{
		STATIC_ASSERT(size > 0);
		buffer[size - 1] = 0;
	}

	inline void SetNullTerminator(wchar_t *buffer, size_t size)
	{
		MPT_ASSERT(size > 0);
		buffer[size - 1] = 0;
	}


	// Remove any chars after the first null char
	template <size_t size>
	void FixNullString(char (&buffer)[size])
	{
		STATIC_ASSERT(size > 0);
		SetNullTerminator(buffer);
		size_t pos = 0;
		// Find the first null char.
		while(pos < size && buffer[pos] != '\0')
		{
			pos++;
		}
		// Remove everything after the null char.
		while(pos < size)
		{
			buffer[pos++] = '\0';
		}
	}

	inline void FixNullString(std::string & str)
	{
		for(std::size_t i = 0; i < str.length(); ++i)
		{
			if(str[i] == '\0')
			{
				// if we copied \0 in the middle of the buffer, terminate std::string here
				str.resize(i);
				break;
			}
		}
	}


	// Copy a string from srcBuffer to destBuffer using a given read mode.
	// Used for reading strings from files.
	// Only use this version of the function if the size of the source buffer is variable.
	template <ReadWriteMode mode, typename Tbyte>
	void Read(std::string &dest, const Tbyte *srcBuffer, size_t srcSize)
	{

		const char *src = mpt::byte_cast<const char*>(srcBuffer);

		dest.clear();

		try
		{
			dest = mpt::String::detail::ReadStringBuffer(mode, src, srcSize);
		} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
		{
			MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e);
		}
	}

	// Copy a charset encoded string from srcBuffer to destBuffer using a given read mode.
	// Used for reading strings from files.
	// Only use this version of the function if the size of the source buffer is variable.
	template <ReadWriteMode mode, typename Tbyte>
	void Read(mpt::ustring &dest, mpt::Charset charset, const Tbyte *srcBuffer, size_t srcSize)
	{
		std::string tmp;
		Read<mode>(tmp, srcBuffer, srcSize);
		dest = mpt::ToUnicode(charset, tmp);
	}

	// Used for reading strings from files.
	// Preferrably use this version of the function, it is safer.
	template <ReadWriteMode mode, size_t srcSize, typename Tbyte>
	void Read(std::string &dest, const Tbyte (&srcBuffer)[srcSize])
	{
		STATIC_ASSERT(srcSize > 0);
		Read<mode>(dest, srcBuffer, srcSize);
	}

	// Used for reading charset encoded strings from files.
	// Preferrably use this version of the function, it is safer.
	template <ReadWriteMode mode, size_t srcSize, typename Tbyte>
	void Read(mpt::ustring &dest, mpt::Charset charset, const Tbyte(&srcBuffer)[srcSize])
	{
		std::string tmp;
		Read<mode>(tmp, srcBuffer);
		dest = mpt::ToUnicode(charset, tmp);
	}

	// Copy a string from srcBuffer to destBuffer using a given read mode.
	// Used for reading strings from files.
	// Only use this version of the function if the size of the source buffer is variable.
	template <ReadWriteMode mode, size_t destSize, typename Tbyte>
	void Read(char (&destBuffer)[destSize], const Tbyte *srcBuffer, size_t srcSize)
	{
		STATIC_ASSERT(destSize > 0);

		char *dst = destBuffer;
		const char *src = mpt::byte_cast<const char*>(srcBuffer);

		if(mode == nullTerminated || mode == spacePaddedNull)
		{
			// We assume that the last character of the source buffer is null.
			if(srcSize > 0)
			{
				srcSize -= 1;
			}
		}

		if(mode == nullTerminated || mode == maybeNullTerminated)
		{

			// Copy string and leave one character space in the destination buffer for null.
			dst = std::copy(src, std::find(src, src + std::min(srcSize, destSize - 1), '\0'), dst);
	
		} else if(mode == spacePadded || mode == spacePaddedNull)
		{

			// Copy string and leave one character space in the destination buffer for null.
			// Convert nulls to spaces while copying and counts the length that contains actual characters.
			std::size_t lengthWithoutNullOrSpace = 0;
			for(std::size_t pos = 0; pos < srcSize; ++pos)
			{
				char c = srcBuffer[pos];
				if(c != '\0' && c != ' ')
				{
					lengthWithoutNullOrSpace = pos + 1;
				}
				if(c == '\0')
				{
					c = ' ';
				}
				if(pos < destSize - 1)
				{
					destBuffer[pos] = c;
				}
			}

			std::size_t destLength = std::min(lengthWithoutNullOrSpace, destSize - 1);

			dst += destLength;

		}

		// Fill rest of string with nulls.
		std::fill(dst, destBuffer + destSize, '\0');

	}

	// Used for reading strings from files.
	// Preferrably use this version of the function, it is safer.
	template <ReadWriteMode mode, size_t destSize, size_t srcSize, typename Tbyte>
	void Read(char (&destBuffer)[destSize], const Tbyte (&srcBuffer)[srcSize])
	{
		STATIC_ASSERT(destSize > 0);
		STATIC_ASSERT(srcSize > 0);
		Read<mode, destSize>(destBuffer, srcBuffer, srcSize);
	}


	// Copy a string from srcBuffer to destBuffer using a given write mode.
	// You should only use this function if src and dest are dynamically sized,
	// otherwise use one of the safer overloads below.
	template <ReadWriteMode mode>
	void Write(char *destBuffer, const size_t destSize, const char *srcBuffer, const size_t srcSize)
	{
		MPT_ASSERT(destSize > 0);

		mpt::String::detail::WriteStringBuffer(mode, destBuffer, destSize, srcBuffer, srcSize);

	}

	// Copy a string from srcBuffer to a dynamically sized std::vector destBuffer using a given write mode.
	// Used for writing strings to files.
	// Only use this version of the function if the size of the source buffer is variable and the destination buffer also has variable size.
	template <ReadWriteMode mode>
	void Write(std::vector<char> &destBuffer, const char *srcBuffer, const size_t srcSize)
	{
		MPT_ASSERT(destBuffer.size() > 0);
		Write<mode>(destBuffer.data(), destBuffer.size(), srcBuffer, srcSize);
	}

	// Copy a string from srcBuffer to destBuffer using a given write mode.
	// Used for writing strings to files.
	// Only use this version of the function if the size of the source buffer is variable.
	template <ReadWriteMode mode, size_t destSize>
	void Write(char (&destBuffer)[destSize], const char *srcBuffer, const size_t srcSize)
	{
		STATIC_ASSERT(destSize > 0);
		Write<mode>(destBuffer, destSize, srcBuffer, srcSize);
	}

	// Copy a string from srcBuffer to destBuffer using a given write mode.
	// Used for writing strings to files.
	// Preferrably use this version of the function, it is safer.
	template <ReadWriteMode mode, size_t destSize, size_t srcSize>
	void Write(char (&destBuffer)[destSize], const char (&srcBuffer)[srcSize])
	{
		STATIC_ASSERT(destSize > 0);
		STATIC_ASSERT(srcSize > 0);
		Write<mode, destSize>(destBuffer, srcBuffer, srcSize);
	}

	template <ReadWriteMode mode>
	void Write(char *destBuffer, const size_t destSize, const std::string &src)
	{
		MPT_ASSERT(destSize > 0);
		Write<mode>(destBuffer, destSize, src.c_str(), src.length());
	}

	template <ReadWriteMode mode>
	void Write(std::vector<char> &destBuffer, const std::string &src)
	{
		MPT_ASSERT(destBuffer.size() > 0);
		Write<mode>(destBuffer, src.c_str(), src.length());
	}

	template <ReadWriteMode mode, size_t destSize>
	void Write(char (&destBuffer)[destSize], const std::string &src)
	{
		STATIC_ASSERT(destSize > 0);
		Write<mode, destSize>(destBuffer, src.c_str(), src.length());
	}


	// Copy from a char array to a fixed size char array.
	template <size_t destSize>
	void CopyN(char (&destBuffer)[destSize], const char *srcBuffer, const size_t srcSize = std::numeric_limits<size_t>::max())
	{
		const size_t copySize = std::min(destSize - 1u, srcSize);
		std::strncpy(destBuffer, srcBuffer, copySize);
		destBuffer[copySize] = '\0';
	}

	// Copy at most srcSize characters from srcBuffer to a std::string.
	static inline void CopyN(std::string &dest, const char *srcBuffer, const size_t srcSize = std::numeric_limits<size_t>::max())
	{
		dest.assign(srcBuffer, srcBuffer + mpt::strnlen(srcBuffer, srcSize));
	}


	// Copy from one fixed size char array to another one.
	template <size_t destSize, size_t srcSize>
	void Copy(char (&destBuffer)[destSize], const char (&srcBuffer)[srcSize])
	{
		CopyN(destBuffer, srcBuffer, srcSize);
	}

	// Copy from a std::string to a fixed size char array.
	template <size_t destSize>
	void Copy(char (&destBuffer)[destSize], const std::string &src)
	{
		CopyN(destBuffer, src.c_str(), src.length());
	}

	// Copy from a fixed size char array to a std::string.
	template <size_t srcSize>
	void Copy(std::string &dest, const char (&srcBuffer)[srcSize])
	{
		CopyN(dest, srcBuffer, srcSize);
	}

	// Copy from a std::string to a std::string.
	static inline void Copy(std::string &dest, const std::string &src)
	{
		dest.assign(src);
	}


#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif // MPT_COMPILER_MSVC


} // namespace String


} // namespace mpt



OPENMPT_NAMESPACE_END
