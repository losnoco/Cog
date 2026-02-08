/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include <algorithm>
#include <snes9x/snes9x.h>
#include "sdd1.h"

#include <snes9x/snes.hpp>
#include <snes9x/smp.hpp>
#include <snes9x/sdsp.hpp>

static int CyclesUntilNext(struct S9xState *st, int hc, int vc)
{
	int32_t total = 0;
	int vpos = st->CPU.V_Counter;
	
	if (vc - vpos > 0)
	{
		// It's still in this frame */
		// Add number of lines
		total += (vc - vpos) * st->Timings.H_Max_Master;
		// If line 240 is in there and we're odd, subtract a dot
		if (vpos <= 240 && vc > 240 && st->Timings.InterlaceField && !st->IPPU.Interlace)
			total -= ONE_DOT_CYCLE;
	}
	else
	{
		if (vc == vpos && hc > st->CPU.Cycles)
			return hc;
		
		total += (st->Timings.V_Max - vpos) * st->Timings.H_Max_Master;
		if (vpos <= 240 && st->Timings.InterlaceField && !st->IPPU.Interlace)
			total -= ONE_DOT_CYCLE;
		
		total += vc * st->Timings.H_Max_Master;
		if (vc > 240 && !st->Timings.InterlaceField && !st->IPPU.Interlace)
			total -= ONE_DOT_CYCLE;
	}
	
	total += hc;

	return total;
}

void S9xUpdateIRQPositions(struct S9xState *st, bool initial)
{
	st->PPU.HTimerPosition = st->PPU.IRQHBeamPos * ONE_DOT_CYCLE + st->Timings.IRQTriggerCycles;
	st->PPU.HTimerPosition -= st->PPU.IRQHBeamPos ? 0 : ONE_DOT_CYCLE;
	st->PPU.HTimerPosition += st->PPU.IRQHBeamPos > 322 ? (ONE_DOT_CYCLE / 2) : 0;
	st->PPU.HTimerPosition += st->PPU.IRQHBeamPos > 326 ? (ONE_DOT_CYCLE / 2) : 0;
	
	st->PPU.VTimerPosition = st->PPU.IRQVBeamPos;
	
	if (st->PPU.VTimerEnabled && st->PPU.VTimerPosition >= st->Timings.V_Max + (st->IPPU.Interlace ? 1 : 0))
		st->Timings.NextIRQTimer = 0x0fffffff;
	else if (!st->PPU.HTimerEnabled && !st->PPU.VTimerEnabled)
		st->Timings.NextIRQTimer = 0x0fffffff;
	else if (st->PPU.HTimerEnabled && !st->PPU.VTimerEnabled)
	{
		int v_pos = st->CPU.V_Counter;

		st->Timings.NextIRQTimer = st->PPU.HTimerPosition;
		if (st->CPU.Cycles > st->Timings.NextIRQTimer - st->Timings.IRQTriggerCycles)
		{
			st->Timings.NextIRQTimer += st->Timings.H_Max;
			++v_pos;
		}

		// Check for short dot scanline
		if (v_pos == 240 && st->Timings.InterlaceField && !st->IPPU.Interlace)
		{
			st->Timings.NextIRQTimer -= st->PPU.IRQHBeamPos <= 322 ? ONE_DOT_CYCLE / 2 : 0;
			st->Timings.NextIRQTimer -= st->PPU.IRQHBeamPos <= 326 ? ONE_DOT_CYCLE / 2 : 0;
		}
	}
	else if (!st->PPU.HTimerEnabled && st->PPU.VTimerEnabled)
	{
		if (st->CPU.V_Counter == st->PPU.VTimerPosition && initial)
			st->Timings.NextIRQTimer = st->CPU.Cycles + st->Timings.IRQTriggerCycles - ONE_DOT_CYCLE;
		else
			st->Timings.NextIRQTimer = CyclesUntilNext(st, st->Timings.IRQTriggerCycles - ONE_DOT_CYCLE, st->PPU.VTimerPosition);
	}
	else
	{
		st->Timings.NextIRQTimer = CyclesUntilNext(st, st->PPU.HTimerPosition, st->PPU.VTimerPosition);

		// Check for short dot scanline
		int field = st->Timings.InterlaceField;

		if (st->PPU.VTimerPosition < st->CPU.V_Counter || (st->PPU.VTimerPosition == st->CPU.V_Counter && st->Timings.NextIRQTimer > st->Timings.H_Max))
			field = !field;

		if (st->PPU.VTimerPosition == 240 && field && !st->IPPU.Interlace)
		{
			st->Timings.NextIRQTimer -= st->PPU.IRQHBeamPos <= 322 ? ONE_DOT_CYCLE / 2 : 0;
			st->Timings.NextIRQTimer -= st->PPU.IRQHBeamPos <= 326 ? ONE_DOT_CYCLE / 2 : 0;
		}
	}
}

void S9xSetPPU(struct S9xState *st, uint8_t Byte, uint16_t Address)
{
	// MAP_PPU: $2000-$3FFF

	if (st->CPU.InDMAorHDMA)
	{
		if (st->CPU.CurrentDMAorHDMAChannel >= 0 && st->DMA[st->CPU.CurrentDMAorHDMAChannel].ReverseTransfer)
			// S9xSetPPU() is called to write to DMA[].AAddress
			return;
		else
		{
			// S9xSetPPU() is called to read from $21xx
			// Take care of DMA wrapping
			if (Address > 0x21ff)
				Address = 0x2100 + (Address & 0xff);
		}
	}

	if ((Address & 0xffc0) == 0x2140) // APUIO0, APUIO1, APUIO2, APUIO3
		// write_port will run the APU until given clock before writing value
		S9xAPUWritePort(st, Address & 3, Byte);
	else if (Address <= 0x2183)
		switch (Address)
		{
			case 0x2100: // INIDISP
				if (Byte != st->Memory.FillRAM[0x2100] && (st->Memory.FillRAM[0x2100] & 0x80) != (Byte & 0x80))
					st->PPU.ForcedBlanking = !!((Byte >> 7) & 1);

				if ((st->Memory.FillRAM[0x2100] & 0x80) && st->CPU.V_Counter == st->PPU.ScreenHeight + FIRST_VISIBLE_LINE)
				{
					st->PPU.OAMAddr = st->PPU.SavedOAMAddr;

					uint8_t tmp = 0;
					if (st->PPU.OAMPriorityRotation)
						tmp = (st->PPU.OAMAddr & 0xfe) >> 1;
					if ((st->PPU.OAMFlip & 1) || st->PPU.FirstSprite != tmp)
						st->PPU.FirstSprite = tmp;

					st->PPU.OAMFlip = 0;
				}

				break;

			case 0x2102: // OAMADDL
				st->PPU.OAMAddr = ((st->Memory.FillRAM[0x2103] & 1) << 8) | Byte;
				st->PPU.OAMFlip = 0;
				st->PPU.SavedOAMAddr = st->PPU.OAMAddr;
				if (st->PPU.OAMPriorityRotation && st->PPU.FirstSprite != (st->PPU.OAMAddr >> 1))
					st->PPU.FirstSprite = (st->PPU.OAMAddr & 0xfe) >> 1;

				break;

			case 0x2103: // OAMADDH
				st->PPU.OAMAddr = ((Byte & 1) << 8) | st->Memory.FillRAM[0x2102];
				st->PPU.OAMPriorityRotation = !!(Byte & 0x80);
				if (st->PPU.OAMPriorityRotation)
				{
					if (st->PPU.FirstSprite != (st->PPU.OAMAddr >> 1))
						st->PPU.FirstSprite = (st->PPU.OAMAddr & 0xfe) >> 1;
				}
				else if (st->PPU.FirstSprite)
					st->PPU.FirstSprite = 0;

				st->PPU.OAMFlip = 0;
				st->PPU.SavedOAMAddr = st->PPU.OAMAddr;

				break;

			case 0x2104: // OAMDATA
				REGISTER_2104(st, Byte);
				break;

			case 0x2105: // BGMODE
				if (Byte != st->Memory.FillRAM[0x2105])
					st->IPPU.Interlace = 0;

				break;

			case 0x210d: // BG1HOFS, M7HOFS
				st->PPU.M7byte = Byte;
				break;

			case 0x210e: // BG1VOFS, M7VOFS
				st->PPU.M7byte = Byte;
				break;

			case 0x2115: // VMAIN
				st->PPU.VMA.High = !!(Byte & 0x80);
				switch (Byte & 3)
				{
					case 0:
						st->PPU.VMA.Increment = 1;
						break;
					case 1:
						st->PPU.VMA.Increment = 32;
						break;
					case 2:
					case 3:
						st->PPU.VMA.Increment = 128;
				}

				if (Byte & 0x0c)
				{
					static const uint16_t Shift[] = { 0, 5, 6, 7 };
					static const uint16_t IncCount[] = { 0, 32, 64, 128 };

					uint8_t i = (Byte & 0x0c) >> 2;
					st->PPU.VMA.FullGraphicCount = IncCount[i];
					st->PPU.VMA.Mask1 = IncCount[i] * 8 - 1;
					st->PPU.VMA.Shift = Shift[i];
				}
				else
					st->PPU.VMA.FullGraphicCount = 0;
				break;

			case 0x2116: // VMADDL
				st->PPU.VMA.Address &= 0xff00;
				st->PPU.VMA.Address |= Byte;

				S9xUpdateVRAMReadBuffer(st);

				break;

			case 0x2117: // VMADDH
				st->PPU.VMA.Address &= 0x00ff;
				st->PPU.VMA.Address |= Byte << 8;

				S9xUpdateVRAMReadBuffer(st);

				break;

			case 0x2118: // VMDATAL
				REGISTER_2118(st, Byte);
				break;

			case 0x2119: // VMDATAH
				REGISTER_2119(st, Byte);
				break;

			case 0x211b: // M7A
				st->PPU.MatrixA = st->PPU.M7byte | (Byte << 8);
				st->PPU.Need16x8Mulitply = true;
				st->PPU.M7byte = Byte;
				break;

			case 0x211c: // M7B
				st->PPU.MatrixB = st->PPU.M7byte | (Byte << 8);
				st->PPU.Need16x8Mulitply = true;
				st->PPU.M7byte = Byte;
				break;

			case 0x211d: // M7C
			case 0x211e: // M7D
			case 0x211f: // M7X
			case 0x2120: // M7Y
				st->PPU.M7byte = Byte;
				break;

			case 0x2121: // CGADD
				st->PPU.CGFLIPRead = false;
				st->PPU.CGADD = Byte;
				break;

			case 0x2133: // SETINI
				if (Byte != st->Memory.FillRAM[0x2133])
				{
					if (Byte & 0x04)
						st->PPU.ScreenHeight = SNES_HEIGHT_EXTENDED;
					else
						st->PPU.ScreenHeight = SNES_HEIGHT;

					if ((st->Memory.FillRAM[0x2133] ^ Byte) & 3)
						st->IPPU.Interlace = !!(Byte & 1);
				}

				break;

			case 0x2180: // WMDATA
				if (!st->CPU.InWRAMDMAorHDMA)
					REGISTER_2180(st, Byte);
				break;

			case 0x2181: // WMADDL
				if (!st->CPU.InWRAMDMAorHDMA)
				{
					st->PPU.WRAM &= 0x1ff00;
					st->PPU.WRAM |= Byte;
				}

				break;

			case 0x2182: // WMADDM
				if (!st->CPU.InWRAMDMAorHDMA)
				{
					st->PPU.WRAM &= 0x100ff;
					st->PPU.WRAM |= Byte << 8;
				}

				break;

			case 0x2183: // WMADDH
				if (!st->CPU.InWRAMDMAorHDMA)
				{
					st->PPU.WRAM &= 0x0ffff;
					st->PPU.WRAM |= Byte << 16;
					st->PPU.WRAM &= 0x1ffff;
				}
		}

	st->Memory.FillRAM[Address] = Byte;
}

uint8_t S9xGetPPU(struct S9xState *st, uint16_t Address)
{
	// MAP_PPU: $2000-$3FFF

	if (Address < 0x2100)
		return st->OpenBus;

	if (st->CPU.InDMAorHDMA)
	{
		if (st->CPU.CurrentDMAorHDMAChannel >= 0 && !st->DMA[st->CPU.CurrentDMAorHDMAChannel].ReverseTransfer)
			// S9xGetPPU() is called to read from DMA[].AAddress
			return st->OpenBus;
		else
		{
			// S9xGetPPU() is called to write to $21xx
			// Take care of DMA wrapping
			if (Address > 0x21ff)
				Address = 0x2100 + (Address & 0xff);
		}
	}

	if ((Address & 0xffc0) == 0x2140) // APUIO0, APUIO1, APUIO2, APUIO3
		// read_port will run the APU until given APU time before reading value
		return S9xAPUReadPort(st, Address & 3);
	else if (Address <= 0x2183)
	{
		uint8_t byte;

		switch (Address)
		{
			case 0x2104: // OAMDATA
			case 0x2105: // BGMODE
			case 0x2106: // MOSAIC
			case 0x2108: // BG2SC
			case 0x2109: // BG3SC
			case 0x210a: // BG4SC
			case 0x2114: // BG4VOFS
			case 0x2115: // VMAIN
			case 0x2116: // VMADDL
			case 0x2118: // VMDATAL
			case 0x2119: // VMDATAH
			case 0x211a: // M7SEL
			case 0x2124: // W34SEL
			case 0x2125: // WOBJSEL
			case 0x2126: // WH0
			case 0x2128: // WH2
			case 0x2129: // WH3
			case 0x212a: // WBGLOG
				return st->PPU.OpenBus1;

			case 0x2134: // MPYL
			case 0x2135: // MPYM
			case 0x2136: // MPYH
				if (st->PPU.Need16x8Mulitply)
				{
					int32_t r = static_cast<int32_t>(st->PPU.MatrixA) * static_cast<int32_t>(st->PPU.MatrixB >> 8);
					st->Memory.FillRAM[0x2134] = static_cast<uint8_t>(r);
					st->Memory.FillRAM[0x2135] = static_cast<uint8_t>(r >> 8);
					st->Memory.FillRAM[0x2136] = static_cast<uint8_t>(r >> 16);
					st->PPU.Need16x8Mulitply = false;
				}
				return (st->PPU.OpenBus1 = st->Memory.FillRAM[Address]);

			case 0x2137: // SLHV
				return st->PPU.OpenBus1;

			case 0x2138: // OAMDATAREAD
				if (st->PPU.OAMAddr & 0x100)
				{
					if (!(st->PPU.OAMFlip & 1))
						byte = st->PPU.OAMData[(st->PPU.OAMAddr & 0x10f) << 1];
					else
					{
						byte = st->PPU.OAMData[((st->PPU.OAMAddr & 0x10f) << 1) + 1];
						st->PPU.OAMAddr = (st->PPU.OAMAddr + 1) & 0x1ff;
						if (st->PPU.OAMPriorityRotation && st->PPU.FirstSprite != (st->PPU.OAMAddr >> 1))
							st->PPU.FirstSprite = (st->PPU.OAMAddr & 0xfe) >> 1;
					}
				}
				else
				{
					if (!(st->PPU.OAMFlip & 1))
						byte = st->PPU.OAMData[st->PPU.OAMAddr << 1];
					else
					{
						byte = st->PPU.OAMData[(st->PPU.OAMAddr << 1) + 1];
						++st->PPU.OAMAddr;
						if (st->PPU.OAMPriorityRotation && st->PPU.FirstSprite != (st->PPU.OAMAddr >> 1))
							st->PPU.FirstSprite = (st->PPU.OAMAddr & 0xfe) >> 1;
					}
				}

				st->PPU.OAMFlip ^= 1;
				return (st->PPU.OpenBus1 = byte);

			case 0x2139: // VMDATALREAD
				byte = st->PPU.VRAMReadBuffer & 0xff;
				if (!st->PPU.VMA.High)
				{
					S9xUpdateVRAMReadBuffer(st);

					st->PPU.VMA.Address += st->PPU.VMA.Increment;
				}
				return (st->PPU.OpenBus1 = byte);

			case 0x213a: // VMDATAHREAD
				byte = (st->PPU.VRAMReadBuffer >> 8) & 0xff;
				if (st->PPU.VMA.High)
				{
					S9xUpdateVRAMReadBuffer(st);

					st->PPU.VMA.Address += st->PPU.VMA.Increment;
				}
				return (st->PPU.OpenBus1 = byte);

			case 0x213b: // CGDATAREAD
				if (st->PPU.CGFLIPRead)
					byte = (st->PPU.OpenBus2 & 0x80) | ((st->PPU.CGDATA[st->PPU.CGADD++] >> 8) & 0x7f);
				else
					byte = st->PPU.CGDATA[st->PPU.CGADD] & 0xff;
				st->PPU.CGFLIPRead = !st->PPU.CGFLIPRead;
				return (st->PPU.OpenBus2 = byte);

			case 0x213c: // OPHCT
				if (st->PPU.HBeamFlip)
					byte = st->PPU.OpenBus2 & 0xfe;
				else
					byte = 0;
				st->PPU.HBeamFlip = !st->PPU.HBeamFlip;
				return (st->PPU.OpenBus2 = byte);

			case 0x213d: // OPVCT
				if (st->PPU.VBeamFlip)
					byte = st->PPU.OpenBus2 & 0xfe;
				else
					byte = 0;
				st->PPU.VBeamFlip = !st->PPU.VBeamFlip;
				return (st->PPU.OpenBus2 = byte);

			case 0x213e: // STAT77
				byte = (st->PPU.OpenBus1 & 0x10) | st->Model->_5C77;
				return (st->PPU.OpenBus1 = byte);

			case 0x213f: // STAT78
				st->PPU.VBeamFlip = st->PPU.HBeamFlip = false;
				byte = (st->PPU.OpenBus2 & 0x20) | (st->Memory.FillRAM[0x213f] & 0xc0) | (st->Settings.PAL ? 0x10 : 0) | st->Model->_5C78;
				st->Memory.FillRAM[0x213f] &= ~0x40;
				return (st->PPU.OpenBus2 = byte);

			case 0x2180: // WMDATA
				if (!st->CPU.InWRAMDMAorHDMA)
				{
					byte = st->Memory.RAM[st->PPU.WRAM++];
					st->PPU.WRAM &= 0x1ffff;
				}
				else
					byte = st->OpenBus;
				return byte;

			default:
				return st->OpenBus;
		}
	}
	else
	{
		switch (Address)
		{
			case 0x21c2:
				if (st->Model->_5C77 == 2)
					return 0x20;
				return st->OpenBus;

			case 0x21c3:
				if (st->Model->_5C77 == 2)
					return 0;
				return st->OpenBus;

			default:
				return st->OpenBus;
		}
	}
}

void S9xSetCPU(struct S9xState *st, uint8_t Byte, uint16_t Address)
{
	if ((Address & 0xff80) == 0x4300)
	{
		if (st->CPU.InDMAorHDMA)
			return;

		int d = (Address >> 4) & 0x7;

		switch (Address & 0xf)
		{
			case 0x0: // 0x43x0: DMAPx
				st->DMA[d].ReverseTransfer = !!(Byte & 0x80);
				st->DMA[d].HDMAIndirectAddressing = !!(Byte & 0x40);
				st->DMA[d].UnusedBit43x0 = !!(Byte & 0x20);
				st->DMA[d].AAddressDecrement = !!(Byte & 0x10);
				st->DMA[d].AAddressFixed = !!(Byte & 0x08);
				st->DMA[d].TransferMode = Byte & 7;
				return;

			case 0x1: // 0x43x1: BBADx
				st->DMA[d].BAddress = Byte;
				return;

			case 0x2: // 0x43x2: A1TxL
				st->DMA[d].AAddress &= 0xff00;
				st->DMA[d].AAddress |= Byte;
				return;

			case 0x3: // 0x43x3: A1TxH
				st->DMA[d].AAddress &= 0xff;
				st->DMA[d].AAddress |= Byte << 8;
				return;

			case 0x4: // 0x43x4: A1Bx
				st->DMA[d].ABank = Byte;
				st->HDMAMemPointers[d] = nullptr;
				return;

			case 0x5: // 0x43x5: DASxL
				st->DMA[d].DMACount_Or_HDMAIndirectAddress &= 0xff00;
				st->DMA[d].DMACount_Or_HDMAIndirectAddress |= Byte;
				st->HDMAMemPointers[d] = nullptr;
				return;

			case 0x6: // 0x43x6: DASxH
				st->DMA[d].DMACount_Or_HDMAIndirectAddress &= 0xff;
				st->DMA[d].DMACount_Or_HDMAIndirectAddress |= Byte << 8;
				st->HDMAMemPointers[d] = nullptr;
				return;

			case 0x7: // 0x43x7: DASBx
				st->DMA[d].IndirectBank = Byte;
				st->HDMAMemPointers[d] = nullptr;
				return;

			case 0x8: // 0x43x8: A2AxL
				st->DMA[d].Address &= 0xff00;
				st->DMA[d].Address |= Byte;
				st->HDMAMemPointers[d] = nullptr;
				return;

			case 0x9: // 0x43x9: A2AxH
				st->DMA[d].Address &= 0xff;
				st->DMA[d].Address |= Byte << 8;
				st->HDMAMemPointers[d] = nullptr;
				return;

			case 0xa: // 0x43xa: NLTRx
				if (Byte & 0x7f)
				{
					st->DMA[d].LineCount = Byte & 0x7f;
					st->DMA[d].Repeat = !(Byte & 0x80);
				}
				else
				{
					st->DMA[d].LineCount = 128;
					st->DMA[d].Repeat = !!(Byte & 0x80);
				}

				return;

			case 0xb: // 0x43xb: ????x
			case 0xf: // 0x43xf: mirror of 0x43xb
				st->DMA[d].UnknownByte = Byte;
				return;
		}
	}
	else if (Address >= 0x4200)
	{
		uint16_t pos;

		switch (Address)
		{
			case 0x4200: // NMITIMEN
				if (Byte == st->Memory.FillRAM[0x4200])
					break;

				st->PPU.VTimerEnabled = !!(Byte & 0x20);
				st->PPU.HTimerEnabled = !!(Byte & 0x10);

				if (!(Byte & 0x10) && !(Byte & 0x20))
					st->CPU.IRQLine = st->CPU.IRQTransition = false;

				if ((Byte & 0x30) != (st->Memory.FillRAM[0x4200] & 0x30))
					// Only allow instantaneous IRQ if turning it completely on or off
					S9xUpdateIRQPositions(st, !(Byte & 0x30) || !(st->Memory.FillRAM[0x4200] & 0x30));

				// NMI can trigger immediately during VBlank as long as NMI_read ($4210) wasn't cleard.
				if ((Byte & 0x80) && !(st->Memory.FillRAM[0x4200] & 0x80) && st->CPU.V_Counter >= st->PPU.ScreenHeight + FIRST_VISIBLE_LINE && (st->Memory.FillRAM[0x4210] & 0x80))
					// FIXME: triggered at HC+=6, checked just before the final CPU cycle,
					// then, when to call S9xOpcode_NMI()?
					st->Timings.IRQFlagChanging |= IRQ_TRIGGER_NMI;

				break;

			case 0x4201: // WRIO
				st->Memory.FillRAM[0x4201] = st->Memory.FillRAM[0x4213] = Byte;
				break;

			case 0x4203: // WRMPYB
			{
				uint32_t res = st->Memory.FillRAM[0x4202] * Byte;
				// FIXME: The update occurs 8 machine cycles after $4203 is set.
				st->Memory.FillRAM[0x4216] = static_cast<uint8_t>(res);
				st->Memory.FillRAM[0x4217] = static_cast<uint8_t>(res >> 8);
				break;
			}

			case 0x4206: // WRDIVB
			{
				uint16_t a = st->Memory.FillRAM[0x4204] + (st->Memory.FillRAM[0x4205] << 8);
				uint16_t div = Byte ? a / Byte : 0xffff;
				uint16_t rem = Byte ? a % Byte : a;
				// FIXME: The update occurs 16 machine cycles after $4206 is set.
				st->Memory.FillRAM[0x4214] = div & 0xff;
				st->Memory.FillRAM[0x4215] = div >> 8;
				st->Memory.FillRAM[0x4216] = rem & 0xff;
				st->Memory.FillRAM[0x4217] = rem >> 8;
				break;
			}

			case 0x4207: // HTIMEL
				pos = st->PPU.IRQHBeamPos;
				st->PPU.IRQHBeamPos = (st->PPU.IRQHBeamPos & 0xff00) | Byte;
				if (st->PPU.IRQHBeamPos != pos)
					S9xUpdateIRQPositions(st, false);
				break;

			case 0x4208: // HTIMEH
				pos = st->PPU.IRQHBeamPos;
				st->PPU.IRQHBeamPos = (st->PPU.IRQHBeamPos & 0xff) | ((Byte & 1) << 8);
				if (st->PPU.IRQHBeamPos != pos)
					S9xUpdateIRQPositions(st, false);
				break;

			case 0x4209: // VTIMEL
				pos = st->PPU.IRQVBeamPos;
				st->PPU.IRQVBeamPos = (st->PPU.IRQVBeamPos & 0xff00) | Byte;
				if (st->PPU.IRQVBeamPos != pos)
					S9xUpdateIRQPositions(st, true);
				break;

			case 0x420a: // VTIMEH
				pos = st->PPU.IRQVBeamPos;
				st->PPU.IRQVBeamPos = (st->PPU.IRQVBeamPos & 0xff) | ((Byte & 1) << 8);
				if (st->PPU.IRQVBeamPos != pos)
					S9xUpdateIRQPositions(st, true);
				break;

			case 0x420b: // MDMAEN
				if (st->CPU.InDMAorHDMA)
					return;
				// XXX: Not quite right...
				if (Byte)
					st->CPU.Cycles += st->Timings.DMACPUSync;
				if (Byte & 0x01)
					S9xDoDMA(st, 0);
				if (Byte & 0x02)
					S9xDoDMA(st, 1);
				if (Byte & 0x04)
					S9xDoDMA(st, 2);
				if (Byte & 0x08)
					S9xDoDMA(st, 3);
				if (Byte & 0x10)
					S9xDoDMA(st, 4);
				if (Byte & 0x20)
					S9xDoDMA(st, 5);
				if (Byte & 0x40)
					S9xDoDMA(st, 6);
				if (Byte & 0x80)
					S9xDoDMA(st, 7);
				break;

			case 0x420c: // HDMAEN
				if (st->CPU.InDMAorHDMA)
					return;
				st->Memory.FillRAM[0x420c] = Byte;
				// Yoshi's Island, Genjyu Ryodan, Mortal Kombat, Tales of Phantasia
				st->PPU.HDMA = Byte & ~st->PPU.HDMAEnded;
				break;

			case 0x420d: // MEMSEL
				if ((Byte & 1) != (st->Memory.FillRAM[0x420d] & 1))
				{
					st->CPU.FastROMSpeed = Byte & 1 ? ONE_CYCLE : SLOW_ONE_CYCLE;
					// we might currently be in FastROMSpeed region, S9xSetPCBase will update CPU.MemSpeed
					S9xSetPCBase(st, st->Registers.PC.xPBPC);
				}
				break;

			case 0x4210: // RDNMI
			case 0x4211: // TIMEUP
			case 0x4212: // HVBJOY
			case 0x4213: // RDIO
			case 0x4214: // RDDIVL
			case 0x4215: // RDDIVH
			case 0x4216: // RDMPYL
			case 0x4217: // RDMPYH
			case 0x4218: // JOY1L
			case 0x4219: // JOY1H
			case 0x421a: // JOY2L
			case 0x421b: // JOY2H
			case 0x421c: // JOY3L
			case 0x421d: // JOY3H
			case 0x421e: // JOY4L
			case 0x421f: // JOY4H
				return;

			default:
				if (st->Settings.SDD1 && Address >= 0x4804 && Address <= 0x4807)
					S9xSetSDD1MemoryMap(st, Address - 0x4804, Byte & 7);
		}
	}

	st->Memory.FillRAM[Address] = Byte;
}

uint8_t S9xGetCPU(struct S9xState *st, uint16_t Address)
{
	if (Address < 0x4200)
		return st->OpenBus;
	else if ((Address & 0xff80) == 0x4300)
	{
		if (st->CPU.InDMAorHDMA)
			return st->OpenBus;

		int d = (Address >> 4) & 0x7;

		switch (Address & 0xf)
		{
			case 0x0: // 0x43x0: DMAPx
				return (st->DMA[d].ReverseTransfer ? 0x80 : 0) | (st->DMA[d].HDMAIndirectAddressing ? 0x40 : 0) | (st->DMA[d].UnusedBit43x0 ? 0x20 : 0) | (st->DMA[d].AAddressDecrement ? 0x10 : 0) |
					(st->DMA[d].AAddressFixed ? 0x08 : 0) | (st->DMA[d].TransferMode & 7);

			case 0x1: // 0x43x1: BBADx
				return st->DMA[d].BAddress;

			case 0x2: // 0x43x2: A1TxL
				return st->DMA[d].AAddress & 0xff;

			case 0x3: // 0x43x3: A1TxH
				return st->DMA[d].AAddress >> 8;

			case 0x4: // 0x43x4: A1Bx
				return st->DMA[d].ABank;

			case 0x5: // 0x43x5: DASxL
				return st->DMA[d].DMACount_Or_HDMAIndirectAddress & 0xff;

			case 0x6: // 0x43x6: DASxH
				return st->DMA[d].DMACount_Or_HDMAIndirectAddress >> 8;

			case 0x7: // 0x43x7: DASBx
				return st->DMA[d].IndirectBank;

			case 0x8: // 0x43x8: A2AxL
				return st->DMA[d].Address & 0xff;

			case 0x9: // 0x43x9: A2AxH
				return st->DMA[d].Address >> 8;

			case 0xa: // 0x43xa: NLTRx
				return st->DMA[d].LineCount ^ (st->DMA[d].Repeat ? 0x00 : 0x80);

			case 0xb: // 0x43xb: ????x
			case 0xf: // 0x43xf: mirror of 0x43xb
				return st->DMA[d].UnknownByte;

			default:
				return st->OpenBus;
		}
	}
	else
	{
		uint8_t byte;

		switch (Address)
		{
			case 0x4210: // RDNMI
				byte = st->Memory.FillRAM[0x4210];
				st->Memory.FillRAM[0x4210] = st->Model->_5A22;
				return (byte & 0x80) | (st->OpenBus & 0x70) | st->Model->_5A22;

			case 0x4211: // TIMEUP
				byte = st->CPU.IRQLine ? 0x80 : 0;
				st->CPU.IRQLine = st->CPU.IRQTransition = false;
				return byte | (st->OpenBus & 0x7f);

			case 0x4212: // HVBJOY
				return REGISTER_4212(st) | (st->OpenBus & 0x3e);

			case 0x4213: // RDIO
				return st->Memory.FillRAM[0x4213];

			case 0x4214: // RDDIVL
			case 0x4215: // RDDIVH
			case 0x4216: // RDMPYL
			case 0x4217: // RDMPYH
				return st->Memory.FillRAM[Address];

			case 0x4218: // JOY1L
			case 0x4219: // JOY1H
			case 0x421a: // JOY2L
			case 0x421b: // JOY2H
			case 0x421c: // JOY3L
			case 0x421d: // JOY3H
			case 0x421e: // JOY4L
			case 0x421f: // JOY4H
				return st->Memory.FillRAM[Address];

			default:
				if (st->Settings.SDD1 && Address >= 0x4800 && Address <= 0x4807)
					return st->Memory.FillRAM[Address];
				return st->OpenBus;
		}
	}
}

void S9xResetPPU(struct S9xState *st)
{
	S9xSoftResetPPU(st);
	st->PPU.M7byte = 0;
}

void S9xSoftResetPPU(struct S9xState *st)
{
	st->PPU.VMA.High = false;
	st->PPU.VMA.Increment = 1;
	st->PPU.VMA.Address = st->PPU.VMA.FullGraphicCount = st->PPU.VMA.Shift = 0;

	st->PPU.WRAM = 0;

	st->PPU.CGFLIPRead = false;
	st->PPU.CGADD = 0;

	for (int c = 0; c < 256; ++c)
	{
		st->IPPU.Red[c] = (c & 7) << 2;
		st->IPPU.Green[c] = ((c >> 3) & 7) << 2;
		st->IPPU.Blue[c] = ((c >> 6) & 2) << 3;
		st->PPU.CGDATA[c] = st->IPPU.Red[c] | (st->IPPU.Green[c] << 5) | (st->IPPU.Blue[c] << 10);
	}

	st->PPU.OAMAddr = st->PPU.SavedOAMAddr = 0;
	st->PPU.OAMPriorityRotation = false;
	st->PPU.OAMFlip = 0;
	st->PPU.OAMWriteRegister = 0;
	std::fill_n(&st->PPU.OAMData[0], sizeof(st->PPU.OAMData), 0);

	st->PPU.FirstSprite = 0;

	st->PPU.HTimerEnabled = st->PPU.VTimerEnabled = false;
	st->PPU.HTimerPosition = st->Timings.H_Max + 1;
	st->PPU.VTimerPosition = st->Timings.V_Max + 1;
	st->PPU.IRQHBeamPos = st->PPU.IRQVBeamPos = 0x1ff;

	st->PPU.HBeamFlip = st->PPU.VBeamFlip = false;

	st->PPU.MatrixA = st->PPU.MatrixB = 0;

	st->PPU.ForcedBlanking = true;

	st->PPU.ScreenHeight = SNES_HEIGHT;

	st->PPU.Need16x8Mulitply = false;

	st->PPU.HDMA = st->PPU.HDMAEnded = 0;

	st->PPU.OpenBus1 = st->PPU.OpenBus2 = 0;

	st->PPU.VRAMReadBuffer = 0; // XXX: FIXME: anything better?
	st->IPPU.Interlace = false;

	for (int c = 0; c < 0x8000; c += 0x100)
		std::fill_n(&st->Memory.FillRAM[c], 0x100, c >> 8);
	std::fill_n(&st->Memory.FillRAM[0x2100], 0x0100, 0);
	std::fill_n(&st->Memory.FillRAM[0x4200], 0x0100, 0);
	std::fill_n(&st->Memory.FillRAM[0x4000], 0x0100, 0);
	// For BS Suttehakkun 2...
	std::fill_n(&st->Memory.FillRAM[0x1000], 0x1000, 0);

	st->Memory.FillRAM[0x4201] = st->Memory.FillRAM[0x4213] = 0xff;
	st->Memory.FillRAM[0x2126] = st->Memory.FillRAM[0x2128] = 1;
}

void S9xUpdateVRAMReadBuffer(struct S9xState *st)
{
	if (st->PPU.VMA.FullGraphicCount)
	{
		uint32_t addr = st->PPU.VMA.Address;
		uint32_t rem = addr & st->PPU.VMA.Mask1;
		uint32_t address = (addr & ~st->PPU.VMA.Mask1) + (rem >> st->PPU.VMA.Shift) + ((rem & (st->PPU.VMA.FullGraphicCount - 1)) << 3);
		st->PPU.VRAMReadBuffer = READ_WORD(&st->Memory.VRAM[(address << 1) & 0xffff]);
	}
	else
		st->PPU.VRAMReadBuffer = READ_WORD(&st->Memory.VRAM[(st->PPU.VMA.Address << 1) & 0xffff]);
}

void REGISTER_2104(struct S9xState *st, uint8_t Byt)
{
	if (st->PPU.OAMAddr & 0x100)
	{
		int addr = ((st->PPU.OAMAddr & 0x10f) << 1) + (st->PPU.OAMFlip & 1);
		if (Byt != st->PPU.OAMData[addr])
			st->PPU.OAMData[addr] = Byt;

		st->PPU.OAMFlip ^= 1;
		if (!(st->PPU.OAMFlip & 1))
		{
			++st->PPU.OAMAddr;
			st->PPU.OAMAddr &= 0x1ff;
			if (st->PPU.OAMPriorityRotation && st->PPU.FirstSprite != (st->PPU.OAMAddr >> 1))
				st->PPU.FirstSprite = (st->PPU.OAMAddr & 0xfe) >> 1;
		}
	}
	else if (!(st->PPU.OAMFlip & 1))
	{
		st->PPU.OAMWriteRegister &= 0xff00;
		st->PPU.OAMWriteRegister |= Byt;
		st->PPU.OAMFlip |= 1;
	}
	else
	{
		st->PPU.OAMWriteRegister &= 0x00ff;
		uint8_t lowbyte = static_cast<uint8_t>(st->PPU.OAMWriteRegister);
		uint8_t highbyte = Byt;
		st->PPU.OAMWriteRegister |= Byt << 8;

		int addr = st->PPU.OAMAddr << 1;
		if (lowbyte != st->PPU.OAMData[addr] || highbyte != st->PPU.OAMData[addr + 1])
		{
			st->PPU.OAMData[addr] = lowbyte;
			st->PPU.OAMData[addr + 1] = highbyte;
		}

		st->PPU.OAMFlip &= ~1;
		++st->PPU.OAMAddr;
		if (st->PPU.OAMPriorityRotation && st->PPU.FirstSprite != (st->PPU.OAMAddr >> 1))
			st->PPU.FirstSprite = (st->PPU.OAMAddr & 0xfe) >> 1;
	}
}

// This code is correct, however due to Snes9x's inaccurate timings, some games might be broken by this chage. :(
inline bool CHECK_INBLANK(struct S9xState *st)
{
	if (st->Settings.BlockInvalidVRAMAccess && !st->PPU.ForcedBlanking && st->CPU.V_Counter < st->PPU.ScreenHeight + FIRST_VISIBLE_LINE)
	{
		st->PPU.VMA.Address += !st->PPU.VMA.High ? st->PPU.VMA.Increment : 0;
		return true;
	}
	else
		return false;
}

void REGISTER_2118(struct S9xState *st, uint8_t Byt)
{
	if (CHECK_INBLANK(st))
		return;

	uint32_t address;

	if (st->PPU.VMA.FullGraphicCount)
	{
		uint32_t rem = st->PPU.VMA.Address & st->PPU.VMA.Mask1;
		address = (((st->PPU.VMA.Address & ~st->PPU.VMA.Mask1) + (rem >> st->PPU.VMA.Shift) + ((rem & (st->PPU.VMA.FullGraphicCount - 1)) << 3)) << 1) & 0xffff;
		st->Memory.VRAM[address] = Byt;
	}
	else
		st->Memory.VRAM[address = (st->PPU.VMA.Address << 1) & 0xffff] = Byt;

	if (!st->PPU.VMA.High)
		st->PPU.VMA.Address += st->PPU.VMA.Increment;
}

void REGISTER_2118_tile(struct S9xState *st, uint8_t Byt)
{
	if (CHECK_INBLANK(st))
		return;

	uint32_t rem = st->PPU.VMA.Address & st->PPU.VMA.Mask1;
	uint32_t address = (((st->PPU.VMA.Address & ~st->PPU.VMA.Mask1) + (rem >> st->PPU.VMA.Shift) + ((rem & (st->PPU.VMA.FullGraphicCount - 1)) << 3)) << 1) & 0xffff;

	st->Memory.VRAM[address] = Byt;

	if (!st->PPU.VMA.High)
		st->PPU.VMA.Address += st->PPU.VMA.Increment;
}

void REGISTER_2118_linear(struct S9xState *st, uint8_t Byt)
{
	if (CHECK_INBLANK(st))
		return;

	uint32_t address;

	st->Memory.VRAM[address = (st->PPU.VMA.Address << 1) & 0xffff] = Byt;

	if (!st->PPU.VMA.High)
		st->PPU.VMA.Address += st->PPU.VMA.Increment;
}

void REGISTER_2119(struct S9xState *st, uint8_t Byt)
{
	if (CHECK_INBLANK(st))
		return;

	uint32_t address;

	if (st->PPU.VMA.FullGraphicCount)
	{
		uint32_t rem = st->PPU.VMA.Address & st->PPU.VMA.Mask1;
		address = ((((st->PPU.VMA.Address & ~st->PPU.VMA.Mask1) + (rem >> st->PPU.VMA.Shift) + ((rem & (st->PPU.VMA.FullGraphicCount - 1)) << 3)) << 1) + 1) & 0xffff;
		st->Memory.VRAM[address] = Byt;
	}
	else
		st->Memory.VRAM[address = ((st->PPU.VMA.Address << 1) + 1) & 0xffff] = Byt;

	if (st->PPU.VMA.High)
		st->PPU.VMA.Address += st->PPU.VMA.Increment;
}

void REGISTER_2119_tile(struct S9xState *st, uint8_t Byt)
{
	if (CHECK_INBLANK(st))
		return;

	uint32_t rem = st->PPU.VMA.Address & st->PPU.VMA.Mask1;
	uint32_t address = ((((st->PPU.VMA.Address & ~st->PPU.VMA.Mask1) + (rem >> st->PPU.VMA.Shift) + ((rem & (st->PPU.VMA.FullGraphicCount - 1)) << 3)) << 1) + 1) & 0xffff;

	st->Memory.VRAM[address] = Byt;

	if (st->PPU.VMA.High)
		st->PPU.VMA.Address += st->PPU.VMA.Increment;
}

void REGISTER_2119_linear(struct S9xState *st, uint8_t Byt)
{
	if (CHECK_INBLANK(st))
		return;

	uint32_t address;

	st->Memory.VRAM[address = ((st->PPU.VMA.Address << 1) + 1) & 0xffff] = Byt;

	if (st->PPU.VMA.High)
		st->PPU.VMA.Address += st->PPU.VMA.Increment;
}

void REGISTER_2180(struct S9xState *st, uint8_t Byt)
{
	st->Memory.RAM[st->PPU.WRAM++] = Byt;
	st->PPU.WRAM &= 0x1ffff;
}

uint8_t REGISTER_4212(struct S9xState *st)
{
	uint8_t byte = 0;

	if (st->CPU.V_Counter >= st->PPU.ScreenHeight + FIRST_VISIBLE_LINE && st->CPU.V_Counter < st->PPU.ScreenHeight + FIRST_VISIBLE_LINE + 3)
		byte = 1;
	if (st->CPU.Cycles < st->Timings.HBlankEnd || st->CPU.Cycles >= st->Timings.HBlankStart)
		byte |= 0x40;
	if (st->CPU.V_Counter >= st->PPU.ScreenHeight + FIRST_VISIBLE_LINE)
		byte |= 0x80;

	return byte;
}
