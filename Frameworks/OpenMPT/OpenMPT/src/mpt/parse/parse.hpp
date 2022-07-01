/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_PARSE_PARSE_HPP
#define MPT_PARSE_PARSE_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/string/types.hpp"
#include "mpt/string_transcode/transcode.hpp"

#include <ios>
#include <locale>
#include <sstream>
#include <string>
#include <type_traits>



namespace mpt {
inline namespace MPT_INLINE_NS {



inline std::string parse_as_internal_string_type(const std::string & s) {
	return s;
}

#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
inline std::wstring parse_as_internal_string_type(const std::wstring & s) {
	return s;
}
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR

#if MPT_USTRING_MODE_WIDE
template <typename Tstring>
inline std::wstring parse_as_internal_string_type(const Tstring & s) {
	return mpt::transcode<std::wstring>(s);
}
#else  // !MPT_USTRING_MODE_WIDE
template <typename Tstring>
inline std::string parse_as_internal_string_type(const Tstring & s) {
	return mpt::transcode<std::string>(mpt::common_encoding::utf8, s);
}
#endif // MPT_USTRING_MODE_WIDE


template <typename T, typename Tstring>
inline T ConvertStringTo(const Tstring & str) {
	std::basic_istringstream<typename decltype(mpt::parse_as_internal_string_type(mpt::as_string(str)))::value_type> stream(mpt::parse_as_internal_string_type(mpt::as_string(str)));
	stream.imbue(std::locale::classic());
	T value;
	if constexpr (std::is_same<T, signed char>::value) {
		signed int tmp;
		if (!(stream >> tmp)) {
			return T{};
		}
		value = static_cast<T>(tmp);
	} else if constexpr (std::is_same<T, unsigned char>::value) {
		unsigned int tmp;
		if (!(stream >> tmp)) {
			return T{};
		}
		value = static_cast<T>(tmp);
	} else {
		if (!(stream >> value)) {
			return T{};
		}
	}
	return value;
}


template <typename T, typename Tstring>
inline T ConvertHexStringTo(const Tstring & str) {
	std::basic_istringstream<typename decltype(mpt::parse_as_internal_string_type(mpt::as_string(str)))::value_type> stream(mpt::parse_as_internal_string_type(mpt::as_string(str)));
	stream.imbue(std::locale::classic());
	T value;
	if constexpr (std::is_same<T, signed char>::value) {
		signed int tmp;
		if (!(stream >> std::hex >> tmp)) {
			return T{};
		}
		value = static_cast<T>(tmp);
	} else if constexpr (std::is_same<T, unsigned char>::value) {
		unsigned int tmp;
		if (!(stream >> std::hex >> tmp)) {
			return T{};
		}
		value = static_cast<T>(tmp);
	} else {
		if (!(stream >> std::hex >> value)) {
			return T{};
		}
	}
	return value;
}



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_PARSE_PARSE_HPP
