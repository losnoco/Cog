// Sets up common environment for Shay Green's libraries.
// To change configuration options, modify blargg_config.h, not this file.

// snes_spc 0.9.0
#pragma once

#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <climits>

#include <snes9x/blargg_config.h>

// BLARGG_COMPILER_HAS_BOOL: If 0, provides bool support for old compiler. If 1,
// compiler is assumed to support bool. If undefined, availability is determined.
#ifndef BLARGG_COMPILER_HAS_BOOL
# if defined(__MWERKS__)
#  if !__option(bool)
#   define BLARGG_COMPILER_HAS_BOOL 0
#  endif
# elif defined(_MSC_VER)
#  if _MSC_VER < 1100
#   define BLARGG_COMPILER_HAS_BOOL 0
#  endif
# elif defined(__GNUC__)
// supports bool
# elif __cplusplus < 199711
#  define BLARGG_COMPILER_HAS_BOOL 0
# endif
#endif
#if defined(BLARGG_COMPILER_HAS_BOOL) && !BLARGG_COMPILER_HAS_BOOL
// If you get errors here, modify your blargg_config.h file
typedef int bool;
const bool true = 1;
const bool false = 0;
#endif

#include <cstdint>
