/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_STRING_TYPES_HPP
#define MPT_STRING_TYPES_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/detect/mfc.hpp"

#include <string>
#include <type_traits>

#include <cstddef>

#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS



namespace mpt {
inline namespace MPT_INLINE_NS {



enum class common_encoding
{
	utf8,
	ascii, // strictly 7-bit ASCII
	iso8859_1,
	iso8859_15,
	cp850,
	cp437,
	windows1252,
};


enum class logical_encoding
{
	locale,        // CP_ACP on windows, system configured C locale otherwise
	active_locale, // active C/C++ global locale
};

// source code / preprocessor (i.e. # token)
inline constexpr auto source_encoding = common_encoding::ascii;

// debug log files
inline constexpr auto logfile_encoding = common_encoding::utf8;

// std::clog / std::cout / std::cerr
inline constexpr auto stdio_encoding = logical_encoding::locale;

// getenv
inline constexpr auto environment_encoding = logical_encoding::locale;

// std::exception::what()
inline constexpr auto exception_encoding = logical_encoding::active_locale;





template <typename T>
struct is_character : public std::false_type { };

template <>
struct is_character<char> : public std::true_type { };
#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
template <>
struct is_character<wchar_t> : public std::true_type { };
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR
#if MPT_CXX_AT_LEAST(20)
template <>
struct is_character<char8_t> : public std::true_type { };
#endif // C++20
template <>
struct is_character<char16_t> : public std::true_type { };
template <>
struct is_character<char32_t> : public std::true_type { };





template <typename T, typename std::enable_if<mpt::is_character<T>::value, bool>::type = true>
MPT_CONSTEXPRINLINE typename std::make_unsigned<T>::type char_value(T x) noexcept {
	return static_cast<typename std::make_unsigned<T>::type>(x);
}





template <typename T>
struct unsafe_char_converter { };

template <>
struct unsafe_char_converter<char> {
	static constexpr char32_t decode(char c) noexcept {
		return static_cast<char32_t>(static_cast<uint32>(static_cast<unsigned char>(c)));
	}
	static constexpr char encode(char32_t c) noexcept {
		return static_cast<char>(static_cast<unsigned char>(static_cast<uint32>(c)));
	}
};

#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
template <>
struct unsafe_char_converter<wchar_t> {
	static constexpr char32_t decode(wchar_t c) noexcept {
		return static_cast<char32_t>(static_cast<uint32>(static_cast<std::make_unsigned<wchar_t>::type>(c)));
	}
	static constexpr wchar_t encode(char32_t c) noexcept {
		return static_cast<wchar_t>(static_cast<std::make_unsigned<wchar_t>::type>(static_cast<uint32>(c)));
	}
};
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR

#if MPT_CXX_AT_LEAST(20)
template <>
struct unsafe_char_converter<char8_t> {
	static constexpr char32_t decode(char8_t c) noexcept {
		return static_cast<char32_t>(static_cast<uint32>(static_cast<uint8>(c)));
	}
	static constexpr char8_t encode(char32_t c) noexcept {
		return static_cast<char8_t>(static_cast<uint8>(static_cast<uint32>(c)));
	}
};
#endif // C++20

template <>
struct unsafe_char_converter<char16_t> {
	static constexpr char32_t decode(char16_t c) noexcept {
		return static_cast<char32_t>(static_cast<uint32>(static_cast<uint16>(c)));
	}
	static constexpr char16_t encode(char32_t c) noexcept {
		return static_cast<char16_t>(static_cast<uint16>(static_cast<uint32>(c)));
	}
};

template <>
struct unsafe_char_converter<char32_t> {
	static constexpr char32_t decode(char32_t c) noexcept {
		return c;
	}
	static constexpr char32_t encode(char32_t c) noexcept {
		return c;
	}
};

template <typename Tdstchar, typename Tsrcchar>
constexpr Tdstchar unsafe_char_convert(Tsrcchar src) noexcept {
	return mpt::unsafe_char_converter<Tdstchar>::encode(mpt::unsafe_char_converter<Tsrcchar>::decode(src));
}





#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
using widestring = std::wstring;
using widechar = wchar_t;
#define MPT_WIDECHAR(x)    L##x
#define MPT_WIDELITERAL(x) L##x
#define MPT_WIDESTRING(x)  std::wstring(L##x)
#else // MPT_COMPILER_QUIRK_NO_WCHAR
using widestring = std::u32string;
using widechar = char32_t;
#define MPT_WIDECHAR(x)    U##x
#define MPT_WIDELITERAL(x) U##x
#define MPT_WIDESTRING(x)  std::u32string(U##x)
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR



template <common_encoding common_encoding_tag>
struct common_encoding_char_traits : std::char_traits<char> {
	static constexpr auto encoding() noexcept {
		return common_encoding_tag;
	}
};

template <logical_encoding logical_encoding_tag>
struct logical_encoding_char_traits : std::char_traits<char> {
	static constexpr auto encoding() noexcept {
		return logical_encoding_tag;
	}
};



using lstring = std::basic_string<char, mpt::logical_encoding_char_traits<logical_encoding::locale>>;

using source_string = std::basic_string<char, mpt::common_encoding_char_traits<source_encoding>>;
using exception_string = std::basic_string<char, mpt::logical_encoding_char_traits<exception_encoding>>;

#if MPT_OS_WINDOWS

template <typename Tchar>
struct windows_char_traits { };
template <>
struct windows_char_traits<CHAR> { using string_type = mpt::lstring; };
template <>
struct windows_char_traits<WCHAR> { using string_type = std::wstring; };

using tstring = windows_char_traits<TCHAR>::string_type;

using winstring = mpt::tstring;

#endif // MPT_OS_WINDOWS



#if MPT_CXX_AT_LEAST(20)

using u8string = std::u8string;
using u8char = char8_t;
#define MPT_U8CHAR(x)    u8##x
#define MPT_U8LITERAL(x) u8##x
#define MPT_U8STRING(x)  std::u8string(u8##x)

#else // !C++20

using u8string = std::basic_string<char, mpt::common_encoding_char_traits<common_encoding::utf8>>;
using u8char = char;
#define MPT_U8CHAR(x)    x
#define MPT_U8LITERAL(x) x
#define MPT_U8STRING(x)  mpt::u8string(x)

// mpt::u8string is a moderately type-safe string that is meant to contain
// UTF-8 encoded char bytes.
//
// mpt::u8string is not implicitely convertible to/from std::string, but
// it is convertible to/from C strings the same way as std::string is.
//
// The implementation of mpt::u8string is a compromise of compatibilty
// with implementation-defined STL details, efficiency, source code size,
// executable bloat, type-safety  and simplicity.
//
// mpt::u8string is not meant to be used directly though.
// mpt::u8string is meant as an alternative implementaion to std::wstring
// for implementing the unicode string type mpt::ustring.

#endif // C++20



#if !defined(MPT_USTRING_MODE_UTF8_FORCE) && (MPT_COMPILER_MSVC || (MPT_DETECTED_MFC && defined(UNICODE)))
// Use wide strings for MSVC because this is the native encoding on
// microsoft platforms.
#define MPT_USTRING_MODE_WIDE 1
#define MPT_USTRING_MODE_UTF8 0
#else
#define MPT_USTRING_MODE_WIDE 0
#define MPT_USTRING_MODE_UTF8 1
#endif

// mpt::ustring
//
// mpt::ustring is a string type that can hold unicode strings.
// It is implemented as a std::basic_string either based on wchar_t (i.e. the
//  same as std::wstring) or a custom-defined char_traits class that is derived
//  from std::char_traits<char>.
// The selection of the underlying implementation is done at compile-time.
// MPT_UCHAR, MPT_ULITERAL and MPT_USTRING are macros that ease construction
//  of ustring char literals, ustring char array literals and ustring objects
//  from ustring char literals that work consistently in both modes.
//  Note that these are not supported for non-ASCII characters appearing in
//  the macro argument.
// Also note that, as both UTF8 and UTF16 (it is less of an issue for UTF32)
//  are variable-length encodings and mpt::ustring is implemented as a
//  std::basic_string, all member functions that require individual character
//  access will not work consistently or even at all in a meaningful way.
//  This in particular affects operator[], find() and substr().
//  The code makes no effort in preventing these or generating warnings when
//  these are used on mpt::ustring objects. However, compiling in the
//  respectively other mpt::ustring mode will catch most of these anyway.

#if MPT_USTRING_MODE_WIDE
#if MPT_USTRING_MODE_UTF8
#error "MPT_USTRING_MODE_WIDE and MPT_USTRING_MODE_UTF8 are mutually exclusive."
#endif

using ustring = std::wstring;
using uchar = wchar_t;
#define MPT_UCHAR(x)    L##x
#define MPT_ULITERAL(x) L##x
#define MPT_USTRING(x)  std::wstring(L##x)

#endif // MPT_USTRING_MODE_WIDE

#if MPT_USTRING_MODE_UTF8
#if MPT_USTRING_MODE_WIDE
#error "MPT_USTRING_MODE_WIDE and MPT_USTRING_MODE_UTF8 are mutually exclusive."
#endif

using ustring = mpt::u8string;
using uchar = mpt::u8char;
#define MPT_UCHAR(x)    MPT_U8CHAR(x)
#define MPT_ULITERAL(x) MPT_U8LITERAL(x)
#define MPT_USTRING(x)  MPT_U8STRING(x)

#endif // MPT_USTRING_MODE_UTF8



template <typename T>
struct make_string_type { };

template <typename T, typename Ttraits>
struct make_string_type<std::basic_string<T, Ttraits>> {
	using type = std::basic_string<T, Ttraits>;
};

template <typename T>
struct make_string_type<const T *> {
	using type = std::basic_string<T>;
};

template <typename T>
struct make_string_type<T *> {
	using type = std::basic_string<T>;
};

template <typename T, std::size_t N>
struct make_string_type<T[N]> {
	using type = typename make_string_type<T *>::type;
};

#if MPT_DETECTED_MFC

template <>
struct make_string_type<CStringW> {
	using type = CStringW;
};

template <>
struct make_string_type<CStringA> {
	using type = CStringA;
};

#endif // MPT_DETECTED_MFC



template <typename T>
struct is_string_type : public std::false_type { };
template <>
struct is_string_type<std::string> : public std::true_type { };
#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
template <>
struct is_string_type<std::wstring> : public std::true_type { };
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR
#if MPT_CXX_AT_LEAST(20)
template <>
struct is_string_type<std::u8string> : public std::true_type { };
#endif // C++20
template <>
struct is_string_type<std::u16string> : public std::true_type { };
template <>
struct is_string_type<std::u32string> : public std::true_type { };
#if MPT_DETECTED_MFC
template <>
struct is_string_type<CStringW> : public std::true_type { };
template <>
struct is_string_type<CStringA> : public std::true_type { };
#endif // MPT_DETECTED_MFC
template <typename T, typename Ttraits>
struct is_string_type<std::basic_string<T, Ttraits>> : public std::true_type { };



template <typename T>
inline typename mpt::make_string_type<T>::type as_string(const T & str) {
	if constexpr (std::is_pointer<typename std::remove_cv<T>::type>::value) {
		return str ? typename mpt::make_string_type<T>::type{str} : typename mpt::make_string_type<T>::type{};
	} else {
		return str;
	}
}



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_STRING_TYPES_HPP
