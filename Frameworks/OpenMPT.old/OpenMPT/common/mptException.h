/*
 * mptException.h
 * --------------
 * Purpose: Exception abstraction, in particular for bad_alloc.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"



#include "mptBaseMacros.h"

#include <exception>
#if !defined(MPT_WITH_MFC)
#include <new>
#endif // !MPT_WITH_MFC

#if defined(MPT_WITH_MFC)
// cppcheck-suppress missingInclude
#include <afx.h>
#endif // MPT_WITH_MFC



OPENMPT_NAMESPACE_BEGIN



// Exception handling helpers, because MFC requires explicit deletion of the exception object,
// Thus, always call exactly one of MPT_EXCEPTION_RETHROW_OUT_OF_MEMORY() or MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e).

#if defined(MPT_WITH_MFC)

#define MPT_EXCEPTION_THROW_OUT_OF_MEMORY()    MPT_DO { AfxThrowMemoryException(); } MPT_WHILE_0
#define MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)   catch ( CMemoryException * e )
#define MPT_EXCEPTION_RETHROW_OUT_OF_MEMORY(e) MPT_DO { MPT_UNUSED_VARIABLE(e); throw; } MPT_WHILE_0
#define MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e)  MPT_DO { if(e) { e->Delete(); e = nullptr; } } MPT_WHILE_0

#else // !MPT_WITH_MFC

#define MPT_EXCEPTION_THROW_OUT_OF_MEMORY()    MPT_DO { throw std::bad_alloc(); } MPT_WHILE_0
#define MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)   catch ( const std::bad_alloc & e )
#define MPT_EXCEPTION_RETHROW_OUT_OF_MEMORY(e) MPT_DO { MPT_UNUSED_VARIABLE(e); throw; } MPT_WHILE_0
#define MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e)  MPT_DO { MPT_UNUSED_VARIABLE(e); } MPT_WHILE_0

#endif // MPT_WITH_MFC



OPENMPT_NAMESPACE_END
