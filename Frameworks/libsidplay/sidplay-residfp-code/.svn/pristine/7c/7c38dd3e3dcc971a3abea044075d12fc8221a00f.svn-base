/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2000 Simon White
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef SIDENDIAN_H
#define SIDENDIAN_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdint.h>

/*
Labeling:
0 - LO
1 - HI
2 - HILO
3 - HIHI
*/

///////////////////////////////////////////////////////////////////
// INT16 FUNCTIONS
///////////////////////////////////////////////////////////////////
/// Set the lo byte (8 bit) in a word (16 bit)
inline void endian_16lo8 (uint_least16_t &word, uint8_t byte)
{
    word &= 0xff00;
    word |= byte;
}

/// Get the lo byte (8 bit) in a word (16 bit)
inline uint8_t endian_16lo8 (uint_least16_t word)
{
    return (uint8_t) word;
}

/// Set the hi byte (8 bit) in a word (16 bit)
inline void endian_16hi8 (uint_least16_t &word, uint8_t byte)
{
    word &= 0x00ff;
    word |= (uint_least16_t) byte << 8;
}

/// Set the hi byte (8 bit) in a word (16 bit)
inline uint8_t endian_16hi8 (uint_least16_t word)
{
    return (uint8_t) (word >> 8);
}

/// Swap word endian.
inline void endian_16swap8 (uint_least16_t &word)
{
    uint8_t lo = endian_16lo8 (word);
    uint8_t hi = endian_16hi8 (word);
    endian_16lo8 (word, hi);
    endian_16hi8 (word, lo);
}

/// Convert high-byte and low-byte to 16-bit word.
inline uint_least16_t endian_16 (uint8_t hi, uint8_t lo)
{
    uint_least16_t word = 0;
    endian_16lo8 (word, lo);
    endian_16hi8 (word, hi);
    return word;
}

/// Convert high-byte and low-byte to 16-bit little endian word.
inline void endian_16 (uint8_t ptr[2], uint_least16_t word)
{
#if defined(WORDS_BIGENDIAN)
    ptr[0] = endian_16hi8 (word);
    ptr[1] = endian_16lo8 (word);
#else
    ptr[0] = endian_16lo8 (word);
    ptr[1] = endian_16hi8 (word);
#endif
}

inline void endian_16 (char ptr[2], uint_least16_t word)
{
    endian_16 ((uint8_t *) ptr, word);
}

/// Convert high-byte and low-byte to 16-bit little endian word.
inline uint_least16_t endian_little16 (const uint8_t ptr[2])
{
    return endian_16 (ptr[1], ptr[0]);
}

/// Write a little-endian 16-bit word to two bytes in memory.
inline void endian_little16 (uint8_t ptr[2], uint_least16_t word)
{
    ptr[0] = endian_16lo8 (word);
    ptr[1] = endian_16hi8 (word);
}

/// Convert high-byte and low-byte to 16-bit big endian word.
inline uint_least16_t endian_big16 (const uint8_t ptr[2])
{
    return endian_16 (ptr[0], ptr[1]);
}

/// Write a little-big 16-bit word to two bytes in memory.
inline void endian_big16 (uint8_t ptr[2], uint_least16_t word)
{
    ptr[0] = endian_16hi8 (word);
    ptr[1] = endian_16lo8 (word);
}


///////////////////////////////////////////////////////////////////
// INT32 FUNCTIONS
///////////////////////////////////////////////////////////////////
/// Set the lo word (16bit) in a dword (32 bit)
inline void endian_32lo16 (uint_least32_t &dword, uint_least16_t word)
{
    dword &= (uint_least32_t) 0xffff0000;
    dword |= word;
}

/// Get the lo word (16bit) in a dword (32 bit)
inline uint_least16_t endian_32lo16 (uint_least32_t dword)
{
    return (uint_least16_t) dword & 0xffff;
}

/// Set the hi word (16bit) in a dword (32 bit)
inline void endian_32hi16 (uint_least32_t &dword, uint_least16_t word)
{
    dword &= (uint_least32_t) 0x0000ffff;
    dword |= (uint_least32_t) word << 16;
}

/// Get the hi word (16bit) in a dword (32 bit)
inline uint_least16_t endian_32hi16 (uint_least32_t dword)
{
    return (uint_least16_t) (dword >> 16);
}

/// Set the lo byte (8 bit) in a dword (32 bit)
inline void endian_32lo8 (uint_least32_t &dword, uint8_t byte)
{
    dword &= (uint_least32_t) 0xffffff00;
    dword |= (uint_least32_t) byte;
}

/// Get the lo byte (8 bit) in a dword (32 bit)
inline uint8_t endian_32lo8 (uint_least32_t dword)
{
    return (uint8_t) dword;
}

/// Set the hi byte (8 bit) in a dword (32 bit)
inline void endian_32hi8 (uint_least32_t &dword, uint8_t byte)
{
    dword &= (uint_least32_t) 0xffff00ff;
    dword |= (uint_least32_t) byte << 8;
}

/// Get the hi byte (8 bit) in a dword (32 bit)
inline uint8_t endian_32hi8 (uint_least32_t dword)
{
    return (uint8_t) (dword >> 8);
}

/// Swap hi and lo words endian in 32 bit dword.
inline void endian_32swap16 (uint_least32_t &dword)
{
    uint_least16_t lo = endian_32lo16 (dword);
    uint_least16_t hi = endian_32hi16 (dword);
    endian_32lo16 (dword, hi);
    endian_32hi16 (dword, lo);
}

/// Swap word endian.
inline void endian_32swap8 (uint_least32_t &dword)
{
    uint_least16_t lo = 0, hi = 0;
    lo = endian_32lo16 (dword);
    hi = endian_32hi16 (dword);
    endian_16swap8 (lo);
    endian_16swap8 (hi);
    endian_32lo16 (dword, hi);
    endian_32hi16 (dword, lo);
}

/// Convert high-byte and low-byte to 32-bit word.
inline uint_least32_t endian_32 (uint8_t hihi, uint8_t hilo, uint8_t hi, uint8_t lo)
{
    uint_least32_t dword = 0;
    uint_least16_t word  = 0;
    endian_32lo8  (dword, lo);
    endian_32hi8  (dword, hi);
    endian_16lo8  (word,  hilo);
    endian_16hi8  (word,  hihi);
    endian_32hi16 (dword, word);
    return dword;
}

/// Convert high-byte and low-byte to 32-bit little endian word.
inline uint_least32_t endian_little32 (const uint8_t ptr[4])
{
    return endian_32 (ptr[3], ptr[2], ptr[1], ptr[0]);
}

/// Write a little-endian 32-bit word to four bytes in memory.
inline void endian_little32 (uint8_t ptr[4], uint_least32_t dword)
{
    uint_least16_t word = 0;
    ptr[0] = endian_32lo8  (dword);
    ptr[1] = endian_32hi8  (dword);
    word   = endian_32hi16 (dword);
    ptr[2] = endian_16lo8  (word);
    ptr[3] = endian_16hi8  (word);
}

/// Convert high-byte and low-byte to 32-bit big endian word.
inline uint_least32_t endian_big32 (const uint8_t ptr[4])
{
    return endian_32 (ptr[0], ptr[1], ptr[2], ptr[3]);
}

/// Write a big-endian 32-bit word to four bytes in memory.
inline void endian_big32 (uint8_t ptr[4], uint_least32_t dword)
{
    uint_least16_t word = 0;
    word   = endian_32hi16 (dword);
    ptr[1] = endian_16lo8  (word);
    ptr[0] = endian_16hi8  (word);
    ptr[2] = endian_32hi8  (dword);
    ptr[3] = endian_32lo8  (dword);
}

#endif // SIDENDIAN_H
