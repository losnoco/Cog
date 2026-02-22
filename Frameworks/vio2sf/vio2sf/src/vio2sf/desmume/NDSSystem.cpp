/*
	Copyright (C) 2006 yopyop
	Copyright (C) 2008-2013 DeSmuME team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <zlib.h>
#include "vio2sf.h"
#include "bios.h"

// ===============================================================

int NDS_Init(vio2sf_state *st)
{
	MMU_Init(st);
	st->nds.VCount = 0;

	// got to print this somewhere..
	//fprintf(stderr, "%s\n", EMU_DESMUME_NAME_AND_VERSION());

	armcpu_new(st, &st->NDS_ARM7, 1);
	armcpu_new(st, &st->NDS_ARM9, 0);

	if (SPU_Init(st, SNDCORE_DUMMY, 740))
		return -1;

	return 0;
}

void NDS_DeInit(vio2sf_state *st)
{
	if (st->MMU.CART_ROM != st->MMU.UNUSED_RAM)
		NDS_FreeROM(st);

	SPU_DeInit(st);
	MMU_DeInit(st);
}

bool NDS_SetROM(vio2sf_state *st, uint8_t *rom, uint32_t mask)
{
	MMU_setRom(st, rom, mask);

	return true;
}

std::unique_ptr<NDS_header> NDS_getROMHeader(vio2sf_state *st)
{
	if (st->MMU.CART_ROM == st->MMU.UNUSED_RAM)
		return std::unique_ptr<NDS_header>();
	auto header = std::unique_ptr<NDS_header>(new NDS_header);

	memcpy(header->gameTile, st->MMU.CART_ROM, 12);
	memcpy(header->gameCode, st->MMU.CART_ROM + 12, 4);
	header->makerCode = T1ReadWord(st->MMU.CART_ROM, 16);
	header->unitCode = st->MMU.CART_ROM[18];
	header->deviceCode = st->MMU.CART_ROM[19];
	header->cardSize = st->MMU.CART_ROM[20];
	memcpy(header->cardInfo, st->MMU.CART_ROM + 21, 8);
	header->flags = st->MMU.CART_ROM[29];
	header->romversion = st->MMU.CART_ROM[30];
	header->ARM9src = T1ReadLong(st->MMU.CART_ROM, 32);
	header->ARM9exe = T1ReadLong(st->MMU.CART_ROM, 36);
	header->ARM9cpy = T1ReadLong(st->MMU.CART_ROM, 40);
	header->ARM9binSize = T1ReadLong(st->MMU.CART_ROM, 44);
	header->ARM7src = T1ReadLong(st->MMU.CART_ROM, 48);
	header->ARM7exe = T1ReadLong(st->MMU.CART_ROM, 52);
	header->ARM7cpy = T1ReadLong(st->MMU.CART_ROM, 56);
	header->ARM7binSize = T1ReadLong(st->MMU.CART_ROM, 60);
	header->FNameTblOff = T1ReadLong(st->MMU.CART_ROM, 64);
	header->FNameTblSize = T1ReadLong(st->MMU.CART_ROM, 68);
	header->FATOff = T1ReadLong(st->MMU.CART_ROM, 72);
	header->FATSize = T1ReadLong(st->MMU.CART_ROM, 76);
	header->ARM9OverlayOff = T1ReadLong(st->MMU.CART_ROM, 80);
	header->ARM9OverlaySize = T1ReadLong(st->MMU.CART_ROM, 84);
	header->ARM7OverlayOff = T1ReadLong(st->MMU.CART_ROM, 88);
	header->ARM7OverlaySize = T1ReadLong(st->MMU.CART_ROM, 92);
	header->unknown2a = T1ReadLong(st->MMU.CART_ROM, 96);
	header->unknown2b = T1ReadLong(st->MMU.CART_ROM, 100);
	header->IconOff = T1ReadLong(st->MMU.CART_ROM, 104);
	header->CRC16 = T1ReadWord(st->MMU.CART_ROM, 108);
	header->ROMtimeout = T1ReadWord(st->MMU.CART_ROM, 110);
	header->ARM9unk = T1ReadLong(st->MMU.CART_ROM, 112);
	header->ARM7unk = T1ReadLong(st->MMU.CART_ROM, 116);
	memcpy(header->unknown3c, st->MMU.CART_ROM + 120, 8);
	header->ROMSize = T1ReadLong(st->MMU.CART_ROM, 128);
	header->HeaderSize = T1ReadLong(st->MMU.CART_ROM, 132);
	memcpy(header->unknown5, st->MMU.CART_ROM + 136, 56);
	memcpy(header->logo, st->MMU.CART_ROM + 192, 156);
	header->logoCRC16 = T1ReadWord(st->MMU.CART_ROM, 348);
	header->headerCRC16 = T1ReadWord(st->MMU.CART_ROM, 350);
	memcpy(header->reserved, st->MMU.CART_ROM + 352, std::min<size_t>(160, st->gameInfo.romsize - 352));

	return header;
}

RomBanner::RomBanner(bool defaultInit)
{
	if (!defaultInit)
		return;
	this->version = 1; //Version  (0001h)
	this->crc16 = 0; //CRC16 across entries 020h..83Fh
	memset(this->reserved, 0, sizeof(this->reserved));
	memset(this->bitmap, 0, sizeof(this->bitmap));
	memset(this->palette, 0, sizeof(this->palette));
	memset(this->titles, 0, sizeof(this->titles));
	memset(this->end0xFF, 0, sizeof(this->end0xFF));
}

void NDS_FreeROM(vio2sf_state *st)
{
	if (st->MMU.CART_ROM == reinterpret_cast<uint8_t *>(&st->gameInfo.romdata[0]))
		st->gameInfo.romdata.reset();
	if (st->MMU.CART_ROM != st->MMU.UNUSED_RAM)
		delete [] st->MMU.CART_ROM;
	MMU_unsetRom(st);
}

void NDS_Sleep(vio2sf_state *st) { st->nds.sleeping = true; }

enum ESI_DISPCNT
{
	ESI_DISPCNT_HStart, ESI_DISPCNT_HStartIRQ, ESI_DISPCNT_HDraw, ESI_DISPCNT_HBlank
};

bool TSequenceItem::isTriggered() const
{
	return this->enabled && st->nds_timer >= this->timestamp;
}

template<int procnum, int num>
bool TSequenceItem_Timer<procnum, num>::isTriggered() const
{
	return this->enabled && st->nds_timer >= st->nds.timerCycle[procnum][num];
}

template<int procnum, int num>
void TSequenceItem_Timer<procnum, num>::schedule()
{
	this->enabled = st->MMU.timerON[procnum][num] && st->MMU.timerMODE[procnum][num] != 0xFFFF;
}

template<int procnum, int num>
uint64_t TSequenceItem_Timer<procnum, num>::next() const
{
	return st->nds.timerCycle[procnum][num];
}

template<int procnum, int num>
void TSequenceItem_Timer<procnum, num>::exec()
{
	uint8_t *regs = !procnum ? st->MMU.ARM9_REG : st->MMU.ARM7_REG;
	bool first = true, over;
	// we'll need to check chained timers..
	for (int i = num; i < 4; ++i)
	{
		// maybe too many checks if this is here, but we need it here for now
		if (!st->MMU.timerON[procnum][i])
			return;

		if (st->MMU.timerMODE[procnum][i] == 0xFFFF)
		{
			++st->MMU.timer[procnum][i];
			over = !st->MMU.timer[procnum][i];
		}
		else
		{
			if (!first)
				break; // this timer isn't chained. break the chain
			first = false;

			over = true;
			int remain = 65536 - st->MMU.timerReload[procnum][i];
			int ctr = 0;
			while (st->nds.timerCycle[procnum][i] <= st->nds_timer)
			{
				st->nds.timerCycle[procnum][i] += remain << st->MMU.timerMODE[procnum][i];
				++ctr;
			}

/*#ifndef NDEBUG
			if (ctr > 1)
				fprintf(stderr, "yikes!!!!! please report!\n");
#endif*/
		}

		if (over)
		{
			st->MMU.timer[procnum][i] = st->MMU.timerReload[procnum][i];
			if (T1ReadWord(regs, 0x102 + i * 4) & 0x40)
				NDS_makeIrq(st, procnum, IRQ_BIT_TIMER_0 + i);
		}
		else
			break; // no more chained timers to trigger. we're done here
	}
}

template<int procnum, int chan>
bool TSequenceItem_DMA<procnum, chan>::isTriggered() const
{
	return this->controller->dmaCheck && st->nds_timer >= this->controller->nextEvent;
}

template<int procnum, int chan>
bool TSequenceItem_DMA<procnum, chan>::isEnabled() const
{
	return this->controller->dmaCheck;
}

template<int procnum, int chan>
uint64_t TSequenceItem_DMA<procnum, chan>::next() const
{
	return this->controller->nextEvent;
}

template<int procnum, int chan>
void TSequenceItem_DMA<procnum, chan>::exec()
{
	//fprintf(stderr, "exec from TSequenceItem_DMA: %d %d\n",procnum,chan);
	this->controller->exec();
}

bool TSequenceItem_divider::isTriggered() const
{
	return st->MMU.divRunning && st->nds_timer >= st->MMU.divCycles;
}

bool TSequenceItem_divider::isEnabled()
{
	return st->MMU.divRunning;
}

uint64_t TSequenceItem_divider::next() const
{
	return st->MMU.divCycles;
}

void TSequenceItem_divider::exec()
{
	st->MMU_new.div.busy = 0;
#if defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
	T1WriteQuad(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x2A0, st->MMU.divResult);
	T1WriteQuad(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x2A8, st->MMU.divMod);
#else
	T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x2A0, st->MMU.divResult & 0xFFFFFFFF);
	T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x2A4, (st->MMU.divResult >> 32) & 0xFFFFFFFF);
	T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x2A8, st->MMU.divMod & 0xFFFFFFFF);
	T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x2AC, (st->MMU.divMod >> 32) & 0xFFFFFFFF);
#endif
	st->MMU.divRunning = false;
}

bool TSequenceItem_sqrtunit::isTriggered() const
{
	return st->MMU.sqrtRunning && st->nds_timer >= st->MMU.sqrtCycles;
}

bool TSequenceItem_sqrtunit::isEnabled()
{
	return st->MMU.sqrtRunning;
}

uint64_t TSequenceItem_sqrtunit::next() const
{
	return st->MMU.sqrtCycles;
}

void TSequenceItem_sqrtunit::exec()
{
	st->MMU_new.sqrt.busy = 0;
	T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x2B4, st->MMU.sqrtResult);
	st->MMU.sqrtRunning = false;
}

void NDS_RescheduleTimers(vio2sf_state *st)
{
#define check(X, Y) st->sequencer.timer_##X##_##Y .schedule();
	check(0, 0); check(0, 1); check(0, 2); check(0, 3);
	check(1, 0); check(1, 1); check(1, 2); check(1, 3);
#undef check

	NDS_Reschedule(st);
}

void NDS_RescheduleDMA(vio2sf_state *st)
{
	//TBD
	NDS_Reschedule(st);
}

Sequencer::Sequencer(vio2sf_state *_st)
	: st(_st),
	  dispcnt(st), wifi(st), divider(st), sqrtunit(st), gxfifo(st),
	  dma_0_0(st), dma_0_1(st), dma_0_2(st), dma_0_3(st),
	  dma_1_0(st), dma_1_1(st), dma_1_2(st), dma_1_3(st),
	  timer_0_0(st), timer_0_1(st), timer_0_2(st), timer_0_3(st),
	  timer_1_0(st), timer_1_1(st), timer_1_2(st), timer_1_3(st)
{
	if(!_st) return;

	init();

	// begin at the very end of the last scanline
	// so that at t=0 we can increment to scanline=0
	_st->nds.VCount = 262;

	nds_vblankEnded = false;
}

// 2196372 ~= (ARM7_CLOCK << 16) / 1000000
// This value makes more sense to me, because:
// ARM7_CLOCK   = 33.51 mhz
//				= 33513982 cycles per second
// 				= 33.513982 cycles per microsecond
//const uint64_t kWifiCycles = 34*2;
//(this isn't very precise. I don't think it needs to be)

void Sequencer::init()
{
	NDS_RescheduleTimers(st);
	NDS_RescheduleDMA(st);

	this->reschedule = false;
	st->nds_timer = 0;
	st->nds_arm9_timer = 0;
	st->nds_arm7_timer = 0;

	this->dispcnt.enabled = true;
	this->dispcnt.param = ESI_DISPCNT_HStart;
	this->dispcnt.timestamp = 0;

	this->dma_0_0.controller = &st->MMU_new.dma[0][0];
	this->dma_0_1.controller = &st->MMU_new.dma[0][1];
	this->dma_0_2.controller = &st->MMU_new.dma[0][2];
	this->dma_0_3.controller = &st->MMU_new.dma[0][3];
	this->dma_1_0.controller = &st->MMU_new.dma[1][0];
	this->dma_1_1.controller = &st->MMU_new.dma[1][1];
	this->dma_1_2.controller = &st->MMU_new.dma[1][2];
	this->dma_1_3.controller = &st->MMU_new.dma[1][3];
}

static void execHardware_hblank(vio2sf_state *st)
{
	// this logic keeps moving around.
	// now, we try and give the game as much time as possible to finish doing its work for the scanline,
	// by drawing scanline N at the end of drawing time (but before subsequent interrupt or hdma-driven events happen)
	// don't try to do this at the end of the scanline, because some games (sonic classics) may use hblank IRQ to set
	// scroll regs for the next scanline
	if (st->nds.VCount < 192)
		// trigger hblank dmas
		// but notice, we do that just after we finished drawing the line
		// (values copied by this hdma should not be used until the next scanline)
		triggerDma(st, EDMAMode_HBlank);

	// turn on hblank status bit
	T1WriteWord(st->MMU.ARM9_REG, 4, T1ReadWord(st->MMU.ARM9_REG, 4) | 2);
	T1WriteWord(st->MMU.ARM7_REG, 4, T1ReadWord(st->MMU.ARM7_REG, 4) | 2);

	// fire hblank interrupts if necessary
	if (T1ReadWord(st->MMU.ARM9_REG, 4) & 0x10)
		NDS_makeIrq(st, ARMCPU_ARM9, IRQ_BIT_LCD_HBLANK);
	if (T1ReadWord(st->MMU.ARM7_REG, 4) & 0x10)
		NDS_makeIrq(st, ARMCPU_ARM7, IRQ_BIT_LCD_HBLANK);

	// emulation housekeeping. for some reason we always do this at hblank,
	// even though it sounds more reasonable to do it at hstart
	SPU_Emulate_core(st);
}

static void execHardware_hstart_vblankEnd(vio2sf_state *st)
{
	st->sequencer.nds_vblankEnded = true;
	st->sequencer.reschedule = true;

	// turn off vblank status bit
	T1WriteWord(st->MMU.ARM9_REG, 4, T1ReadWord(st->MMU.ARM9_REG, 4) & ~1);
	T1WriteWord(st->MMU.ARM7_REG, 4, T1ReadWord(st->MMU.ARM7_REG, 4) & ~1);
}

static void execHardware_hstart_vblankStart(vio2sf_state *st)
{
	//fprintf(stderr, "--------VBLANK!!!--------\n");

	// fire vblank interrupts if necessary
	for (int i = 0; i < 2; ++i)
		if (st->MMU.reg_IF_pending[i] & (1 << IRQ_BIT_LCD_VBLANK))
		{
			st->MMU.reg_IF_pending[i] &= ~(1 << IRQ_BIT_LCD_VBLANK);
			NDS_makeIrq(st, i, IRQ_BIT_LCD_VBLANK);
		}

	// trigger vblank dmas
	triggerDma(st, EDMAMode_VBlank);
}

static uint16_t execHardware_gen_vmatch_goal(vio2sf_state *st)
{
	uint16_t vmatch = T1ReadWord(st->MMU.ARM9_REG, 4);
	vmatch = (vmatch >> 8) | ((vmatch << 1) & (1 << 8));
	return vmatch;
}

static void execHardware_hstart_vcount_irq(vio2sf_state *st)
{
	// trigger pending VMATCH irqs
	if (st->MMU.reg_IF_pending[ARMCPU_ARM9] & (1 << IRQ_BIT_LCD_VMATCH))
	{
		st->MMU.reg_IF_pending[ARMCPU_ARM9] &= ~(1 << IRQ_BIT_LCD_VMATCH);
		NDS_makeIrq(st, ARMCPU_ARM9, IRQ_BIT_LCD_VMATCH);
	}
	if(st->MMU.reg_IF_pending[ARMCPU_ARM7] & (1 << IRQ_BIT_LCD_VMATCH))
	{
		st->MMU.reg_IF_pending[ARMCPU_ARM7] &= ~(1 << IRQ_BIT_LCD_VMATCH);
		NDS_makeIrq(st, ARMCPU_ARM7, IRQ_BIT_LCD_VMATCH);
	}
}

static void execHardware_hstart_vcount(vio2sf_state *st)
{
	uint16_t vmatch = execHardware_gen_vmatch_goal(st);
	if (st->nds.VCount == vmatch)
	{
		// arm9 vmatch
		T1WriteWord(st->MMU.ARM9_REG, 4, T1ReadWord(st->MMU.ARM9_REG, 4) | 4);
		if (T1ReadWord(st->MMU.ARM9_REG, 4) & 32)
			st->MMU.reg_IF_pending[ARMCPU_ARM9] |= 1 << IRQ_BIT_LCD_VMATCH;
	}
	else
		T1WriteWord(st->MMU.ARM9_REG, 4, T1ReadWord(st->MMU.ARM9_REG, 4) & 0xFFFB);

	vmatch = T1ReadWord(st->MMU.ARM7_REG, 4);
	vmatch = (vmatch >> 8) | ((vmatch << 1) & (1 << 8));
	if (st->nds.VCount == vmatch)
	{
		// arm7 vmatch
		T1WriteWord(st->MMU.ARM7_REG, 4, T1ReadWord(st->MMU.ARM7_REG, 4) | 4);
		if (T1ReadWord(st->MMU.ARM7_REG, 4) & 32)
			st->MMU.reg_IF_pending[ARMCPU_ARM7] |= 1 << IRQ_BIT_LCD_VMATCH;
	}
	else
		T1WriteWord(st->MMU.ARM7_REG, 4, T1ReadWord(st->MMU.ARM7_REG, 4) & 0xFFFB);
}

static void execHardware_hstart_irq(vio2sf_state *st)
{
	// this function very soon after the registers get updated to trigger IRQs
	// this is necessary to fix "egokoro kyoushitsu" which idles waiting for vcount=192, which never happens due to a long vblank irq
	// 100% accurate emulation would require the read of VCOUNT to be in the pipeline already with the irq coming in behind it, thus
	// allowing the vcount to register as 192 occasionally (maybe about 1 out of 28 frames)
	// the actual length of the delay is in execHardware() where the events are scheduled
	st->sequencer.reschedule = true;
	if (st->nds.VCount == 192)
		// when the vcount hits 192, vblank begins
		execHardware_hstart_vblankStart(st);

	execHardware_hstart_vcount_irq(st);
}

static void execHardware_hstart(vio2sf_state *st)
{
	++st->nds.VCount;

	if (st->nds.VCount == 263)
		// when the vcount hits 263 it rolls over to 0
		st->nds.VCount = 0;
	if (st->nds.VCount == 262)
		// when the vcount hits 262, vblank ends (oam pre-renders by one scanline)
		execHardware_hstart_vblankEnd(st);
	else if (st->nds.VCount == 192)
	{
		// turn on vblank status bit
		T1WriteWord(st->MMU.ARM9_REG, 4, T1ReadWord(st->MMU.ARM9_REG, 4) | 1);
		T1WriteWord(st->MMU.ARM7_REG, 4, T1ReadWord(st->MMU.ARM7_REG, 4) | 1);

		// check whether we'll need to fire vblank irqs
		if (T1ReadWord(st->MMU.ARM9_REG, 4) & 0x8)
			st->MMU.reg_IF_pending[ARMCPU_ARM9] |= 1 << IRQ_BIT_LCD_VBLANK;
		if (T1ReadWord(st->MMU.ARM7_REG, 4) & 0x8)
			st->MMU.reg_IF_pending[ARMCPU_ARM7] |= 1 << IRQ_BIT_LCD_VBLANK;
	}

	// write the new vcount
	T1WriteWord(st->MMU.ARM9_REG, 6, st->nds.VCount & 0xFFFF);
	T1WriteWord(st->MMU.ARM9_REG, 0x1006, st->nds.VCount & 0xFFFF);
	T1WriteWord(st->MMU.ARM7_REG, 6, st->nds.VCount & 0xFFFF);
	T1WriteWord(st->MMU.ARM7_REG, 0x1006, st->nds.VCount & 0xFFFF);

	// turn off hblank status bit
	T1WriteWord(st->MMU.ARM9_REG, 4, T1ReadWord(st->MMU.ARM9_REG, 4) & 0xFFFD);
	T1WriteWord(st->MMU.ARM7_REG, 4, T1ReadWord(st->MMU.ARM7_REG, 4) & 0xFFFD);

	// handle vcount status
	execHardware_hstart_vcount(st);

	// trigger hstart dmas
	triggerDma(st, EDMAMode_HStart);

	if (st->nds.VCount < 192)
		// this is hacky.
		// there is a corresponding hack in doDMA.
		// it should be driven by a fifo (and generate just in time as the scanline is displayed)
		// but that isnt even possible until we have some sort of sub-scanline timing.
		// it may not be necessary.
		triggerDma(st, EDMAMode_MemDisplay);
}

void NDS_Reschedule(vio2sf_state *st)
{
	st->sequencer.reschedule = true;
}

static inline uint64_t _fast_min(uint64_t a, uint64_t b)
{
	// you might find that this is faster on a 64bit system; someone should try it
	// http://aggregate.org/MAGIC/#Integer%20Selection
	//uint64_t ret = (((((s64)(a-b)) >> (64-1)) & (a^b)) ^ b);
	//assert(ret==min(a,b));
	//return ret;

	// but this ends up being the fastest on 32bits
	return a<b?a:b;

	// just for the record, I tried to do the 64bit math on a 32bit proc
	// using sse2 and it was really slow
	//__m128i __a; __a.m128i_u64[0] = a;
	//__m128i __b; __b.m128i_u64[0] = b;
	//__m128i xorval = _mm_xor_si128(__a,__b);
	//__m128i temp = _mm_sub_epi64(__a,__b);
	//temp.m128i_i64[0] >>= 63; //no 64bit shra in sse2, what a disappointment
	//temp = _mm_and_si128(temp,xorval);
	//temp = _mm_xor_si128(temp,__b);
	//return temp.m128i_u64[0];
}

uint64_t Sequencer::findNext()
{
	// this one is always enabled so dont bother to check it
	uint64_t next = this->dispcnt.next();

	if (this->divider.isEnabled())
		next = _fast_min(next, this->divider.next());
	if (this->sqrtunit.isEnabled())
		next = _fast_min(next, this->sqrtunit.next());

#define test(X, Y) \
	if (this->dma_##X##_##Y .isEnabled()) \
		next = _fast_min(next, this->dma_##X##_##Y .next());
	test(0, 0); test(0, 1); test(0, 2); test(0, 3);
	test(1, 0); test(1, 1); test(1, 2); test(1, 3);
#undef test
#define test(X, Y) \
	if (this->timer_##X##_##Y .enabled) \
		next = _fast_min(next, this->timer_##X##_##Y .next());
	test(0, 0); test(0, 1); test(0, 2); test(0, 3);
	test(1, 0); test(1, 1); test(1, 2); test(1, 3);
#undef test

	return next;
}

void Sequencer::execHardware()
{
	if (this->dispcnt.isTriggered())
	{
		switch (this->dispcnt.param)
		{
			case ESI_DISPCNT_HStart:
				execHardware_hstart(st);
				// (used to be 3168)
				// hstart is actually 8 dots before the visible drawing begins
				// we're going to run 1 here and then run 7 in the next case
				this->dispcnt.timestamp += 12;
				this->dispcnt.param = ESI_DISPCNT_HStartIRQ;
				break;
			case ESI_DISPCNT_HStartIRQ:
				execHardware_hstart_irq(st);
				this->dispcnt.timestamp += 84;
				this->dispcnt.param = ESI_DISPCNT_HDraw;
				break;
			case ESI_DISPCNT_HDraw:
				// duration of non-blanking period is ~1606 clocks (gbatek agrees) [but says its different on arm7]
				// im gonna call this 267 dots = 267*6=1602
				// so, this event lasts 267 dots minus the 8 dot preroll
				this->dispcnt.timestamp += 3108;
				this->dispcnt.param = ESI_DISPCNT_HBlank;
				break;
			case ESI_DISPCNT_HBlank:
				execHardware_hblank(st);
				// (once this was 1092 or 1092/12=91 dots.)
				// there are surely 355 dots per scanline, less 267 for non-blanking period. the rest is hblank and then after that is hstart
				this->dispcnt.timestamp += 1056;
				this->dispcnt.param = ESI_DISPCNT_HStart;
		}
	}

	if (this->divider.isTriggered())
		this->divider.exec();
	if (this->sqrtunit.isTriggered())
		this->sqrtunit.exec();

#define test(X, Y) \
	if (this->dma_##X##_##Y .isTriggered()) \
		this->dma_##X##_##Y .exec();
	test(0, 0); test(0, 1); test(0, 2); test(0, 3);
	test(1, 0); test(1, 1); test(1, 2); test(1, 3);
#undef test
#define test(X, Y) \
	if (this->timer_##X##_##Y .enabled && this->timer_##X##_##Y .isTriggered()) \
		this->timer_##X##_##Y .exec();
	test(0, 0); test(0, 1); test(0, 2); test(0, 3);
	test(1, 0); test(1, 1); test(1, 2); test(1, 3);
#undef test
}

void execHardware_interrupts(vio2sf_state *st);

// these have not been tuned very well yet.
static const int kMaxWork = 4000;
static const int kIrqWait = 4000;

template<bool doarm9, bool doarm7> static inline int32_t minarmtime(int32_t arm9, int32_t arm7)
{
	if (doarm9)
	{
		if (doarm7)
			return std::min(arm9, arm7);
		else
			return arm9;
	}
	else
		return arm7;
}

template<bool doarm9, bool doarm7>
static std::pair<int32_t, int32_t> armInnerLoop(vio2sf_state *st, uint64_t nds_timer_base, int32_t s32next, int32_t arm9, int32_t arm7)
{
	int32_t timer = minarmtime<doarm9, doarm7>(arm9, arm7);
	while (timer < s32next && !st->sequencer.reschedule && st->execute)
	{
		if (doarm9 && (!doarm7 || arm9 <= timer))
		{
			if (!st->NDS_ARM9.waitIRQ && !st->nds.freezeBus)
			{
				arm9 += armcpu_exec<ARMCPU_ARM9>(&st->NDS_ARM9);
			}
			else
				arm9 = std::min(s32next, arm9 + kIrqWait);
		}
		if (doarm7 && (!doarm9 || arm7 <= timer))
		{
			if (!st->NDS_ARM7.waitIRQ && !st->nds.freezeBus)
			{
				arm7 += armcpu_exec<ARMCPU_ARM7>(&st->NDS_ARM7) << 1;
			}
			else
			{
				arm7 = std::min(s32next, arm7 + kIrqWait);
				if (arm7 == s32next)
				{
					st->nds_timer = nds_timer_base + minarmtime<doarm9, false>(arm9, arm7);
					return armInnerLoop<doarm9, false>(st, nds_timer_base, s32next, arm9, arm7);
				}
			}
		}

		timer = minarmtime<doarm9, doarm7>(arm9, arm7);
		st->nds_timer = nds_timer_base + timer;
	}

	return std::make_pair(arm9, arm7);
}

template<bool FORCE> void NDS_exec(vio2sf_state *st, int32_t)
{
	st->sequencer.nds_vblankEnded = false;

	if (st->nds.sleeping)
	{
		// speculative code: if ANY irq happens, wake up the arm7.
		// I think the arm7 program analyzes the system and may decide not to wake up
		// if it is dissatisfied with the conditions
		if (st->MMU.reg_IE[1] & st->MMU.gen_IF<1>())
			st->nds.sleeping = false;
	}
	else
	{
		for (;;)
		{
			st->sequencer.execHardware();

			// break out once per frame
			if (st->sequencer.nds_vblankEnded)
				break;
			// it should be benign to execute execHardware in the next frame,
			// since there won't be anything for it to do (everything should be scheduled in the future)

			// bail in case the system halted
			if (!st->execute)
				break;

			execHardware_interrupts(st);

			// find next work unit:
			uint64_t next = st->sequencer.findNext();
			next = std::min(next, st->nds_timer + kMaxWork); // lets set an upper limit for now

			//fprintf(stderr, "%d\n", next - nds_timer);

			st->sequencer.reschedule = false;

			// cast these down to 32bits so that things run faster on 32bit procs
			uint64_t nds_timer_base = st->nds_timer;
			int32_t arm9 = (st->nds_arm9_timer - st->nds_timer) & 0xFFFFFFFF;
			int32_t arm7 = (st->nds_arm7_timer - st->nds_timer) & 0xFFFFFFFF;
			int32_t s32next = (next - st->nds_timer) & 0xFFFFFFFF;

			auto arm9arm7 = armInnerLoop<true, true>(st, nds_timer_base, s32next, arm9, arm7);

			arm9 = arm9arm7.first;
			arm7 = arm9arm7.second;
			st->nds_arm7_timer = nds_timer_base + arm7;
			st->nds_arm9_timer = nds_timer_base + arm9;

			// if we were waiting for an irq, don't wait too long:
			// let's re-analyze it after this hardware event (this rolls back a big burst of irq waiting which may have been interrupted by a resynch)
			if (st->NDS_ARM9.waitIRQ)
				st->nds_arm9_timer = st->nds_timer;
			if (st->NDS_ARM7.waitIRQ)
				st->nds_arm7_timer = st->nds_timer;
		}
	}
}

template<int PROCNUM> static void execHardware_interrupts_core(vio2sf_state *st)
{
	uint32_t IF = st->MMU.gen_IF<PROCNUM>();
	uint32_t IE = st->MMU.reg_IE[PROCNUM];
	uint32_t masked = IF & IE;
	if (ARMPROC.halt_IE_and_IF && masked)
	{
		ARMPROC.halt_IE_and_IF = false;
		ARMPROC.waitIRQ = false;
	}

	if (masked && st->MMU.reg_IME[PROCNUM] && !ARMPROC.CPSR.bits.I)
	{
		//fprintf(stderr, "Executing IRQ on procnum %d with IF = %08X and IE = %08X\n",PROCNUM,IF,IE);
		armcpu_irqException(&ARMPROC);
	}
}

void execHardware_interrupts(vio2sf_state *st)
{
	execHardware_interrupts_core<ARMCPU_ARM9>(st);
	execHardware_interrupts_core<ARMCPU_ARM7>(st);
}

static void PrepareBiosARM7(vio2sf_state *st)
{
	st->NDS_ARM7.BIOS_loaded = false;
	memset(st->MMU.ARM7_BIOS, 0, sizeof(st->MMU.ARM7_BIOS));
#if 0
	if (st->CommonSettings.UseExtBIOS)
	{
		// read arm7 bios from inputfile and flag it if it succeeds
		FILE *arm7inf = fopen(st->CommonSettings.ARM7BIOS, "rb");
		if (fread(st->MMU.ARM7_BIOS, 1, 16384, arm7inf) == 16384)
			st->NDS_ARM7.BIOS_loaded = true;
		fclose(arm7inf);
	}
#endif

	// choose to use SWI emulation or routines from bios
	if (st->CommonSettings.SWIFromBIOS && st->NDS_ARM7.BIOS_loaded)
	{
		st->NDS_ARM7.swi_tab = 0;

		// if we used routines from bios, apply patches
		if (st->CommonSettings.PatchSWI3)
			_MMU_write16<ARMCPU_ARM7>(st, 0x00002F08, 0x4770);
	}
	else
		st->NDS_ARM7.swi_tab = ARM_swi_tab[ARMCPU_ARM7];

	if (!st->NDS_ARM7.BIOS_loaded)
	{
		// fake bios content, critical to normal operations, since we dont have a real bios.

#if 0
		// someone please document what is in progress here
		// TODO
		T1WriteLong(st->MMU.ARM7_BIOS, 0x0000, 0xEAFFFFFE); // loop for Reset !!!
		T1WriteLong(st->MMU.ARM7_BIOS, 0x0004, 0xEAFFFFFE); // loop for Undef instr expection
		T1WriteLong(st->MMU.ARM7_BIOS, 0x0008, 0xEA00009C); // SWI
		T1WriteLong(st->MMU.ARM7_BIOS, 0x000C, 0xEAFFFFFE); // loop for Prefetch Abort
		T1WriteLong(st->MMU.ARM7_BIOS, 0x0010, 0xEAFFFFFE); // loop for Data Abort
		T1WriteLong(st->MMU.ARM7_BIOS, 0x0014, 0x00000000); // Reserved
		T1WriteLong(st->MMU.ARM7_BIOS, 0x001C, 0x00000000); // Fast IRQ
#endif
		T1WriteLong(st->MMU.ARM7_BIOS, 0x0000, 0xE25EF002);
		T1WriteLong(st->MMU.ARM7_BIOS, 0x0018, 0xEA000000);
		T1WriteLong(st->MMU.ARM7_BIOS, 0x0020, 0xE92D500F);
		T1WriteLong(st->MMU.ARM7_BIOS, 0x0024, 0xE3A00301);
		T1WriteLong(st->MMU.ARM7_BIOS, 0x0028, 0xE28FE000);
		T1WriteLong(st->MMU.ARM7_BIOS, 0x002C, 0xE510F004);
		T1WriteLong(st->MMU.ARM7_BIOS, 0x0030, 0xE8BD500F);
		T1WriteLong(st->MMU.ARM7_BIOS, 0x0034, 0xE25EF004);
	}
}

static void PrepareBiosARM9(vio2sf_state *st)
{
	memset(st->MMU.ARM9_BIOS, 0, sizeof(st->MMU.ARM9_BIOS));
	st->NDS_ARM9.BIOS_loaded = false;
#if 0
	if (st->CommonSettings.UseExtBIOS)
	{
		// read arm9 bios from inputfile and flag it if it succeeds
		FILE *arm9inf = fopen(st->CommonSettings.ARM9BIOS, "rb");
		if (fread(st->MMU.ARM9_BIOS, 1, 4096, arm9inf) == 4096)
			st->NDS_ARM9.BIOS_loaded = true;
		fclose(arm9inf);
	}
#endif

	// choose to use SWI emulation or routines from bios
	if (st->CommonSettings.SWIFromBIOS && st->NDS_ARM9.BIOS_loaded)
	{
		st->NDS_ARM9.swi_tab = 0;

		// if we used routines from bios, apply patches
		if (st->CommonSettings.PatchSWI3)
			_MMU_write16<ARMCPU_ARM9>(st, 0xFFFF07CC, 0x4770);
	}
	else
		st->NDS_ARM9.swi_tab = ARM_swi_tab[ARMCPU_ARM9];

	if (!st->NDS_ARM9.BIOS_loaded)
	{
		// fake bios content, critical to normal operations, since we dont have a real bios.
		// it'd be cool if we could write this in some kind of assembly language, inline or otherwise, without some bulky dependencies
		// perhaps we could build it with devkitarm? but thats bulky (offline) dependencies, to be sure..

		// reminder: bios chains data abort to fast irq

		// exception vectors:
		T1WriteLong(st->MMU.ARM9_BIOS, 0x0000, 0xEAFFFFFE); // (infinite loop for) Reset !!!
		//T1WriteLong(st->MMU.ARM9_BIOS, 0x0004, 0xEAFFFFFE); // (infinite loop for) Undefined instruction
		T1WriteLong(st->MMU.ARM9_BIOS, 0x0004, 0xEA000004); // Undefined instruction -> Fast IRQ (just guessing)
		T1WriteLong(st->MMU.ARM9_BIOS, 0x0008, 0xEA00009C); // SWI -> ?????
		T1WriteLong(st->MMU.ARM9_BIOS, 0x000C, 0xEAFFFFFE); // (infinite loop for) Prefetch Abort
		T1WriteLong(st->MMU.ARM9_BIOS, 0x0010, 0xEA000001); // Data Abort -> Fast IRQ
		T1WriteLong(st->MMU.ARM9_BIOS, 0x0014, 0x00000000); // Reserved
		T1WriteLong(st->MMU.ARM9_BIOS, 0x0018, 0xEA000095); // Normal IRQ -> 0x0274
		T1WriteLong(st->MMU.ARM9_BIOS, 0x001C, 0xEA00009D); // Fast IRQ -> 0x0298

		static const uint8_t logo_data[] =
		{
			0x24, 0xFF, 0xAE, 0x51, 0x69, 0x9A, 0xA2, 0x21, 0x3D, 0x84, 0x82, 0x0A, 0x84, 0xE4, 0x09, 0xAD,
			0x11, 0x24, 0x8B, 0x98, 0xC0, 0x81, 0x7F, 0x21, 0xA3, 0x52, 0xBE, 0x19, 0x93, 0x09, 0xCE, 0x20,
			0x10, 0x46, 0x4A, 0x4A, 0xF8, 0x27, 0x31, 0xEC, 0x58, 0xC7, 0xE8, 0x33, 0x82, 0xE3, 0xCE, 0xBF,
			0x85, 0xF4, 0xDF, 0x94, 0xCE, 0x4B, 0x09, 0xC1, 0x94, 0x56, 0x8A, 0xC0, 0x13, 0x72, 0xA7, 0xFC,
			0x9F, 0x84, 0x4D, 0x73, 0xA3, 0xCA, 0x9A, 0x61, 0x58, 0x97, 0xA3, 0x27, 0xFC, 0x03, 0x98, 0x76,
			0x23, 0x1D, 0xC7, 0x61, 0x03, 0x04, 0xAE, 0x56, 0xBF, 0x38, 0x84, 0x00, 0x40, 0xA7, 0x0E, 0xFD,
			0xFF, 0x52, 0xFE, 0x03, 0x6F, 0x95, 0x30, 0xF1, 0x97, 0xFB, 0xC0, 0x85, 0x60, 0xD6, 0x80, 0x25,
			0xA9, 0x63, 0xBE, 0x03, 0x01, 0x4E, 0x38, 0xE2, 0xF9, 0xA2, 0x34, 0xFF, 0xBB, 0x3E, 0x03, 0x44,
			0x78, 0x00, 0x90, 0xCB, 0x88, 0x11, 0x3A, 0x94, 0x65, 0xC0, 0x7C, 0x63, 0x87, 0xF0, 0x3C, 0xAF,
			0xD6, 0x25, 0xE4, 0x8B, 0x38, 0x0A, 0xAC, 0x72, 0x21, 0xD4, 0xF8, 0x07
		};

		// logo (do some games fail to boot without this? example?)
		for (int t = 0; t < 0x9C; ++t)
			st->MMU.ARM9_BIOS[t + 0x20] = logo_data[t];

		//...0xBC:

		// (now what goes in this gap??)

		// IRQ handler: get dtcm address and jump to a vector in it
		T1WriteLong(st->MMU.ARM9_BIOS, 0x0274, 0xE92D500F); //STMDB SP!, {R0-R3,R12,LR}
		T1WriteLong(st->MMU.ARM9_BIOS, 0x0278, 0xEE190F11); //MRC CP15, 0, R0, CR9, CR1, 0
		T1WriteLong(st->MMU.ARM9_BIOS, 0x027C, 0xE1A00620); //MOV R0, R0, LSR #C
		T1WriteLong(st->MMU.ARM9_BIOS, 0x0280, 0xE1A00600); //MOV R0, R0, LSL #C
		T1WriteLong(st->MMU.ARM9_BIOS, 0x0284, 0xE2800C40); //ADD R0, R0, #4000
		T1WriteLong(st->MMU.ARM9_BIOS, 0x0288, 0xE28FE000); //ADD LR, PC, #0
		T1WriteLong(st->MMU.ARM9_BIOS, 0x028C, 0xE510F004); //LDR PC, [R0, -#4]

		// ????
		T1WriteLong(st->MMU.ARM9_BIOS, 0x0290, 0xE8BD500F); // LDMIA SP!, {R0-R3,R12,LR}
		T1WriteLong(st->MMU.ARM9_BIOS, 0x0294, 0xE25EF004); // SUBS PC, LR, #4

		// -------
		// FIQ and abort exception handler
		// TODO - this code is copied from the bios. refactor it
		// friendly reminder: to calculate an immediate offset: encoded = (desired_address-cur_address-8)

		T1WriteLong(st->MMU.ARM9_BIOS, 0x0298, 0xE10FD000); // MRS SP, CPSR
		T1WriteLong(st->MMU.ARM9_BIOS, 0x029C, 0xE38DD0C0); // ORR SP, SP, #C0

		T1WriteLong(st->MMU.ARM9_BIOS, 0x02A0, 0xE12FF00D); // MSR CPSR_fsxc, SP
		T1WriteLong(st->MMU.ARM9_BIOS, 0x02A4, 0xE59FD028); // LDR SP, [FFFF02D4]
		T1WriteLong(st->MMU.ARM9_BIOS, 0x02A8, 0xE28DD001); // ADD SP, SP, #1
		T1WriteLong(st->MMU.ARM9_BIOS, 0x02AC, 0xE92D5000); // STMDB SP!, {R12,LR}

		T1WriteLong(st->MMU.ARM9_BIOS, 0x02B0, 0xE14FE000); // MRS LR, SPSR
		T1WriteLong(st->MMU.ARM9_BIOS, 0x02B4, 0xEE11CF10); // MRC CP15, 0, R12, CR1, CR0, 0
		T1WriteLong(st->MMU.ARM9_BIOS, 0x02B8, 0xE92D5000); // STMDB SP!, {R12,LR}
		T1WriteLong(st->MMU.ARM9_BIOS, 0x02BC, 0xE3CCC001); // BIC R12, R12, #1

		T1WriteLong(st->MMU.ARM9_BIOS, 0x02C0, 0xEE01CF10); // MCR CP15, 0, R12, CR1, CR0, 0
		T1WriteLong(st->MMU.ARM9_BIOS, 0x02C4, 0xE3CDC001); // BIC R12, SP, #1
		T1WriteLong(st->MMU.ARM9_BIOS, 0x02C8, 0xE59CC010); // LDR R12, [R12, #10]
		T1WriteLong(st->MMU.ARM9_BIOS, 0x02CC, 0xE35C0000); // CMP R12, #0

		T1WriteLong(st->MMU.ARM9_BIOS, 0x02D0, 0x112FFF3C); // BLXNE R12
		T1WriteLong(st->MMU.ARM9_BIOS, 0x02D4, 0x027FFD9C); // 0x027FFD9C
		// ---------
	}
}

void NDS_Reset(vio2sf_state *st)
{
	bool fw_success = false;
	auto header = NDS_getROMHeader(st);

	if (!header)
		return;

	st->nds.sleeping = false;
	st->nds.cardEjected = false;
	st->nds.freezeBus = 0;

	st->nds_timer = 0;
	st->nds_arm9_timer = 0;
	st->nds_arm7_timer = 0;

	SPU_DeInit(st);

	MMU_Reset(st);

	PrepareBiosARM7(st);
	PrepareBiosARM9(st);

	// according to smea, this is initialized to 3 by the time we get into a user game program. who does this?
	// well, the firmware load process is about to write a boot program into SIWRAM for the arm7. so we need it setup by now.
	// but, this is a bit weird.. I would be expecting the bioses to do that. maybe we have some more detail to emulate.
	// * is this setting the default, or does the bios do it before loading the firmware programs?
	// at any, it's important that this be done long before the user code ever runs
	_MMU_write08<ARMCPU_ARM9>(st, REG_WRAMCNT, 3);

	st->firmware.reset(new CFIRMWARE());
	fw_success = st->firmware->load();

	if (st->NDS_ARM7.BIOS_loaded && st->NDS_ARM9.BIOS_loaded && st->CommonSettings.BootFromFirmware && fw_success)
	{
		// Copy secure area to memory if needed.
		// could we get a comment about what's going on here?
		// how does this stuff get copied before anything ever even runs?
		// does it get mapped straight to the rom somehow?
		// This code could be made more clear too.
		if (header->ARM9src >= 0x4000 && header->ARM9src < 0x8000)
		{
			uint32_t src = header->ARM9src;
			uint32_t dst = header->ARM9cpy;

			uint32_t size = (0x8000 - src) >> 2;

			for (uint32_t i = 0; i < size; ++i)
			{
				_MMU_write32<ARMCPU_ARM9>(st, dst, T1ReadLong(st->MMU.CART_ROM, src));
				src += 4;
				dst += 4;
			}
		}

		// TODO someone describe why here
		if (st->firmware->patched)
		{
			armcpu_init(&st->NDS_ARM7, 0x00000008);
			armcpu_init(&st->NDS_ARM9, 0xFFFF0008);
		}
		else
		{
			// set the cpus to an initial state with their respective firmware program entrypoints
			armcpu_init(&st->NDS_ARM7, st->firmware->ARM7bootAddr);
			armcpu_init(&st->NDS_ARM9, st->firmware->ARM9bootAddr);
		}

		// set REG_POSTFLG to the value indicating pre-firmware status
		st->MMU.ARM9_REG[0x300] = 0;
		st->MMU.ARM7_REG[0x300] = 0;
	}
	else
	{
		// fake firmware boot-up process

		// copy the arm9 program to the address specified by rom header
		uint32_t src = header->ARM9src;
		uint32_t dst = header->ARM9cpy;
		for (uint32_t i = 0; i < (header->ARM9binSize >> 2); ++i)
		{
			_MMU_write32<ARMCPU_ARM9>(st, dst, T1ReadLong(st->MMU.CART_ROM, src));
			dst += 4;
			src += 4;
		}

		// copy the arm7 program to the address specified by rom header
		src = header->ARM7src;
		dst = header->ARM7cpy;

		for (uint32_t i = 0; i < (header->ARM7binSize >> 2); ++i)
		{
			_MMU_write32<ARMCPU_ARM7>(st, dst, T1ReadLong(st->MMU.CART_ROM, src));
			dst += 4;
			src += 4;
		}

		// set the cpus to an initial state with their respective programs entrypoints
		armcpu_init(&st->NDS_ARM7, header->ARM7exe);
		armcpu_init(&st->NDS_ARM9, header->ARM9exe);

		// set REG_POSTFLG to the value indicating post-firmware status
		st->MMU.ARM9_REG[0x300] = 1;
		st->MMU.ARM7_REG[0x300] = 1;
	}

	// only ARM9 have co-processor
	reconstruct(&st->cp15);
	st->cp15.reset(&st->NDS_ARM9);

	// bitbox 4k demo is so stripped down it relies on default stack values
	// otherwise the arm7 will crash before making a sound
	// (these according to gbatek softreset bios docs)
	st->NDS_ARM7.R13_svc = 0x0380FFDC;
	st->NDS_ARM7.R13_irq = 0x0380FFB0;
	st->NDS_ARM7.R13_usr = 0x0380FF00;
	st->NDS_ARM7.R[13] = st->NDS_ARM7.R13_usr;
	// and let's set these for the arm9 while we're at it, though we have no proof
	st->NDS_ARM9.R13_svc = 0x00803FC0;
	st->NDS_ARM9.R13_irq = 0x00803FA0;
	st->NDS_ARM9.R13_usr = 0x00803EC0;
	st->NDS_ARM9.R13_abt = st->NDS_ARM9.R13_usr; // ?????
	// I think it is wrong to take gbatek's "SYS" and put it in USR--maybe USR doesnt matter.
	// i think SYS is all the misc modes. please verify by setting nonsensical stack values for USR here
	st->NDS_ARM9.R[13] = st->NDS_ARM9.R13_usr;
	// n.b.: im not sure about all these, I dont know enough about arm9 svc/irq/etc modes
	// and how theyre named in desmume to match them up correctly. i just guessed.

	memset(st->nds.timerCycle, 0, sizeof(uint64_t) * 8);
	st->nds.old = 0;
	SetupMMU(st, false, st->nds.Is_DSI());

	_MMU_write16<ARMCPU_ARM9>(st, REG_KEYINPUT, 0x3FF);
	_MMU_write16<ARMCPU_ARM7>(st, REG_KEYINPUT, 0x3FF);
	_MMU_write08<ARMCPU_ARM7>(st, REG_EXTKEYIN, 0x43);

	// Setup a copy of the firmware user settings in memory.
	// (this is what the DS firmware would do).
	{
		uint8_t temp_buffer[NDS_FW_USER_SETTINGS_MEM_BYTE_COUNT];

		if (copy_firmware_user_data(temp_buffer, &st->MMU.fw.data[0]))
			for (int fw_index = 0; fw_index < NDS_FW_USER_SETTINGS_MEM_BYTE_COUNT; ++fw_index)
				_MMU_write08<ARMCPU_ARM9>(st, 0x027FFC80 + fw_index, temp_buffer[fw_index]);
	}

	// Copy the whole header to Main RAM 0x27FFE00 on startup. (http://nocash.emubase.de/gbatek.htm#dscartridgeheader)
	// once upon a time this copied 0x90 more. this was thought to be wrong, and changed.
	if (st->nds.Is_DSI())
	{
		// dsi needs this copied later in memory. there are probably a number of things that  get copied to a later location in memory.. thats where the NDS consoles tend to stash stuff.
		for (int i = 0; i < 92; ++i)
			_MMU_write32<ARMCPU_ARM9>(st, 0x02FFFE00 + i * 4, LE_TO_LOCAL_32(reinterpret_cast<uint32_t *>(st->MMU.CART_ROM)[i]));
	}
	else
	{
		for (int i = 0; i < 92; ++i)
			_MMU_write32<ARMCPU_ARM9>(st, 0x027FFE00 + i * 4, LE_TO_LOCAL_32(reinterpret_cast<uint32_t *>(st->MMU.CART_ROM)[i]));
	}

	// Write the header checksum to memory (the firmware needs it to see the cart)
	_MMU_write16<ARMCPU_ARM9>(st, 0x027FF808, T1ReadWord(st->MMU.CART_ROM, 0x15E));

	if (st->firmware->patched && st->CommonSettings.UseExtBIOS && st->CommonSettings.BootFromFirmware && fw_success)
	{
		// HACK! for flashme
		_MMU_write32<ARMCPU_ARM9>(st, 0x27FFE24, st->firmware->ARM9bootAddr);
		_MMU_write32<ARMCPU_ARM7>(st, 0x27FFE34, st->firmware->ARM7bootAddr);
	}

	// make system think it's booted from card -- EXTREMELY IMPORTANT!!! Thanks to cReDiAr
	_MMU_write08<ARMCPU_ARM9>(st, 0x02FFFC40, 0x1);
	_MMU_write08<ARMCPU_ARM7>(st, 0x02FFFC40, 0x1);

	reconstruct(st, &st->sequencer);

	SPU_ReInit(st);
}

// these templates needed to be instantiated manually
template void NDS_exec<false>(vio2sf_state *st, int32_t nb);
template void NDS_exec<true>(vio2sf_state *st, int32_t nb);
