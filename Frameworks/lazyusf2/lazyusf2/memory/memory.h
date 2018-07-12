/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - memory.h                                                *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef M64P_MEMORY_MEMORY_H
#define M64P_MEMORY_MEMORY_H

#include <stdint.h>

#define read_word_in_memory() state->readmem[state->address>>16](state)
#define read_byte_in_memory() state->readmemb[state->address>>16](state)
#define read_hword_in_memory() state->readmemh[state->address>>16](state)
#define read_dword_in_memory() state->readmemd[state->address>>16](state)
#define write_word_in_memory() state->writemem[state->address>>16](state)
#define write_byte_in_memory() state->writememb[state->address >>16](state)
#define write_hword_in_memory() state->writememh[state->address >>16](state)
#define write_dword_in_memory() state->writememd[state->address >>16](state)

#ifndef M64P_BIG_ENDIAN
#if defined(__GNUC__) && (__GNUC__ > 4  || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))
#define sl(x) __builtin_bswap32(x)
#else
#define sl(mot) \
( \
((mot & 0x000000FF) << 24) | \
((mot & 0x0000FF00) <<  8) | \
((mot & 0x00FF0000) >>  8) | \
((mot & 0xFF000000) >> 24) \
)
#endif
#define S8 3
#define S16 2
#define Sh16 1

#else

#define sl(mot) mot
#define S8 0
#define S16 0
#define Sh16 0

#endif

#include "osal/preproc.h"

static osal_inline void masked_write(uint32_t* dst, uint32_t value, uint32_t mask)
{
    *dst = (*dst & ~mask) | (value & mask);
}

int init_memory(usf_state_t *, uint32_t rdram_size);

void map_region(usf_state_t *,
                uint16_t region,
                int type,
				void (osal_fastcall *read8)(usf_state_t *),
				void (osal_fastcall *read16)(usf_state_t *),
				void (osal_fastcall *read32)(usf_state_t *),
				void (osal_fastcall *read64)(usf_state_t *),
				void (osal_fastcall *write8)(usf_state_t *),
				void (osal_fastcall *write16)(usf_state_t *),
				void (osal_fastcall *write32)(usf_state_t *),
				void (osal_fastcall *write64)(usf_state_t *));

/* XXX: cannot make them static because of dynarec + rdp fb */
void osal_fastcall read_rdram(usf_state_t *);
void osal_fastcall read_rdramb(usf_state_t *);
void osal_fastcall read_rdramh(usf_state_t *);
void osal_fastcall read_rdramd(usf_state_t *);
void osal_fastcall write_rdram(usf_state_t *);
void osal_fastcall write_rdramb(usf_state_t *);
void osal_fastcall write_rdramh(usf_state_t *);
void osal_fastcall write_rdramd(usf_state_t *);
void osal_fastcall read_rdramFB(usf_state_t *);
void osal_fastcall read_rdramFBb(usf_state_t *);
void osal_fastcall read_rdramFBh(usf_state_t *);
void osal_fastcall read_rdramFBd(usf_state_t *);
void osal_fastcall write_rdramFB(usf_state_t *);
void osal_fastcall write_rdramFBb(usf_state_t *);
void osal_fastcall write_rdramFBh(usf_state_t *);
void osal_fastcall write_rdramFBd(usf_state_t *);

/* Returns a pointer to a block of contiguous memory
 * Can access RDRAM, SP_DMEM, SP_IMEM and ROM, using TLB if necessary
 * Useful for getting fast access to a zone with executable code. */
unsigned int * osal_fastcall fast_mem_access(usf_state_t *, unsigned int address);

#endif

