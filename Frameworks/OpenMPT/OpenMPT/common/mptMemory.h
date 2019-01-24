/*
 * mptMemory.h
 * -----------
 * Purpose: Raw memory manipulation
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"


#include "mptAssert.h"
#include "mptBaseTypes.h"
#include "mptSpan.h"

#if MPT_CXX_AT_LEAST(20)
#include <bit>
#endif
#include <utility>

#include <cstring>

#include <string.h>



OPENMPT_NAMESPACE_BEGIN



namespace mpt {



typedef mpt::span<mpt::byte> byte_span;
typedef mpt::span<const mpt::byte> const_byte_span;



// Tell which types are safe for mpt::byte_cast.
// signed char is actually not allowed to alias into an object representation,
// which means that, if the actual type is not itself signed char but char or
// unsigned char instead, dereferencing the signed char pointer is undefined
// behaviour.
template <typename T> struct is_byte_castable : public std::false_type { };
template <> struct is_byte_castable<char>                : public std::true_type { };
template <> struct is_byte_castable<unsigned char>       : public std::true_type { };
#if MPT_BYTE_IS_STD_BYTE
template <> struct is_byte_castable<mpt::byte>           : public std::true_type { };
#endif
template <> struct is_byte_castable<const char>          : public std::true_type { };
template <> struct is_byte_castable<const unsigned char> : public std::true_type { };
#if MPT_BYTE_IS_STD_BYTE
template <> struct is_byte_castable<const mpt::byte>     : public std::true_type { };
#endif


template <typename T> struct is_byte        : public std::false_type { };
template <> struct is_byte<mpt::byte>       : public std::true_type  { };
template <> struct is_byte<const mpt::byte> : public std::true_type  { };


// Tell which types are safe to binary write into files.
// By default, no types are safe.
// When a safe type gets defined,
// also specialize this template so that IO functions will work.
template <typename T> struct is_binary_safe : public std::false_type { }; 

// Specialization for byte types.
template <> struct is_binary_safe<char>      : public std::true_type { };
template <> struct is_binary_safe<uint8>     : public std::true_type { };
template <> struct is_binary_safe<int8>      : public std::true_type { };
#if MPT_BYTE_IS_STD_BYTE
template <> struct is_binary_safe<mpt::byte> : public std::true_type { };
#endif

// Generic Specialization for arrays.
template <typename T, std::size_t N> struct is_binary_safe<T[N]> : public is_binary_safe<T> { };
template <typename T, std::size_t N> struct is_binary_safe<const T[N]> : public is_binary_safe<T> { };
template <typename T, std::size_t N> struct is_binary_safe<std::array<T, N>> : public is_binary_safe<T> { };
template <typename T, std::size_t N> struct is_binary_safe<const std::array<T, N>> : public is_binary_safe<T> { };


} // namespace mpt

#define MPT_BINARY_STRUCT(type, size) \
	MPT_STATIC_ASSERT(sizeof( type ) == (size) ); \
	MPT_STATIC_ASSERT(alignof( type ) == 1); \
	MPT_STATIC_ASSERT(std::is_standard_layout< type >::value); \
	namespace mpt { \
		template <> struct is_binary_safe< type > : public std::true_type { }; \
	} \
/**/



template <typename T>
struct value_initializer
{
	inline void operator () (T & x)
	{
		x = T();
	}
};

template <typename T, std::size_t N>
struct value_initializer<T[N]>
{
	inline void operator () (T (& a)[N])
	{
		for(auto & e : a)
		{
			value_initializer<T>()(e);
		}
	}
};

template <typename T>
inline void Clear(T & x)
{
	MPT_STATIC_ASSERT(!std::is_pointer<T>::value);
	value_initializer<T>()(x);
}


// Memset given object to zero.
template <class T>
inline void MemsetZero(T &a)
{
	static_assert(std::is_pointer<T>::value == false, "Won't memset pointers.");
#if MPT_GCC_BEFORE(5,1,0) || (MPT_COMPILER_CLANG && defined(__GLIBCXX__))
	MPT_STATIC_ASSERT(std::is_standard_layout<T>::value);
	MPT_STATIC_ASSERT(std::is_trivial<T>::value || mpt::is_binary_safe<T>::value); // approximation
#else // default
	MPT_STATIC_ASSERT(std::is_standard_layout<T>::value);
	MPT_STATIC_ASSERT((std::is_trivially_default_constructible<T>::value && std::is_trivially_copyable<T>::value) || mpt::is_binary_safe<T>::value); // C++11, but not supported on most compilers we care about
#endif
	std::memset(&a, 0, sizeof(T));
}



namespace mpt {



#if MPT_CXX_AT_LEAST(20)
using std::bit_cast;
#else
// C++2a compatible bit_cast.
// See <http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0476r1.html>.
// Not implementing constexpr because this is not easily possible pre C++2a.
template <typename Tdst, typename Tsrc>
MPT_FORCEINLINE Tdst bit_cast(const Tsrc & src) noexcept
{
	MPT_STATIC_ASSERT(sizeof(Tdst) == sizeof(Tsrc));
#if MPT_GCC_BEFORE(5,1,0) || (MPT_COMPILER_CLANG && defined(__GLIBCXX__))
	MPT_STATIC_ASSERT(std::is_trivial<Tdst>::value); // approximation
	MPT_STATIC_ASSERT(std::is_trivial<Tsrc>::value); // approximation
#else // default
	MPT_STATIC_ASSERT(std::is_trivially_copyable<Tdst>::value);
	MPT_STATIC_ASSERT(std::is_trivially_copyable<Tsrc>::value);
#endif
	#if MPT_COMPILER_GCC || MPT_COMPILER_MSVC
		// Compiler supports type-punning through unions. This is not stricly standard-conforming.
		// For GCC, this is documented, for MSVC this is apparently not documented, but we assume it.
		union {
			Tsrc src;
			Tdst dst;
		} conv;
		conv.src = src;
		return conv.dst;
	#else // MPT_COMPILER
		// Compiler does not support type-punning through unions. std::memcpy is used instead.
		// This is the safe fallback and strictly standard-conforming.
		// Another standard-compliant alternative would be casting pointers to a character type pointer.
		// This results in rather unreadable code and,
		// in most cases, compilers generate better code by just inlining the memcpy anyway.
		// (see <http://blog.regehr.org/archives/959>).
		Tdst dst{};
		std::memcpy(&dst, &src, sizeof(Tdst));
		return dst;
	#endif // MPT_COMPILER
}
#endif



template <typename Tdst, typename Tsrc>
struct byte_cast_impl
{
	inline Tdst operator () (Tsrc src) const
	{
		STATIC_ASSERT(sizeof(Tsrc) == sizeof(mpt::byte));
		STATIC_ASSERT(sizeof(Tdst) == sizeof(mpt::byte));
		// not checking is_byte_castable here because we are actually
		// doing a static_cast and converting the value
		STATIC_ASSERT(std::is_integral<Tsrc>::value || mpt::is_byte<Tsrc>::value);
		STATIC_ASSERT(std::is_integral<Tdst>::value || mpt::is_byte<Tdst>::value);
		return static_cast<Tdst>(src);
	}
};
template <typename Tdst, typename Tsrc>
struct byte_cast_impl<mpt::span<Tdst>, mpt::span<Tsrc> >
{
	inline mpt::span<Tdst> operator () (mpt::span<Tsrc> src) const
	{
		STATIC_ASSERT(sizeof(Tsrc) == sizeof(mpt::byte));
		STATIC_ASSERT(sizeof(Tdst) == sizeof(mpt::byte));
		STATIC_ASSERT(mpt::is_byte_castable<Tsrc>::value);
		STATIC_ASSERT(mpt::is_byte_castable<Tdst>::value);
		STATIC_ASSERT(std::is_integral<Tsrc>::value || mpt::is_byte<Tsrc>::value);
		STATIC_ASSERT(std::is_integral<Tdst>::value || mpt::is_byte<Tdst>::value);
		return mpt::as_span(mpt::byte_cast_impl<Tdst*, Tsrc*>()(src.begin()), mpt::byte_cast_impl<Tdst*, Tsrc*>()(src.end()));
	}
};
template <typename Tdst, typename Tsrc>
struct byte_cast_impl<Tdst*, Tsrc*>
{
	inline Tdst* operator () (Tsrc* src) const
	{
		STATIC_ASSERT(sizeof(Tsrc) == sizeof(mpt::byte));
		STATIC_ASSERT(sizeof(Tdst) == sizeof(mpt::byte));
		STATIC_ASSERT(mpt::is_byte_castable<Tsrc>::value);
		STATIC_ASSERT(mpt::is_byte_castable<Tdst>::value);
		STATIC_ASSERT(std::is_integral<Tsrc>::value || mpt::is_byte<Tsrc>::value);
		STATIC_ASSERT(std::is_integral<Tdst>::value || mpt::is_byte<Tdst>::value);
		return reinterpret_cast<Tdst*>(src);
	}
};

template <typename Tdst, typename Tsrc>
struct void_cast_impl;

template <typename Tdst>
struct void_cast_impl<Tdst*, void*>
{
	inline Tdst* operator () (void* src) const
	{
		STATIC_ASSERT(sizeof(Tdst) == sizeof(mpt::byte));
		STATIC_ASSERT(mpt::is_byte_castable<Tdst>::value);
		STATIC_ASSERT(std::is_integral<Tdst>::value || mpt::is_byte<Tdst>::value);
		return reinterpret_cast<Tdst*>(src);
	}
};
template <typename Tdst>
struct void_cast_impl<Tdst*, const void*>
{
	inline Tdst* operator () (const void* src) const
	{
		STATIC_ASSERT(sizeof(Tdst) == sizeof(mpt::byte));
		STATIC_ASSERT(mpt::is_byte_castable<Tdst>::value);
		STATIC_ASSERT(std::is_integral<Tdst>::value || mpt::is_byte<Tdst>::value);
		return reinterpret_cast<Tdst*>(src);
	}
};
template <typename Tsrc>
struct void_cast_impl<void*, Tsrc*>
{
	inline void* operator () (Tsrc* src) const
	{
		STATIC_ASSERT(sizeof(Tsrc) == sizeof(mpt::byte));
		STATIC_ASSERT(mpt::is_byte_castable<Tsrc>::value);
		STATIC_ASSERT(std::is_integral<Tsrc>::value || mpt::is_byte<Tsrc>::value);
		return reinterpret_cast<void*>(src);
	}
};
template <typename Tsrc>
struct void_cast_impl<const void*, Tsrc*>
{
	inline const void* operator () (Tsrc* src) const
	{
		STATIC_ASSERT(sizeof(Tsrc) == sizeof(mpt::byte));
		STATIC_ASSERT(mpt::is_byte_castable<Tsrc>::value);
		STATIC_ASSERT(std::is_integral<Tsrc>::value || mpt::is_byte<Tsrc>::value);
		return reinterpret_cast<const void*>(src);
	}
};

// casts between different byte (char) types or pointers to these types
template <typename Tdst, typename Tsrc>
inline Tdst byte_cast(Tsrc src)
{
	return byte_cast_impl<Tdst, Tsrc>()(src);
}

// casts between pointers to void and pointers to byte
template <typename Tdst, typename Tsrc>
inline Tdst void_cast(Tsrc src)
{
	return void_cast_impl<Tdst, Tsrc>()(src);
}



template <typename T>
MPT_CONSTEXPR14_FUN mpt::byte as_byte(T src) noexcept
{
	MPT_STATIC_ASSERT(std::is_integral<T>::value);
	return static_cast<mpt::byte>(static_cast<uint8>(src));
}



template <typename T>
struct GetRawBytesFunctor
{
	inline mpt::const_byte_span operator () (const T & v) const
	{
		STATIC_ASSERT(mpt::is_binary_safe<typename std::remove_const<T>::type>::value);
		return mpt::as_span(reinterpret_cast<const mpt::byte *>(&v), sizeof(T));
	}
	inline mpt::byte_span operator () (T & v) const
	{
		STATIC_ASSERT(mpt::is_binary_safe<typename std::remove_const<T>::type>::value);
		return mpt::as_span(reinterpret_cast<mpt::byte *>(&v), sizeof(T));
	}
};

template <typename T, std::size_t N>
struct GetRawBytesFunctor<T[N]>
{
	inline mpt::const_byte_span operator () (const T (&v)[N]) const
	{
		STATIC_ASSERT(mpt::is_binary_safe<typename std::remove_const<T>::type>::value);
		return mpt::as_span(reinterpret_cast<const mpt::byte *>(v), N * sizeof(T));
	}
	inline mpt::byte_span operator () (T (&v)[N]) const
	{
		STATIC_ASSERT(mpt::is_binary_safe<typename std::remove_const<T>::type>::value);
		return mpt::as_span(reinterpret_cast<mpt::byte *>(v), N * sizeof(T));
	}
};

template <typename T, std::size_t N>
struct GetRawBytesFunctor<const T[N]>
{
	inline mpt::const_byte_span operator () (const T (&v)[N]) const
	{
		STATIC_ASSERT(mpt::is_binary_safe<typename std::remove_const<T>::type>::value);
		return mpt::as_span(reinterpret_cast<const mpt::byte *>(v), N * sizeof(T));
	}
};

// In order to be able to partially specialize it,
// as_raw_memory is implemented via a class template.
// Do not overload or specialize as_raw_memory directly.
// Using a wrapper (by default just around a cast to const mpt::byte *),
// allows for implementing raw memory access
// via on-demand generating a cached serialized representation.
template <typename T> inline mpt::const_byte_span as_raw_memory(const T & v)
{
	return mpt::GetRawBytesFunctor<T>()(v);
}
template <typename T> inline mpt::byte_span as_raw_memory(T & v)
{
	return mpt::GetRawBytesFunctor<T>()(v);
}



} // namespace mpt



OPENMPT_NAMESPACE_END
