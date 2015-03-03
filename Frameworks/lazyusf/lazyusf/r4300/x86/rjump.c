/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - rjump.c                                                 *
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

#include <stdlib.h>

#include "usf/usf.h"

#include "usf/usf_internal.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "r4300/cached_interp.h"
#include "r4300/recomp.h"
#include "r4300/r4300.h"
#include "r4300/macros.h"
#include "r4300/ops.h"
#include "r4300/recomph.h"

void dyna_jump(usf_state_t * state)
{
    if (state->stop == 1)
    {
        dyna_stop(state);
        return;
    }

    if (state->PC->reg_cache_infos.need_map)
        *state->return_address = (unsigned long) (state->PC->reg_cache_infos.jump_wrapper);
    else
        *state->return_address = (unsigned long) (state->actual->code + state->PC->local_addr);
}

#if defined(WIN32) && !defined(__GNUC__) /* this warning disable only works if placed outside of the scope of a function */
#pragma warning(disable:4731) /* frame pointer register 'ebp' modified by inline assembly code */
#endif

void dyna_start(usf_state_t * state, void *code)
{
  /* save the base and stack pointers */
  /* make a call and a pop to retrieve the instruction pointer and save it too */
  /* then call the code(), which should theoretically never return.  */
  /* When dyna_stop() sets the *return_address to the saved EIP, the emulator thread will come back here. */
  /* It will jump to label 2, restore the base and stack pointers, and exit this function */
#if defined(WIN32) && !defined(__GNUC__)
   __asm
   {
     push ebp
     push ebx
     push esi
     push edi
	 mov ebx, code
	 mov esi, state
     mov [esi].save_esp, esp
     call point1
     jmp point2
   point1:
     pop eax
     mov [esi].save_eip, eax

     sub esp, 0x10
     and esp, 0xfffffff0
	 mov [esi].return_address, esp
	 sub [esi].return_address, 4

     call ebx

   point2:
     mov esp, [esi].save_esp
     pop edi
     pop esi
     pop ebx
     pop ebp
   }
#elif defined(__GNUC__) && defined(__i386__)
  #if defined(__PIC__) && !defined(__APPLE__)
    /* for -fPIC (shared libraries) */
    #ifndef __GNUC_PREREQ
    #  if defined __GNUC__ && defined __GNUC_MINOR__
    #    define __GNUC_PREREQ(maj, min) \
                ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
    #  else
    #    define __GNUC_PREREQ(maj, min) 0
    #  endif
    #endif

    #if __GNUC_PREREQ (4, 7)
    #  define GET_PC_THUNK_STR(reg) "__x86.get_pc_thunk." #reg
    #else
    #  define GET_PC_THUNK_STR(reg) "__i686.get_pc_thunk." #reg
    #endif
    #define STORE_EBX
    #define LOAD_EBX "call  " GET_PC_THUNK_STR(bx) "     \n" \
                     "addl $_GLOBAL_OFFSET_TABLE_, %%ebx \n"
  #else
    /* for non-PIC binaries */
    #define STORE_EBX "pushl %%ebx \n"
    #define LOAD_EBX  "popl %%ebx \n"
  #endif

  asm volatile
    (" pushl %%ebp \n"
     STORE_EBX
     " pushl %%esi \n"
     " pushl %%edi \n"
     " movl %[state], %%esi \n"
     " movl %%esp, %c[save_esp](%%esi) \n"
     " call    1f              \n"
     " jmp     2f              \n"
     "1:                       \n"
     " popl %%eax              \n"
     " movl %%eax, %c[save_eip](%%esi) \n"

     " subl $16, %%esp         \n" /* save 16 bytes of padding just in case */
     " andl $-16, %%esp        \n" /* align stack on 16-byte boundary for OSX */
     " movl %%esp, %c[return_address](%%esi) \n"
     " subl $4, %c[return_address](%%esi) \n"

     " call *%[codeptr]        \n"
     "2:                       \n"
     " movl %c[save_esp](%%esi), %%esp \n"
     " popl %%edi \n"
     " popl %%esi \n"
     LOAD_EBX
     " popl %%ebp \n"
     :
     : [save_esp]"i"(offsetof(usf_state_t,save_esp)), [save_eip]"i"(offsetof(usf_state_t,save_eip)), [return_address]"i"(offsetof(usf_state_t,return_address)), [codeptr]"r"(code), [state]"r"(state)
     : "eax", "ecx", "edx", "esi", "memory"
     );
#endif

    /* clear the registers so we don't return here a second time; that would be a bug */
    /* this is also necessary to prevent compiler from optimizing out the static variables */
    state->save_esp=0;
    state->save_eip=0;
}

void dyna_stop(usf_state_t * state)
{
  if (state->save_eip == 0)
    DebugMessage(state, M64MSG_WARNING, "instruction pointer is 0 at dyna_stop()");
  else
  {
    *state->return_address = (unsigned long) state->save_eip;
  }
}

