/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_DETECT_LIBC_HPP
#define MPT_BASE_DETECT_LIBC_HPP



#include "mpt/base/detect_compiler.hpp"
#include "mpt/base/detect_os.hpp"
#include "mpt/base/detect_quirks.hpp"

#include <cstddef>



// order of checks is important!
#if MPT_COMPILER_GENERIC
#define MPT_LIBC_GENERIC 1
#elif MPT_COMPILER_GCC && (defined(__MINGW32__) || defined(__MINGW64__))
#define MPT_LIBC_MS 1
#elif defined(__GNU_LIBRARY__)
#define MPT_LIBC_GLIBC 1
#elif MPT_COMPILER_MSVC
#define MPT_LIBC_MS 1
#elif MPT_COMPILER_CLANG && MPT_OS_WINDOWS
#define MPT_LIBC_MS 1
#else
#define MPT_LIBC_GENERIC 1
#endif

#ifndef MPT_LIBC_GENERIC
#define MPT_LIBC_GENERIC 0
#endif
#ifndef MPT_LIBC_GLIBC
#define MPT_LIBC_GLIBC 0
#endif
#ifndef MPT_LIBC_MS
#define MPT_LIBC_MS 0
#endif



#endif // MPT_BASE_DETECT_LIBC_HPP
