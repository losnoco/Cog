/*
 * openmpt123_config.hpp
 * ---------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_CONFIG_HPP
#define OPENMPT123_CONFIG_HPP

#define MPT_PP_DEFER(m, ...) m(__VA_ARGS__)
#define MPT_PP_STRINGIFY(x) #x
#define MPT_PP_JOIN_HELPER(a, b) a ## b
#define MPT_PP_JOIN(a, b) MPT_PP_JOIN_HELPER(a, b)
#define MPT_PP_UNIQUE_IDENTIFIER(prefix) MPT_PP_JOIN(prefix , __LINE__)

#if defined(__clang__)
#define MPT_WARNING(text)           _Pragma(MPT_PP_STRINGIFY(GCC warning text))
#define MPT_WARNING_STATEMENT(text) _Pragma(MPT_PP_STRINGIFY(GCC warning text))
#elif defined(_MSC_VER)
#define MPT_WARNING(text)           __pragma(message(__FILE__ "(" MPT_PP_DEFER(MPT_PP_STRINGIFY, __LINE__) "): Warning: " text))
#define MPT_WARNING_STATEMENT(text) __pragma(message(__FILE__ "(" MPT_PP_DEFER(MPT_PP_STRINGIFY, __LINE__) "): Warning: " text))
#elif defined(__GNUC__)
#define MPT_WARNING(text)           _Pragma(MPT_PP_STRINGIFY(GCC warning text))
#define MPT_WARNING_STATEMENT(text) _Pragma(MPT_PP_STRINGIFY(GCC warning text))
#else
#define MPT_WARNING(text) \
	static inline int MPT_PP_UNIQUE_IDENTIFIER(MPT_WARNING_NAME) () noexcept { \
		int warning [[deprecated("Warning: " text)]] = 0; \
		return warning; \
	} \
/**/
#define MPT_WARNING_STATEMENT(text) \
	int MPT_PP_UNIQUE_IDENTIFIER(MPT_WARNING_NAME) = [](){ \
		int warning [[deprecated("Warning: " text)]] = 0; \
		return warning; \
	}() \
/**/
#endif

#if defined(HAVE_CONFIG_H)
// wrapper for autoconf macros
#include "config.h"
#endif // HAVE_CONFIG_H

#if defined(_WIN32)
#ifndef WIN32
#define WIN32
#endif
#endif // _WIN32

#if defined(WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#endif // WIN32

#if defined(WIN32)
#define MPT_WITH_MMIO
#endif // WIN32

#if defined(MPT_BUILD_MSVC)

#define MPT_WITH_FLAC
#define MPT_WITH_PORTAUDIO

#if defined(MPT_BUILD_MSVC_STATIC)
#define FLAC__NO_DLL
#endif

#endif // MPT_BUILD_MSVC

#define OPENMPT123_VERSION_STRING OPENMPT_API_VERSION_STRING

#endif // OPENMPT123_CONFIG_HPP
