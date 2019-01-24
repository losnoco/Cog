/*
 * mptBaseMacros.h
 * ---------------
 * Purpose: Basic assorted compiler-related helpers.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"



#include <iterator>

#include <cstddef>
#include <cstdint>

#include <stddef.h>
#include <stdint.h>



OPENMPT_NAMESPACE_BEGIN



// Compile time assert.
#if (MPT_CXX >= 17)
#define MPT_STATIC_ASSERT static_assert
#else
#define MPT_STATIC_ASSERT(expr) static_assert((expr), "compile time assertion failed: " #expr)
#endif
// legacy
#define STATIC_ASSERT(x) MPT_STATIC_ASSERT(x)



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
#define MPT_CONSTEXPR11_FUN constexpr MPT_FORCEINLINE
#define MPT_CONSTEXPR11_VAR constexpr
#if MPT_CXX_AT_LEAST(14) && !MPT_MSVC_BEFORE(2017,5)
#define MPT_CONSTEXPR14_FUN constexpr MPT_FORCEINLINE
#define MPT_CONSTEXPR14_VAR constexpr
#else
#define MPT_CONSTEXPR14_FUN MPT_FORCEINLINE
#define MPT_CONSTEXPR14_VAR const
#endif
#if MPT_CXX_AT_LEAST(17) && !MPT_MSVC_BEFORE(2017,5)
#define MPT_CONSTEXPR17_FUN constexpr MPT_FORCEINLINE
#define MPT_CONSTEXPR17_VAR constexpr
#else
#define MPT_CONSTEXPR17_FUN MPT_FORCEINLINE
#define MPT_CONSTEXPR17_VAR const
#endif



// C++17 std::size
#if MPT_CXX_AT_LEAST(17)
namespace mpt {
using std::size;
} // namespace mpt
#else
namespace mpt {
template <typename T>
MPT_CONSTEXPR11_FUN auto size(const T & v) -> decltype(v.size())
{
	return v.size();
}
template <typename T, std::size_t N>
MPT_CONSTEXPR11_FUN std::size_t size(const T(&)[N]) noexcept
{
	return N;
}
} // namespace mpt
#endif
// legacy
#if MPT_COMPILER_MSVC
OPENMPT_NAMESPACE_END
#include <cstdlib>
#include <stdlib.h>
OPENMPT_NAMESPACE_BEGIN
#define MPT_ARRAY_COUNT(x) _countof(x)
#else
#define MPT_ARRAY_COUNT(x) (sizeof((x))/sizeof((x)[0]))
#endif
#define CountOf(x) MPT_ARRAY_COUNT(x)



// Use MPT_RESTRICT to indicate that a pointer is guaranteed to not be aliased.
#if MPT_COMPILER_MSVC || MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define MPT_RESTRICT __restrict
#else
#define MPT_RESTRICT
#endif



// Some functions might be deprecated although they are still in use.
// Tag them with "MPT_DEPRECATED".
#if MPT_COMPILER_MSVC
#define MPT_DEPRECATED __declspec(deprecated)
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define MPT_DEPRECATED __attribute__((deprecated))
#else
#define MPT_DEPRECATED
#endif
#if defined(MODPLUG_TRACKER)
#define MPT_DEPRECATED_TRACKER    MPT_DEPRECATED
#define MPT_DEPRECATED_LIBOPENMPT 
#elif defined(LIBOPENMPT_BUILD)
#define MPT_DEPRECATED_TRACKER    
#define MPT_DEPRECATED_LIBOPENMPT MPT_DEPRECATED
#else
#define MPT_DEPRECATED_TRACKER    MPT_DEPRECATED
#define MPT_DEPRECATED_LIBOPENMPT MPT_DEPRECATED
#endif



#if MPT_CXX_AT_LEAST(17)
#define MPT_CONSTANT_IF if constexpr
#endif

#if MPT_COMPILER_MSVC
#if !defined(MPT_CONSTANT_IF)
#define MPT_CONSTANT_IF(x) \
  __pragma(warning(push)) \
  __pragma(warning(disable:4127)) \
  if(x) \
  __pragma(warning(pop)) \
/**/
#endif
#define MPT_MAYBE_CONSTANT_IF(x) \
  __pragma(warning(push)) \
  __pragma(warning(disable:4127)) \
  if(x) \
  __pragma(warning(pop)) \
/**/
#endif

#if MPT_COMPILER_GCC
#define MPT_MAYBE_CONSTANT_IF(x) \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wtype-limits\"") \
  if(x) \
  _Pragma("GCC diagnostic pop") \
/**/
#endif

#if MPT_COMPILER_CLANG
#define MPT_MAYBE_CONSTANT_IF(x) \
  _Pragma("clang diagnostic push") \
  _Pragma("clang diagnostic ignored \"-Wunknown-pragmas\"") \
  _Pragma("clang diagnostic ignored \"-Wtype-limits\"") \
  _Pragma("clang diagnostic ignored \"-Wtautological-constant-out-of-range-compare\"") \
  if(x) \
  _Pragma("clang diagnostic pop") \
/**/
#endif

#if !defined(MPT_CONSTANT_IF)
// MPT_CONSTANT_IF disables compiler warnings for conditions that are either always true or always false for some reason (dependent on template arguments for example)
#define MPT_CONSTANT_IF(x) if(x)
#endif

#if !defined(MPT_MAYBE_CONSTANT_IF)
// MPT_MAYBE_CONSTANT_IF disables compiler warnings for conditions that may in some case be either always false or always true (this may turn out to be useful in ASSERTions in some cases).
#define MPT_MAYBE_CONSTANT_IF(x) if(x)
#endif



#if MPT_COMPILER_MSVC
// MSVC warns for the well-known and widespread "do { } while(0)" idiom with warning level 4 ("conditional expression is constant").
// It does not warn with "while(0,0)". However this again causes warnings with other compilers.
// Solve it with a macro.
#define MPT_DO do
#define MPT_WHILE_0 while(0,0)
#endif

#ifndef MPT_DO
#define MPT_DO do
#endif
#ifndef MPT_WHILE_0
#define MPT_WHILE_0 while(0)
#endif



#if MPT_COMPILER_MSVC && defined(UNREFERENCED_PARAMETER)
#define MPT_UNREFERENCED_PARAMETER(x) UNREFERENCED_PARAMETER(x)
#else
#define MPT_UNREFERENCED_PARAMETER(x) (void)(x)
#endif

#define MPT_UNUSED_VARIABLE(x) MPT_UNREFERENCED_PARAMETER(x)



// Macro for marking intentional fall-throughs in switch statements - can be used for static analysis if supported.
#if (MPT_CXX >= 17)
	#define MPT_FALLTHROUGH [[fallthrough]]
#elif MPT_COMPILER_MSVC
	#define MPT_FALLTHROUGH __fallthrough
#elif MPT_COMPILER_CLANG
	#define MPT_FALLTHROUGH [[clang::fallthrough]]
#elif MPT_COMPILER_GCC && MPT_GCC_AT_LEAST(7,1,0)
	#define MPT_FALLTHROUGH __attribute__((fallthrough))
#elif defined(__has_cpp_attribute)
	#if __has_cpp_attribute(fallthrough)
		#define MPT_FALLTHROUGH [[fallthrough]]
	#else
		#define MPT_FALLTHROUGH MPT_DO { } MPT_WHILE_0
	#endif
#else
	#define MPT_FALLTHROUGH MPT_DO { } MPT_WHILE_0
#endif



#if MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define MPT_PRINTF_FUNC(formatstringindex,varargsindex) __attribute__((format(printf, formatstringindex, varargsindex)))
#else
#define MPT_PRINTF_FUNC(formatstringindex,varargsindex)
#endif



#if MPT_COMPILER_MSVC
// warning LNK4221: no public symbols found; archive member will be inaccessible
// There is no way to selectively disable linker warnings.
// #pragma warning does not apply and a command line option does not exist.
// Some options:
//  1. Macro which generates a variable with a unique name for each file (which means we have to pass the filename to the macro)
//  2. unnamed namespace containing any symbol (does not work for c++11 compilers because they actually have internal linkage now)
//  3. An unused trivial inline function.
// Option 3 does not actually solve the problem though, which leaves us with option 1.
// In any case, for optimized builds, the linker will just remove the useless symbol.
#define MPT_MSVC_WORKAROUND_LNK4221_CONCAT_DETAIL(x,y) x##y
#define MPT_MSVC_WORKAROUND_LNK4221_CONCAT(x,y) MPT_MSVC_WORKAROUND_LNK4221_CONCAT_DETAIL(x,y)
#define MPT_MSVC_WORKAROUND_LNK4221(x) int MPT_MSVC_WORKAROUND_LNK4221_CONCAT(mpt_msvc_workaround_lnk4221_,x) = 0;
#endif

#ifndef MPT_MSVC_WORKAROUND_LNK4221
#define MPT_MSVC_WORKAROUND_LNK4221(x)
#endif



OPENMPT_NAMESPACE_END
