// CPU Byte Order Utilities

// snes_spc 0.9.0
#pragma once

#include <snes9x/blargg_commonSNSF.h>

// BLARGG_CPU_CISC: Defined if CPU has very few general-purpose registers (< 16)
#if defined(_M_IX86) || defined(_M_IA64) || defined(__i486__) || defined(__x86_64__) || defined(__ia64__) || defined(__i386__)
# define BLARGG_CPU_X86 1
# define BLARGG_CPU_CISC 1
#else
# define BLARGG_CPU_X86 0
# define BLARGG_CPU_CISC 0
#endif

#if defined(__powerpc__) || defined(__ppc__) || defined(__POWERPC__) || defined(__powerc)
# define BLARGG_CPU_POWERPC 1
# define BLARGG_CPU_RISC 1
#else
# define BLARGG_CPU_POWERPC 0
# define BLARGG_CPU_RISC 0
#endif

// BLARGG_BIG_ENDIAN, BLARGG_LITTLE_ENDIAN: Determined automatically, otherwise only
// one may be #defined to 1. Only needed if something actually depends on byte order.
#if !defined(BLARGG_BIG_ENDIAN) && !defined(BLARGG_LITTLE_ENDIAN)
# ifdef __GLIBC__
// GCC handles this for us
#  include <endian.h>
#  if __BYTE_ORDER == __LITTLE_ENDIAN
#   define BLARGG_LITTLE_ENDIAN 1
#   define BLARGG_BIG_ENDIAN 0
#  elif __BYTE_ORDER == __BIG_ENDIAN
#   define BLARGG_BIG_ENDIAN 1
#   define BLARGG_LITTLE_ENDIAN 0
#  endif
# else
#  if defined(LSB_FIRST) || defined(__LITTLE_ENDIAN__) || BLARGG_CPU_X86 || (defined(LITTLE_ENDIAN) && LITTLE_ENDIAN + 0 != 1234)
#   define BLARGG_LITTLE_ENDIAN 1
#   define BLARGG_BIG_ENDIAN 0
#  elif defined(MSB_FIRST) || defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN) || defined(__sparc__) || BLARGG_CPU_POWERPC || (defined(BIG_ENDIAN) && BIG_ENDIAN + 0 != 4321)
#   define BLARGG_BIG_ENDIAN 1
#   define BLARGG_LITTLE_ENDIAN 0
#  else
// No endian specified; assume little-endian, since it's most common
#   define BLARGG_LITTLE_ENDIAN 1
#   define BLARGG_BIG_ENDIAN 0
#  endif
# endif
#endif

#if BLARGG_LITTLE_ENDIAN && BLARGG_BIG_ENDIAN
# undef BLARGG_LITTLE_ENDIAN
# undef BLARGG_BIG_ENDIAN
#endif

inline void blargg_verify_byte_order()
{
#ifndef NDEBUG
	volatile int i = 1;
	assert(*reinterpret_cast<volatile char *>(&i));
#endif
}

inline unsigned get_le16(const uint8_t *p)
{
	return static_cast<unsigned>(p[1] << 8) | static_cast<unsigned>(p[0]);
}

inline void set_le16(uint8_t *p, unsigned n)
{
	p[1] = static_cast<unsigned char>(n >> 8);
	p[0] = static_cast<unsigned char>(n);
}
