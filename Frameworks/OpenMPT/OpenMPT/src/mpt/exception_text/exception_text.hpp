/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_EXCEPTION_TEXT_EXCEPTION_TEXT_HPP
#define MPT_EXCEPTION_TEXT_EXCEPTION_TEXT_HPP



#include "mpt/base/namespace.hpp"
#include "mpt/string/types.hpp"
#include "mpt/string_transcode/transcode.hpp"

#include <exception>

#include <cstring>



namespace mpt {
inline namespace MPT_INLINE_NS {



template <typename Tstring>
inline Tstring get_exception_text(const std::exception & e) {
	if (e.what() && (std::strlen(e.what()) > 0)) {
		return mpt::transcode<Tstring>(mpt::exception_string{e.what()});
	} else if (typeid(e).name() && (std::strlen(typeid(e).name()) > 0)) {
		return mpt::transcode<Tstring>(mpt::source_string{typeid(e).name()});
	} else {
		return mpt::transcode<Tstring>(mpt::source_string{"unknown exception name"});
	}
}

template <>
inline std::string get_exception_text<std::string>(const std::exception & e) {
	if (e.what() && (std::strlen(e.what()) > 0)) {
		return std::string{e.what()};
	} else if (typeid(e).name() && (std::strlen(typeid(e).name()) > 0)) {
		return std::string{typeid(e).name()};
	} else {
		return std::string{"unknown exception name"};
	}
}


template <typename Tstring>
inline Tstring get_current_exception_text() {
	try {
		throw;
	} catch (const std::exception & e) {
		return mpt::get_exception_text<Tstring>(e);
	} catch (...) {
		return mpt::transcode<Tstring>(mpt::source_string{"unknown exception"});
	}
}

template <>
inline std::string get_current_exception_text<std::string>() {
	try {
		throw;
	} catch (const std::exception & e) {
		return mpt::get_exception_text<std::string>(e);
	} catch (...) {
		return std::string{"unknown exception"};
	}
}


} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_EXCEPTION_TEXT_EXCEPTION_TEXT_HPP
