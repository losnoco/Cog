// custom stdbool.h to for 1-byte bool types
#ifndef __CST_STDBOOL_H__
#define __CST_STDBOOL_H__

#undef __HAVE_STDBOOL_H__
#if defined __has_include
#if __has_include(<stdbool.h>)
#define __HAVE_STDBOOL_H__ 1
#endif
#elif defined(_MSC_VER) && _MSC_VER >= 1900
#define __HAVE_STDBOOL_H__ 1
#endif

#if defined(__HAVE_STDBOOL_H__)
#include <stdbool.h>
#elif !defined(__cplusplus) // C++ already has the bool-type

// the MS VC++ 6 compiler uses a one-byte-type (unsigned char, to be exact), so I'll reproduce this here
typedef unsigned char	bool;

#define false	0x00
#define true	0x01

#endif // ! __cplusplus

#endif // ! __CST_STDBOOL_H__
