/******************************************************************************\
* Authors:  Iconoclast                                                         *
* Release:  2013.12.12                                                         *
* License:  CC0 Public Domain Dedication                                       *
*                                                                              *
* To the extent possible under law, the author(s) have dedicated all copyright *
* and related and neighboring rights to this software to the public domain     *
* worldwide. This software is distributed without any warranty.                *
*                                                                              *
* You should have received a copy of the CC0 Public Domain Dedication along    *
* with this software.                                                          *
* If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.             *
\******************************************************************************/
#ifndef _RSP_H_
#define _RSP_H_

#ifdef _MSC_VER
#define INLINE      __forceinline
#define NOINLINE    __declspec(noinline)
#define ALIGNED     _declspec(align(16))
#else
#define INLINE      inline __attribute__((always_inline))
#define NOINLINE    __attribute__((noinline))
#define ALIGNED     __attribute__((aligned(16)))
#endif

/*
 * Streaming SIMD Extensions version import management
 */
#ifdef ARCH_MIN_SSSE3
#define ARCH_MIN_SSE2
#include <tmmintrin.h>
#endif
#ifdef ARCH_MIN_SSE2
#include <emmintrin.h>
#endif
#ifdef ARCH_MIN_ARM_NEON
#include <arm_neon.h>
#endif

typedef unsigned char byte;

#ifndef RCPREG_DEFINED
#define RCPREG_DEFINED
typedef uint32_t RCPREG;
#endif

NOINLINE static void message(usf_state_t * state, const char* body, int priority)
{
    (void)body;
    (void)priority;
    if ( priority > 1 )
        DebugMessage( state, 5, "%s", body );
}

/*
 * Update RSP configuration memory from local file resource.
 */
#define CHARACTERS_PER_LINE     (80)
/* typical standard DOS text file limit per line */
NOINLINE static void update_conf(const char* source)
{
    (void)source;
}

#ifdef SP_EXECUTE_LOG
extern void step_SP_commands(usf_state_t * state, int PC, uint32_t inst);
#endif

#include "su.h"
#include "vu/vu.h"

int32_t init_rsp_lle(usf_state_t * state);

/* Allocate the RSP CPU loop to its own functional space. */
NOINLINE static void run_task(usf_state_t * state);
#include "execute.h"

#ifdef SP_EXECUTE_LOG
#include "matrix.h"

void step_SP_commands(usf_state_t * state, int PC, uint32_t inst)
{
    const char digits[16] = {
        '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
    };
    char text[256];
    char offset[4] = "";
    char code[9] = "";
    char disasm[24];
    unsigned char endian_swap[4];

    endian_swap[00] = (unsigned char)(inst >> 24);
    endian_swap[01] = (unsigned char)(inst >> 16);
    endian_swap[02] = (unsigned char)(inst >>  8);
    endian_swap[03] = (unsigned char)inst;
    offset[00] = digits[(PC & 0xF00) >> 8];
    offset[01] = digits[(PC & 0x0F0) >> 4];
    offset[02] = digits[(PC & 0x00F) >> 0];
    code[00] = digits[(inst & 0xF0000000) >> 28];
    code[01] = digits[(inst & 0x0F000000) >> 24];
    code[02] = digits[(inst & 0x00F00000) >> 20];
    code[03] = digits[(inst & 0x000F0000) >> 16];
    code[04] = digits[(inst & 0x0000F000) >> 12];
    code[05] = digits[(inst & 0x00000F00) >>  8];
    code[06] = digits[(inst & 0x000000F0) >>  4];
    code[07] = digits[(inst & 0x0000000F) >>  0];
    strcpy(text, "RSP:\t");
    strcat(text, offset);
    strcat(text, ":\t");
    strcat(text, code);
    strcat(text, "\t");
    disassemble(disasm, inst);
    strcat(text, disasm);
    strcat(text, "\n");
    fputs(text, stdout);
}
#endif

#endif
