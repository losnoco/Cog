/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_STRING_UTILITY_HPP
#define MPT_STRING_UTILITY_HPP



#include "mpt/base/namespace.hpp"
#include "mpt/detect/mfc.hpp"

#include <string>
#include <vector>

#include <cstddef>

#if MPT_DETECTED_MFC
// cppcheck-suppress missingInclude
#include <afx.h>
#endif // MPT_DETECTED_MFC


namespace mpt {
inline namespace MPT_INLINE_NS {



// string_traits abstract the API of underlying string classes, in particular they allow adopting to CString without having to specialize for CString explicitly

template <typename Tstring>
struct string_traits {

	using string_type = Tstring;
	using size_type = typename string_type::size_type;
	using char_type = typename string_type::value_type;

	static inline std::size_t length(const string_type & str) {
		return str.length();
	}

	static inline void reserve(string_type & str, std::size_t size) {
		str.reserve(size);
	}

	static inline string_type & append(string_type & str, const string_type & a) {
		return str.append(a);
	}
	static inline string_type & append(string_type & str, string_type && a) {
		return str.append(std::move(a));
	}
	static inline string_type & append(string_type & str, std::size_t count, char_type c) {
		return str.append(count, c);
	}

	static inline string_type pad(string_type str, std::size_t left, std::size_t right) {
		str.insert(str.begin(), left, char_type(' '));
		str.insert(str.end(), right, char_type(' '));
		return str;
	}
};

#if MPT_DETECTED_MFC

template <>
struct string_traits<CStringA> {

	using string_type = CStringA;
	using size_type = int;
	using char_type = typename CStringA::XCHAR;

	static inline size_type length(const string_type & str) {
		return str.GetLength();
	}

	static inline void reserve(string_type & str, size_type size) {
		str.Preallocate(size);
	}

	static inline string_type & append(string_type & str, const string_type & a) {
		str += a;
		return str;
	}
	static inline string_type & append(string_type & str, size_type count, char_type c) {
		while (count--) {
			str.AppendChar(c);
		}
		return str;
	}

	static inline string_type pad(const string_type & str, size_type left, size_type right) {
		string_type tmp;
		while (left--) {
			tmp.AppendChar(char_type(' '));
		}
		tmp += str;
		while (right--) {
			tmp.AppendChar(char_type(' '));
		}
		return tmp;
	}
};

template <>
struct string_traits<CStringW> {

	using string_type = CStringW;
	using size_type = int;
	using char_type = typename CStringW::XCHAR;

	static inline size_type length(const string_type & str) {
		return str.GetLength();
	}

	static inline void reserve(string_type & str, size_type size) {
		str.Preallocate(size);
	}

	static inline string_type & append(string_type & str, const string_type & a) {
		str += a;
		return str;
	}
	static inline string_type & append(string_type & str, size_type count, char_type c) {
		while (count--) {
			str.AppendChar(c);
		}
		return str;
	}

	static inline string_type pad(const string_type & str, size_type left, size_type right) {
		string_type tmp;
		while (left--) {
			tmp.AppendChar(char_type(' '));
		}
		tmp += str;
		while (right--) {
			tmp.AppendChar(char_type(' '));
		}
		return tmp;
	}
};

#endif // MPT_DETECTED_MFC


template <typename Tchar>
struct char_constants {
	static inline constexpr Tchar space = ' ';
	static inline constexpr Tchar a = 'a';
	static inline constexpr Tchar z = 'z';
	static inline constexpr Tchar A = 'A';
	static inline constexpr Tchar Z = 'Z';
	static inline constexpr Tchar lf = '\n';
	static inline constexpr Tchar cr = '\r';
	static inline constexpr Tchar tab = '\t';
	static inline constexpr Tchar comma = ',';
};

template <>
struct char_constants<char> {
	static inline constexpr char space = ' ';
	static inline constexpr char a = 'a';
	static inline constexpr char z = 'z';
	static inline constexpr char A = 'A';
	static inline constexpr char Z = 'Z';
	static inline constexpr char lf = '\n';
	static inline constexpr char cr = '\r';
	static inline constexpr char tab = '\t';
	static inline constexpr char comma = ',';
};

#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
template <>
struct char_constants<wchar_t> {
	static inline constexpr wchar_t space = L' ';
	static inline constexpr wchar_t a = L'a';
	static inline constexpr wchar_t z = L'z';
	static inline constexpr wchar_t A = L'A';
	static inline constexpr wchar_t Z = L'Z';
	static inline constexpr wchar_t lf = L'\n';
	static inline constexpr wchar_t cr = L'\r';
	static inline constexpr wchar_t tab = L'\t';
	static inline constexpr wchar_t comma = L',';
};
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR

#if MPT_CXX_AT_LEAST(20)
template <>
struct char_constants<char8_t> {
	static inline constexpr char8_t space = u8' ';
	static inline constexpr char8_t a = u8'a';
	static inline constexpr char8_t z = u8'z';
	static inline constexpr char8_t A = u8'A';
	static inline constexpr char8_t Z = u8'Z';
	static inline constexpr char8_t lf = u8'\n';
	static inline constexpr char8_t cr = u8'\r';
	static inline constexpr char8_t tab = u8'\t';
	static inline constexpr char8_t comma = u8',';
};
#endif

template <>
struct char_constants<char16_t> {
	static inline constexpr char16_t space = u' ';
	static inline constexpr char16_t a = u'a';
	static inline constexpr char16_t z = u'z';
	static inline constexpr char16_t A = u'A';
	static inline constexpr char16_t Z = u'Z';
	static inline constexpr char16_t lf = u'\n';
	static inline constexpr char16_t cr = u'\r';
	static inline constexpr char16_t tab = u'\t';
	static inline constexpr char16_t comma = u',';
};

template <>
struct char_constants<char32_t> {
	static inline constexpr char32_t space = U' ';
	static inline constexpr char32_t a = U'a';
	static inline constexpr char32_t z = U'z';
	static inline constexpr char32_t A = U'A';
	static inline constexpr char32_t Z = U'Z';
	static inline constexpr char32_t lf = U'\n';
	static inline constexpr char32_t cr = U'\r';
	static inline constexpr char32_t tab = U'\t';
	static inline constexpr char32_t comma = U',';
};


template <typename Tchar>
constexpr bool is_any_line_ending(Tchar c) noexcept {
	return (c == char_constants<Tchar>::cr) || (c == char_constants<Tchar>::lf);
}


template <typename Tstring>
inline Tstring default_whitespace() {
	Tstring result;
	result.reserve(4);
	result.push_back(char_constants<typename Tstring::value_type>::space);
	result.push_back(char_constants<typename Tstring::value_type>::lf);
	result.push_back(char_constants<typename Tstring::value_type>::cr);
	result.push_back(char_constants<typename Tstring::value_type>::tab);
	return result;
}


// Remove whitespace at start of string
template <typename Tstring>
inline Tstring trim_left(Tstring str, const Tstring & whitespace = default_whitespace<Tstring>()) {
	typename Tstring::size_type pos = str.find_first_not_of(whitespace);
	if (pos != Tstring::npos) {
		str.erase(str.begin(), str.begin() + pos);
	} else if (pos == Tstring::npos && str.length() > 0 && str.find_last_of(whitespace) == str.length() - 1) {
		return Tstring();
	}
	return str;
}

// Remove whitespace at end of string
template <typename Tstring>
inline Tstring trim_right(Tstring str, const Tstring & whitespace = default_whitespace<Tstring>()) {
	typename Tstring::size_type pos = str.find_last_not_of(whitespace);
	if (pos != Tstring::npos) {
		str.erase(str.begin() + pos + 1, str.end());
	} else if (pos == Tstring::npos && str.length() > 0 && str.find_first_of(whitespace) == 0) {
		return Tstring();
	}
	return str;
}

// Remove whitespace at start and end of string
template <typename Tstring>
inline Tstring trim(Tstring str, const Tstring & whitespace = default_whitespace<Tstring>()) {
	return trim_right(trim_left(str, whitespace), whitespace);
}


template <typename Tstring, typename Tmatch>
inline bool starts_with(const Tstring & str, const Tmatch & match) {
	return (str.find(typename mpt::make_string_type<Tmatch>::type{match}) == 0);
}

template <typename Tstring, typename Tmatch>
inline bool ends_with(const Tstring & str, const Tmatch & match) {
	return (str.rfind(typename mpt::make_string_type<Tmatch>::type{match}) == (str.length() - typename mpt::make_string_type<Tmatch>::type{match}.length()));
}


template <typename Tstring, typename Treplace>
inline Tstring replace(Tstring str, const Treplace & old_str, const Treplace & new_str) {
	std::size_t pos = 0;
	while ((pos = str.find(typename mpt::make_string_type<Treplace>::type{old_str}, pos)) != Tstring::npos) {
		str.replace(pos, typename mpt::make_string_type<Treplace>::type{old_str}.length(), typename mpt::make_string_type<Treplace>::type{new_str});
		pos += typename mpt::make_string_type<Treplace>::type{new_str}.length();
	}
	return str;
}


template <typename Tstring>
inline Tstring truncate(Tstring str, std::size_t max_len) {
	if (str.length() > max_len) {
		str.resize(max_len);
	}
	return str;
}


template <typename Tchar>
inline constexpr Tchar to_lower_ascii(Tchar c) noexcept {
	if (char_constants<Tchar>::A <= c && c <= char_constants<Tchar>::Z) {
		c += char_constants<Tchar>::a - char_constants<Tchar>::A;
	}
	return c;
}

template <typename Tchar>
inline constexpr Tchar to_upper_ascii(Tchar c) noexcept {
	if (char_constants<Tchar>::a <= c && c <= char_constants<Tchar>::z) {
		c -= char_constants<Tchar>::a - char_constants<Tchar>::A;
	}
	return c;
}


template <typename Tstring>
inline std::vector<Tstring> split(const Tstring & str, const Tstring & sep = Tstring(1, char_constants<typename Tstring::value_type>::comma)) {
	std::vector<Tstring> vals;
	std::size_t pos = 0;
	while (str.find(sep, pos) != std::string::npos) {
		vals.push_back(str.substr(pos, str.find(sep, pos) - pos));
		pos = str.find(sep, pos) + sep.length();
	}
	if (!vals.empty() || (str.substr(pos).length() > 0)) {
		vals.push_back(str.substr(pos));
	}
	return vals;
}


template <typename Tstring>
inline Tstring join(const std::vector<Tstring> & vals, const Tstring & sep = Tstring(1, char_constants<typename Tstring::value_type>::comma)) {
	Tstring str;
	for (std::size_t i = 0; i < vals.size(); ++i) {
		if (i > 0) {
			str += sep;
		}
		str += vals[i];
	}
	return str;
}



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_STRING_UTILITY_HPP
