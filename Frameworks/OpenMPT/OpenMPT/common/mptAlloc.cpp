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
	#if MPT_CXX_AT_LEAST(17) && (!MPT_COMPILER_MSVC && !MPT_GCC_BEFORE(8,1,0) && !MPT_CLANG_BEFORE(5,0,0)) && !(MPT_COMPILER_GCC && defined(__GLIBCXX__) && (defined(__MINGW32__) || defined(__MINGW64__))) && !(MPT_COMPILER_CLANG && defined(__GLIBCXX__)) && !(MPT_COMPILER_CLANG && MPT_OS_MACOSX_OR_IOS) && !MPT_OS_EMSCRIPTEN
		std::size_t space = count * size;
		void* mem = std::aligned_alloc(alignment, space);
		if(!mem)
		{
			MPT_EXCEPTION_THROW_OUT_OF_MEMORY();
		}
		return aligned_raw_memory{mem, mem};
	#elif MPT_COMPILER_MSVC
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
	#if MPT_CXX_AT_LEAST(17) && (!MPT_COMPILER_MSVC && !MPT_GCC_BEFORE(8,1,0) && !MPT_CLANG_BEFORE(5,0,0)) && !(MPT_COMPILER_GCC && defined(__GLIBCXX__) && (defined(__MINGW32__) || defined(__MINGW64__))) && !(MPT_COMPILER_CLANG && defined(__GLIBCXX__)) && !(MPT_COMPILER_CLANG && MPT_OS_MACOSX_OR_IOS) && !MPT_OS_EMSCRIPTEN
		std::free(raw.mem);
	#elif MPT_COMPILER_MSVC
		_aligned_free(raw.mem);
	#else
		std::free(raw.mem);
	#endif
}



} // namespace mpt



OPENMPT_NAMESPACE_END
