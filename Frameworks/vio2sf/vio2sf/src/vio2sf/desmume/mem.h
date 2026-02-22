/*
	Copyright (C) 2005 Theo Berkau
	Copyright (C) 2005-2006 Guillaume Duhamel
	Copyright (C) 2008-2010 DeSmuME team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <cstdlib>
#include <cassert>
#include <vio2sf/types.h>

// this was originally declared in MMU.h but we suffered some organizational problems and had to remove it
enum MMU_ACCESS_TYPE
{
	MMU_AT_CODE, // used for cpu prefetches
	MMU_AT_DATA, // used for cpu read/write
	MMU_AT_GPU, // used for gpu read/write
	MMU_AT_DMA, // used for dma read/write (blocks access to TCM)
	MMU_AT_DEBUG // used for emulator debugging functions (bypasses some debug handling)
};

inline uint8_t T1ReadByte(const uint8_t *const mem, uint32_t addr)
{
	return mem[addr];
}

inline uint16_t T1ReadWord_guaranteedAligned(const uint8_t *const mem, uint32_t addr)
{
	assert(!(addr & 1));
#ifdef WORDS_BIGENDIAN
	return (mem[addr + 1] << 8) | mem[addr];
#else
	return *reinterpret_cast<const uint16_t *>(mem + addr);
#endif
}

inline uint16_t T1ReadWord(const uint8_t *const mem, uint32_t addr)
{
#ifdef WORDS_BIGENDIAN
	return (mem[addr + 1] << 8) | mem[addr];
#else
	return *reinterpret_cast<const uint16_t *>(mem + addr);
#endif
}

inline uint32_t T1ReadLong_guaranteedAligned(const uint8_t *const mem, uint32_t addr)
{
	assert(!(addr & 3));
#ifdef WORDS_BIGENDIAN
	return (mem[addr + 3] << 24) | (mem[addr + 2] << 16) | (mem[addr + 1] << 8) | mem[addr];
#else
	return *reinterpret_cast<const uint32_t *>(mem + addr);
#endif
}

inline uint32_t T1ReadLong(const uint8_t *const mem, uint32_t addr)
{
	addr &= ~3;
#ifdef WORDS_BIGENDIAN
	return (mem[addr + 3] << 24) | (mem[addr + 2] << 16) | (mem[addr + 1] << 8) | mem[addr];
#else
	return *reinterpret_cast<const uint32_t *>(mem + addr);
#endif
}

inline uint64_t T1ReadQuad(const uint8_t *const mem, uint32_t addr)
{
#ifdef WORDS_BIGENDIAN
	return (mem[addr + 7] << 56) | (mem[addr + 6] << 48) | (mem[addr + 5] << 40) | (mem[addr + 4] << 32) | (mem[addr + 3] << 24) | (mem[addr + 2] << 16) | (mem[addr + 1] << 8)  | mem[addr];
#else
	return *reinterpret_cast<const uint64_t *>(mem + addr);
#endif
}

inline void T1WriteByte(uint8_t *const mem, uint32_t addr, uint8_t val)
{
	mem[addr] = val;
}

inline void T1WriteWord(uint8_t *const mem, uint32_t addr, uint16_t val)
{
#ifdef WORDS_BIGENDIAN
	mem[addr + 1] = val >> 8;
	mem[addr] = val & 0xFF;
#else
	*reinterpret_cast<uint16_t *>(mem + addr) = val;
#endif
}

inline void T1WriteLong(uint8_t *const mem, uint32_t addr, uint32_t val)
{
#ifdef WORDS_BIGENDIAN
	mem[addr + 3] = val >> 24;
	mem[addr + 2] = (val >> 16) & 0xFF;
	mem[addr + 1] = (val >> 8) & 0xFF;
	mem[addr] = val & 0xFF;
#else
	*reinterpret_cast<uint32_t *>(mem + addr) = val;
#endif
}

inline void T1WriteQuad(uint8_t *const mem, uint32_t addr, uint64_t val)
{
#ifdef WORDS_BIGENDIAN
	mem[addr + 7] = val >> 56;
	mem[addr + 6] = (val >> 48) & 0xFF;
	mem[addr + 5] = (val >> 40) & 0xFF;
	mem[addr + 4] = (val >> 32) & 0xFF;
	mem[addr + 3] = (val >> 24) & 0xFF;
	mem[addr + 2] = (val >> 16) & 0xFF;
	mem[addr + 1] = (val >> 8) & 0xFF;
	mem[addr] = val & 0xFF;
#else
	*reinterpret_cast<uint64_t *>(mem + addr) = val;
#endif
}
