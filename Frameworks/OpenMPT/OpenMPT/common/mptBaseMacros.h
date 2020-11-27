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



#include <array>
#include <iterator>
#include <type_traits>

#include <cstddef>
#include <cstdint>

#include <stddef.h>
#include <stdint.h>



OPENMPT_NAMESPACE_BEGIN



#define MPT_PP_DEFER(m, ...) m(__VA_ARGS__)

#define MPT_PP_STRINGIFY(x) #x

#define MPT_PP_JOIN_HELPER(a, b) a ## b
#define MPT_PP_JOIN(a, b) MPT_PP_JOIN_HELPER(a, b)

#define MPT_PP_UNIQUE_IDENTIFIER(prefix) MPT_PP_JOIN(prefix , __LINE__)



#if MPT_COMPILER_MSVC

#define MPT_WARNING(text)           __pragma(message(__FILE__ "(" MPT_PP_DEFER(MPT_PP_STRINGIFY, __LINE__) "): Warning: " text))
#define MPT_WARNING_STATEMENT(text) __pragma(message(__FILE__ "(" MPT_PP_DEFER(MPT_PP_STRINGIFY, __LINE__) "): Warning: " text))

#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG

#define MPT_WARNING(text)           _Pragma(MPT_PP_STRINGIFY(GCC warning text))
#define MPT_WARNING_STATEMENT(text) _Pragma(MPT_PP_STRINGIFY(GCC warning text))

#else

// portable #pragma message or #warning replacement
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
#define MPT_CONSTEXPR14_FUN constexpr MPT_FORCEINLINE
#define MPT_CONSTEXPR17_FUN constexpr MPT_FORCEINLINE
#if MPT_CXX_AT_LEAST(20)
#define MPT_CONSTEXPR20_FUN constexpr MPT_FORCEINLINE
#define MPT_CONSTEXPR20_VAR constexpr
#else // !C++20
#define MPT_CONSTEXPR20_FUN MPT_FORCEINLINE
#define MPT_CONSTEXPR20_VAR const
#endif // C++20



namespace mpt
{
template <auto V> struct constant_value { static constexpr decltype(V) value() { return V; } };
#define MPT_FORCE_CONSTEXPR(expr) (mpt::constant_value<( expr )>::value())
}  // namespace mpt



#if MPT_CXX_AT_LEAST(20)
#define MPT_IS_CONSTANT_EVALUATED20() std::is_constant_evaluated()
#define MPT_IS_CONSTANT_EVALUATED() std::is_constant_evaluated()
#else // !C++20
#define MPT_IS_CONSTANT_EVALUATED20() false
// this pessimizes the case for C++17 by always assuming constexpr context, which implies always running constexpr-friendly code
#define MPT_IS_CONSTANT_EVALUATED() true
#endif // C++20



namespace mpt
{

template <typename T>
struct stdarray_extent : std::integral_constant<std::size_t, 0> {};

template <typename T, std::size_t N>
struct stdarray_extent<std::array<T, N>> : std::integral_constant<std::size_t, N> {};

template <typename T>
struct is_stdarray : std::false_type {};

template <typename T, std::size_t N>
struct is_stdarray<std::array<T, N>> : std::true_type {};

// mpt::extent is the same as std::extent,
// but also works for std::array,
// and asserts that the given type is actually an array type instead of returning 0.
// use as:
// mpt::extent<decltype(expr)>()
// mpt::extent<decltype(variable)>()
// mpt::extent<decltype(type)>()
// mpt::extent<type>()
template <typename T>
constexpr std::size_t extent() noexcept
{
	using Tarray = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
	static_assert(std::is_array<Tarray>::value || mpt::is_stdarray<Tarray>::value);
	if constexpr(mpt::is_stdarray<Tarray>::value)
	{
		return mpt::stdarray_extent<Tarray>();
	} else
	{
		return std::extent<Tarray>();
	}
}

} // namespace mpt

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



#define MPT_ATTR_NODISCARD [[nodiscard]]
#define MPT_DISCARD(expr) static_cast<void>(expr)



#if MPT_COMPILER_MSVC
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
