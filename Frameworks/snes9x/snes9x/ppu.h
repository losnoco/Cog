/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#pragma once

inline constexpr int32_t FIRST_VISIBLE_LINE = 1;

enum
{
	TILE_2BIT,
	TILE_4BIT,
	TILE_8BIT,
	TILE_2BIT_EVEN,
	TILE_2BIT_ODD,
	TILE_4BIT_EVEN,
	TILE_4BIT_ODD
};

inline constexpr size_t MAX_2BIT_TILES = 4096;
inline constexpr size_t MAX_4BIT_TILES = 2048;
inline constexpr size_t MAX_8BIT_TILES = 1024;

struct InternalPPU
{
	bool Interlace;
	uint32_t Red[256];
	uint32_t Green[256];
	uint32_t Blue[256];
};

struct SPPU
{
	struct
	{
		bool High;
		uint8_t Increment;
		uint16_t Address;
		uint16_t Mask1;
		uint16_t FullGraphicCount;
		uint16_t Shift;
	} VMA;

	uint32_t WRAM;

	bool CGFLIPRead;
	uint8_t CGADD;
	uint16_t CGDATA[256];

	uint16_t OAMAddr;
	uint16_t SavedOAMAddr;
	bool OAMPriorityRotation;
	uint8_t OAMFlip;
	uint16_t OAMWriteRegister;
	uint8_t OAMData[512 + 32];

	uint8_t FirstSprite;

	bool HTimerEnabled;
	bool VTimerEnabled;
	short HTimerPosition;
	short VTimerPosition;
	uint16_t IRQHBeamPos;
	uint16_t IRQVBeamPos;

	bool HBeamFlip;
	bool VBeamFlip;

	short MatrixA;
	short MatrixB;

	bool ForcedBlanking;

	uint16_t ScreenHeight;

	bool Need16x8Mulitply;
	uint8_t M7byte;

	uint8_t HDMA;
	uint8_t HDMAEnded;

	uint8_t OpenBus1;
	uint8_t OpenBus2;

	uint16_t VRAMReadBuffer;
};

void S9xResetPPU(struct S9xState *st);
void S9xSoftResetPPU(struct S9xState *st);
void S9xSetPPU(struct S9xState *st, uint8_t, uint16_t);
uint8_t S9xGetPPU(struct S9xState *st, uint16_t);
void S9xSetCPU(struct S9xState *st, uint8_t, uint16_t);
uint8_t S9xGetCPU(struct S9xState *st, uint16_t);
void S9xUpdateIRQPositions(struct S9xState *st, bool);

struct SnesModel
{
	uint8_t _5C77;
	uint8_t _5C78;
	uint8_t _5A22;
};

void S9xUpdateVRAMReadBuffer(struct S9xState *st);

void REGISTER_2104(struct S9xState *st, uint8_t Byt);
void REGISTER_2118(struct S9xState *st, uint8_t Byt);
void REGISTER_2118_tile(struct S9xState *st, uint8_t Byt);
void REGISTER_2118_linear(struct S9xState *st, uint8_t Byt);
void REGISTER_2119(struct S9xState *st, uint8_t Byt);
void REGISTER_2119_tile(struct S9xState *st, uint8_t Byt);
void REGISTER_2119_linear(struct S9xState *st, uint8_t Byt);
void REGISTER_2180(struct S9xState *st, uint8_t Byt);
uint8_t REGISTER_4212(struct S9xState *st);
