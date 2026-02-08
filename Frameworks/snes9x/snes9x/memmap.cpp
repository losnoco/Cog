/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include <vector>
#include <algorithm>
#include <string>
#include <numeric>
#include <cassert>
#include <snes9x/snes9x.h>
#include <snes9x/apu.h>
#include "sdd1.h"

#include <snes9x/snes.hpp>
#include <snes9x/smp.hpp>
#include <snes9x/sdsp.hpp>

// deinterleave

static void S9xDeinterleaveType1(int size, uint8_t *base)
{
	uint8_t blocks[256];
	int nblocks = size >> 16;

	for (int i = 0; i < nblocks; ++i)
	{
		blocks[i * 2] = i + nblocks;
		blocks[i * 2 + 1] = i;
	}

	for (int i = 0; i < nblocks * 2; ++i)
		for (int j = i; j < nblocks * 2; ++j)
			if (blocks[j] == i)
			{
				std::swap_ranges(&base[blocks[i] * 0x8000], &base[(blocks[i] + 1) * 0x8000], &base[blocks[j] * 0x8000]);
				std::swap(blocks[i], blocks[j]);
				break;
			}
}

// allocation and deallocation

bool CMemory::Init(struct S9xState *st)
{
	this->st = st;
	safe.reset();
	safe_len = 0;

	this->RAM.reset(new uint8_t[0x20000]);
	this->SRAM.reset(new uint8_t[0x20000]);
	this->VRAM.reset(new uint8_t[0x10000]);
	this->RealROM.reset(new uint8_t[MAX_ROM_SIZE + 0x200 + 0x8000]);

	if (!this->RAM || !this->SRAM || !this->VRAM || !this->RealROM)
	{
		this->Deinit();
		return false;
	}

	std::fill_n(&this->RAM[0], 0x20000, 0);
	std::fill_n(&this->SRAM[0], 0x20000, 0);
	std::fill_n(&this->VRAM[0], 0x10000, 0);
	std::fill_n(&this->RealROM[0], MAX_ROM_SIZE + 0x200 + 0x8000, 0);

	// FillRAM uses first 32K of ROM image area, otherwise space just
	// wasted. Might be read by the SuperFX code.

	this->FillRAM = &this->RealROM[0];

	// Add 0x8000 to ROM image pointer to stop SuperFX code accessing
	// unallocated memory (can cause crash on some ports).

	this->ROM = &this->RealROM[0x8000];

	return true;
}

void CMemory::Deinit()
{
	this->RAM.reset();
	this->SRAM.reset();
	this->VRAM.reset();
	this->ROM = nullptr;
	this->RealROM.reset();

	this->Safe(nullptr);
}

// file management and ROM detection

static bool allASCII(uint8_t *b, int size)
{
	for (int i = 0; i < size; ++i)
		if (b[i] < 32 || b[i] > 126)
			return false;

	return true;
}

int CMemory::ScoreHiROM(bool skip_header, int32_t romoff)
{
	uint8_t *buf = &this->ROM[0xff00 + romoff + (skip_header ? 0x200 : 0)];
	int score = 0;

	if (buf[0xd5] & 0x1)
		score += 2;

	// Mode23 is SA-1
	if (buf[0xd5] == 0x23)
		score -= 2;

	if (buf[0xd4] == 0x20)
		score += 2;

	if ((buf[0xdc] + (buf[0xdd] << 8)) + (buf[0xde] + (buf[0xdf] << 8)) == 0xffff)
	{
		score += 2;
		if (buf[0xde] + (buf[0xdf] << 8))
			++score;
	}

	if (buf[0xda] == 0x33)
		score += 2;

	if ((buf[0xd5] & 0xf) < 4)
		score += 2;

	if (!(buf[0xfd] & 0x80))
		score -= 6;

	if ((buf[0xfc] + (buf[0xfd] << 8)) > 0xffb0)
		score -= 2; // reduced after looking at a scan by Cowering

	if (this->CalculatedSize > 1024 * 1024 * 3)
		score += 4;

	if ((1 << (buf[0xd7] - 7)) > 48)
		--score;

	if (!allASCII(&buf[0xb0], 6))
		--score;

	if (!allASCII(&buf[0xc0], ROM_NAME_LEN - 1))
		--score;

	return score;
}

int CMemory::ScoreLoROM(bool skip_header, int32_t romoff)
{
	uint8_t *buf = &this->ROM[0x7f00 + romoff + (skip_header ? 0x200 : 0)];
	int score = 0;

	if (!(buf[0xd5] & 0x1))
		score += 3;

	// Mode23 is SA-1
	if (buf[0xd5] == 0x23)
		score += 2;

	if ((buf[0xdc] + (buf[0xdd] << 8)) + (buf[0xde] + (buf[0xdf] << 8)) == 0xffff)
	{
		score += 2;
		if (buf[0xde] + (buf[0xdf] << 8))
			++score;
	}

	if (buf[0xda] == 0x33)
		score += 2;

	if ((buf[0xd5] & 0xf) < 4)
		score += 2;

	if (!(buf[0xfd] & 0x80))
		score -= 6;

	if ((buf[0xfc] + (buf[0xfd] << 8)) > 0xffb0)
		score -= 2; // reduced per Cowering suggestion

	if (this->CalculatedSize <= 1024 * 1024 * 16)
		score += 2;

	if ((1 << (buf[0xd7] - 7)) > 48)
		--score;

	if (!allASCII(&buf[0xb0], 6))
		--score;

	if (!allASCII(&buf[0xc0], ROM_NAME_LEN - 1))
		--score;

	return score;
}

int CMemory::First512BytesCountZeroes() const
{
	const uint8_t *buf = ROM;
	int zeroCount = 0;
	for (int i = 0; i < 512; ++i)
		if (buf[i] == 0)
			++zeroCount;
	return zeroCount;
}

bool CMemory::LoadROMSNSF(const uint8_t *lrombuf, int32_t lromsize, const uint8_t *srambuf, int32_t sramsize)
{
	std::fill_n(&this->ROM[0], static_cast<int>(MAX_ROM_SIZE), 0);

	this->CalculatedSize = 0;
	this->ExtendedFormat = NOPE;

	int32_t totalFileSize = std::min<int32_t>(MAX_ROM_SIZE, lromsize);
	if (!totalFileSize)
		return false;
	std::copy_n(&lrombuf[0], totalFileSize, &this->ROM[0]);
	std::fill_n(&this->SRAM[0], 0x20000, 0xff);
	if (srambuf && sramsize)
		std::copy_n(&srambuf[0], sramsize, &this->SRAM[0]);

	int hi_score = this->ScoreHiROM(false);
	int lo_score = this->ScoreLoROM(false);
	int score_nonheadered = std::max(hi_score, lo_score);
	int score_headered = std::max(this->ScoreHiROM(true), this->ScoreLoROM(true));

	bool size_is_likely_headered = !((totalFileSize - 512) & 0xFFFF);
	if (size_is_likely_headered)
		score_headered += 2;
	else
		score_headered -= 2;
	if (this->First512BytesCountZeroes() >= 0x1E0)
		score_headered += 2;
	else
		score_headered -= 2;

	bool headered_score_highest = score_headered > score_nonheadered;

	if (headered_score_highest)
	{
		std::copy_n(&this->ROM[512], totalFileSize - 512, &this->ROM[0]);
		totalFileSize -= 512;
		// modifying ROM, so we need to rescore
		hi_score = this->ScoreHiROM(false);
		lo_score = this->ScoreLoROM(false);
	}

	this->CalculatedSize = ((totalFileSize + 0x1fff) / 0x2000) * 0x2000;

	if (this->CalculatedSize > 0x400000 &&
		this->ROM[0x7fd5] + (this->ROM[0x7fd6] << 8) != 0x3423 && // exclude SA-1
		this->ROM[0x7fd5] + (this->ROM[0x7fd6] << 8) != 0x3523 &&
		this->ROM[0x7fd5] + (this->ROM[0x7fd6] << 8) != 0x4332 && // exclude S-DD1
		this->ROM[0x7fd5] + (this->ROM[0x7fd6] << 8) != 0x4532 &&
		this->ROM[0xffd5] + (this->ROM[0xffd6] << 8) != 0xF93a && // exclude SPC7110
		this->ROM[0xffd5] + (this->ROM[0xffd6] << 8) != 0xF53a)
		this->ExtendedFormat = YEAH;

	// if both vectors are invalid, it's type 1 interleaved LoROM
	if (this->ExtendedFormat == NOPE && this->ROM[0x7ffc] + (this->ROM[0x7ffd] << 8) < 0x8000 && this->ROM[0xfffc] + (this->ROM[0xfffd] << 8) < 0x8000)
	{
		if (!st->Settings.ForceNotInterleaved)
			S9xDeinterleaveType1(totalFileSize, &this->ROM[0]);
	}

	// CalculatedSize is now set, so rescore
	hi_score = this->ScoreHiROM(false);
	lo_score = this->ScoreLoROM(false);

	uint8_t *RomHeader = &this->ROM[0];

	if (this->ExtendedFormat != NOPE)
	{
		int swappedhirom = this->ScoreHiROM(false, 0x400000);
		int swappedlorom = this->ScoreLoROM(false, 0x400000);

		// set swapped here
		if (std::max(swappedlorom, swappedhirom) >= std::max(lo_score, hi_score))
		{
			this->ExtendedFormat = BIGFIRST;
			hi_score = swappedhirom;
			lo_score = swappedlorom;
			RomHeader += 0x400000;
		}
		else
			this->ExtendedFormat = SMALLFIRST;
	}

	bool tales = false;

	bool interleaved = false;

	if (lo_score >= hi_score)
	{
		this->LoROM = true;
		this->HiROM = false;

		// ignore map type byte if not 0x2x or 0x3x
		if ((RomHeader[0x7fd5] & 0xf0) == 0x20 || (RomHeader[0x7fd5] & 0xf0) == 0x30)
			switch (RomHeader[0x7fd5] & 0xf)
			{
				case 1:
					interleaved = true;
					break;

				case 5:
					interleaved = tales = true;
			}
	}
	else
	{
		this->LoROM = false;
		this->HiROM = true;

		if ((RomHeader[0xffd5] & 0xf0) == 0x20 || (RomHeader[0xffd5] & 0xf0) == 0x30)
			switch (RomHeader[0xffd5] & 0xf)
			{
				case 0:
				case 3:
					interleaved = true;
			}
	}

	// this two games fail to be detected
	if (!strncmp(reinterpret_cast<char *>(&this->ROM[0x7fc0]), "YUYU NO QUIZ DE GO!GO!", 22) || !strncmp(reinterpret_cast<char *>(&this->ROM[0xffc0]), "BATMAN--REVENGE JOKER",  21))
	{
		this->LoROM = true;
		this->HiROM = interleaved = tales = false;
	}

	if (!st->Settings.ForceNotInterleaved && interleaved)
	{
		if (tales)
		{
			if (this->ExtendedFormat == BIGFIRST)
			{
				S9xDeinterleaveType1(0x400000, &this->ROM[0]);
				S9xDeinterleaveType1(this->CalculatedSize - 0x400000, &this->ROM[0x400000]);
			}
			else
			{
				S9xDeinterleaveType1(this->CalculatedSize - 0x400000, &this->ROM[0]);
				S9xDeinterleaveType1(0x400000, &this->ROM[this->CalculatedSize - 0x400000]);
			}

			this->LoROM = false;
			this->HiROM = true;
		}
		else
		{
			std::swap(this->LoROM, this->HiROM);
			S9xDeinterleaveType1(this->CalculatedSize, &this->ROM[0]);
		}

		hi_score = this->ScoreHiROM(false);
		lo_score = this->ScoreLoROM(false);

		if ((this->HiROM && (lo_score >= hi_score || hi_score < 0)) || (this->LoROM && (hi_score > lo_score || lo_score < 0)))
		{
			st->Settings.ForceNotInterleaved = true;
			return false;
		}
	}

	if (this->ExtendedFormat == SMALLFIRST)
		tales = true;

	if (tales)
	{
		auto tmp = std::vector<uint8_t>(this->CalculatedSize - 0x400000);
		std::copy_n(&this->ROM[0], this->CalculatedSize - 0x400000, &tmp[0]);
		std::copy_n(&this->ROM[this->CalculatedSize - 0x400000], 0x400000, &this->ROM[0]);
		std::copy_n(&tmp[0], this->CalculatedSize - 0x400000, &this->ROM[0x400000]);
	}

	memset(&st->SNESGameFixes, 0, sizeof(st->SNESGameFixes));

	this->InitROM();

	S9xReset(st);

	return true;
}

// initialization

char *CMemory::Safe(const char *s)
{
	if (!s)
	{
		if (safe)
			safe.reset();

		return nullptr;
	}

	int len = static_cast<int>(strlen(s));
	if (!safe || len + 1 > safe_len)
	{
		safe_len = len + 1;
		safe.reset(new char[safe_len]);
	}

	for (int i = 0; i < len; ++i)
	{
		if (s[i] >= 32 && s[i] < 127)
			safe[i] = s[i];
		else
			safe[i] = '_';
	}

	safe[len] = 0;

	return safe.get();
}

void CMemory::ParseSNESHeader(uint8_t *RomHeader)
{
	strncpy(this->ROMName, reinterpret_cast<char *>(&RomHeader[0x10]), ROM_NAME_LEN - 1);

	this->SRAMSize = RomHeader[0x28];
	this->ROMSpeed = RomHeader[0x25];
	this->ROMType = RomHeader[0x26];
	this->ROMRegion = RomHeader[0x29];

	std::copy_n(&RomHeader[0x02], 4, &this->ROMId[0]);
}

void CMemory::InitROM()
{
	//// Parse ROM header and read ROM informatoin

	std::fill_n(&this->ROMId[0], 5, 0);

	uint8_t *RomHeader = &this->ROM[0x7FB0];
	if (this->ExtendedFormat == BIGFIRST)
		RomHeader += 0x400000;
	if (this->HiROM)
		RomHeader += 0x8000;

	this->ParseSNESHeader(RomHeader);

	//// Detect and initialize chips
	//// detection codes are compatible with NSRT

	uint32_t identifier = ((this->ROMType & 0xff) << 8) + (this->ROMSpeed & 0xff);

	st->Settings.SDD1 = identifier == 0x4332 || identifier == 0x4532;

	//// Map memory and calculate checksum

	this->Map_Initialize();

	if (this->HiROM)
	{
		if (this->ExtendedFormat != NOPE)
			this->Map_ExtendedHiROMMap();
		else
			this->Map_HiROMMap();
	}
	else
	{
		if (st->Settings.SDD1)
			this->Map_SDD1LoROMMap();
		else if (this->ExtendedFormat != NOPE)
			this->Map_JumboLoROMMap();
		else if (!strncmp(this->ROMName, "WANDERERS FROM YS", 17))
			this->Map_NoMAD1LoROMMap();
		else if (!strncmp(this->ROMName, "SOUND NOVEL-TCOOL", 17) || !strncmp(this->ROMName, "DERBY STALLION 96", 17))
			this->Map_ROM24MBSLoROMMap();
		else if (!strncmp(this->ROMName, "THOROUGHBRED BREEDER3", 21) || !strncmp(this->ROMName, "RPG-TCOOL 2", 11))
			this->Map_SRAM512KLoROMMap();
		else
			this->Map_LoROMMap();
	}

	//// Build more ROM information

	// NTSC/PAL
	st->Settings.PAL = this->ROMRegion >= 2 && this->ROMRegion <= 12;

	// truncate cart name
	this->ROMName[ROM_NAME_LEN - 1] = 0;
	if (strlen(this->ROMName))
	{
		char *p = this->ROMName + strlen(this->ROMName);
		if (p > this->ROMName + 21 && this->ROMName[20] == ' ')
			p = this->ROMName + 21;
		while (p > this->ROMName && *(p - 1) == ' ')
			--p;
		*p = 0;
	}

	// SRAM size
	this->SRAMMask = this->SRAMSize ? ((1 << (this->SRAMSize + 3)) * 128) - 1 : 0;

	//// Initialize emulation

	st->Timings.H_Max_Master = SNES_CYCLES_PER_SCANLINE;
	st->Timings.H_Max = st->Timings.H_Max_Master;
	st->Timings.HBlankStart = SNES_HBLANK_START_HC;
	st->Timings.HBlankEnd = SNES_HBLANK_END_HC;
	st->Timings.HDMAInit = SNES_HDMA_INIT_HC;
	st->Timings.HDMAStart = SNES_HDMA_START_HC;
	st->Timings.RenderPos = SNES_RENDER_START_HC;
	st->Timings.V_Max_Master = st->Settings.PAL ? SNES_MAX_PAL_VCOUNTER : SNES_MAX_NTSC_VCOUNTER;
	st->Timings.V_Max = st->Timings.V_Max_Master;
	/* From byuu: The total delay time for both the initial (H)DMA sync (to the DMA clock),
	   and the end (H)DMA sync (back to the last CPU cycle's mcycle rate (6, 8, or 12)) always takes between 12-24 mcycles.
	   Possible delays: { 12, 14, 16, 18, 20, 22, 24 }
	   XXX: Snes9x can't emulate this timing :( so let's use the average value... */
	st->Timings.DMACPUSync = 18;
	/* If the CPU is halted (i.e. for DMA) while /NMI goes low, the NMI will trigger
	   after the DMA completes (even if /NMI goes high again before the DMA
	   completes). In this case, there is a 24-30 cycle delay between the end of DMA
	   and the NMI handler, time enough for an instruction or two. */
	// Wild Guns, Mighty Morphin Power Rangers - The Fighting Edition
	st->Timings.NMIDMADelay = 24;

	//// Hack games

	this->ApplyROMFixes();

	//sprintf(this->ROMName, "%s", Safe(this->ROMName));
	//sprintf(this->ROMId, "%s", Safe(this->ROMId));

	st->Settings.ForceNotInterleaved = false;
}

// memory map

ssize_t CMemory::map_mirror(uint32_t size, uint32_t pos)
{
	// from bsnes
	if (!size)
		return 0;
	if (pos < size)
		return pos;

	uint32_t mask = 1 << 31;
	while (!(pos & mask))
		mask >>= 1;

	if (size <= (pos & mask))
		return this->map_mirror(size, pos - mask);
	else
		return mask + this->map_mirror(size - mask, pos - mask);
}

void CMemory::map_lorom(uint32_t bank_s, uint32_t bank_e, uint32_t addr_s, uint32_t addr_e, uint32_t size)
{
	for (uint32_t c = bank_s; c <= bank_e; ++c)
		for (uint32_t i = addr_s; i <= addr_e; i += 0x1000)
		{
			uint32_t p = (c << 4) | (i >> 12);
			uint32_t addr = (c & 0x7f) * 0x8000;
			this->Map[p] = &this->ROM[this->map_mirror(size, addr) - (i & 0x8000)];
			this->BlockIsROM[p] = true;
			this->BlockIsRAM[p] = false;
		}
}

void CMemory::map_hirom(uint32_t bank_s, uint32_t bank_e, uint32_t addr_s, uint32_t addr_e, uint32_t size)
{
	for (uint32_t c = bank_s; c <= bank_e; ++c)
		for (uint32_t i = addr_s; i <= addr_e; i += 0x1000)
		{
			uint32_t p = (c << 4) | (i >> 12);
			uint32_t addr = c << 16;
			this->Map[p] = &this->ROM[this->map_mirror(size, addr)];
			this->BlockIsROM[p] = true;
			this->BlockIsRAM[p] = false;
		}
}

void CMemory::map_lorom_offset(uint32_t bank_s, uint32_t bank_e, uint32_t addr_s, uint32_t addr_e, uint32_t size, uint32_t offset)
{
	for (uint32_t c = bank_s; c <= bank_e; ++c)
		for (uint32_t i = addr_s; i <= addr_e; i += 0x1000)
		{
			uint32_t p = (c << 4) | (i >> 12);
			uint32_t addr = ((c - bank_s) & 0x7f) * 0x8000;
			this->Map[p] = &this->ROM[offset + this->map_mirror(size, addr) - (i & 0x8000)];
			this->BlockIsROM[p] = true;
			this->BlockIsRAM[p] = false;
		}
}

void CMemory::map_hirom_offset(uint32_t bank_s, uint32_t bank_e, uint32_t addr_s, uint32_t addr_e, uint32_t size, uint32_t offset)
{
	for (uint32_t c = bank_s; c <= bank_e; ++c)
		for (uint32_t i = addr_s; i <= addr_e; i += 0x1000)
		{
			uint32_t p = (c << 4) | (i >> 12);
			uint32_t addr = (c - bank_s) << 16;
			this->Map[p] = &this->ROM[offset + this->map_mirror(size, addr)];
			this->BlockIsROM[p] = true;
			this->BlockIsRAM[p] = false;
		}
}

void CMemory::map_space(uint32_t bank_s, uint32_t bank_e, uint32_t addr_s, uint32_t addr_e, uint8_t *data)
{
	for (uint32_t c = bank_s; c <= bank_e; ++c)
		for (uint32_t i = addr_s; i <= addr_e; i += 0x1000)
		{
			uint32_t p = (c << 4) | (i >> 12);
			this->Map[p] = data;
			this->BlockIsROM[p] = false;
			this->BlockIsRAM[p] = true;
		}
}

void CMemory::map_index(uint32_t bank_s, uint32_t bank_e, uint32_t addr_s, uint32_t addr_e, int index, int type)
{
	bool isRAM = type != MAP_TYPE_I_O;

	for (uint32_t c = bank_s; c <= bank_e; ++c)
		for (uint32_t i = addr_s; i <= addr_e; i += 0x1000)
		{
			uint32_t p = (c << 4) | (i >> 12);
			this->Map[p] = reinterpret_cast<uint8_t *>(static_cast<intptr_t>(index));
			this->BlockIsROM[p] = false;
			this->BlockIsRAM[p] = isRAM;
		}
}

void CMemory::map_System()
{
	// will be overwritten
	this->map_space(0x00, 0x3f, 0x0000, 0x1fff, &this->RAM[0]);
	this->map_index(0x00, 0x3f, 0x2000, 0x3fff, MAP_PPU, MAP_TYPE_I_O);
	this->map_index(0x00, 0x3f, 0x4000, 0x5fff, MAP_CPU, MAP_TYPE_I_O);
	this->map_space(0x80, 0xbf, 0x0000, 0x1fff, &this->RAM[0]);
	this->map_index(0x80, 0xbf, 0x2000, 0x3fff, MAP_PPU, MAP_TYPE_I_O);
	this->map_index(0x80, 0xbf, 0x4000, 0x5fff, MAP_CPU, MAP_TYPE_I_O);
}

void CMemory::map_WRAM()
{
	// will overwrite others
	this->map_space(0x7e, 0x7e, 0x0000, 0xffff, &this->RAM[0]);
	this->map_space(0x7f, 0x7f, 0x0000, 0xffff, &this->RAM[0x10000]);
}

void CMemory::map_LoROMSRAM()
{
	if (!this->SRAMSize)
		return;

	uint32_t hi = this->SRAMSize > 5 ? 0x7fff : 0xffff;

	this->map_index(0x70, 0x7d, 0x0000, hi, MAP_LOROM_SRAM, MAP_TYPE_RAM);
	this->map_index(0xf0, 0xff, 0x0000, hi, MAP_LOROM_SRAM, MAP_TYPE_RAM);
}

void CMemory::map_HiROMSRAM()
{
	this->map_index(0x20, 0x3f, 0x6000, 0x7fff, MAP_HIROM_SRAM, MAP_TYPE_RAM);
	this->map_index(0xa0, 0xbf, 0x6000, 0x7fff, MAP_HIROM_SRAM, MAP_TYPE_RAM);
}

void CMemory::map_WriteProtectROM()
{
	std::copy_n(&this->Map[0], 0x1000, &this->WriteMap[0]);

	for (int c = 0; c < 0x1000; ++c)
		if (this->BlockIsROM[c])
			this->WriteMap[c] = reinterpret_cast<uint8_t *>(MAP_NONE);
}

void CMemory::Map_Initialize()
{
	for (int c = 0; c < 0x1000; ++c)
	{
		this->Map[c] = this->WriteMap[c] = reinterpret_cast<uint8_t *>(MAP_NONE);
		this->BlockIsROM[c] = this->BlockIsRAM[c] = false;
	}
}

void CMemory::Map_LoROMMap()
{
	this->map_System();

	this->map_lorom(0x00, 0x3f, 0x8000, 0xffff, this->CalculatedSize);
	this->map_lorom(0x40, 0x7f, 0x0000, 0xffff, this->CalculatedSize);
	this->map_lorom(0x80, 0xbf, 0x8000, 0xffff, this->CalculatedSize);
	this->map_lorom(0xc0, 0xff, 0x0000, 0xffff, this->CalculatedSize);

	this->map_LoROMSRAM();
	this->map_WRAM();

	this->map_WriteProtectROM();
}

void CMemory::Map_NoMAD1LoROMMap()
{
	this->map_System();

	this->map_lorom(0x00, 0x3f, 0x8000, 0xffff, this->CalculatedSize);
	this->map_lorom(0x40, 0x7f, 0x0000, 0xffff, this->CalculatedSize);
	this->map_lorom(0x80, 0xbf, 0x8000, 0xffff, this->CalculatedSize);
	this->map_lorom(0xc0, 0xff, 0x0000, 0xffff, this->CalculatedSize);

	this->map_index(0x70, 0x7f, 0x0000, 0xffff, MAP_LOROM_SRAM, MAP_TYPE_RAM);
	this->map_index(0xf0, 0xff, 0x0000, 0xffff, MAP_LOROM_SRAM, MAP_TYPE_RAM);

	this->map_WRAM();

	this->map_WriteProtectROM();
}

void CMemory::Map_JumboLoROMMap()
{
	// XXX: Which game uses this?
	this->map_System();

	this->map_lorom_offset(0x00, 0x3f, 0x8000, 0xffff, this->CalculatedSize - 0x400000, 0x400000);
	this->map_lorom_offset(0x40, 0x7f, 0x0000, 0xffff, this->CalculatedSize - 0x600000, 0x600000);
	this->map_lorom_offset(0x80, 0xbf, 0x8000, 0xffff, 0x400000, 0);
	this->map_lorom_offset(0xc0, 0xff, 0x0000, 0xffff, 0x400000, 0x200000);

	this->map_LoROMSRAM();
	this->map_WRAM();

	this->map_WriteProtectROM();
}

void CMemory::Map_ROM24MBSLoROMMap()
{
	// PCB: BSC-1A5M-01, BSC-1A7M-10
	this->map_System();

	this->map_lorom_offset(0x00, 0x1f, 0x8000, 0xffff, 0x100000, 0);
	this->map_lorom_offset(0x20, 0x3f, 0x8000, 0xffff, 0x100000, 0x100000);
	this->map_lorom_offset(0x80, 0x9f, 0x8000, 0xffff, 0x100000, 0x200000);
	this->map_lorom_offset(0xa0, 0xbf, 0x8000, 0xffff, 0x100000, 0x100000);

	this->map_LoROMSRAM();
	this->map_WRAM();

	this->map_WriteProtectROM();
}

void CMemory::Map_SRAM512KLoROMMap()
{
	this->map_System();

	this->map_lorom(0x00, 0x3f, 0x8000, 0xffff, this->CalculatedSize);
	this->map_lorom(0x40, 0x7f, 0x0000, 0xffff, this->CalculatedSize);
	this->map_lorom(0x80, 0xbf, 0x8000, 0xffff, this->CalculatedSize);
	this->map_lorom(0xc0, 0xff, 0x0000, 0xffff, this->CalculatedSize);

	this->map_space(0x70, 0x70, 0x0000, 0xffff, &this->SRAM[0]);
	this->map_space(0x71, 0x71, 0x0000, 0xffff, &this->SRAM[0x8000]);
	this->map_space(0x72, 0x72, 0x0000, 0xffff, &this->SRAM[0x10000]);
	this->map_space(0x73, 0x73, 0x0000, 0xffff, &this->SRAM[0x18000]);

	this->map_WRAM();

	this->map_WriteProtectROM();
}

void CMemory::Map_SDD1LoROMMap()
{
	this->map_System();

	this->map_lorom(0x00, 0x3f, 0x8000, 0xffff, this->CalculatedSize);
	this->map_lorom(0x80, 0xbf, 0x8000, 0xffff, this->CalculatedSize);

	this->map_hirom_offset(0x60, 0x7f, 0x0000, 0xffff, this->CalculatedSize, 0);
	this->map_hirom_offset(0xc0, 0xff, 0x0000, 0xffff, this->CalculatedSize, 0); // will be overwritten dynamically

	this->map_index(0x70, 0x7f, 0x0000, 0x7fff, MAP_LOROM_SRAM, MAP_TYPE_RAM);
	this->map_index(0xa0, 0xbf, 0x6000, 0x7fff, MAP_LOROM_SRAM, MAP_TYPE_RAM);

	this->map_WRAM();

	this->map_WriteProtectROM();
}

void CMemory::Map_HiROMMap()
{
	this->map_System();

	this->map_hirom(0x00, 0x3f, 0x8000, 0xffff, CalculatedSize);
	this->map_hirom(0x40, 0x7f, 0x0000, 0xffff, CalculatedSize);
	this->map_hirom(0x80, 0xbf, 0x8000, 0xffff, CalculatedSize);
	this->map_hirom(0xc0, 0xff, 0x0000, 0xffff, CalculatedSize);

	this->map_HiROMSRAM();
	this->map_WRAM();

	this->map_WriteProtectROM();
}

void CMemory::Map_ExtendedHiROMMap()
{
	this->map_System();

	this->map_hirom_offset(0x00, 0x3f, 0x8000, 0xffff, this->CalculatedSize - 0x400000, 0x400000);
	this->map_hirom_offset(0x40, 0x7f, 0x0000, 0xffff, this->CalculatedSize - 0x400000, 0x400000);
	this->map_hirom_offset(0x80, 0xbf, 0x8000, 0xffff, 0x400000, 0);
	this->map_hirom_offset(0xc0, 0xff, 0x0000, 0xffff, 0x400000, 0);

	this->map_HiROMSRAM();
	this->map_WRAM();

	this->map_WriteProtectROM();
}

// hack

bool CMemory::match_na(const char *str)
{
	return !strcmp(this->ROMName, str);
}

bool CMemory::match_nn(const char *str)
{
	return !strncmp(this->ROMName, str, strlen(str));
}

bool CMemory::match_id(const char *str)
{
	return !strncmp(this->ROMId, str, strlen(str));
}

void CMemory::ApplyROMFixes()
{
	st->Settings.BlockInvalidVRAMAccess = st->Settings.BlockInvalidVRAMAccessMaster;

	//// APU timing hacks :(

	st->Timings.APUSpeedup = 0;

	if (!st->Settings.DisableGameSpecificHacks)
	{
		//if (this->match_id("AVCJ")) // Rendering Ranger R2
		//	Timings.APUSpeedup = 2;
		if (this->match_na("CIRCUIT USA"))
			st->Timings.APUSpeedup = 3;
	}

	S9xAPUTimingSetSpeedup(st, st->Timings.APUSpeedup);

	//// Other timing hacks :(

	st->Timings.HDMAStart = SNES_HDMA_START_HC + st->Settings.HDMATimingHack - 100;
	st->Timings.HBlankStart = SNES_HBLANK_START_HC + st->Timings.HDMAStart - SNES_HDMA_START_HC;
	st->Timings.IRQTriggerCycles = 14;

	if (!st->Settings.DisableGameSpecificHacks)
	{
		// The delay to sync CPU and DMA which Snes9x cannot emulate.
		// Some games need really severe delay timing...
		if (this->match_na("BATTLE GRANDPRIX")) // Battle Grandprix
			st->Timings.DMACPUSync = 20;
		else if (match_na("KORYU NO MIMI ENG")) // Koryu no Mimi translation by rpgone)
			// An infinite loop reads $4210 and checks NMI flag. This only works if LDA instruction executes before the NMI triggers,
			// which doesn't work very well with s9x's default DMA timing.
			st->Timings.DMACPUSync = 20;

		//// SRAM initial value

		if (this->match_na("HITOMI3"))
		{
			SRAMSize = 1;
			SRAMMask = ((1 << (SRAMSize + 3)) * 128) - 1;
		}

		// others: BS and ST-01x games are 0x00.

		//// OAM hacks :(

		// OAM hacks because we don't fully understand the behavior of the SNES.
		// Totally wacky display in 2P mode...
		// seems to need a disproven behavior, so we're definitely overlooking some other bug?
		if (match_nn("UNIRACERS")) // Uniracers
			st->SNESGameFixes.Uniracers = true;
	}
}
