/*
 * mptAlloc.h
 * ----------
 * Purpose: Dynamic memory allocation.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"



#include "mptBaseMacros.h"
#include "mptMemory.h"
#include "mptSpan.h"

#include <array>
#include <memory>
#include <new>
#include <vector>



OPENMPT_NAMESPACE_BEGIN



namespace mpt {



template <typename T> inline mpt::span<T> as_span(std::vector<T> & cont) { return mpt::span<T>(cont); }

template <typename T> inline mpt::span<const T> as_span(const std::vector<T> & cont) { return mpt::span<const T>(cont); }



template <typename T> inline std::vector<typename std::remove_const<T>::type> make_vector(T * beg, T * end) { return std::vector<typename std::remove_const<T>::type>(beg, end); }

template <typename T> inline std::vector<typename std::remove_const<T>::type> make_vector(T * data, std::size_t size) { return std::vector<typename std::remove_const<T>::type>(data, data + size); }

template <typename T> inline std::vector<typename std::remove_const<T>::type> make_vector(mpt::span<T> data) { return std::vector<typename std::remove_const<T>::type>(data.data(), data.data() + data.size()); }

template <typename T, std::size_t N> inline std::vector<typename std::remove_const<T>::type> make_vector(T (&arr)[N]) { return std::vector<typename std::remove_const<T>::type>(std::begin(arr), std::end(arr)); }



template <typename T>
struct GetRawBytesFunctor<std::vector<T>>
{
	inline mpt::const_byte_span operator () (const std::vector<T> & v) const
	{
		static_assert(mpt::is_binary_safe<typename std::remove_const<T>::type>::value);
		return mpt::as_span(reinterpret_cast<const std::byte *>(v.data()), v.size() * sizeof(T));
	}
	inline mpt::byte_span operator () (std::vector<T> & v) const
	{
		static_assert(mpt::is_binary_safe<typename std::remove_const<T>::type>::value);
		return mpt::as_span(reinterpret_cast<std::byte *>(v.data()), v.size() * sizeof(T));
	}
};

template <typename T>
struct GetRawBytesFunctor<const std::vector<T>>
{
	inline mpt::const_byte_span operator () (const std::vector<T> & v) const
	{
		static_assert(mpt::is_binary_safe<typename std::remove_const<T>::type>::value);
		return mpt::as_span(reinterpret_cast<const std::byte *>(v.data()), v.size() * sizeof(T));
	}
};



} // namespace mpt



#if defined(MPT_ENABLE_ALIGNED_ALLOC)



namespace mpt
{



#if !(MPT_COMPILER_CLANG && defined(__GLIBCXX__)) && !(MPT_COMPILER_CLANG && MPT_OS_MACOSX_OR_IOS)
using std::launder;
#else
template <class T>
MPT_NOINLINE T* launder(T* p) noexcept
{
	return p;
}
#endif



template <typename T, std::size_t count, std::align_val_t alignment>
struct alignas(static_cast<std::size_t>(alignment)) aligned_array
	: std::array<T, count>
{
	static_assert(static_cast<std::size_t>(alignment) >= alignof(T));
	static_assert(((count * sizeof(T)) % static_cast<std::size_t>(alignment)) == 0);
	static_assert(sizeof(std::array<T, count>) == (sizeof(T) * count));
};

static_assert(sizeof(mpt::aligned_array<float, 4, std::align_val_t{sizeof(float) * 4}>) == sizeof(std::array<float, 4>));



} // namespace mpt



#endif // MPT_ENABLE_ALIGNED_ALLOC



OPENMPT_NAMESPACE_END
