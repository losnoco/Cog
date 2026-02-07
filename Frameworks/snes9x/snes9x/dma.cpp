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

#include "snes9x.h"

static inline void ADD_CYCLES(struct S9xState *st, int32_t n) { st->CPU.PrevCycles = st->CPU.Cycles; st->CPU.Cycles += n; S9xCheckInterrupts(st); }

// globals.cpp
extern const int HDMA_ModeByteCounts[8];

static inline bool addCyclesInDMA(struct S9xState *st, uint8_t dma_channel)
{
	// Add 8 cycles per byte, sync APU, and do HC related events.
	// If HDMA was done in S9xDoHEventProcessing(), check if it used the same channel as DMA.
	ADD_CYCLES(st, SLOW_ONE_CYCLE);
	while (st->CPU.Cycles >= st->CPU.NextEvent)
		S9xDoHEventProcessing(st);

	if (st->CPU.HDMARanInDMA & (1 << dma_channel))
	{
		st->CPU.HDMARanInDMA = 0;
		// If HDMA triggers in the middle of DMA transfer and it uses the same channel,
		// it kills the DMA transfer immediately. $43x2 and $43x5 stop updating.
		return false;
	}

	st->CPU.HDMARanInDMA = 0;
	return true;
}

bool S9xDoDMA(struct S9xState *st, uint8_t Channel)
{
	st->CPU.InDMA = st->CPU.InDMAorHDMA = true;
	st->CPU.CurrentDMAorHDMAChannel = Channel;

	SDMA *d = &st->DMA[Channel];

	// Check invalid DMA first
	if ((d->ABank == 0x7E || d->ABank == 0x7F) && d->BAddress == 0x80 && !d->ReverseTransfer)
	{
		// Attempting a DMA from WRAM to $2180 will not work, WRAM will not be written.
		// Attempting a DMA from $2180 to WRAM will similarly not work,
		// the value written is (initially) the OpenBus value.
		// In either case, the address in $2181-3 is not incremented.

		// Does an invalid DMA actually take time?
		// I'd say yes, since 'invalid' is probably just the WRAM chip
		// not being able to read and write itself at the same time
		// And no, PPU.WRAM should not be updated.

		int32_t c = d->DMACount_Or_HDMAIndirectAddress;
		// Writing $0000 to $43x5 actually results in a transfer of $10000 bytes, not 0.
		if (!c)
			c = 0x10000;

		// 8 cycles per channel
		ADD_CYCLES(st, SLOW_ONE_CYCLE);
		// 8 cycles per byte
		while (c)
		{
			--d->DMACount_Or_HDMAIndirectAddress;
			++d->AAddress;
			--c;
			if (!addCyclesInDMA(st, Channel))
			{
				st->CPU.InDMA = false;
				st->CPU.InDMAorHDMA = false;
				st->CPU.CurrentDMAorHDMAChannel = -1;
				return false;
			}
		}

		st->CPU.InDMA = false;
		st->CPU.InDMAorHDMA = false;
		st->CPU.CurrentDMAorHDMAChannel = -1;
		return true;
	}

	// Prepare for accessing $2118-2119

	int32_t inc = d->AAddressFixed ? 0 : (!d->AAddressDecrement ? 1 : -1);
	int32_t count = d->DMACount_Or_HDMAIndirectAddress;
	// Writing $0000 to $43x5 actually results in a transfer of $10000 bytes, not 0.
	if (!count)
		count = 0x10000;

	// Prepare for custom chip DMA

	// S-DD1

	uint8_t *in_sdd1_dma = nullptr;

	if (st->Settings.SDD1)
	{
		if (d->AAddressFixed && st->Memory.FillRAM[0x4801] > 0)
		{
			// XXX: Should probably verify that we're DMAing from ROM?
			// And somewhere we should make sure we're not running across a mapping boundary too.
			// Hacky support for pre-decompressed S-DD1 data
			inc = !d->AAddressDecrement ? 1 : -1;

			uint8_t *in_ptr = S9xGetBasePointer(st, (d->ABank << 16) | d->AAddress);
			if (in_ptr)
				in_ptr += d->AAddress;

			in_sdd1_dma = st->sdd1_decode_buffer;
		}

		st->Memory.FillRAM[0x4801] = 0;
	}

	// Do Transfer

	uint8_t Work;

	// 8 cycles per channel
	ADD_CYCLES(st, SLOW_ONE_CYCLE);

	if (!d->ReverseTransfer)
	{
		// CPU -> PPU
		int32_t b = 0;
		uint16_t p = d->AAddress;
		uint8_t *base = S9xGetBasePointer(st, (d->ABank << 16) + d->AAddress);

		int32_t rem = count;
		// Transfer per block if d->AAdressFixed is false
		count = d->AAddressFixed ? rem : (d->AAddressDecrement ? ((p & MEMMAP_MASK) + 1) : (MEMMAP_BLOCK_SIZE - (p & MEMMAP_MASK)));

		// Settings for custom chip DMA
		if (in_sdd1_dma)
		{
			base = in_sdd1_dma;
			p = 0;
			count = rem;
		}

		bool inWRAM_DMA = !in_sdd1_dma && (d->ABank == 0x7e || d->ABank == 0x7f || (!(d->ABank & 0x40) && d->AAddress < 0x2000));

		// 8 cycles per byte
		auto UPDATE_COUNTERS = [&]() -> bool
		{
			--d->DMACount_Or_HDMAIndirectAddress;
			d->AAddress += inc;
			p += inc;
			if (!addCyclesInDMA(st, Channel))
			{
				st->CPU.InDMA = st->CPU.InDMAorHDMA = st->CPU.InWRAMDMAorHDMA = false;
				st->CPU.CurrentDMAorHDMAChannel = -1;
				return false;
			}
			return true;
		};

		while (1)
		{
			if (count > rem)
				count = rem;
			rem -= count;

			st->CPU.InWRAMDMAorHDMA = inWRAM_DMA;

			if (!base)
			{
				// DMA SLOW PATH
				if (!d->TransferMode || d->TransferMode == 2 || d->TransferMode == 6)
				{
					do
					{
						Work = S9xGetByte(st, (d->ABank << 16) + p);
						S9xSetPPU(st, Work, 0x2100 + d->BAddress);
						if (!UPDATE_COUNTERS())
							return false;
					} while (--count > 0);
				}
				else if (d->TransferMode == 1 || d->TransferMode == 5)
				{
					// This is a variation on Duff's Device. It is legal C/C++.
					switch (b)
					{
						default:
						while (count > 1)
						{
							Work = S9xGetByte(st, (d->ABank << 16) + p);
							S9xSetPPU(st, Work, 0x2100 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							--count;

						case 1:
							Work = S9xGetByte(st, (d->ABank << 16) + p);
							S9xSetPPU(st, Work, 0x2101 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							--count;
						}
					}

					if (count == 1)
					{
						Work = S9xGetByte(st, (d->ABank << 16) + p);
						S9xSetPPU(st, Work, 0x2100 + d->BAddress);
						if (!UPDATE_COUNTERS())
							return false;
						b = 1;
					}
					else
						b = 0;
				}
				else if (d->TransferMode == 3 || d->TransferMode == 7)
				{
					switch (b)
					{
						default:
						do
						{
							Work = S9xGetByte(st, (d->ABank << 16) + p);
							S9xSetPPU(st, Work, 0x2100 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 1;
								break;
							}

						case 1:
							Work = S9xGetByte(st, (d->ABank << 16) + p);
							S9xSetPPU(st, Work, 0x2100 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 2;
								break;
							}

						case 2:
							Work = S9xGetByte(st, (d->ABank << 16) + p);
							S9xSetPPU(st, Work, 0x2101 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 3;
								break;
							}

						case 3:
							Work = S9xGetByte(st, (d->ABank << 16) + p);
							S9xSetPPU(st, Work, 0x2101 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 0;
								break;
							}
						} while (1);
					}
				}
				else if (d->TransferMode == 4)
				{
					switch (b)
					{
						default:
						do
						{
							Work = S9xGetByte(st, (d->ABank << 16) + p);
							S9xSetPPU(st, Work, 0x2100 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 1;
								break;
							}

						case 1:
							Work = S9xGetByte(st, (d->ABank << 16) + p);
							S9xSetPPU(st, Work, 0x2101 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 2;
								break;
							}

						case 2:
							Work = S9xGetByte(st, (d->ABank << 16) + p);
							S9xSetPPU(st, Work, 0x2102 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 3;
								break;
							}

						case 3:
							Work = S9xGetByte(st, (d->ABank << 16) + p);
							S9xSetPPU(st, Work, 0x2103 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 0;
								break;
							}
						} while (1);
					}
				}
			}
			else
			{
				// DMA FAST PATH
				if (!d->TransferMode || d->TransferMode == 2 || d->TransferMode == 6)
				{
					switch (d->BAddress)
					{
						case 0x04: // OAMDATA
							do
							{
								Work = *(base + p);
								REGISTER_2104(st, Work);
								if (!UPDATE_COUNTERS())
									return false;
							} while (--count > 0);

							break;

						case 0x18: // VMDATAL
							if (!st->PPU.VMA.FullGraphicCount)
							{
								do
								{
									Work = *(base + p);
									REGISTER_2118_linear(st, Work);
									if (!UPDATE_COUNTERS())
										return false;
								} while (--count > 0);
							}
							else
							{
								do
								{
									Work = *(base + p);
									REGISTER_2118_tile(st, Work);
									if (!UPDATE_COUNTERS())
										return false;
								} while (--count > 0);
							}

							break;

						case 0x19: // VMDATAH
							if (!st->PPU.VMA.FullGraphicCount)
							{
								do
								{
									Work = *(base + p);
									REGISTER_2119_linear(st, Work);
									if (!UPDATE_COUNTERS())
										return false;
								} while (--count > 0);
							}
							else
							{
								do
								{
									Work = *(base + p);
									REGISTER_2119_tile(st, Work);
									if (!UPDATE_COUNTERS())
										return false;
								} while (--count > 0);
							}

							break;

						case 0x22: // CGDATA
							do
							{
								Work = *(base + p);
								if (!UPDATE_COUNTERS())
									return false;
							} while (--count > 0);

							break;

						case 0x80: // WMDATA
							if (!st->CPU.InWRAMDMAorHDMA)
							{
								do
								{
									Work = *(base + p);
									REGISTER_2180(st, Work);
									if (!UPDATE_COUNTERS())
										return false;
								} while (--count > 0);
							}
							else
							{
								do
								{
									if (!UPDATE_COUNTERS())
										return false;
								} while (--count > 0);
							}

							break;

						default:
							do
							{
								Work = *(base + p);
								S9xSetPPU(st, Work, 0x2100 + d->BAddress);
								if (!UPDATE_COUNTERS())
									return false;
							} while (--count > 0);

							break;
					}
				}
				else if (d->TransferMode == 1 || d->TransferMode == 5)
				{
					if (d->BAddress == 0x18)
					{
						// VMDATAL
						if (!st->PPU.VMA.FullGraphicCount)
						{
							switch (b)
							{
								default:
								while (count > 1)
								{
									Work = *(base + p);
									REGISTER_2118_linear(st, Work);
									if (!UPDATE_COUNTERS())
										return false;
									--count;

								case 1:
									Work = *(base + p);
									REGISTER_2119_linear(st, Work);
									if (!UPDATE_COUNTERS())
										return false;
									--count;
								}
							}

							if (count == 1)
							{
								Work = *(base + p);
								REGISTER_2118_linear(st, Work);
								if (!UPDATE_COUNTERS())
									return false;
								b = 1;
							}
							else
								b = 0;
						}
						else
						{
							switch (b)
							{
								default:
								while (count > 1)
								{
									Work = *(base + p);
									REGISTER_2118_tile(st, Work);
									if (!UPDATE_COUNTERS())
										return false;
									--count;

								case 1:
									Work = *(base + p);
									REGISTER_2119_tile(st, Work);
									if (!UPDATE_COUNTERS())
										return false;
									--count;
								}
							}

							if (count == 1)
							{
								Work = *(base + p);
								REGISTER_2118_tile(st, Work);
								if (!UPDATE_COUNTERS())
									return false;
								b = 1;
							}
							else
								b = 0;
						}
					}
					else
					{
						// DMA mode 1 general case
						switch (b)
						{
							default:
							while (count > 1)
							{
								Work = *(base + p);
								S9xSetPPU(st, Work, 0x2100 + d->BAddress);
								if (!UPDATE_COUNTERS())
									return false;
								--count;

							case 1:
								Work = *(base + p);
								S9xSetPPU(st, Work, 0x2101 + d->BAddress);
								if (!UPDATE_COUNTERS())
									return false;
								--count;
							}
						}

						if (count == 1)
						{
							Work = *(base + p);
							S9xSetPPU(st, Work, 0x2100 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							b = 1;
						}
						else
							b = 0;
					}
				}
				else if (d->TransferMode == 3 || d->TransferMode == 7)
				{
					switch (b)
					{
						default:
						do
						{
							Work = *(base + p);
							S9xSetPPU(st, Work, 0x2100 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 1;
								break;
							}

						case 1:
							Work = *(base + p);
							S9xSetPPU(st, Work, 0x2100 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 2;
								break;
							}

						case 2:
							Work = *(base + p);
							S9xSetPPU(st, Work, 0x2101 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 3;
								break;
							}

						case 3:
							Work = *(base + p);
							S9xSetPPU(st, Work, 0x2101 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 0;
								break;
							}
						} while (1);
					}
				}
				else if (d->TransferMode == 4)
				{
					switch (b)
					{
						default:
						do
						{
							Work = *(base + p);
							S9xSetPPU(st, Work, 0x2100 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 1;
								break;
							}

						case 1:
							Work = *(base + p);
							S9xSetPPU(st, Work, 0x2101 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 2;
								break;
							}

						case 2:
							Work = *(base + p);
							S9xSetPPU(st, Work, 0x2102 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 3;
								break;
							}

						case 3:
							Work = *(base + p);
							S9xSetPPU(st, Work, 0x2103 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 0;
								break;
							}
						} while (1);
					}
				}
			}

			if (rem <= 0)
				break;

			base = S9xGetBasePointer(st, (d->ABank << 16) + d->AAddress);
			count = MEMMAP_BLOCK_SIZE;
			inWRAM_DMA = !in_sdd1_dma && (d->ABank == 0x7e || d->ABank == 0x7f || (!(d->ABank & 0x40) && d->AAddress < 0x2000));
		}
	}
	else
	{
		// PPU -> CPU

		// 8 cycles per byte
		auto UPDATE_COUNTERS = [&]() -> bool
		{
			--d->DMACount_Or_HDMAIndirectAddress;
			d->AAddress += inc;
			if (!addCyclesInDMA(st, Channel))
			{
				st->CPU.InDMA = st->CPU.InDMAorHDMA = st->CPU.InWRAMDMAorHDMA = false;
				st->CPU.CurrentDMAorHDMAChannel = -1;
				return false;
			}
			return true;
		};

		if (d->BAddress > 0x80 - 4 && d->BAddress <= 0x83 && !(d->ABank & 0x40))
		{
			// REVERSE-DMA REALLY-SLOW PATH
			do
			{
				switch (d->TransferMode)
				{
					case 0:
					case 2:
					case 6:
						st->CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(st, 0x2100 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						--count;

						break;

					case 1:
					case 5:
						st->CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(st, 0x2100 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						st->CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(st, 0x2101 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						--count;

						break;

					case 3:
					case 7:
						st->CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(st, 0x2100 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						st->CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(st, 0x2100 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						st->CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(st, 0x2101 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						st->CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(st, 0x2101 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						--count;

						break;

					case 4:
						st->CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(st, 0x2100 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						st->CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(st, 0x2101 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						st->CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(st, 0x2102 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						st->CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(st, 0x2103 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						--count;

						break;

					default:
						while (count)
						{
							if (!UPDATE_COUNTERS())
								return false;
							--count;
						}
				}
			} while (count);
		}
		else
		{
			// REVERSE-DMA FASTER PATH
			st->CPU.InWRAMDMAorHDMA = d->ABank == 0x7e || d->ABank == 0x7f;
			do
			{
				switch (d->TransferMode)
				{
					case 0:
					case 2:
					case 6:
						Work = S9xGetPPU(st, 0x2100 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						--count;

						break;

					case 1:
					case 5:
						Work = S9xGetPPU(st, 0x2100 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						Work = S9xGetPPU(st, 0x2101 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						--count;

						break;

					case 3:
					case 7:
						Work = S9xGetPPU(st, 0x2100 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						Work = S9xGetPPU(st, 0x2100 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						Work = S9xGetPPU(st, 0x2101 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						Work = S9xGetPPU(st, 0x2101 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						--count;

						break;

					case 4:
						Work = S9xGetPPU(st, 0x2100 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						Work = S9xGetPPU(st, 0x2101 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						Work = S9xGetPPU(st, 0x2102 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						Work = S9xGetPPU(st, 0x2103 + d->BAddress);
						S9xSetByte(st, Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						--count;

						break;

					default:
						while (count)
						{
							if (!UPDATE_COUNTERS())
								return false;
							--count;
						}
				}
			} while (count);
		}
	}

	if (st->CPU.NMILine && (st->Timings.NMITriggerPos != 0xffff))
	{
		st->Timings.NMITriggerPos = st->CPU.Cycles + st->Timings.NMIDMADelay;
		if (st->Timings.NMITriggerPos >= st->Timings.H_Max)
			st->Timings.NMITriggerPos -= st->Timings.H_Max;
	}

	st->CPU.InDMA = st->CPU.InDMAorHDMA = st->CPU.InWRAMDMAorHDMA = false;
	st->CPU.CurrentDMAorHDMAChannel = -1;

	return true;
}

static inline bool HDMAReadLineCount(struct S9xState *st, int d)
{
	// CPU.InDMA is set, so S9xGetXXX() / S9xSetXXX() incur no charges.

	uint8_t line = S9xGetByte(st, (st->DMA[d].ABank << 16) + st->DMA[d].Address);
	ADD_CYCLES(st, SLOW_ONE_CYCLE);

	if (!line)
	{
		st->DMA[d].Repeat = false;
		st->DMA[d].LineCount = 128;

		if (st->DMA[d].HDMAIndirectAddressing)
		{
			if (st->PPU.HDMA & (0xfe << d))
			{
				++st->DMA[d].Address;
				ADD_CYCLES(st, SLOW_ONE_CYCLE << 1);
			}
			else
				ADD_CYCLES(st, SLOW_ONE_CYCLE);

			st->DMA[d].DMACount_Or_HDMAIndirectAddress = S9xGetWord(st, (st->DMA[d].ABank << 16) + st->DMA[d].Address);
			++st->DMA[d].Address;
		}

		++st->DMA[d].Address;
		st->HDMAMemPointers[d] = nullptr;

		return false;
	}
	else if (line == 0x80)
	{
		st->DMA[d].Repeat = true;
		st->DMA[d].LineCount = 128;
	}
	else
	{
		st->DMA[d].Repeat = !(line & 0x80);
		st->DMA[d].LineCount = line & 0x7f;
	}

	++st->DMA[d].Address;
	st->DMA[d].DoTransfer = true;

	if (st->DMA[d].HDMAIndirectAddressing)
	{
		ADD_CYCLES(st, SLOW_ONE_CYCLE << 1);
		st->DMA[d].DMACount_Or_HDMAIndirectAddress = S9xGetWord(st, (st->DMA[d].ABank << 16) + st->DMA[d].Address);
		st->DMA[d].Address += 2;
		st->HDMAMemPointers[d] = S9xGetMemPointer(st, (st->DMA[d].IndirectBank << 16) + st->DMA[d].DMACount_Or_HDMAIndirectAddress);
	}
	else
		st->HDMAMemPointers[d] = S9xGetMemPointer(st, (st->DMA[d].ABank << 16) + st->DMA[d].Address);

	return true;
}

void S9xStartHDMA(struct S9xState *st)
{
	st->PPU.HDMA = st->Memory.FillRAM[0x420c];

	st->PPU.HDMAEnded = 0;

	st->CPU.InHDMA = st->CPU.InDMAorHDMA = true;
	int32_t tmpch = st->CPU.CurrentDMAorHDMAChannel;

	// XXX: Not quite right...
	if (st->PPU.HDMA)
		ADD_CYCLES(st, st->Timings.DMACPUSync);

	for (uint8_t i = 0; i < 8; ++i)
	{
		if (st->PPU.HDMA & (1 << i))
		{
			st->CPU.CurrentDMAorHDMAChannel = i;

			st->DMA[i].Address = st->DMA[i].AAddress;

			if (!HDMAReadLineCount(st, i))
			{
				st->PPU.HDMA &= ~(1 << i);
				st->PPU.HDMAEnded |= 1 << i;
			}
		}
		else
			st->DMA[i].DoTransfer = false;
	}

	st->CPU.InHDMA = false;
	st->CPU.InDMAorHDMA = st->CPU.InDMA;
	st->CPU.HDMARanInDMA = st->CPU.InDMA ? st->PPU.HDMA : 0;
	st->CPU.CurrentDMAorHDMAChannel = tmpch;
}

uint8_t S9xDoHDMA(struct S9xState *st, uint8_t byte)
{
	SDMA *p = &st->DMA[0];

	int d = 0;

	st->CPU.InHDMA = st->CPU.InDMAorHDMA = true;
	st->CPU.HDMARanInDMA = st->CPU.InDMA ? byte : 0;
	bool temp = st->CPU.InWRAMDMAorHDMA;
	int32_t tmpch = st->CPU.CurrentDMAorHDMAChannel;

	// XXX: Not quite right...
	ADD_CYCLES(st, st->Timings.DMACPUSync);

	for (uint8_t mask = 1; mask; mask <<= 1, ++p, ++d)
	{
		if (byte & mask)
		{
			st->CPU.InWRAMDMAorHDMA = false;
			st->CPU.CurrentDMAorHDMAChannel = d;

			uint32_t ShiftedIBank;
			uint16_t IAddr;
			if (p->HDMAIndirectAddressing)
			{
				ShiftedIBank = p->IndirectBank << 16;
				IAddr = p->DMACount_Or_HDMAIndirectAddress;
			}
			else
			{
				ShiftedIBank = p->ABank << 16;
				IAddr = p->Address;
			}

			if (!st->HDMAMemPointers[d])
				st->HDMAMemPointers[d] = S9xGetMemPointer(st, ShiftedIBank + IAddr);

			if (p->DoTransfer)
			{
				// XXX: Hack for Uniracers, because we don't understand
				// OAM Address Invalidation
				if (p->BAddress == 0x04)
				{
					if (st->SNESGameFixes.Uniracers)
					{
						st->PPU.OAMAddr = 0x10c;
						st->PPU.OAMFlip = 0;
					}
				}

				if (!p->ReverseTransfer)
				{
					if ((IAddr & MEMMAP_MASK) + HDMA_ModeByteCounts[p->TransferMode] >= MEMMAP_BLOCK_SIZE)
					{
						// HDMA REALLY-SLOW PATH
						st->HDMAMemPointers[d] = nullptr;

						auto DOBYTE = [&](uint16_t Addr, uint16_t RegOff)
						{
							st->CPU.InWRAMDMAorHDMA = ShiftedIBank == 0x7e0000 || ShiftedIBank == 0x7f0000 || (!(ShiftedIBank & 0x400000) && Addr < 0x2000);
							S9xSetPPU(st, S9xGetByte(st, ShiftedIBank + Addr), 0x2100 + p->BAddress + RegOff);
						};

						switch (p->TransferMode)
						{
							case 0:
								DOBYTE(IAddr, 0);
								ADD_CYCLES(st, SLOW_ONE_CYCLE);
								break;

							case 5:
								DOBYTE(IAddr, 0);
								ADD_CYCLES(st, SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 1, 1);
								ADD_CYCLES(st, SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 2, 0);
								ADD_CYCLES(st, SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 3, 1);
								ADD_CYCLES(st, SLOW_ONE_CYCLE);
								break;

							case 1:
								DOBYTE(IAddr, 0);
								ADD_CYCLES(st, SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 1, 1);
								ADD_CYCLES(st, SLOW_ONE_CYCLE);
								break;

							case 2:
							case 6:
								DOBYTE(IAddr, 0);
								ADD_CYCLES(st, SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 1, 0);
								ADD_CYCLES(st, SLOW_ONE_CYCLE);
								break;

							case 3:
							case 7:
								DOBYTE(IAddr, 0);
								ADD_CYCLES(st, SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 1, 0);
								ADD_CYCLES(st, SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 2, 1);
								ADD_CYCLES(st, SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 3, 1);
								ADD_CYCLES(st, SLOW_ONE_CYCLE);
								break;

							case 4:
								DOBYTE(IAddr, 0);
								ADD_CYCLES(st, SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 1, 1);
								ADD_CYCLES(st, SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 2, 2);
								ADD_CYCLES(st, SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 3, 3);
								ADD_CYCLES(st, SLOW_ONE_CYCLE);
						}
					}
					else
					{
						st->CPU.InWRAMDMAorHDMA = ShiftedIBank == 0x7e0000 || ShiftedIBank == 0x7f0000 || (!(ShiftedIBank & 0x400000) && IAddr < 0x2000);

						if (!st->HDMAMemPointers[d])
						{
							// HDMA SLOW PATH
							uint32_t Addr = ShiftedIBank + IAddr;

							switch (p->TransferMode)
							{
								case 0:
									S9xSetPPU(st, S9xGetByte(st, Addr), 0x2100 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									break;

								case 5:
									S9xSetPPU(st, S9xGetByte(st, Addr), 0x2100 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									S9xSetPPU(st, S9xGetByte(st, Addr + 1), 0x2101 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									Addr += 2;
									/* fall through */
								case 1:
									S9xSetPPU(st, S9xGetByte(st, Addr), 0x2100 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									S9xSetPPU(st, S9xGetByte(st, Addr + 1), 0x2101 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									break;

								case 2:
								case 6:
									S9xSetPPU(st, S9xGetByte(st, Addr), 0x2100 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									S9xSetPPU(st, S9xGetByte(st, Addr + 1), 0x2100 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									break;

								case 3:
								case 7:
									S9xSetPPU(st, S9xGetByte(st, Addr), 0x2100 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									S9xSetPPU(st, S9xGetByte(st, Addr + 1), 0x2100 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									S9xSetPPU(st, S9xGetByte(st, Addr + 2), 0x2101 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									S9xSetPPU(st, S9xGetByte(st, Addr + 3), 0x2101 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									break;

								case 4:
									S9xSetPPU(st, S9xGetByte(st, Addr), 0x2100 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									S9xSetPPU(st, S9xGetByte(st, Addr + 1), 0x2101 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									S9xSetPPU(st, S9xGetByte(st, Addr + 2), 0x2102 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									S9xSetPPU(st, S9xGetByte(st, Addr + 3), 0x2103 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
							}
						}
						else
						{
							// HDMA FAST PATH
							switch (p->TransferMode)
							{
								case 0:
									S9xSetPPU(st, *st->HDMAMemPointers[d]++, 0x2100 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									break;

								case 5:
									S9xSetPPU(st, *st->HDMAMemPointers[d], 0x2100 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									S9xSetPPU(st, *(st->HDMAMemPointers[d] + 1), 0x2101 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									st->HDMAMemPointers[d] += 2;
									/* fall through */
								case 1:
									S9xSetPPU(st, *st->HDMAMemPointers[d], 0x2100 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									S9xSetPPU(st, *(st->HDMAMemPointers[d] + 1), 0x2101 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									st->HDMAMemPointers[d] += 2;
									break;

								case 2:
								case 6:
									S9xSetPPU(st, *st->HDMAMemPointers[d], 0x2100 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									S9xSetPPU(st, *(st->HDMAMemPointers[d] + 1), 0x2100 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									st->HDMAMemPointers[d] += 2;
									break;

								case 3:
								case 7:
									S9xSetPPU(st, *st->HDMAMemPointers[d], 0x2100 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									S9xSetPPU(st, *(st->HDMAMemPointers[d] + 1), 0x2100 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									S9xSetPPU(st, *(st->HDMAMemPointers[d] + 2), 0x2101 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									S9xSetPPU(st, *(st->HDMAMemPointers[d] + 3), 0x2101 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									st->HDMAMemPointers[d] += 4;
									break;

								case 4:
									S9xSetPPU(st, *st->HDMAMemPointers[d], 0x2100 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									S9xSetPPU(st, *(st->HDMAMemPointers[d] + 1), 0x2101 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									S9xSetPPU(st, *(st->HDMAMemPointers[d] + 2), 0x2102 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									S9xSetPPU(st, *(st->HDMAMemPointers[d] + 3), 0x2103 + p->BAddress);
									ADD_CYCLES(st, SLOW_ONE_CYCLE);
									st->HDMAMemPointers[d] += 4;
							}
						}
					}
				}
				else
				{
					// REVERSE HDMA REALLY-SLOW PATH
					// anomie says: Since this is apparently never used
					// (otherwise we would have noticed before now), let's not bother with faster paths.
					st->HDMAMemPointers[d] = nullptr;

					auto DOBYTE = [&](uint16_t Addr, uint16_t RegOff)
					{
						st->CPU.InWRAMDMAorHDMA = ShiftedIBank == 0x7e0000 || ShiftedIBank == 0x7f0000 || (!(ShiftedIBank & 0x400000) && Addr < 0x2000);
						S9xSetByte(st, S9xGetPPU(st, 0x2100 + p->BAddress + RegOff), ShiftedIBank + Addr);
					};

					switch (p->TransferMode)
					{
						case 0:
							DOBYTE(IAddr, 0);
							ADD_CYCLES(st, SLOW_ONE_CYCLE);
							break;

						case 5:
							DOBYTE(IAddr, 0);
							ADD_CYCLES(st, SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 1, 1);
							ADD_CYCLES(st, SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 2, 0);
							ADD_CYCLES(st, SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 3, 1);
							ADD_CYCLES(st, SLOW_ONE_CYCLE);
							break;

						case 1:
							DOBYTE(IAddr, 0);
							ADD_CYCLES(st, SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 1, 1);
							ADD_CYCLES(st, SLOW_ONE_CYCLE);
							break;

						case 2:
						case 6:
							DOBYTE(IAddr, 0);
							ADD_CYCLES(st, SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 1, 0);
							ADD_CYCLES(st, SLOW_ONE_CYCLE);
							break;

						case 3:
						case 7:
							DOBYTE(IAddr, 0);
							ADD_CYCLES(st, SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 1, 0);
							ADD_CYCLES(st, SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 2, 1);
							ADD_CYCLES(st, SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 3, 1);
							ADD_CYCLES(st, SLOW_ONE_CYCLE);
							break;

						case 4:
							DOBYTE(IAddr, 0);
							ADD_CYCLES(st, SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 1, 1);
							ADD_CYCLES(st, SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 2, 2);
							ADD_CYCLES(st, SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 3, 3);
							ADD_CYCLES(st, SLOW_ONE_CYCLE);
					}
				}

				if (p->HDMAIndirectAddressing)
					p->DMACount_Or_HDMAIndirectAddress += HDMA_ModeByteCounts[p->TransferMode];
				else
					p->Address += HDMA_ModeByteCounts[p->TransferMode];
			}

			p->DoTransfer = !p->Repeat;

			if (!--p->LineCount)
			{
				if (!HDMAReadLineCount(st, d))
				{
					byte &= ~mask;
					st->PPU.HDMAEnded |= mask;
					p->DoTransfer = false;
					continue;
				}
			}
			else
				ADD_CYCLES(st, SLOW_ONE_CYCLE);
		}
	}

	st->CPU.InHDMA = false;
	st->CPU.InDMAorHDMA = st->CPU.InDMA;
	st->CPU.InWRAMDMAorHDMA = temp;
	st->CPU.CurrentDMAorHDMAChannel = tmpch;

	return byte;
}

void S9xResetDMA(struct S9xState *st)
{
	for (int d = 0; d < 8; ++d)
	{
		st->DMA[d].ReverseTransfer = st->DMA[d].HDMAIndirectAddressing = st->DMA[d].AAddressFixed = st->DMA[d].AAddressDecrement = true;
		st->DMA[d].TransferMode = 7;
		st->DMA[d].BAddress = 0xff;
		st->DMA[d].AAddress = 0xffff;
		st->DMA[d].ABank = 0xff;
		st->DMA[d].DMACount_Or_HDMAIndirectAddress = 0xffff;
		st->DMA[d].IndirectBank = 0xff;
		st->DMA[d].Address = 0xffff;
		st->DMA[d].Repeat = false;
		st->DMA[d].LineCount = 0x7f;
		st->DMA[d].UnknownByte = 0xff;
		st->DMA[d].DoTransfer = false;
		st->DMA[d].UnusedBit43x0 = true;
	}
}
