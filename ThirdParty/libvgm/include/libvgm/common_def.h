#ifndef __COMMON_DEF_H__
#define __COMMON_DEF_H__

#include "stdtype.h"
#include "_stdbool.h"

#ifndef INLINE
#if defined(_MSC_VER)
#define INLINE	static __inline	// __forceinline
#elif defined(__GNUC__)
#define INLINE	static __inline__
#else
#define INLINE	static inline
#endif
#endif	// INLINE

#endif	// __COMMON_DEF_H__
