/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#pragma once

#include <snes9x/ppu.h>

struct SOpcodes
{
	void (*S9xOpcode)(struct S9xState *st);
};

struct SICPU
{
	const SOpcodes *S9xOpcodes;
	const uint8_t *S9xOpLengths;
	uint8_t _Carry;
	uint8_t _Zero;
	uint8_t _Negative;
	uint8_t _Overflow;
	uint32_t ShiftedPB;
	uint32_t ShiftedDB;
};

extern const SOpcodes S9xOpcodesE1[256];
extern const SOpcodes S9xOpcodesM1X1[256];
extern const SOpcodes S9xOpcodesM1X0[256];
extern const SOpcodes S9xOpcodesM0X1[256];
extern const SOpcodes S9xOpcodesM0X0[256];
extern const SOpcodes S9xOpcodesSlow[256];
extern const uint8_t S9xOpLengthsM1X1[256];
extern const uint8_t S9xOpLengthsM1X0[256];
extern const uint8_t S9xOpLengthsM0X1[256];
extern const uint8_t S9xOpLengthsM0X0[256];

void CHECK_FOR_IRQ_CHANGE(struct S9xState *st);

void S9xMainLoop(struct S9xState *st);
void S9xReset(struct S9xState *st);
void S9xDoHEventProcessing(struct S9xState *st);

#include <snes9x/65c816.h>

void S9xUnpackStatus(struct S9xState *st);
void S9xPackStatus(struct S9xState *st);
void S9xFixCycles(struct S9xState *st);
