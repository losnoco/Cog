/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_FORMAT_SIMPLE_FLOATINGPOINT_HPP
#define MPT_FORMAT_SIMPLE_FLOATINGPOINT_HPP



#include "mpt/base/detect.hpp"

#if !defined(MPT_LIBCXX_QUIRK_NO_TO_CHARS_FLOAT)
#define MPT_FORMAT_FORMAT_SIMPLE_FLOAT_CXX17 1
#else
#define MPT_FORMAT_FORMAT_SIMPLE_FLOAT_CXX17 0
#endif

#if MPT_FORMAT_FORMAT_SIMPLE_FLOAT_CXX17
#include "mpt/base/algorithm.hpp"
#endif
#include "mpt/base/namespace.hpp"
#include "mpt/format/default_floatingpoint.hpp"
#include "mpt/format/helpers.hpp"
#include "mpt/format/simple_spec.hpp"
#include "mpt/string/types.hpp"
#include "mpt/string_transcode/transcode.hpp"

#if MPT_FORMAT_FORMAT_SIMPLE_FLOAT_CXX17
#include <charconv>
#endif
#if !MPT_FORMAT_FORMAT_SIMPLE_FLOAT_CXX17
#include <iomanip>
#include <ios>
#endif
#if MPT_FORMAT_FORMAT_SIMPLE_FLOAT_CXX17
#include <iterator>
#endif
#if !MPT_FORMAT_FORMAT_SIMPLE_FLOAT_CXX17
#include <limits>
#include <locale>
#include <sstream>
#endif
#include <string>
#if MPT_FORMAT_FORMAT_SIMPLE_FLOAT_CXX17
#include <system_error>
#endif
#include <type_traits>



namespace mpt {
inline namespace MPT_INLINE_NS {



#if MPT_FORMAT_FORMAT_SIMPLE_FLOAT_CXX17


template <typename Tstring, typename T>
inline Tstring format_simple_floatingpoint_to_chars(const T & x, std::chars_format fmt) {
	std::string str(1, '\0');
	bool done = false;
	while (!done) {
		std::to_chars_result result = std::to_chars(str.data(), str.data() + str.size(), x, fmt);
		if (result.ec != std::errc{}) {
			str.resize(mpt::exponential_grow(str.size()), '\0');
		} else {
			str.resize(result.ptr - str.data());
			done = true;
		}
	}
	return mpt::convert_formatted_simple<Tstring>(str);
}


template <typename Tstring, typename T>
inline Tstring format_simple_floatingpoint_to_chars(const T & x, std::chars_format fmt, int precision) {
	std::string str(1, '\0');
	bool done = false;
	while (!done) {
		std::to_chars_result result = std::to_chars(str.data(), str.data() + str.size(), x, fmt, precision);
		if (result.ec != std::errc{}) {
			str.resize(mpt::exponential_grow(str.size()), '\0');
		} else {
			str.resize(result.ptr - str.data());
			done = true;
		}
	}
	return mpt::convert_formatted_simple<Tstring>(str);
}

template <typename Tstring>
inline Tstring format_simple_floatingpoint_postprocess_width(Tstring str, const format_simple_spec & format) {
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
		if (str.length() - std::distance(str.begin(), pos) < width) {
			str.insert(pos, width - str.length() - std::distance(str.begin(), pos), '0');
		}
	} else {
		if (str.length() < width) {
			str.insert(0, width - str.length(), ' ');
		}
	}
	return str;
}


template <typename Tstring, typename T, std::enable_if_t<std::is_floating_point<T>::value, bool> = true>
inline Tstring format_simple(const T & x, const format_simple_spec & f) {
	using format_string_type = typename mpt::select_format_string_type<Tstring>::type;
	if (f.GetPrecision() != -1) {
		if (f.GetFlags() & format_simple_base::NotaSci) {
			return mpt::transcode<Tstring>(mpt::format_simple_floatingpoint_postprocess_width(format_simple_floatingpoint_to_chars<format_string_type>(x, std::chars_format::scientific, f.GetPrecision()), f));
		} else if (f.GetFlags() & format_simple_base::NotaFix) {
			return mpt::transcode<Tstring>(mpt::format_simple_floatingpoint_postprocess_width(format_simple_floatingpoint_to_chars<format_string_type>(x, std::chars_format::fixed, f.GetPrecision()), f));
		} else {
			return mpt::transcode<Tstring>(mpt::format_simple_floatingpoint_postprocess_width(format_simple_floatingpoint_to_chars<format_string_type>(x, std::chars_format::general, f.GetPrecision()), f));
		}
	} else {
		if (f.GetFlags() & format_simple_base::NotaSci) {
			return mpt::transcode<Tstring>(mpt::format_simple_floatingpoint_postprocess_width(format_simple_floatingpoint_to_chars<format_string_type>(x, std::chars_format::scientific), f));
		} else if (f.GetFlags() & format_simple_base::NotaFix) {
			return mpt::transcode<Tstring>(mpt::format_simple_floatingpoint_postprocess_width(format_simple_floatingpoint_to_chars<format_string_type>(x, std::chars_format::fixed), f));
		} else {
			return mpt::transcode<Tstring>(mpt::format_simple_floatingpoint_postprocess_width(format_simple_floatingpoint_to_chars<format_string_type>(x, std::chars_format::general), f));
		}
	}
}


#else // !MPT_FORMAT_FORMAT_SIMPLE_FLOAT_CXX17


template <typename Tchar>
struct NumPunct : std::numpunct<Tchar> {
private:
	unsigned int group;
	char sep;

public:
	NumPunct(unsigned int g, char s)
		: group(g)
		, sep(s) { }
	std::string do_grouping() const override {
		return std::string(1, static_cast<char>(group));
	}
	Tchar do_thousands_sep() const override {
		return static_cast<Tchar>(sep);
	}
};

template <typename Tostream, typename T>
inline void format_simple_floatingpoint_apply_stream_format(Tostream & o, const format_simple_spec & format, const T &) {
	if (format.GetGroup() > 0)
	{
		o.imbue(std::locale(o.getloc(), new NumPunct<typename Tostream::char_type>(format.GetGroup(), format.GetGroupSep())));
	}
	format_simple_flags f = format.GetFlags();
	std::size_t width = format.GetWidth();
	int precision = format.GetPrecision();
	if (precision != -1 && width != 0 && !(f & format_simple_base::NotaFix) && !(f & format_simple_base::NotaSci))
	{
		// fixup:
		// precision behaves differently from .#
		// avoid default format when precision and width are set
		f &= ~format_simple_base::NotaNrm;
		f |= format_simple_base::NotaFix;
	}
	if (f & format_simple_base::BaseDec) {
		o << std::dec;
	} else if (f & format_simple_base::BaseHex) {
		o << std::hex;
	}
	if (f & format_simple_base::NotaNrm) { /*nothing*/
	} else if (f & format_simple_base::NotaFix) {
		o << std::setiosflags(std::ios::fixed);
	} else if (f & format_simple_base::NotaSci) {
		o << std::setiosflags(std::ios::scientific);
	}
	if (f & format_simple_base::CaseLow) {
		o << std::nouppercase;
	} else if (f & format_simple_base::CaseUpp) {
		o << std::uppercase;
	}
	if (f & format_simple_base::FillOff) { /* nothing */
	} else if (f & format_simple_base::FillNul) {
		o << std::setw(width) << std::setfill(typename Tostream::char_type('0'));
	}
	if (precision != -1)
	{
		o << std::setprecision(precision);
	} else
	{
		if constexpr (std::is_floating_point<T>::value)
		{
			if (f & format_simple_base::NotaNrm)
			{
				o << std::setprecision(std::numeric_limits<T>::max_digits10);
			} else if (f & format_simple_base::NotaFix)
			{
				o << std::setprecision(std::numeric_limits<T>::digits10);
			} else if (f & format_simple_base::NotaSci)
			{
				o << std::setprecision(std::numeric_limits<T>::max_digits10 - 1);
			} else
			{
				o << std::setprecision(std::numeric_limits<T>::max_digits10);
			}
		}
	}
}


template <typename Tstring, typename T>
inline Tstring format_simple_floatingpoint_stream(const T & x, const format_simple_spec & f) {
	using stream_char_type = typename mpt::select_format_char_type<typename Tstring::value_type>::type;
	std::basic_ostringstream<stream_char_type> s;
	s.imbue(std::locale::classic());
	mpt::format_simple_floatingpoint_apply_stream_format(s, f, x);
	s << x;
	return mpt::convert_formatted_simple<Tstring>(s.str());
}


template <typename Tstring, typename T, std::enable_if_t<std::is_floating_point<T>::value, bool> = true>
inline Tstring format_simple(const T & x, const format_simple_spec & format) {
	return mpt::transcode<Tstring>(mpt::format_simple_floatingpoint_stream<typename mpt::select_format_string_type<Tstring>::type>(x, format));
}



#endif // MPT_FORMAT_FORMAT_SIMPLE_FLOAT_CXX17



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_FORMAT_SIMPLE_FLOATINGPOINT_HPP
