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

#define MPT_INLINE_NS mpt_openmpt123

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
#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE
#endif
#endif
#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
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
