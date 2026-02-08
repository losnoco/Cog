/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#pragma once

#include <snes9x/cpuexec.h>

void addCyclesInMemoryAccess(struct S9xState *st, int32_t speed);
void addCyclesInMemoryAccess_x2(struct S9xState *st, int32_t speed);
int32_t memory_speed(struct S9xState *st, uint32_t address);
uint8_t S9xGetByte(struct S9xState *st, uint32_t Address);
uint16_t S9xGetWord(struct S9xState *st, uint32_t Address, s9xwrap_t w = WRAP_NONE);
void S9xSetByte(struct S9xState *st, uint8_t Byt, uint32_t Address);
void S9xSetWord(struct S9xState *st, uint16_t Word, uint32_t Address, s9xwrap_t w = WRAP_NONE, s9xwriteorder_t o = WRITE_01);
void S9xSetPCBase(struct S9xState *st, uint32_t Address);
uint8_t *S9xGetBasePointer(struct S9xState *st, uint32_t Address);
uint8_t *S9xGetMemPointer(struct S9xState *st, uint32_t Address);
