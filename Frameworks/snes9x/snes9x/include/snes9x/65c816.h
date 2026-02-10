/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#pragma once

inline constexpr uint16_t Carry = 1;
inline constexpr uint16_t Zero = 2;
inline constexpr uint16_t IRQ = 4;
inline constexpr uint16_t Decimal = 8;
inline constexpr uint16_t IndexFlag = 16;
inline constexpr uint16_t MemoryFlag = 32;
inline constexpr uint16_t Overflow = 64;
inline constexpr uint16_t Negative = 128;
inline constexpr uint16_t Emulation = 256;

union pair
{
#ifdef LSB_FIRST
	struct { uint8_t l, h; } B;
#else
	struct { uint8_t h, l; } B;
#endif
	uint16_t W;
};

union PC_t
{
#ifdef LSB_FIRST
	struct { uint8_t xPCl, xPCh, xPB, z; } B;
	struct { uint16_t xPC, d; } W;
#else
	struct { uint8_t z, xPB, xPCh, xPCl; } B;
	struct { uint16_t d, xPC; } W;
#endif
	uint32_t xPBPC;
};

struct SRegisters
{
	uint8_t DB;
	pair P;
	pair A;
	pair D;
	pair S;
	pair X;
	pair Y;
	PC_t PC;
};

// cpu.cpp
void SetCarry(struct S9xState *st);
void ClearCarry(struct S9xState *st);
void SetIRQ(struct S9xState *st);
void ClearIRQ(struct S9xState *st);
void SetDecimal(struct S9xState *st);
void ClearDecimal(struct S9xState *st);
void SetOverflow(struct S9xState *st);
void ClearOverflow(struct S9xState *st);

bool CheckCarry(struct S9xState *st);
bool CheckZero(struct S9xState *st);
bool CheckDecimal(struct S9xState *st);
bool CheckIndex(struct S9xState *st);
bool CheckMemory(struct S9xState *st);
bool CheckOverflow(struct S9xState *st);
bool CheckNegative(struct S9xState *st);
bool CheckEmulation(struct S9xState *st);

void SetFlags(struct S9xState *st, uint16_t f);
void ClearFlags(struct S9xState *st, uint16_t f);
bool CheckFlag(struct S9xState *st, uint16_t f);
