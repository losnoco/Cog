/*
 *  Copyright (C) 2002-2009  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef _MSC_VER
#define INLINE __forceinline
#define DB_FASTCALL __fastcall
#else
#define INLINE inline
#define DB_FASTCALL __attribute__((fastcall))
#endif

typedef         double		Real64;
/* The internal types */

#ifdef HAVE_STDINT_H
#include <stdint.h>

typedef uint8_t         	Bit8u;
typedef int8_t         		Bit8s;
typedef uint16_t        	Bit16u;
typedef int16_t     		Bit16s;
typedef uint32_t    		Bit32u;
typedef int32_t     		Bit32s;
typedef uint64_t        	Bit64u;
typedef int64_t         	Bit64s;
#else
typedef  unsigned char		Bit8u;
typedef    signed char		Bit8s;
typedef unsigned short		Bit16u;
typedef   signed short		Bit16s;
typedef  unsigned int		Bit32u;
typedef    signed int		Bit32s;
#ifdef _MSC_VER
typedef unsigned __int64	Bit64u;
typedef   signed __int64	Bit64s;
#else
typedef unsigned long long  Bit64u;
typedef   signed long long  Bit64s;
#endif
#endif

typedef unsigned int		Bitu;
typedef signed int			Bits;

Bitu Chip_GetSize();

void Chip_Init( void *chip );
void Chip_Setup( void *chip, Bit32u clock, Bit32u rate );

void Chip_WriteReg( void *chip, Bit32u reg, Bit8u val );
Bit32u Chip_WriteAddr( void *chip, Bit32u port, Bit8u val );

void Chip_GenerateBlock_Mono( void *chip, Bitu total, Bit32s* output );
void Chip_GenerateBlock_Stereo( void *chip, Bitu total, Bit32s* output );
