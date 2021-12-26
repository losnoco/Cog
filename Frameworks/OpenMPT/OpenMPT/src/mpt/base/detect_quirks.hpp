/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_DETECT_QUIRKS_HPP
#define MPT_BASE_DETECT_QUIRKS_HPP



#include "mpt/base/detect_compiler.hpp"
#include "mpt/base/detect_os.hpp"



#if MPT_COMPILER_MSVC
// Compiler has multiplication/division semantics when shifting signed integers.
#define MPT_COMPILER_SHIFT_SIGNED 1
#endif

#ifndef MPT_COMPILER_SHIFT_SIGNED
#define MPT_COMPILER_SHIFT_SIGNED 0
#endif



// This should really be based on __STDCPP_THREADS__, but that is not defined by
// GCC or clang. Stupid.
// Just assume multithreaded and disable for platforms we know are
// singlethreaded later on.
#define MPT_PLATFORM_MULTITHREADED 1

#if MPT_OS_DJGPP
#undef MPT_PLATFORM_MULTITHREADED
#define MPT_PLATFORM_MULTITHREADED 0
#endif

#if (MPT_OS_EMSCRIPTEN && !defined(__EMSCRIPTEN_PTHREADS__))
#undef MPT_PLATFORM_MULTITHREADED
#define MPT_PLATFORM_MULTITHREADED 0
#endif



#if MPT_OS_WINDOWS && MPT_COMPILER_MSVC
#if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0600) // _WIN32_WINNT_VISTA
#define MPT_COMPILER_QUIRK_COMPLEX_STD_MUTEX
#endif
#endif



#if MPT_OS_EMSCRIPTEN && defined(MPT_BUILD_AUDIOWORKLETPROCESSOR)
#define MPT_COMPILER_QUIRK_CHRONO_NO_HIGH_RESOLUTION_CLOCK
#endif



#if MPT_OS_EMSCRIPTEN && defined(MPT_BUILD_AUDIOWORKLETPROCESSOR)
#define MPT_COMPILER_QUIRK_RANDOM_NO_RANDOM_DEVICE
#endif



#if MPT_OS_DJGPP
#define MPT_COMPILER_QUIRK_NO_WCHAR
#endif



#if MPT_OS_WINDOWS && MPT_GCC_BEFORE(9, 1, 0)
// GCC C++ library has no wchar_t overloads
#define MPT_COMPILER_QUIRK_WINDOWS_FSTREAM_NO_WCHAR
#endif



#if defined(__arm__)

#if defined(__SOFTFP__)
#define MPT_COMPILER_QUIRK_FLOAT_EMULATED 1
#else
#define MPT_COMPILER_QUIRK_FLOAT_EMULATED 0
#endif
#if defined(__VFP_FP__)
// native-endian IEEE754
#define MPT_COMPILER_QUIRK_FLOAT_NOTNATIVEENDIAN 0
#define MPT_COMPILER_QUIRK_FLOAT_NOTIEEE754      0
#elif defined(__MAVERICK__)
// little-endian IEEE754, we assume native-endian though
#define MPT_COMPILER_QUIRK_FLOAT_NOTNATIVEENDIAN 1
#define MPT_COMPILER_QUIRK_FLOAT_NOTIEEE754      0
#else
// not IEEE754
#define MPT_COMPILER_QUIRK_FLOAT_NOTNATIVEENDIAN 1
#define MPT_COMPILER_QUIRK_FLOAT_NOTIEEE754      1
#endif

#elif defined(__mips__)

#if defined(__mips_soft_float)
#define MPT_COMPILER_QUIRK_FLOAT_EMULATED 1
#else
#define MPT_COMPILER_QUIRK_FLOAT_EMULATED 0
#endif

#endif

#if MPT_OS_EMSCRIPTEN
#define MPT_COMPILER_QUIRK_FLOAT_PREFER64 1
#endif

#ifndef MPT_COMPILER_QUIRK_FLOAT_PREFER32
#define MPT_COMPILER_QUIRK_FLOAT_PREFER32 0
#endif
#ifndef MPT_COMPILER_QUIRK_FLOAT_PREFER64
#define MPT_COMPILER_QUIRK_FLOAT_PREFER64 0
#endif
#ifndef MPT_COMPILER_QUIRK_FLOAT_EMULATED
#define MPT_COMPILER_QUIRK_FLOAT_EMULATED 0
#endif
#ifndef MPT_COMPILER_QUIRK_FLOAT_NOTNATIVEENDIAN
#define MPT_COMPILER_QUIRK_FLOAT_NOTNATIVEENDIAN 0
#endif
#ifndef MPT_COMPILER_QUIRK_FLOAT_NOTIEEE754
#define MPT_COMPILER_QUIRK_FLOAT_NOTIEEE754 0
#endif



#endif // MPT_BASE_DETECT_QUIRKS_HPP
