/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_FORMAT_SIMPLE_INTEGER_HPP
#define MPT_FORMAT_SIMPLE_INTEGER_HPP


#include "mpt/base/algorithm.hpp"
#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/format/helpers.hpp"
#include "mpt/format/simple_spec.hpp"
#include "mpt/string/types.hpp"

#include <charconv>
#include <string>
#include <system_error>
#include <type_traits>

#include <cstddef>



namespace mpt {
inline namespace MPT_INLINE_NS {


template <typename Tstring, typename T, std::enable_if_t<std::is_integral<T>::value, bool> = true>
inline Tstring format_simple_integer_to_chars(const T & x, int base) {
	std::string str(1, '\0');
	bool done = false;
	while (!done) {
		if constexpr (std::is_same<T, bool>::value) {
			std::to_chars_result result = std::to_chars(str.data(), str.data() + str.size(), static_cast<int>(x), base);
			if (result.ec != std::errc{}) {
				str.resize(mpt::exponential_grow(str.size()), '\0');
			} else {
				str.resize(result.ptr - str.data());
				done = true;
			}
		} else {
			std::to_chars_result result = std::to_chars(str.data(), str.data() + str.size(), x, base);
			if (result.ec != std::errc{}) {
				str.resize(mpt::exponential_grow(str.size()), '\0');
			} else {
				str.resize(result.ptr - str.data());
				done = true;
			}
		}
	}
	return mpt::convert_formatted_simple<Tstring>(str);
}


template <typename Tstring>
inline Tstring format_simple_integer_postprocess_case(Tstring str, const format_simple_spec & format) {
	format_simple_flags f = format.GetFlags();
	if (f & format_simple_base::CaseUpp) {
		for (auto & c : str) {
			if (mpt::unsafe_char_convert<typename Tstring::value_type>('a') <= c && c <= mpt::unsafe_char_convert<typename Tstring::value_type>('z')) {
				c -= mpt::unsafe_char_convert<typename Tstring::value_type>('a') - mpt::unsafe_char_convert<typename Tstring::value_type>('A');
			}
		}
	}
	return str;
}


template <typename Tstring>
inline Tstring format_simple_integer_postprocess_digits(Tstring str, const format_simple_spec & format) {
	format_simple_flags f = format.GetFlags();
	std::size_t width = format.GetWidth();
	if (f & format_simple_base::FillNul) {
		auto pos = str.begin();
		if (str.length() > 0) {
			if (str[0] == mpt::unsafe_char_convert<typename Tstring::value_type>('+')) {
				pos++;
				width++;
			} else if (str[0] == mpt::unsafe_char_convert<typename Tstring::value_type>('-')) {
				pos++;
				width++;
			}
		}
		if (str.length() < width) {
			str.insert(pos, width - str.length(), mpt::unsafe_char_convert<typename Tstring::value_type>('0'));
		}
	}
	return str;
}


#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4723) // potential divide by 0
#endif                          // MPT_COMPILER_MSVC
template <typename Tstring>
inline Tstring format_simple_integer_postprocess_group(Tstring str, const format_simple_spec & format) {
	if (format.GetGroup() > 0) {
		const unsigned int groupSize = format.GetGroup();
		const char groupSep = format.GetGroupSep();
		std::size_t len = str.length();
		for (std::size_t n = 0; n < len; ++n) {
			if (n > 0 && (n % groupSize) == 0) {
				if (!(n == (len - 1) && (str[0] == mpt::unsafe_char_convert<typename Tstring::value_type>('+') || str[0] == mpt::unsafe_char_convert<typename Tstring::value_type>('-')))) {
					str.insert(str.begin() + (len - n), 1, mpt::unsafe_char_convert<typename Tstring::value_type>(groupSep));
				}
			}
		}
	}
	return str;
}
#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif // MPT_COMPILER_MSVC


template <typename Tstring, typename T, std::enable_if_t<std::is_integral<T>::value, bool> = true>
inline Tstring format_simple(const T & x, const format_simple_spec & format) {
	int base = 10;
	if (format.GetFlags() & format_simple_base::BaseDec) {
		base = 10;
	}
	if (format.GetFlags() & format_simple_base::BaseHex) {
		base = 16;
	}
	using format_string_type = typename mpt::select_format_string_type<Tstring>::type;
	return mpt::transcode<Tstring>(mpt::format_simple_integer_postprocess_group(mpt::format_simple_integer_postprocess_digits(mpt::format_simple_integer_postprocess_case(mpt::format_simple_integer_to_chars<format_string_type>(x, base), format), format), format));
}


} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_FORMAT_SIMPLE_INTEGER_HPP
