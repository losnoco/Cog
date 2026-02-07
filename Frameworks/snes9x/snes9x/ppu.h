/***********************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.

  (c) Copyright 1996 - 2002  Gary Henderson (gary.henderson@ntlworld.com),
                             Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2002 - 2004  Matthew Kendora

  (c) Copyright 2002 - 2005  Peter Bortas (peter@bortas.org)

  (c) Copyright 2004 - 2005  Joel Yliluoma (http://iki.fi/bisqwit/)

  (c) Copyright 2001 - 2006  John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2006  funkyass (funkyass@spam.shaw.ca),
                             Kris Bleakley (codeviolation@hotmail.com)

  (c) Copyright 2002 - 2010  Brad Jorsch (anomie@users.sourceforge.net),
                             Nach (n-a-c-h@users.sourceforge.net),

  (c) Copyright 2002 - 2011  zones (kasumitokoduck@yahoo.com)

  (c) Copyright 2006 - 2007  nitsuja

  (c) Copyright 2009 - 2011  BearOso,
                             OV2


  BS-X C emulator code
  (c) Copyright 2005 - 2006  Dreamer Nom,
                             zones

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003  _Demo_ (_demo_@zsnes.com),
                             Nach,
                             zsKnight (zsknight@zsnes.com)

  C4 C++ code
  (c) Copyright 2003 - 2006  Brad Jorsch,
                             Nach

  DSP-1 emulator code
  (c) Copyright 1998 - 2006  _Demo_,
                             Andreas Naive (andreasnaive@gmail.com),
                             Gary Henderson,
                             Ivar (ivar@snes9x.com),
                             John Weidman,
                             Kris Bleakley,
                             Matthew Kendora,
                             Nach,
                             neviksti (neviksti@hotmail.com)

  DSP-2 emulator code
  (c) Copyright 2003         John Weidman,
                             Kris Bleakley,
                             Lord Nightmare (lord_nightmare@users.sourceforge.net),
                             Matthew Kendora,
                             neviksti

  DSP-3 emulator code
  (c) Copyright 2003 - 2006  John Weidman,
                             Kris Bleakley,
                             Lancer,
                             z80 gaiden

  DSP-4 emulator code
  (c) Copyright 2004 - 2006  Dreamer Nom,
                             John Weidman,
                             Kris Bleakley,
                             Nach,
                             z80 gaiden

  OBC1 emulator code
  (c) Copyright 2001 - 2004  zsKnight,
                             pagefault (pagefault@zsnes.com),
                             Kris Bleakley
                             Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code used in 1.39-1.51
  (c) Copyright 2002         Matthew Kendora with research by
                             zsKnight,
                             John Weidman,
                             Dark Force

  SPC7110 and RTC C++ emulator code used in 1.52+
  (c) Copyright 2009         byuu,
                             neviksti

  S-DD1 C emulator code
  (c) Copyright 2003         Brad Jorsch with research by
                             Andreas Naive,
                             John Weidman

  S-RTC C emulator code
  (c) Copyright 2001 - 2006  byuu,
                             John Weidman

  ST010 C++ emulator code
  (c) Copyright 2003         Feather,
                             John Weidman,
                             Kris Bleakley,
                             Matthew Kendora

  Super FX x86 assembler emulator code
  (c) Copyright 1998 - 2003  _Demo_,
                             pagefault,
                             zsKnight

  Super FX C emulator code
  (c) Copyright 1997 - 1999  Ivar,
                             Gary Henderson,
                             John Weidman

  Sound emulator code used in 1.5-1.51
  (c) Copyright 1998 - 2003  Brad Martin
  (c) Copyright 1998 - 2006  Charles Bilyue'

  Sound emulator code used in 1.52+
  (c) Copyright 2004 - 2007  Shay Green (gblargg@gmail.com)

  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004  Marcus Comstedt (marcus@mc.pp.se)

  2xSaI filter
  (c) Copyright 1999 - 2001  Derek Liauw Kie Fa

  HQ2x, HQ3x, HQ4x filters
  (c) Copyright 2003         Maxim Stepin (maxim@hiend3d.com)

  NTSC filter
  (c) Copyright 2006 - 2007  Shay Green

  GTK+ GUI code
  (c) Copyright 2004 - 2011  BearOso

  Win32 GUI code
  (c) Copyright 2003 - 2006  blip,
                             funkyass,
                             Matthew Kendora,
                             Nach,
                             nitsuja
  (c) Copyright 2009 - 2011  OV2

  Mac OS GUI code
  (c) Copyright 1998 - 2001  John Stiles
  (c) Copyright 2001 - 2011  zones


  Specific ports contains the works of other authors. See headers in
  individual files.


  Snes9x homepage: http://www.snes9x.com/

  Permission to use, copy, modify and/or distribute Snes9x in both binary
  and source form, for non-commercial purposes, is hereby granted without
  fee, providing that this license information and copyright notice appear
  with all copies and any derived work.

  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software or it's derivatives.

  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes,
  but is not limited to, charging money for Snes9x or software derived from
  Snes9x, including Snes9x or derivatives in commercial game bundles, and/or
  using Snes9x as a promotion for your commercial product.

  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.

  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
 ***********************************************************************************/

#pragma once

const int32_t FIRST_VISIBLE_LINE = 1;

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

const size_t MAX_2BIT_TILES = 4096;
const size_t MAX_4BIT_TILES = 2048;
const size_t MAX_8BIT_TILES = 1024;

struct InternalPPU
{
	uint16_t VRAMReadBuffer;
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
};

void S9xResetPPU(struct S9xState *st);
void S9xSoftResetPPU(struct S9xState *st);
void S9xSetPPU(struct S9xState *st, uint8_t, uint16_t);
uint8_t S9xGetPPU(struct S9xState *st, uint16_t);
void S9xSetCPU(struct S9xState *st, uint8_t, uint16_t);
uint8_t S9xGetCPU(struct S9xState *st, uint16_t);
void S9xUpdateHVTimerPosition(struct S9xState *st);

struct SnesModel
{
	uint8_t _5C77;
	uint8_t _5C78;
	uint8_t _5A22;
};

void REGISTER_2104(struct S9xState *st, uint8_t Byt);
bool CHECK_INBLANK(struct S9xState *st);
void REGISTER_2118(struct S9xState *st, uint8_t Byt);
void REGISTER_2119(struct S9xState *st, uint8_t Byt);
void REGISTER_2118_tile(struct S9xState *st, uint8_t Byt);
void REGISTER_2119_tile(struct S9xState *st, uint8_t Byt);
void REGISTER_2118_linear(struct S9xState *st, uint8_t Byt);
void REGISTER_2119_linear(struct S9xState *st, uint8_t Byt);
void REGISTER_2180(struct S9xState *st, uint8_t Byt);
uint8_t REGISTER_4212(struct S9xState *st);