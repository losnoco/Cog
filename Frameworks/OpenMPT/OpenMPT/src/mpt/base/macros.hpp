/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_MACROS_HPP
#define MPT_BASE_MACROS_HPP



#include "mpt/base/detect.hpp"

#include <type_traits>

#if MPT_COMPILER_MSVC && MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_COMPILER_MSVC && MPT_OS_WINDOWS



// Advanced inline attributes
#if MPT_COMPILER_MSVC
#define MPT_FORCEINLINE __forceinline
#define MPT_NOINLINE    __declspec(noinline)
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define MPT_FORCEINLINE __attribute__((always_inline)) inline
#define MPT_NOINLINE    __attribute__((noinline))
#else
#define MPT_FORCEINLINE inline
#define MPT_NOINLINE
#endif



// constexpr
#define MPT_CONSTEXPRINLINE constexpr MPT_FORCEINLINE
#if MPT_CXX_AT_LEAST(20)
#define MPT_CONSTEXPR20_FUN constexpr MPT_FORCEINLINE
#define MPT_CONSTEXPR20_VAR constexpr
#else // !C++20
#define MPT_CONSTEXPR20_FUN MPT_FORCEINLINE
#define MPT_CONSTEXPR20_VAR const
#endif // C++20



#define MPT_FORCE_CONSTEXPR(expr) [&]() { \
	constexpr auto x = (expr); \
	return x; \
}()



#if MPT_CXX_AT_LEAST(20)
#define MPT_IS_CONSTANT_EVALUATED20() std::is_constant_evaluated()
#define MPT_IS_CONSTANT_EVALUATED()   std::is_constant_evaluated()
#else // !C++20
#define MPT_IS_CONSTANT_EVALUATED20() false
// this pessimizes the case for C++17 by always assuming constexpr context, which implies always running constexpr-friendly code
#define MPT_IS_CONSTANT_EVALUATED()   true
#endif // C++20



#if MPT_COMPILER_MSVC
#define MPT_MAYBE_CONSTANT_IF(x) \
	__pragma(warning(push)) \
	__pragma(warning(disable : 4127)) \
	if (x) \
		__pragma(warning(pop)) \
/**/
#endif

#if MPT_COMPILER_GCC
#define MPT_MAYBE_CONSTANT_IF(x) \
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Wtype-limits\"") \
	if (x) \
		_Pragma("GCC diagnostic pop") \
/**/
#endif

#if MPT_COMPILER_CLANG
#define MPT_MAYBE_CONSTANT_IF(x) \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wunknown-pragmas\"") \
	_Pragma("clang diagnostic ignored \"-Wtype-limits\"") \
	_Pragma("clang diagnostic ignored \"-Wtautological-constant-out-of-range-compare\"") \
	if (x) \
		_Pragma("clang diagnostic pop") \
/**/
#endif

#if !defined(MPT_MAYBE_CONSTANT_IF)
// MPT_MAYBE_CONSTANT_IF disables compiler warnings for conditions that may in some case be either always false or always true (this may turn out to be useful in ASSERTions in some cases).
#define MPT_MAYBE_CONSTANT_IF(x) if (x)
#endif



#if MPT_COMPILER_MSVC && MPT_OS_WINDOWS
#define MPT_UNUSED(x) UNREFERENCED_PARAMETER(x)
#else
#define MPT_UNUSED(x) static_cast<void>(x)
#endif



#define MPT_DISCARD(expr) static_cast<void>(expr)



// Use MPT_RESTRICT to indicate that a pointer is guaranteed to not be aliased.
#if MPT_COMPILER_MSVC || MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define MPT_RESTRICT __restrict
#else
#define MPT_RESTRICT
#endif



#endif // MPT_BASE_MACROS_HPP
