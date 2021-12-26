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

#include "mptMemory.h"
#include "mptString.h"

#include <algorithm>
#include <string>
#include <vector>



OPENMPT_NAMESPACE_BEGIN



namespace mpt
{


namespace String
{


	enum ReadWriteMode : uint8
	{
		// Reading / Writing: Standard null-terminated string handling.
		nullTerminated = 1,
		// Reading: Source string is not guaranteed to be null-terminated (if it fills the whole char array).
		// Writing: Destination string is not guaranteed to be null-terminated (if it fills the whole char array).
		maybeNullTerminated = 2,
		// Reading: String may contain null characters anywhere. They should be treated as spaces.
		// Writing: A space-padded string is written.
		spacePadded = 3,
		// Reading: String may contain null characters anywhere. The last character is ignored (it is supposed to be 0).
		// Writing: A space-padded string with a trailing null is written.
		spacePaddedNull = 4,
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
		static_assert(sizeof(Tchar) == sizeof(typename Tstring::value_type));
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
	bool empty() const
	{
		return buf[0] == Tchar('\0');
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
		static_assert(sizeof(Tchar) == sizeof(typename Tstring::value_type));
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
	bool empty() const
	{
		return buf[0] == Tchar('\0');
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

template <std::size_t len, mpt::String::ReadWriteMode mode = static_cast<mpt::String::ReadWriteMode>(0)> struct charbuf;

template <std::size_t len>
struct charbuf<len, static_cast<mpt::String::ReadWriteMode>(0)>
{
public:
	typedef char Tchar;
	using char_type = Tchar;
	using string_type = std::basic_string<Tchar>;
	constexpr std::size_t static_length() const { return len; }
public:
	Tchar buf[len];
public:
	charbuf()
	{
		Clear(buf);
	}
	charbuf(const charbuf &) = default;
	charbuf(charbuf &&) = default;
	charbuf & operator = (const charbuf &) = default;
	charbuf & operator = (charbuf &&) = default;
	const Tchar & operator[](std::size_t i) const { return buf[i]; }
	std::string str() const { return static_cast<std::string>(*this); }
	operator string_type () const
	{
		return mpt::String::ReadAutoBuf(buf);
	}
	bool empty() const
	{
		return mpt::String::ReadAutoBuf(buf).empty();
	}
	charbuf & operator = (const string_type & str)
	{
		mpt::String::WriteAutoBuf(buf) = str;
		return *this;
	}
public:
	friend bool operator!=(const std::string & a, const charbuf & b) { return a != b.str(); }
	friend bool operator!=(const charbuf & a, const std::string & b) { return a.str() != b; }
	friend bool operator==(const std::string & a, const charbuf & b) { return a == b.str(); }
	friend bool operator==(const charbuf & a, const std::string & b) { return a.str() == b; }
};

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
		static_assert(sizeof(Tchar) == 1);
	}
	StringModeBufRefImpl(const StringModeBufRefImpl &) = delete;
	StringModeBufRefImpl(StringModeBufRefImpl &&) = default;
	StringModeBufRefImpl & operator = (const StringModeBufRefImpl &) = delete;
	StringModeBufRefImpl & operator = (StringModeBufRefImpl &&) = delete;
	operator std::string () const
	{
		return String::detail::ReadStringBuffer(mode, buf, size);
	}
	bool empty() const
	{
		return String::detail::ReadStringBuffer(mode, buf, size).empty();
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
		static_assert(sizeof(Tchar) == 1);
	}
	StringModeBufRefImpl(const StringModeBufRefImpl &) = delete;
	StringModeBufRefImpl(StringModeBufRefImpl &&) = default;
	StringModeBufRefImpl & operator = (const StringModeBufRefImpl &) = delete;
	StringModeBufRefImpl & operator = (StringModeBufRefImpl &&) = delete;
	operator std::string () const
	{
		return String::detail::ReadStringBuffer(mode, buf, size);
	}
	bool empty() const
	{
		return String::detail::ReadStringBuffer(mode, buf, size).empty();
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

template <std::size_t len, mpt::String::ReadWriteMode mode>
struct charbuf
{
public:
	typedef char Tchar;
	using char_type = Tchar;
	using string_type = std::basic_string<Tchar>;
public:
	Tchar buf[len];
public:
	charbuf() = default;
	charbuf(const charbuf &) = default;
	charbuf(charbuf &&) = default;
	charbuf & operator = (const charbuf &) = default;
	charbuf & operator = (charbuf &&) = default;
	operator string_type () const
	{
		return mpt::String::ReadBuf(mode, buf);
	}
	bool empty() const
	{
		return mpt::String::ReadBuf(mode, buf).empty();
	}
	charbuf & operator = (const string_type & str)
	{
		mpt::String::WriteBuf(mode, buf) = str;
		return *this;
	}
};


// see MPT_BINARY_STRUCT
template <std::size_t len, mpt::String::ReadWriteMode mode>
struct is_binary_safe<typename mpt::charbuf<len, mode>> : public std::true_type { };
template <std::size_t len>
struct is_binary_safe<typename mpt::charbuf<len, static_cast<mpt::String::ReadWriteMode>(0)>> : public std::false_type { };
static_assert(sizeof(mpt::charbuf<7>) == 7);
static_assert(alignof(mpt::charbuf<7>) == 1);
static_assert(std::is_standard_layout<mpt::charbuf<7>>::value);


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

#if defined(MPT_WITH_MFC)

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

#endif // MPT_WITH_MFC

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
		static_assert(size > 0);
		buffer[size - 1] = 0;
	}

	inline void SetNullTerminator(char *buffer, size_t size)
	{
		MPT_ASSERT(size > 0);
		buffer[size - 1] = 0;
	}

#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)

	template <size_t size>
	void SetNullTerminator(wchar_t (&buffer)[size])
	{
		static_assert(size > 0);
		buffer[size - 1] = 0;
	}

	inline void SetNullTerminator(wchar_t *buffer, size_t size)
	{
		MPT_ASSERT(size > 0);
		buffer[size - 1] = 0;
	}

#endif // !MPT_COMPILER_QUIRK_NO_WCHAR


	// Remove any chars after the first null char
	template <size_t size>
	void FixNullString(char (&buffer)[size])
	{
		static_assert(size > 0);
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



#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif // MPT_COMPILER_MSVC


} // namespace String


} // namespace mpt



OPENMPT_NAMESPACE_END
