/*
	Copyright (C) 2006 yopyop
	Copyright (C) 2007 shash
	Copyright (C) 2007-2012 DeSmuME team

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

#pragma once

#include <vio2sf/FIFO.h>
#include <vio2sf/mem.h>
#include <vio2sf/registers.h>
#include <vio2sf/mc.h>
#include <vio2sf/bits.h>
#include <vio2sf/readwrite.h>

struct vio2sf_state;

static const unsigned VRAM_LCDC_PAGES = 41;

#define ARMCPU_ARM7 1
#define ARMCPU_ARM9 0
#define ARMPROC (PROCNUM ? st->NDS_ARM7 : st->NDS_ARM9)

typedef const uint8_t TWaitState;

enum EDMAMode
{
	EDMAMode_Immediate = 0,
	EDMAMode_VBlank = 1,
	EDMAMode_HBlank = 2,
	EDMAMode_HStart = 3,
	EDMAMode_MemDisplay = 4,
	EDMAMode_Card = 5,
	EDMAMode_GBASlot = 6,
	EDMAMode_GXFifo = 7,
	EDMAMode7_Wifi = 8,
	EDMAMode7_GBASlot = 9
};

enum EDMABitWidth
{
	EDMABitWidth_16 = 0,
	EDMABitWidth_32 = 1
};

enum EDMASourceUpdate
{
	EDMASourceUpdate_Increment = 0,
	EDMASourceUpdate_Decrement = 1,
	EDMASourceUpdate_Fixed = 2,
	EDMASourceUpdate_Invalid = 3
};

enum EDMADestinationUpdate
{
	EDMADestinationUpdate_Increment = 0,
	EDMADestinationUpdate_Decrement = 1,
	EDMADestinationUpdate_Fixed = 2,
	EDMADestinationUpdate_IncrementReload = 3
};

// TODO
// n.b. this may be a bad idea, for complex registers like the dma control register.
// we need to know exactly what part was written to, instead of assuming all 32bits were written.
class TRegister_32
{
public:
	virtual ~TRegister_32() { }

	virtual uint32_t read32() = 0;
	virtual void write32(uint32_t val) = 0;
	void write(int size, uint32_t adr, uint32_t val)
	{
		if (size == 32)
			this->write32(val);
		else
		{
			uint32_t offset = adr & 3;
			if (size == 8)
			{
				printf("WARNING! 8BIT DMA ACCESS\n");
				uint32_t mask = 0xFF << (offset << 3);
				this->write32((this->read32() & ~mask) | (val << (offset << 3)));
			}
			else if (size == 16)
			{
				uint32_t mask = 0xFFFF << (offset << 3);
				this->write32((this->read32() & ~mask) | (val << (offset << 3)));
			}
		}
	}

	uint32_t read(int size, uint32_t adr)
	{
		if (size == 32)
			return this->read32();
		else
		{
			uint32_t offset = adr & 3;
			if (size == 8)
			{
				printf("WARNING! 8BIT DMA ACCESS\n");
				return (this->read32() >> (offset << 3)) & 0xFF;
			}
			else
				return (this->read32() >> (offset << 3)) & 0xFFFF;
		}
	}
};

struct TGXSTAT : public TRegister_32
{
	TGXSTAT()
	{
		this->gxfifo_irq = this->se = this->tr = this->tb = this->sb = 0;
		this->fifo_empty = true;
		this->fifo_low = false;
	}
	virtual ~TGXSTAT() { }
	uint8_t tb; // test busy
	uint8_t tr; // test result
	uint8_t se; // stack error
	uint8_t sb; // stack busy
	uint8_t gxfifo_irq; // irq configuration

	bool fifo_empty, fifo_low;

	virtual uint32_t read32();
	virtual void write32(uint32_t val);
};

void triggerDma(vio2sf_state *st, EDMAMode mode);

class DivController
{
public:
	DivController() : mode(0), busy(0) { }
	void exec();
	uint8_t mode, busy, div0;
	uint16_t read16() { return this->mode | (this->busy << 15) | (this->div0 << 14); }
	void write16(uint16_t val)
	{
		this->mode = val & 3;
		// todo - do we clear the div0 flag here or is that strictly done by the divider unit?
	}
};

class SqrtController
{
public:
	SqrtController() : mode(0), busy(0) { }
	void exec();
	uint8_t mode, busy;
	uint16_t read16() { return this->mode | (this->busy << 15); }
	void write16(uint16_t val) { this->mode = val & 1; }
};

class DmaController
{
public:
	uint8_t enable, irq, repeatMode, _startmode;
	uint8_t userEnable;
	uint32_t wordcount;
	EDMAMode startmode;
	EDMABitWidth bitWidth;
	EDMASourceUpdate sar;
	EDMADestinationUpdate dar;
	uint32_t saddr, daddr;
	uint32_t saddr_user, daddr_user;

	// indicates whether the dma needs to be checked for triggering
	bool dmaCheck;

	// indicates whether the dma right now is logically running
	// (though for now we copy all the data when it triggers)
	bool running;

	bool paused;

	// this flag will sometimes be set when a start condition is triggered
	// other conditions may be automatically triggered based on scanning conditions
	bool triggered;

	uint64_t nextEvent;

	int procnum, chan;

	void exec();
	template<int PROCNUM> void doCopy();
	void doPause();
	void doStop();
	void doSchedule();
	void tryTrigger(EDMAMode mode);

	DmaController(vio2sf_state *_st) :
		st(_st),
		enable(0), irq(0), repeatMode(0), _startmode(0),
		wordcount(0), startmode(EDMAMode_Immediate),
		bitWidth(EDMABitWidth_16),
		sar(EDMASourceUpdate_Increment), dar(EDMADestinationUpdate_Increment),
		// if saddr isnt cleared then rings of fate will trigger copy protection
		// by inspecting dma3 saddr when it boots
		saddr(0), daddr(0),
		saddr_user(0), daddr_user(0),
		dmaCheck(false),
		running(false),
		paused(false),
		triggered(false),
		nextEvent(0),
		sad(&saddr_user),
		dad(&daddr_user)
	{
		this->sad.controller = this;
		this->dad.controller = this;
		this->ctrl.controller = this;
		this->regs[0] = &this->sad;
		this->regs[1] = &this->dad;
		this->regs[2] = &this->ctrl;
	}

	class AddressRegister : public TRegister_32
	{
	public:
		// we pass in a pointer to the controller here so we can alert it if anything changes
		DmaController *controller;
		uint32_t *const ptr;
		AddressRegister(uint32_t *_ptr) : ptr(_ptr) { }
		virtual ~AddressRegister() { }
		virtual uint32_t read32()
		{
			return *this->ptr;
		}
		virtual void write32(uint32_t val)
		{
			*this->ptr = val;
		}
	};

	class ControlRegister : public TRegister_32
	{
	public:
		// we pass in a pointer to the controller here so we can alert it if anything changes
		DmaController *controller;
		ControlRegister() { }
		virtual ~ControlRegister() { }
		virtual uint32_t read32()
		{
			return this->controller->read32();
		}
		virtual void write32(uint32_t val)
		{
			return this->controller->write32(val);
		}
	};

	AddressRegister sad, dad;
	ControlRegister ctrl;
	TRegister_32 *regs[3];

	void write32(uint32_t val);
	uint32_t read32();

	vio2sf_state *st;
};

enum ECardMode
{
	CardMode_Normal = 0,
	CardMode_KEY1,
	CardMode_KEY2
};

struct nds_dscard
{
	uint8_t command[8];

	uint32_t address;
	uint32_t transfer_count;

	ECardMode mode;

	// NJSD stuff
	int blocklen;
};

#define DUP2(x)  x, x
#define DUP4(x)  x, x, x, x
#define DUP8(x)  x, x, x, x,  x, x, x, x
#define DUP16(x) x, x, x, x,  x, x, x, x,  x, x, x, x,  x, x, x, x

struct MMU_struct
{
	//ARM9 mem
	uint8_t ARM9_ITCM[0x8000];
	uint8_t ARM9_DTCM[0x4000];

	//u8 MAIN_MEM[4*1024*1024]; // expanded from 4MB to 8MB to support debug consoles
	//u8 MAIN_MEM[8*1024*1024]; // expanded from 8MB to 16MB to support dsi
	uint8_t MAIN_MEM[16*1024*1024]; // expanded from 8MB to 16MB to support dsi
	uint8_t ARM9_REG[0x1000000];
	uint8_t ARM9_BIOS[0x8000];
	uint8_t ARM9_VMEM[0x800];

#include <vio2sf/PACKED.h>
	struct
	{
		uint8_t ARM9_LCD[0xA4000];
		// an extra 128KB for blank memory, directly after arm9_lcd, so that
		// we can easily map things to the end of arm9_lcd to represent
		// an unmapped state
		uint8_t blank_memory[0x100000 - 0xA4000];
	};
#include <vio2sf/PACKED_END.h>

	uint8_t ARM9_OAM[0x800];

	uint8_t *ExtPal[2][4];
	uint8_t *ObjExtPal[2][2];

	struct TextureInfo
	{
		uint8_t *texPalSlot[6];
		uint8_t *textureSlotAddr[4];
	} texInfo;

	// ARM7 mem
	uint8_t ARM7_BIOS[0x4000];
	uint8_t ARM7_ERAM[0x10000]; // 64KB of exclusive WRAM
	uint8_t ARM7_REG[0x10000];
	uint8_t ARM7_WIRAM[0x10000]; // WIFI ram

	// VRAM mapping
	uint8_t VRAM_MAP[4][32];
	uint32_t LCD_VRAM_ADDR[10];
	uint8_t LCDCenable[10];

	// 32KB of shared WRAM - can be switched between ARM7 & ARM9 in two blocks
	uint8_t SWIRAM[0x8000];

	// Card rom & ram
	uint8_t * CART_ROM;

	// Unused ram
	uint8_t UNUSED_RAM[4];

	// this is here so that we can trap glitchy emulator code
	// which is accessing offsets 5,6,7 of unused ram due to unaligned accesses
	// (also since the emulator doesn't prevent unaligned accesses)
	uint8_t MORE_UNUSED_RAM[4];

	uint8_t *MMU_MEM[2][256];
	static uint32_t MMU_MASK[2][256];

	uint8_t ARM9_RW_MODE;

	uint32_t DTCMRegion;
	uint32_t ITCMRegion;

	uint16_t timer[2][4];
	int32_t timerMODE[2][4];
	uint32_t timerON[2][4];
	uint32_t timerRUN[2][4];
	uint16_t timerReload[2][4];

	uint32_t reg_IME[2];
	uint32_t reg_IE[2];

	// these are the user-controlled IF bits. some IF bits are generated as necessary from hardware conditions
	uint32_t reg_IF_bits[2];
	// these flags are set occasionally to indicate that an irq should have entered the pipeline, and processing will be deferred a tiny bit to help emulate things
	uint32_t reg_IF_pending[2];

	template<int PROCNUM> uint32_t gen_IF();

	bool divRunning;
	int64_t divResult;
	int64_t divMod;
	uint64_t divCycles;

	bool sqrtRunning;
	uint32_t sqrtResult;
	uint64_t sqrtCycles;

	uint16_t SPI_CNT;
	uint16_t SPI_CMD;
	uint16_t AUX_SPI_CNT;
	uint16_t AUX_SPI_CMD;

	uint8_t WRAMCNT;

	uint8_t powerMan_CntReg;
	bool powerMan_CntRegWritten;
	uint8_t powerMan_Reg[5];

	memory_chip_t fw;

	nds_dscard dscard[2];
};

// everything in here is derived from libnds behaviours. no hardware tests yet
class DSI_TSC
{
public:
	DSI_TSC();
	void reset_command();
	uint16_t write16(uint16_t val);
private:
	uint16_t read16();
	uint8_t reg_selection;
	uint8_t read_flag;
	int32_t state;
	int32_t readcount;

	// registers[0] contains the current page.
	// we are going to go ahead and save these out in case we want to change the way this is emulated in the future..
	// we may want to poke registers in here at more convenient times and have the TSC dumbly pluck them out,
	// rather than generate the values on the fly
	uint8_t registers[0x80];
};

// this contains things which can't be memzeroed because they are smarter classes
struct MMU_struct_new
{
	MMU_struct_new(vio2sf_state *st);
	BackupDevice backupDevice;
	DmaController dma[2][4];
	TGXSTAT gxstat;
	SqrtController sqrt;
	DivController div;
	DSI_TSC dsi_tsc;
	vio2sf_state *st;

	void write_dma(int proc, int size, uint32_t adr, uint32_t val);
	uint32_t read_dma(int proc, int size, uint32_t adr);
	bool is_dma(uint32_t adr) { return adr >= _REG_DMA_CONTROL_MIN && adr <= _REG_DMA_CONTROL_MAX; }
};

void MMU_Init(vio2sf_state *st);
void MMU_DeInit(vio2sf_state *st);

void MMU_Reset(vio2sf_state *st);

void MMU_setRom(vio2sf_state *st, uint8_t *rom, uint32_t mask);
void MMU_unsetRom(vio2sf_state *st);

#define VRAM_BANKS 9
#define VRAM_BANK_A 0
#define VRAM_BANK_B 1
#define VRAM_BANK_C 2
#define VRAM_BANK_D 3
#define VRAM_BANK_E 4
#define VRAM_BANK_F 5
#define VRAM_BANK_G 6
#define VRAM_BANK_H 7
#define VRAM_BANK_I 8

#define VRAM_PAGE_ABG 0
#define VRAM_PAGE_BBG 128
#define VRAM_PAGE_AOBJ 256
#define VRAM_PAGE_BOBJ 384

struct VramConfiguration
{
	enum Purpose
	{
		OFF,
		INVALID,
		ABG,
		BBG,
		AOBJ,
		BOBJ,
		LCDC,
		ARM7,
		TEX,
		TEXPAL,
		ABGEXTPAL,
		BBGEXTPAL,
		AOBJEXTPAL,
		BOBJEXTPAL
	};

	struct BankInfo
	{
		Purpose purpose;
		int ofs;
	} banks[VRAM_BANKS];

	void clear()
	{
		for (int i = 0; i < VRAM_BANKS; ++i)
		{
			banks[i].ofs = 0;
			banks[i].purpose = OFF;
		}
	}
};

const int VRAM_ARM9_PAGES = 512;

// formerly inlined
uint8_t _MMU_read08(vio2sf_state *st, int PROCNUM, MMU_ACCESS_TYPE AT, uint32_t addr);
uint16_t _MMU_read16(vio2sf_state *st, int PROCNUM, MMU_ACCESS_TYPE AT, uint32_t addr);
uint32_t _MMU_read32(vio2sf_state *st, int PROCNUM, MMU_ACCESS_TYPE AT, uint32_t addr);
void _MMU_write08(vio2sf_state *st, int PROCNUM, MMU_ACCESS_TYPE AT, uint32_t addr, uint8_t val);
void _MMU_write16(vio2sf_state *st, int PROCNUM, MMU_ACCESS_TYPE AT, uint32_t addr, uint16_t val);
void _MMU_write32(vio2sf_state *st, int PROCNUM, MMU_ACCESS_TYPE AT, uint32_t addr, uint32_t val);

template<int PROCNUM, MMU_ACCESS_TYPE AT> uint8_t _MMU_read08(vio2sf_state *st, uint32_t addr);
template<int PROCNUM, MMU_ACCESS_TYPE AT> uint16_t _MMU_read16(vio2sf_state *st, uint32_t addr);
template<int PROCNUM, MMU_ACCESS_TYPE AT> uint32_t _MMU_read32(vio2sf_state *st, uint32_t addr);
template<int PROCNUM, MMU_ACCESS_TYPE AT> void _MMU_write08(vio2sf_state *st, uint32_t addr, uint8_t val);
template<int PROCNUM, MMU_ACCESS_TYPE AT> void _MMU_write16(vio2sf_state *st, uint32_t addr, uint16_t val);
template<int PROCNUM, MMU_ACCESS_TYPE AT> void _MMU_write32(vio2sf_state *st, uint32_t addr, uint32_t val);

template<int PROCNUM> uint8_t _MMU_read08(vio2sf_state *st, uint32_t addr) { return _MMU_read08<PROCNUM, MMU_AT_DATA>(st, addr); }
template<int PROCNUM> uint16_t _MMU_read16(vio2sf_state *st, uint32_t addr) { return _MMU_read16<PROCNUM, MMU_AT_DATA>(st, addr); }
template<int PROCNUM> uint32_t _MMU_read32(vio2sf_state *st, uint32_t addr) { return _MMU_read32<PROCNUM, MMU_AT_DATA>(st, addr); }
template<int PROCNUM> void _MMU_write08(vio2sf_state *st, uint32_t addr, uint8_t val) { _MMU_write08<PROCNUM, MMU_AT_DATA>(st,addr,val); }
template<int PROCNUM> void _MMU_write16(vio2sf_state *st, uint32_t addr, uint16_t val) { _MMU_write16<PROCNUM, MMU_AT_DATA>(st,addr,val); }
template<int PROCNUM> void _MMU_write32(vio2sf_state *st, uint32_t addr, uint32_t val) { _MMU_write32<PROCNUM, MMU_AT_DATA>(st,addr,val); }

void FASTCALL _MMU_ARM9_write08(vio2sf_state *st, uint32_t adr, uint8_t val);
void FASTCALL _MMU_ARM9_write16(vio2sf_state *st, uint32_t adr, uint16_t val);
void FASTCALL _MMU_ARM9_write32(vio2sf_state *st, uint32_t adr, uint32_t val);
uint8_t  FASTCALL _MMU_ARM9_read08(vio2sf_state *st, uint32_t adr);
uint16_t FASTCALL _MMU_ARM9_read16(vio2sf_state *st, uint32_t adr);
uint32_t FASTCALL _MMU_ARM9_read32(vio2sf_state *st, uint32_t adr);

void FASTCALL _MMU_ARM7_write08(vio2sf_state *st, uint32_t adr, uint8_t val);
void FASTCALL _MMU_ARM7_write16(vio2sf_state *st, uint32_t adr, uint16_t val);
void FASTCALL _MMU_ARM7_write32(vio2sf_state *st, uint32_t adr, uint32_t val);
uint8_t  FASTCALL _MMU_ARM7_read08(vio2sf_state *st, uint32_t adr);
uint16_t FASTCALL _MMU_ARM7_read16(vio2sf_state *st, uint32_t adr);
uint32_t FASTCALL _MMU_ARM7_read32(vio2sf_state *st, uint32_t adr);

void SetupMMU(vio2sf_state *st, bool debugConsole, bool dsi);

// define ST
#define READ32(a,b)		_MMU_read32<PROCNUM>(ST, (b) & 0xFFFFFFFC)
#define WRITE32(a,b,c)	_MMU_write32<PROCNUM>(ST, (b) & 0xFFFFFFFC,c)
#define READ16(a,b)		_MMU_read16<PROCNUM>(ST, (b) & 0xFFFFFFFE)
#define WRITE16(a,b,c)	_MMU_write16<PROCNUM>(ST, (b) & 0xFFFFFFFE,c)
#define READ8(a,b)		_MMU_read08<PROCNUM>(ST, b)
#define WRITE8(a,b,c)	_MMU_write08<PROCNUM>(ST, b, c)

template<int PROCNUM, MMU_ACCESS_TYPE AT> inline uint8_t _MMU_read08(vio2sf_state *st, uint32_t addr) { return _MMU_read08(st, PROCNUM, AT, addr); }
template<int PROCNUM, MMU_ACCESS_TYPE AT> inline uint16_t _MMU_read16(vio2sf_state *st, uint32_t addr) { return _MMU_read16(st, PROCNUM, AT, addr); }
template<int PROCNUM, MMU_ACCESS_TYPE AT> inline uint32_t _MMU_read32(vio2sf_state *st, uint32_t addr) { return _MMU_read32(st, PROCNUM, AT, addr); }
template<int PROCNUM, MMU_ACCESS_TYPE AT> inline void _MMU_write08(vio2sf_state *st, uint32_t addr, uint8_t val) { _MMU_write08(st, PROCNUM, AT, addr, val); }
template<int PROCNUM, MMU_ACCESS_TYPE AT> inline void _MMU_write16(vio2sf_state *st, uint32_t addr, uint16_t val) { _MMU_write16(st, PROCNUM, AT, addr, val); }
template<int PROCNUM, MMU_ACCESS_TYPE AT> inline void _MMU_write32(vio2sf_state *st, uint32_t addr, uint32_t val) { _MMU_write32(st, PROCNUM, AT, addr, val); }
