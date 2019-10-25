/*
 * mptAlloc.cpp
 * ------------
 * Purpose: Dynamic memory allocation.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */



#include "stdafx.h"
#include "mptAlloc.h"

#include "mptBaseTypes.h"

#include <memory>
#include <new>

#include <cstddef>
#include <cstdlib>

#if MPT_COMPILER_MSVC
#include <malloc.h>
#endif



OPENMPT_NAMESPACE_BEGIN



#if defined(MPT_ENABLE_ALIGNED_ALLOC)



namespace mpt
{



#if MPT_CXX_AT_LEAST(17)
#else
void* align(std::size_t alignment, std::size_t size, void* &ptr, std::size_t &space) noexcept
{
	std::size_t offset = static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(ptr) & (alignment - 1));
	if(offset != 0)
	{
		offset = alignment - offset;
	}
	if((space < offset) || ((space - offset) < size))
	{
		return nullptr;
	}
	ptr = static_cast<mpt::byte*>(ptr) + offset;
	space -= offset;
	return ptr;
}
#endif

aligned_raw_memory aligned_alloc_impl(std::size_t size, std::size_t count, std::size_t alignment)
{
	#if MPT_CXX_AT_LEAST(17) && !defined(MPT_COMPILER_QUIRK_NO_ALIGNEDALLOC)
		std::size_t space = count * size;
		void* mem = std::aligned_alloc(alignment, space);
		if(!mem)
		{
			MPT_EXCEPTION_THROW_OUT_OF_MEMORY();
		}
		return aligned_raw_memory{mem, mem};
	#elif MPT_COMPILER_MSVC || (defined(__clang__) && defined(_MSC_VER))
		std::size_t space = count * size;
		void* mem = _aligned_malloc(space, alignment);
		if(!mem)
		{
			MPT_EXCEPTION_THROW_OUT_OF_MEMORY();
		}
		return aligned_raw_memory{mem, mem};
	#else
		if(alignment > alignof(mpt::max_align_t))
		{
			std::size_t space = count * size + (alignment - 1);
			void* mem = std::malloc(space);
			if(!mem)
			{
				MPT_EXCEPTION_THROW_OUT_OF_MEMORY();
			}
			void* aligned_mem = mem;
			void* aligned = mpt::align(alignment, size * count, aligned_mem, space);
			if(!aligned)
			{
				MPT_EXCEPTION_THROW_OUT_OF_MEMORY();
			}
			return aligned_raw_memory{aligned, mem};
		} else
		{
			std::size_t space = count * size;
			void* mem = std::malloc(space);
			if(!mem)
			{
				MPT_EXCEPTION_THROW_OUT_OF_MEMORY();
			}
			return aligned_raw_memory{mem, mem};
		}
	#endif
}

void aligned_free(aligned_raw_memory raw)
{
	#if MPT_CXX_AT_LEAST(17) && !defined(MPT_COMPILER_QUIRK_NO_ALIGNEDALLOC)
		std::free(raw.mem);
	#elif MPT_COMPILER_MSVC || (defined(__clang__) && defined(_MSC_VER))
		_aligned_free(raw.mem);
	#else
		std::free(raw.mem);
	#endif
}



} // namespace mpt



#endif // MPT_ENABLE_ALIGNED_ALLOC



OPENMPT_NAMESPACE_END
