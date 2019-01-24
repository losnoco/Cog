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

#include <memory>
#include <utility>
#include <vector>



OPENMPT_NAMESPACE_BEGIN



namespace mpt {



template <typename T> inline span<T> as_span(std::vector<T> & cont) { return span<T>(cont); }

template <typename T> inline span<const T> as_span(const std::vector<T> & cont) { return span<const T>(cont); }



template <typename T> inline std::vector<typename std::remove_const<T>::type> make_vector(T * beg, T * end) { return std::vector<typename std::remove_const<T>::type>(beg, end); }

template <typename T> inline std::vector<typename std::remove_const<T>::type> make_vector(T * data, std::size_t size) { return std::vector<typename std::remove_const<T>::type>(data, data + size); }

template <typename T> inline std::vector<typename std::remove_const<T>::type> make_vector(mpt::span<T> data) { return std::vector<typename std::remove_const<T>::type>(data.data(), data.data() + data.size()); }

template <typename T, std::size_t N> inline std::vector<typename std::remove_const<T>::type> make_vector(T (&arr)[N]) { return std::vector<typename std::remove_const<T>::type>(std::begin(arr), std::end(arr)); }



template <typename T>
struct GetRawBytesFunctor<std::vector<T>>
{
	inline mpt::const_byte_span operator () (const std::vector<T> & v) const
	{
		STATIC_ASSERT(mpt::is_binary_safe<typename std::remove_const<T>::type>::value);
		return mpt::as_span(reinterpret_cast<const mpt::byte *>(v.data()), v.size() * sizeof(T));
	}
	inline mpt::byte_span operator () (std::vector<T> & v) const
	{
		STATIC_ASSERT(mpt::is_binary_safe<typename std::remove_const<T>::type>::value);
		return mpt::as_span(reinterpret_cast<mpt::byte *>(v.data()), v.size() * sizeof(T));
	}
};

template <typename T>
struct GetRawBytesFunctor<const std::vector<T>>
{
	inline mpt::const_byte_span operator () (const std::vector<T> & v) const
	{
		STATIC_ASSERT(mpt::is_binary_safe<typename std::remove_const<T>::type>::value);
		return mpt::as_span(reinterpret_cast<const mpt::byte *>(v.data()), v.size() * sizeof(T));
	}
};



} // namespace mpt



#if MPT_CXX_AT_LEAST(14)
namespace mpt {
using std::make_unique;
} // namespace mpt
#else
namespace mpt {
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
} // namespace mpt
#endif



namespace mpt
{



#if MPT_CXX_AT_LEAST(17) && !(MPT_COMPILER_CLANG && defined(__GLIBCXX__))
using std::launder;
#else
template <class T>
MPT_NOINLINE T* launder(T* p) noexcept
{
	return p;
}
#endif


#if MPT_CXX_AT_LEAST(17)
using std::align;
#else
// pre-C++17, std::align does not support over-alignement
void* align(std::size_t alignment, std::size_t size, void* &ptr, std::size_t &space) noexcept;
#endif



struct aligned_raw_memory
{
	void* aligned;
	void* mem;
};

aligned_raw_memory aligned_alloc_impl(std::size_t size, std::size_t count, std::size_t alignment);

template <std::size_t alignment>
inline aligned_raw_memory aligned_alloc(std::size_t size, std::size_t count)
{
	MPT_STATIC_ASSERT(alignment > 0);
	MPT_CONSTEXPR14_ASSERT(mpt::weight(alignment) == 1);
	return aligned_alloc_impl(size, count, alignment);
}

void aligned_free(aligned_raw_memory raw);

template <typename T>
struct aligned_raw_buffer
{
	T* elements;
	void* mem;
};

template <typename T, std::size_t alignment>
inline aligned_raw_buffer<T> aligned_alloc(std::size_t count)
{
	MPT_STATIC_ASSERT(alignment >= alignof(T));
	aligned_raw_memory raw = aligned_alloc<alignment>(sizeof(T), count);
	return aligned_raw_buffer<T>{mpt::launder(reinterpret_cast<T*>(raw.aligned)), raw.mem};
}

template <typename T>
inline void aligned_free(aligned_raw_buffer<T> buf)
{
	aligned_free(aligned_raw_memory{buf.elements, buf.mem});
}

template <typename T>
struct aligned_raw_objects
{
	T* elements;
	std::size_t count;
	void* mem;
};

template <typename T, std::size_t alignment>
inline aligned_raw_objects<T> aligned_new(std::size_t count, T init = T())
{
	aligned_raw_buffer<T> buf = aligned_alloc<T, alignment>(count);
	std::size_t constructed = 0;
	try
	{
		for(std::size_t i = 0; i < count; ++i)
		{
			new(&(buf.elements[i])) T(init);
			constructed++;
		}
	} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
	{
		while(constructed--)
		{
			mpt::launder(&(buf.elements[constructed - 1]))->~T();
		}
		aligned_free(buf);
		MPT_EXCEPTION_RETHROW_OUT_OF_MEMORY(e);
	} catch(...)
	{
		while(constructed--)
		{
			mpt::launder(&(buf.elements[constructed - 1]))->~T();
		}
		aligned_free(buf);
		throw;
	}
	return aligned_raw_objects<T>{mpt::launder(buf.elements), count, buf.mem};
}

template <typename T>
inline void aligned_delete(aligned_raw_objects<T> objs)
{
	if(objs.elements)
	{
		std::size_t constructed = objs.count;
		while(constructed--)
		{
			objs.elements[constructed - 1].~T();
		}
	}
	aligned_free(aligned_raw_buffer<T>{objs.elements, objs.mem});
}

template <typename T, std::size_t alignment>
class aligned_buffer
{
private:
	aligned_raw_objects<T> objs;
public:
	explicit aligned_buffer(std::size_t count = 0)
		: objs(aligned_new<T, alignment>(count))
	{
	}
	aligned_buffer(const aligned_buffer&) = delete;
	aligned_buffer& operator=(const aligned_buffer&) = delete;
	~aligned_buffer()
	{
		aligned_delete(objs);
	}
public:
	void destructive_resize(std::size_t count)
	{
		aligned_raw_objects<T> tmpobjs = aligned_new<T, alignment>(count);
		{
			using namespace std;
			swap(objs, tmpobjs);
		}
		aligned_delete(tmpobjs);
	}
public:
	T* begin() noexcept { return objs.elements; }
	const T* begin() const noexcept { return objs.elements; }
	T* end() noexcept { return objs.elements + objs.count; }
	const T* end() const noexcept { return objs.elements + objs.count; }
	const T* cbegin() const noexcept { return objs.elements; }
	const T* cend() const noexcept { return objs.elements + objs.count; }
	T& operator[](std::size_t i) noexcept { return objs.elements[i]; }
	const T& operator[](std::size_t i) const noexcept { return objs.elements[i]; }
	T* data() noexcept { return objs.elements; }
	const T* data() const noexcept { return objs.elements; }
	std::size_t size() const noexcept { return objs.count; }
};



} // namespace mpt



OPENMPT_NAMESPACE_END
