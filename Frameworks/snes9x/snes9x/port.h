/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#pragma once

#include <cstdio>
#include <cstdlib>
#include <climits>
#include <ctime>
#include <cstring>
#include <cstdint>
#include <sys/types.h>

#ifdef _WIN32
# include "windowsh_wrapper.h"
#endif

#ifndef _WIN32
# ifndef PATH_MAX
#  define PATH_MAX 1024
# endif
# define _MAX_DRIVE 1
# define _MAX_DIR PATH_MAX
# define _MAX_FNAME PATH_MAX
# define _MAX_EXT PATH_MAX
# define _MAX_PATH PATH_MAX
#else
# ifndef PATH_MAX
#  define PATH_MAX _MAX_PATH
# endif
#endif

#ifdef _MSC_VER
# define strcasecmp stricmp
#endif

#if defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(__x86_64__) || defined(__alpha__) || defined(__MIPSEL__) || defined(_M_IX86) || defined(_M_X64) || (defined(__BYTE_ORDER__) && __BYTE_ORDER == __ORDER_LITTLE_ENDIAN__)
# define LSB_FIRST
# define FAST_LSB_WORD_ACCESS
#else
# define MSB_FIRST
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

# define LSB_FIRST
# define FAST_LSB_WORD_ACCESS


#ifdef FAST_LSB_WORD_ACCESS
inline uint16_t READ_WORD(const uint8_t *s) { return *reinterpret_cast<const uint16_t *>(s); }
inline uint32_t READ_3WORD(const uint8_t *s) { return *reinterpret_cast<const uint32_t *>(s) & 0x00FFFFFF; }
inline void WRITE_WORD(uint8_t *s, uint16_t d) { *reinterpret_cast<uint16_t *>(s) = d; }
#else
inline uint16_t READ_WORD(const uint8_t *s) { return (*s) | (*(s + 1) << 8); }
inline uint32_t READ_3WORD(const uint8_t *s) { return (*s) | (*(s + 1) << 8) | (*(s + 2) << 16); }
inline void WRITE_WORD(uint8_t *s, uint16_t d) { *s = static_cast<uint8_t>(d); *(s + 1) = static_cast<uint8_t>(d >> 8); }
#endif
