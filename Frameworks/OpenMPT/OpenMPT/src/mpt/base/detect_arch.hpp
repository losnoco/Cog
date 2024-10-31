/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_DETECT_ARCH_HPP
#define MPT_BASE_DETECT_ARCH_HPP



#include "mpt/base/detect_compiler.hpp"



// The order of the checks matters!



#if MPT_COMPILER_GENERIC



#define MPT_ARCH_GENERIC 1



#elif MPT_COMPILER_MSVC



#if defined(_M_ARM64) || defined(_M_ARM64EC)
#define MPT_ARCH_AARCH64 1

#elif defined(_M_ARM)
#define MPT_ARCH_ARM 1

#elif defined(_M_AMD64) || defined(_M_X64)
#define MPT_ARCH_AMD64 1

#elif defined(_M_IX86)
#define MPT_ARCH_X86 1

#endif



#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG



#if defined(__wasm64__) || defined(__wasm64)
#define MPT_ARCH_WASM64 1

#elif defined(__wasm32__) || defined(__wasm32)
#define MPT_ARCH_WASM32 1

#elif defined(__aarch64__)
#define MPT_ARCH_AARCH64 1

#elif defined(__arm__)
#define MPT_ARCH_ARM 1

#elif defined(__amd64__) || defined(__x86_64__)
#define MPT_ARCH_AMD64 1

#elif defined(__i386__) || defined(_X86_)
#define MPT_ARCH_X86 1

#endif



#else // MPT_COMPILER



#if defined(__wasm64__) || defined(__wasm64)
#define MPT_ARCH_WASM64 1

#elif defined(__wasm32__) || defined(__wasm32)
#define MPT_ARCH_WASM32 1

#elif defined(__aarch64__) || defined(_M_ARM64)
#define MPT_ARCH_AARCH64 1

#elif defined(__arm__) || defined(_M_ARM)
#define MPT_ARCH_ARM 1

#elif defined(__amd64__) || defined(__x86_64__) || defined(_M_AMD64) || defined(_M_X64) || defined(__amd64) || defined(__x86_64)
#define MPT_ARCH_AMD64 1

#elif defined(__i386__) || defined(_X86_) || defined(_M_IX86) || defined(__i386) || defined(__X86__)
#define MPT_ARCH_X86 1

#endif



#endif // MPT_COMPILER



#ifndef MPT_ARCH_GENERIC
#define MPT_ARCH_GENERIC 0
#endif
#ifndef MPT_ARCH_WASM64
#define MPT_ARCH_WASM64 0
#endif
#ifndef MPT_ARCH_WASM32
#define MPT_ARCH_WASM32 0
#endif
#ifndef MPT_ARCH_AARCH64
#define MPT_ARCH_AARCH64 0
#endif
#ifndef MPT_ARCH_ARM
#define MPT_ARCH_ARM 0
#endif
#ifndef MPT_ARCH_AMD64
#define MPT_ARCH_AMD64 0
#endif
#ifndef MPT_ARCH_X86
#define MPT_ARCH_X86 0
#endif



#if !MPT_COMPILER_GENERIC

#if MPT_COMPILER_MSVC
#define MPT_ARCH_LITTLE_ENDIAN
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define MPT_ARCH_BIG_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define MPT_ARCH_LITTLE_ENDIAN
#endif
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && defined(__ORDER_LITTLE_ENDIAN__)
#if __ORDER_BIG_ENDIAN__ != __ORDER_LITTLE_ENDIAN__
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define MPT_ARCH_BIG_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define MPT_ARCH_LITTLE_ENDIAN
#endif
#endif
#endif

// fallback:
#if !defined(MPT_ARCH_BIG_ENDIAN) && !defined(MPT_ARCH_LITTLE_ENDIAN)
#if (defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)) \
	|| (defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)) \
	|| (defined(_STLP_BIG_ENDIAN) && !defined(_STLP_LITTLE_ENDIAN))
#define MPT_ARCH_BIG_ENDIAN
#elif (defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)) \
	|| (defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)) \
	|| (defined(_STLP_LITTLE_ENDIAN) && !defined(_STLP_BIG_ENDIAN))
#define MPT_ARCH_LITTLE_ENDIAN
#elif defined(__hpux) || defined(__hppa) \
	|| defined(_MIPSEB) \
	|| defined(__s390__)
#define MPT_ARCH_BIG_ENDIAN
#elif defined(__i386__) || defined(_M_IX86) \
	|| defined(__amd64) || defined(__amd64__) || defined(_M_AMD64) || defined(__x86_64) || defined(__x86_64__) || defined(_M_X64) \
	|| defined(__bfin__)
#define MPT_ARCH_LITTLE_ENDIAN
#endif
#endif

#endif // !MPT_COMPILER_GENERIC



#endif // MPT_BASE_DETECT_ARCH_HPP
