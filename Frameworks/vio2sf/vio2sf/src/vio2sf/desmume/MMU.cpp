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

#include <sstream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <cassert>

#include "vio2sf.h"

// http://home.utah.edu/~nahaj/factoring/isqrt.c.html
static uint64_t isqrt(uint64_t x)
{
	if (x < 1)
		return 0;

	/* Load the binary constant 01 00 00 ... 00, where the number
	 * of zero bits to the right of the single one bit
	 * is even, and the one bit is as far left as is consistant
	 * with that condition.)
	 */
	uint64_t squaredbit = (~0LL >> 1) & ~(~0LL >> 2);
	/* This portable load replaces the loop that used to be
	 * here, and was donated by  legalize@xmission.com
	 */

	/* Form bits of the answer. */
	uint64_t remainder = x, root = 0;
	while (squaredbit > 0)
	{
		if (remainder >= (squaredbit | root))
		{
			remainder -= squaredbit | root;
			root >>= 1;
			root |= squaredbit;
		}
		else
			root >>= 1;
		squaredbit >>= 2;
	}

	return root;
}

void MMU_init_mem_ptrs(MMU_struct &MMU)
{
	const uint8_t *MMU_MEM_init[2][256] =
{
	//arm9
	{
		/* 0X*/	DUP16(MMU.ARM9_ITCM),
		/* 1X*/	//DUP16(MMU.ARM9_ITCM)
		/* 1X*/	DUP16(MMU.UNUSED_RAM),
		/* 2X*/	DUP16(MMU.MAIN_MEM),
		/* 3X*/	DUP16(MMU.SWIRAM),
		/* 4X*/	DUP16(MMU.ARM9_REG),
		/* 5X*/	DUP16(MMU.ARM9_VMEM),
		/* 6X*/	DUP16(MMU.ARM9_LCD),
		/* 7X*/	DUP16(MMU.ARM9_OAM),
		/* 8X*/	DUP16(nullptr),
		/* 9X*/	DUP16(nullptr),
		/* AX*/	DUP16(MMU.UNUSED_RAM),
		/* BX*/	DUP16(MMU.UNUSED_RAM),
		/* CX*/	DUP16(MMU.UNUSED_RAM),
		/* DX*/	DUP16(MMU.UNUSED_RAM),
		/* EX*/	DUP16(MMU.UNUSED_RAM),
		/* FX*/	DUP16(MMU.ARM9_BIOS)
	},
	//arm7
	{
		/* 0X*/	DUP16(MMU.ARM7_BIOS),
		/* 1X*/	DUP16(MMU.UNUSED_RAM),
		/* 2X*/	DUP16(MMU.MAIN_MEM),
		/* 3X*/	DUP8(MMU.SWIRAM),
				DUP8(MMU.ARM7_ERAM),
		/* 4X*/	DUP8(MMU.ARM7_REG),
				DUP8(MMU.ARM7_WIRAM),
		/* 5X*/	DUP16(MMU.UNUSED_RAM),
		/* 6X*/	DUP16(MMU.ARM9_LCD),
		/* 7X*/	DUP16(MMU.UNUSED_RAM),
		/* 8X*/	DUP16(nullptr),
		/* 9X*/	DUP16(nullptr),
		/* AX*/	DUP16(MMU.UNUSED_RAM),
		/* BX*/	DUP16(MMU.UNUSED_RAM),
		/* CX*/	DUP16(MMU.UNUSED_RAM),
		/* DX*/	DUP16(MMU.UNUSED_RAM),
		/* EX*/	DUP16(MMU.UNUSED_RAM),
		/* FX*/	DUP16(MMU.UNUSED_RAM)
	}
};
	memcpy(&MMU.MMU_MEM, &MMU_MEM_init, sizeof(MMU.MMU_MEM));
}

uint32_t MMU_struct::MMU_MASK[2][256] =
{
	//arm9
	{
		/* 0X*/	DUP16(0x00007FFF),
		/* 1X*/	//DUP16(0x00007FFF)
		/* 1X*/	DUP16(0x00000003),
		/* 2X*/	DUP16(0x003FFFFF),
		/* 3X*/	DUP16(0x00007FFF),
		/* 4X*/	DUP16(0x00FFFFFF),
		/* 5X*/	DUP16(0x000007FF),
		/* 6X*/	DUP16(0x000FFFFF),
		/* 7X*/	DUP16(0x000007FF),
		/* 8X*/	DUP16(0x00000003),
		/* 9X*/	DUP16(0x00000003),
		/* AX*/	DUP16(0x00000003),
		/* BX*/	DUP16(0x00000003),
		/* CX*/	DUP16(0x00000003),
		/* DX*/	DUP16(0x00000003),
		/* EX*/	DUP16(0x00000003),
		/* FX*/	DUP16(0x00007FFF)
	},
	//arm7
	{
		/* 0X*/	DUP16(0x00003FFF),
		/* 1X*/	DUP16(0x00000003),
		/* 2X*/	DUP16(0x003FFFFF),
		/* 3X*/	DUP8(0x00007FFF),
				DUP8(0x0000FFFF),
		/* 4X*/	DUP8(0x0000FFFF),
				DUP8(0x0000FFFF),
		/* 5X*/	DUP16(0x00000003),
		/* 6X*/	DUP16(0x000FFFFF),
		/* 7X*/	DUP16(0x00000003),
		/* 8X*/	DUP16(0x00000003),
		/* 9X*/	DUP16(0x00000003),
		/* AX*/	DUP16(0x00000003),
		/* BX*/	DUP16(0x00000003),
		/* CX*/	DUP16(0x00000003),
		/* DX*/	DUP16(0x00000003),
		/* EX*/	DUP16(0x00000003),
		/* FX*/	DUP16(0x00000003)
	}
};

//////////////////////////////////////////////////////////////

// -------------
// VRAM MEMORY MAPPING
// -------------
// (Everything is mapped through to ARM9_LCD in blocks of 16KB)

// for all of the below, values = 41 indicate unmapped memory
static const uint8_t VRAM_PAGE_UNMAPPED = 41;

//static uint8_t vram_lcdc_map[VRAM_LCDC_PAGES];

// in the range of 0x06000000 - 0x06800000 in 16KB pages (the ARM9 vram mappable area)
// this maps to 16KB pages in the LCDC buffer which is what will actually contain the data
//uint8_t vram_arm9_map[VRAM_ARM9_PAGES];

// this chooses which banks are mapped in the 128K banks starting at 0x06000000 in ARM7
//static uint8_t vram_arm7_map[2];

struct TVramBankInfo
{
	uint8_t page_addr, num_pages;
};

static const TVramBankInfo vram_bank_info[VRAM_BANKS] =
{
	{0, 8},
	{8, 8},
	{16, 8},
	{24, 8},
	{32, 4},
	{36, 1},
	{37, 1},
	{38, 2},
	{40, 1}
};

// this is to remind you that the LCDC mapping returns a strange value (not 0x06800000) as you would expect
// in order to play nicely with the MMU address and mask tables
static const uint32_t LCDC_HACKY_LOCATION = 0x06000000;

static const uint32_t ARM7_HACKY_IWRAM_LOCATION = 0x03800000;
static const uint32_t ARM7_HACKY_SIWRAM_LOCATION = 0x03000000;

// maps an ARM9 BG/OBJ or LCDC address into an LCDC address, and informs the caller of whether it isn't mapped
// TODO - in cases where this does some mapping work, we could bypass the logic at the end of the _read* and _write* routines
// this is a good optimization to consider
// NOTE - this whole approach is probably fundamentally wrong.
// according to dasShiny research, its possible to map multiple banks to the same addresses. something more sophisticated would be needed.
// however, it hasnt proven necessary yet for any known test case.
static inline uint32_t MMU_LCDmap(vio2sf_state *st, int PROCNUM, uint32_t addr, bool &unmapped, bool &restricted)
{
	unmapped = false;
	restricted = false; // this will track whether 8bit writes are allowed

	// handle SIWRAM and non-shared IWRAM in here too, since it is quite similar to vram.
	// in fact it is probably implemented with the same pieces of hardware.
	// its sort of like arm7 non-shared IWRAM is lowest priority, and then SIWRAM goes on top.
	// however, we implement it differently than vram in emulator for historical reasons.
	// instead of keeping a page map like we do vram, we just have a list of all possible page maps (there are only 4 each for arm9 and arm7)
	if (addr >= 0x03000000 && addr < 0x04000000)
	{
		// blocks 0,1,2,3 is arm7 non-shared IWRAM and blocks 4,5 is SIWRAM, and block 8 is un-mapped zeroes
		int iwram_block_16k;
		int iwram_offset = addr & 0x3FFF;
		addr &= 0x00FFFFFF;
		if (PROCNUM == ARMCPU_ARM7)
		{
			static const int arm7_siwram_blocks[][4][4] =
			{
				{
					{0, 1, 2, 3}, //WRAMCNT = 0 -> map to IWRAM
					{4, 4, 4, 4}, //WRAMCNT = 1 -> map to SIWRAM block 0
					{5, 5, 5, 5}, //WRAMCNT = 2 -> map to SIWRAM block 1
					{4, 5, 4, 5}, //WRAMCNT = 3 -> map to SIWRAM blocks 0,1
				},
				//high region; always maps to non-shared IWRAM
				{
					{0, 1, 2, 3},
					{0, 1, 2, 3},
					{0, 1, 2, 3},
					{0, 1, 2, 3}
				}
			};
			int region = (addr >> 23) & 1;
			int block = (addr >> 14) & 3;
			assert(region < 2);
			assert(block < 4);
			iwram_block_16k = arm7_siwram_blocks[region][st->MMU.WRAMCNT][block];
		} //PROCNUM == ARMCPU_ARM7
		else
		{
			// PROCNUM == ARMCPU_ARM9
			static const int arm9_siwram_blocks[][4] =
			{
				{4, 5, 4, 5}, //WRAMCNT = 0 -> map to SIWRAM blocks 0,1
				{5, 5, 5, 5}, //WRAMCNT = 1 -> map to SIWRAM block 1
				{4, 4, 4, 4}, //WRAMCNT = 2 -> map to SIWRAM block 0
				{8, 8, 8, 8}, //WRAMCNT = 3 -> unmapped
			};
			int block = (addr >> 14) & 3;
			assert(block < 4);
			iwram_block_16k = arm9_siwram_blocks[st->MMU.WRAMCNT][block];
		}

		switch (iwram_block_16k >> 2)
		{
			case 0: // arm7 non-shared IWRAM
				return ARM7_HACKY_IWRAM_LOCATION + (iwram_block_16k << 14) + iwram_offset;
			case 1: //SIWRAM
				return ARM7_HACKY_SIWRAM_LOCATION + ((iwram_block_16k & 3) << 14) + iwram_offset;
			case 2: //zeroes
			CASE2:
				unmapped = true;
				return 0;
			default:
				assert(false); //how did this happen?
				goto CASE2;
		}
	}

	// in case the address is entirely outside of the interesting VRAM ranges
	if (addr < 0x06000000)
		return addr;
	if (addr >= 0x07000000)
		return addr;

	restricted = true;

	// handle LCD memory mirroring
	// TODO - this is gross! this should be renovated if the vram mapping is ever done in a more sophisticated way taking into account dasShiny research
	if (addr >= 0x068A4000)
		addr = 0x06800000 +
		//(addr % 0xA4000); // yuck!! is this even how it mirrors? but we have to keep from overrunning the buffer somehow
		(addr & 0x80000); // just as likely to be right (I have no clue how it should work) but faster.

	uint32_t vram_page;
	uint32_t ofs = addr & 0x3FFF;

	// return addresses in LCDC range
	if (addr >= 0x06800000)
	{
		// already in LCDC range. just look it up to see whether it is unmapped
		vram_page = (addr >> 14) & 63;
		assert(vram_page < VRAM_LCDC_PAGES);
		vram_page = st->vram_lcdc_map[vram_page];
	}
	else
	{
		// map addresses in BG/OBJ range to an LCDC range
		vram_page = (addr >> 14) & (VRAM_ARM9_PAGES - 1);
		assert(vram_page < VRAM_ARM9_PAGES);
		vram_page = st->vram_arm9_map[vram_page];
	}

	if (vram_page == VRAM_PAGE_UNMAPPED)
	{
		unmapped = true;
		return 0;
	}
	else
		return LCDC_HACKY_LOCATION + (vram_page << 14) + ofs;
}

// maps the specified bank to LCDC
static inline void MMU_vram_lcdc(vio2sf_state *st, int bank)
{
	for (int i = 0; i < vram_bank_info[bank].num_pages; ++i)
	{
		int page = vram_bank_info[bank].page_addr + i;
		st->vram_lcdc_map[page] = page;
	}
}

// maps the specified bank to ARM9 at the provided page offset
static inline void MMU_vram_arm9(vio2sf_state *st, int bank, int offset)
{
	for (int i = 0; i < vram_bank_info[bank].num_pages; ++i)
		st->vram_arm9_map[i + offset] = vram_bank_info[bank].page_addr + i;
}

static inline uint8_t *MMU_vram_physical(vio2sf_state *st, int page)
{
	return st->MMU.ARM9_LCD + (page/**ADDRESS_STEP_16KB*/);
}

// todo - templateize
static inline void MMU_VRAMmapRefreshBank(vio2sf_state *st, int bank)
{
	int block = bank;
	if (bank >= VRAM_BANK_H)
		++block;

	uint8_t VRAMBankCnt = T1ReadByte(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x240 + block);

	// do nothing if the bank isnt enabled
	uint8_t en = VRAMBankCnt & 0x80;
	if (!en)
		return;

	int mst, ofs = 0;
	switch (bank)
	{
		case VRAM_BANK_A:
		case VRAM_BANK_B:
			mst = VRAMBankCnt & 3;
			ofs = (VRAMBankCnt >> 3) & 3;
			switch (mst)
			{
				case 0: // LCDC
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::LCDC;
					MMU_vram_lcdc(st, bank);
					break;
				case 1: // ABG
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::ABG;
					MMU_vram_arm9(st, bank, VRAM_PAGE_ABG + ofs * 8);
					break;
				case 2: // AOBJ
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::AOBJ;
					switch (ofs)
					{
						case 0:
						case 1:
							MMU_vram_arm9(st, bank, VRAM_PAGE_AOBJ + ofs * 8);
					}
					break;
				case 3: // texture
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::TEX;
					st->MMU.texInfo.textureSlotAddr[ofs] = MMU_vram_physical(st, vram_bank_info[bank].page_addr);
					break;
				default:
					goto unsupported_mst;
			}
			break;

		case VRAM_BANK_C:
		case VRAM_BANK_D:
			mst = VRAMBankCnt & 7;
			ofs = (VRAMBankCnt >> 3) & 3;
			switch (mst)
			{
				case 0: // LCDC
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::LCDC;
					MMU_vram_lcdc(st, bank);
					break;
				case 1: // ABG
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::ABG;
					MMU_vram_arm9(st, bank, VRAM_PAGE_ABG + ofs * 8);
					break;
				case 2: // arm7
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::ARM7;
					if (bank == 2)
						T1WriteByte(st->MMU.MMU_MEM[ARMCPU_ARM7][0x40], 0x240, T1ReadByte(st->MMU.MMU_MEM[ARMCPU_ARM7][0x40], 0x240) | 1);
					if (bank == 3)
						T1WriteByte(st->MMU.MMU_MEM[ARMCPU_ARM7][0x40], 0x240, T1ReadByte(st->MMU.MMU_MEM[ARMCPU_ARM7][0x40], 0x240) | 2);
					switch (ofs)
					{
						case 0:
						case 1:
							st->vram_arm7_map[ofs] = vram_bank_info[bank].page_addr;
					}
					break;
				case 3: // texture
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::TEX;
					st->MMU.texInfo.textureSlotAddr[ofs] = MMU_vram_physical(st, vram_bank_info[bank].page_addr);
					break;
				case 4: // BGB or BOBJ
					if (bank == VRAM_BANK_C)
					{
						st->vramConfiguration.banks[bank].purpose = VramConfiguration::BBG;
						MMU_vram_arm9(st, bank, VRAM_PAGE_BBG); // BBG
					}
					else
					{
						st->vramConfiguration.banks[bank].purpose = VramConfiguration::BOBJ;
						MMU_vram_arm9(st, bank, VRAM_PAGE_BOBJ); // BOBJ
					}
					break;
				default:
					goto unsupported_mst;
			}
			break;

		case VRAM_BANK_E:
			mst = VRAMBankCnt & 7;
			switch (mst)
			{
				case 0: // LCDC
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::LCDC;
					MMU_vram_lcdc(st, bank);
					break;
				case 1: // ABG
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::ABG;
					MMU_vram_arm9(st, bank, VRAM_PAGE_ABG);
					break;
				case 2: // AOBJ
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::AOBJ;
					MMU_vram_arm9(st, bank, VRAM_PAGE_AOBJ);
					break;
				case 3: // texture palette
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::TEXPAL;
					st->MMU.texInfo.texPalSlot[0] = MMU_vram_physical(st, vram_bank_info[bank].page_addr);
					st->MMU.texInfo.texPalSlot[1] = MMU_vram_physical(st, vram_bank_info[bank].page_addr + 1);
					st->MMU.texInfo.texPalSlot[2] = MMU_vram_physical(st, vram_bank_info[bank].page_addr + 2);
					st->MMU.texInfo.texPalSlot[3] = MMU_vram_physical(st, vram_bank_info[bank].page_addr + 3);
					break;
				case 4: // ABG extended palette
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::ABGEXTPAL;
					st->MMU.ExtPal[0][0] = MMU_vram_physical(st, vram_bank_info[bank].page_addr);
					st->MMU.ExtPal[0][1] = st->MMU.ExtPal[0][0]/* + ADDRESS_STEP_8KB*/;
					st->MMU.ExtPal[0][2] = st->MMU.ExtPal[0][1]/* + ADDRESS_STEP_8KB*/;
					st->MMU.ExtPal[0][3] = st->MMU.ExtPal[0][2]/* + ADDRESS_STEP_8KB*/;
					break;
				default:
					goto unsupported_mst;
			}
			break;

		case VRAM_BANK_F:
		case VRAM_BANK_G:
		{
			mst = VRAMBankCnt & 7;
			ofs = (VRAMBankCnt >> 3) & 3;
			const int pageofslut[] = {0, 1, 4, 5};
			int pageofs = pageofslut[ofs];
			switch (mst)
			{
				case 0: // LCDC
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::LCDC;
					MMU_vram_lcdc(st, bank);
					break;
				case 1: // ABG
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::ABG;
					MMU_vram_arm9(st, bank, VRAM_PAGE_ABG + pageofs);
					MMU_vram_arm9(st, bank, VRAM_PAGE_ABG + pageofs + 2); // unexpected mirroring (required by spyro eternal night)
					break;
				case 2: // AOBJ
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::AOBJ;
					MMU_vram_arm9(st, bank, VRAM_PAGE_AOBJ + pageofs);
					MMU_vram_arm9(st, bank, VRAM_PAGE_AOBJ + pageofs + 2); // unexpected mirroring - I have no proof, but it is inferred from the ABG above
					break;
				case 3: // texture palette
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::TEXPAL;
					st->MMU.texInfo.texPalSlot[pageofs] = MMU_vram_physical(st, vram_bank_info[bank].page_addr);
					break;
				case 4: // ABG extended palette
					switch (ofs)
					{
						case 0:
						case 1:
							st->vramConfiguration.banks[bank].purpose = VramConfiguration::ABGEXTPAL;
							st->MMU.ExtPal[0][ofs * 2] = MMU_vram_physical(st, vram_bank_info[bank].page_addr);
							st->MMU.ExtPal[0][ofs * 2 + 1] = st->MMU.ExtPal[0][ofs * 2]/* + ADDRESS_STEP_8KB*/;
							break;
						default:
							st->vramConfiguration.banks[bank].purpose = VramConfiguration::INVALID;
					}
					break;
				case 5: // AOBJ extended palette
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::AOBJEXTPAL;
					st->MMU.ObjExtPal[0][0] = MMU_vram_physical(st, vram_bank_info[bank].page_addr);
					st->MMU.ObjExtPal[0][1] = st->MMU.ObjExtPal[0][1]/* + ADDRESS_STEP_8KB*/;
					break;
				default:
					goto unsupported_mst;
			}
			break;
		}

		case VRAM_BANK_H:
			mst = VRAMBankCnt & 3;
			switch (mst)
			{
				case 0: // LCDC
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::LCDC;
					MMU_vram_lcdc(st, bank);
					break;
				case 1: // BBG
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::BBG;
					MMU_vram_arm9(st, bank, VRAM_PAGE_BBG);
					MMU_vram_arm9(st, bank, VRAM_PAGE_BBG + 4); // unexpected mirroring
					break;
				case 2: // BBG extended palette
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::BBGEXTPAL;
					st->MMU.ExtPal[1][0] = MMU_vram_physical(st, vram_bank_info[bank].page_addr);
					st->MMU.ExtPal[1][1] = st->MMU.ExtPal[1][0]/* + ADDRESS_STEP_8KB*/;
					st->MMU.ExtPal[1][2] = st->MMU.ExtPal[1][1]/* + ADDRESS_STEP_8KB*/;
					st->MMU.ExtPal[1][3] = st->MMU.ExtPal[1][2]/* + ADDRESS_STEP_8KB*/;
					break;
				default:
					goto unsupported_mst;
			}
			break;

		case VRAM_BANK_I:
			mst = VRAMBankCnt & 3;
			switch (mst)
			{
				case 0: // LCDC
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::LCDC;
					MMU_vram_lcdc(st, bank);
					break;
				case 1: // BBG
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::BBG;
					MMU_vram_arm9(st, bank, VRAM_PAGE_BBG + 2);
					MMU_vram_arm9(st, bank, VRAM_PAGE_BBG + 3); // unexpected mirroring
					break;
				case 2: // BOBJ
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::BOBJ;
					MMU_vram_arm9(st, bank, VRAM_PAGE_BOBJ);
					MMU_vram_arm9(st, bank, VRAM_PAGE_BOBJ + 1); // FF3 end scene (lens flare sprite) needs this as it renders a sprite off the end of the 16KB and back around
					break;
				case 3: // BOBJ extended palette
					st->vramConfiguration.banks[bank].purpose = VramConfiguration::BOBJEXTPAL;
					st->MMU.ObjExtPal[1][0] = MMU_vram_physical(st, vram_bank_info[bank].page_addr);
					st->MMU.ObjExtPal[1][1] = st->MMU.ObjExtPal[1][1]/* + ADDRESS_STEP_8KB*/;
					break;
				default:
					goto unsupported_mst;
			}
			break;
	} // switch(bank)

	st->vramConfiguration.banks[bank].ofs = ofs;

	return;

unsupported_mst:
	st->vramConfiguration.banks[bank].purpose = VramConfiguration::INVALID;
}

void MMU_VRAM_unmap_all(vio2sf_state *st)
{
	st->vramConfiguration.clear();

	st->vram_arm7_map[0] = VRAM_PAGE_UNMAPPED;
	st->vram_arm7_map[1] = VRAM_PAGE_UNMAPPED;

	for (unsigned i = 0; i < VRAM_LCDC_PAGES; ++i)
		st->vram_lcdc_map[i] = VRAM_PAGE_UNMAPPED;
	for (int i = 0; i < VRAM_ARM9_PAGES; ++i)
		st->vram_arm9_map[i] = VRAM_PAGE_UNMAPPED;

	for (int i = 0; i < 4; ++i)
	{
		st->MMU.ExtPal[0][i] = st->MMU.blank_memory;
		st->MMU.ExtPal[1][i] = st->MMU.blank_memory;
	}

	st->MMU.ObjExtPal[0][0] = st->MMU.blank_memory;
	st->MMU.ObjExtPal[0][1] = st->MMU.blank_memory;
	st->MMU.ObjExtPal[1][0] = st->MMU.blank_memory;
	st->MMU.ObjExtPal[1][1] = st->MMU.blank_memory;

	for (int i = 0; i < 6; ++i)
		st->MMU.texInfo.texPalSlot[i] = st->MMU.blank_memory;

	for (int i = 0; i < 4; ++i)
		st->MMU.texInfo.textureSlotAddr[i] = st->MMU.blank_memory;
}

static inline void MMU_VRAMmapControl(vio2sf_state *st, uint8_t block, uint8_t VRAMBankCnt)
{
	// handle WRAM, first of all
	if (block == 7)
	{
		st->MMU.WRAMCNT = VRAMBankCnt & 3;
		return;
	}

	// first, save the texture info so we can check it for changes and trigger purges of the texcache
	//MMU_struct::TextureInfo oldTexInfo = MMU.texInfo;

	// unmap everything
	MMU_VRAM_unmap_all(st);

	// unmap VRAM_BANK_C and VRAM_BANK_D from arm7. theyll get mapped again in a moment if necessary
	T1WriteByte(st->MMU.MMU_MEM[ARMCPU_ARM7][0x40], 0x240, 0);

	// write the new value to the reg
	T1WriteByte(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x240 + block, VRAMBankCnt);

	// refresh all bank settings
	// zero XX-XX-200X (long before jun 2012)
	// these are enumerated so that we can tune the order they get applied
	// in order to emulate prioritization rules for memory regions
	// with multiple banks mapped.
	// We're probably still not mapping things 100% correctly, but this helped us get closer:
	// goblet of fire "care of magical creatures" maps I and D to BOBJ (the I is an accident)
	// and requires A to override it.
	// This may create other bugs....
	MMU_VRAMmapRefreshBank(st, VRAM_BANK_I);
	MMU_VRAMmapRefreshBank(st, VRAM_BANK_H);
	MMU_VRAMmapRefreshBank(st, VRAM_BANK_G);
	MMU_VRAMmapRefreshBank(st, VRAM_BANK_F);
	MMU_VRAMmapRefreshBank(st, VRAM_BANK_E);
	// zero 21-jun-2012
	// tomwi's streaming music demo sets A and D to ABG (the A is an accident).
	// in this case, D should get priority.
	// this is somewhat risky. will it break other things?
	MMU_VRAMmapRefreshBank(st, VRAM_BANK_A);
	MMU_VRAMmapRefreshBank(st, VRAM_BANK_B);
	MMU_VRAMmapRefreshBank(st, VRAM_BANK_C);
	MMU_VRAMmapRefreshBank(st, VRAM_BANK_D);

	//fprintf(stderr, vramConfiguration.describe().c_str());
	//fprintf(stderr, "vram remapped at vcount=%d\n",nds.VCount);

	// -------------------------------
	// set up arm9 mirrorings
	// these are probably not entirely accurate. more study will be necessary.
	// in general, we find that it is not uncommon at all for games to accidentally do this.
	//
	// being able to easily do these experiments was one of the primary motivations for this remake of the vram mapping system

	// see the "unexpected mirroring" comments above for some more mirroring
	// so far "unexpected mirrorings" are tested by combining these games:
	// despereaux - storybook subtitles
	// NSMB - world map sub screen
	// drill spirits EU - mission select (just for control purposes, as it doesnt use H or I)
	// ...
	// note that the "unexpected mirroring" items above may at some point rely on being executed in a certain order.
	// (sequentially A..I)

	const int types[] = { VRAM_PAGE_ABG, VRAM_PAGE_BBG, VRAM_PAGE_AOBJ, VRAM_PAGE_BOBJ };
	const int sizes[] = {32, 8, 16, 8};
	for (int t = 0; t < 4; ++t)
	{
		// the idea here is to pad out the mirrored space with copies of the mappable area,
		// without respect to what is mapped within that mappable area.
		// we hope that this is correct in all cases
		// required for driller spirits in mission select (mapping is simple A,B,C,D to each purpose)
		int size = sizes[t];
		int mask = size - 1;
		int type = types[t];
		for (int i = size; i < 128; ++i)
		{
			int page = type + i;
			st->vram_arm9_map[page] = st->vram_arm9_map[type + (i & mask)];
		}
	}
}

//////////////////////////////////////////////////////////////
//end vram
//////////////////////////////////////////////////////////////

void MMU_Init(vio2sf_state *st)
{
	memset(&st->MMU, 0, sizeof(MMU_struct));

	MMU_init_mem_ptrs(st->MMU);

	st->MMU.CART_ROM = st->MMU.UNUSED_RAM;

	// even though apps may change dtcm immediately upon startup, this is the correct hardware starting value:
	st->MMU.DTCMRegion = 0x08000000;
	st->MMU.ITCMRegion = 0x00000000;

	IPC_FIFOinit(st, ARMCPU_ARM9);
	IPC_FIFOinit(st, ARMCPU_ARM7);
	new(&st->MMU_new) MMU_struct_new(st);

	mc_init(&st->MMU.fw, MC_TYPE_FLASH); /* init fw device */
	mc_alloc(&st->MMU.fw, NDS_FW_SIZE_V1);
	//st->MMU.fw.fp = nullptr;
	st->MMU.fw.isFirmware = true;
}

void MMU_DeInit(vio2sf_state *st)
{
	mc_free(&st->MMU.fw);
}

void MMU_Reset(vio2sf_state *st)
{
	memset(st->MMU.ARM9_DTCM, 0, sizeof(st->MMU.ARM9_DTCM));
	memset(st->MMU.ARM9_ITCM, 0, sizeof(st->MMU.ARM9_ITCM));
	memset(st->MMU.ARM9_LCD, 0, sizeof(st->MMU.ARM9_LCD));
	memset(st->MMU.ARM9_OAM, 0, sizeof(st->MMU.ARM9_OAM));
	memset(st->MMU.ARM9_REG, 0, sizeof(st->MMU.ARM9_REG));
	memset(st->MMU.ARM9_VMEM, 0, sizeof(st->MMU.ARM9_VMEM));
	memset(st->MMU.MAIN_MEM, 0, sizeof(st->MMU.MAIN_MEM));

	memset(st->MMU.blank_memory, 0, sizeof(st->MMU.blank_memory));
	memset(st->MMU.UNUSED_RAM, 0, sizeof(st->MMU.UNUSED_RAM));
	memset(st->MMU.MORE_UNUSED_RAM, 0, sizeof(st->MMU.UNUSED_RAM));

	memset(st->MMU.ARM7_ERAM, 0, sizeof(st->MMU.ARM7_ERAM));
	memset(st->MMU.ARM7_REG, 0, sizeof(st->MMU.ARM7_REG));
	memset(st->MMU.ARM7_WIRAM, 0, sizeof(st->MMU.ARM7_WIRAM));
	memset(st->MMU.SWIRAM, 0, sizeof(st->MMU.SWIRAM));

	IPC_FIFOinit(st, ARMCPU_ARM9);
	IPC_FIFOinit(st, ARMCPU_ARM7);

	st->MMU.DTCMRegion = 0x027C0000;
	st->MMU.ITCMRegion = 0x00000000;

	memset(st->MMU.timer, 0, sizeof(uint16_t) * 8);
	memset(st->MMU.timerMODE, 0, sizeof(int32_t) * 8);
	memset(st->MMU.timerON, 0, sizeof(uint32_t) * 8);
	memset(st->MMU.timerRUN, 0, sizeof(uint32_t) * 8);
	memset(st->MMU.timerReload, 0, sizeof(uint16_t) * 8);

	memset(st->MMU.reg_IME, 0, sizeof(uint32_t) * 2);
	memset(st->MMU.reg_IE, 0, sizeof(uint32_t) * 2);
	memset(st->MMU.reg_IF_bits, 0, sizeof(uint32_t) * 2);
	memset(st->MMU.reg_IF_pending, 0, sizeof(uint32_t) * 2);

	memset(st->MMU.dscard, 0, sizeof(nds_dscard) * 2);

	st->MMU.divRunning = 0;
	st->MMU.divResult = 0;
	st->MMU.divMod = 0;
	st->MMU.divCycles = 0;

	st->MMU.sqrtRunning = 0;
	st->MMU.sqrtResult = 0;
	st->MMU.sqrtCycles = 0;

	st->MMU.SPI_CNT = 0;
	st->MMU.AUX_SPI_CNT = 0;

	st->MMU.WRAMCNT = 0;

	// Enable the sound speakers
	T1WriteWord(st->MMU.ARM7_REG, 0x304, 0x0001);

	MMU_VRAM_unmap_all(st);

	st->MMU.powerMan_CntReg = 0x00;
	st->MMU.powerMan_CntRegWritten = false;
	st->MMU.powerMan_Reg[0] = 0x0B;
	st->MMU.powerMan_Reg[1] = 0x00;
	st->MMU.powerMan_Reg[2] = 0x01;
	st->MMU.powerMan_Reg[3] = 0x00;

	st->partie = 1;

	memset(st->MMU.dscard[ARMCPU_ARM9].command, 0, 8);
	st->MMU.dscard[ARMCPU_ARM9].address = 0;
	st->MMU.dscard[ARMCPU_ARM9].transfer_count = 0;
	st->MMU.dscard[ARMCPU_ARM9].mode = CardMode_Normal;

	memset(st->MMU.dscard[ARMCPU_ARM7].command, 0, 8);
	st->MMU.dscard[ARMCPU_ARM7].address = 0;
	st->MMU.dscard[ARMCPU_ARM7].transfer_count = 0;
	st->MMU.dscard[ARMCPU_ARM7].mode = CardMode_Normal;

	// HACK!!!
	// until we improve all our session tracking stuff, we need to save the backup memory filename
	std::string bleh = st->MMU_new.backupDevice.getFilename();
	BackupDevice tempBackupDevice;
	reconstruct(st, &st->MMU_new);
	st->MMU_new.backupDevice.load_rom(bleh);

	st->MMU_timing.arm7codeFetch.Reset();
	st->MMU_timing.arm7dataFetch.Reset();
	st->MMU_timing.arm9codeFetch.Reset();
	st->MMU_timing.arm9dataFetch.Reset();
	st->MMU_timing.arm9codeCache.Reset();
	st->MMU_timing.arm9dataCache.Reset();
}

void SetupMMU(vio2sf_state *st, bool debugConsole, bool dsi)
{
	if (debugConsole)
		st->_MMU_MAIN_MEM_MASK = 0x7FFFFF;
	else
		st->_MMU_MAIN_MEM_MASK = 0x3FFFFF;
	if (dsi)
		st->_MMU_MAIN_MEM_MASK = 0xFFFFFF;
	st->_MMU_MAIN_MEM_MASK16 = st->_MMU_MAIN_MEM_MASK & ~1;
	st->_MMU_MAIN_MEM_MASK32 = st->_MMU_MAIN_MEM_MASK & ~3;
}

void MMU_setRom(vio2sf_state *st, uint8_t *rom, uint32_t)
{
	st->MMU.CART_ROM = rom;
}

void MMU_unsetRom(vio2sf_state *st)
{
	st->MMU.CART_ROM = st->MMU.UNUSED_RAM;
}

static void execsqrt(vio2sf_state *st)
{
	uint32_t ret;
	uint8_t mode = st->MMU_new.sqrt.mode;
	st->MMU_new.sqrt.busy = 1;

	if (mode)
	{
		uint64_t v = T1ReadQuad(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x2B8);
		ret = isqrt(v) & 0xFFFFFFFF;
	}
	else
	{
		uint32_t v = T1ReadLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x2B8);
		ret = isqrt(v) & 0xFFFFFFFF;
	}

	// clear the result while the sqrt unit is busy
	// todo - is this right? is it reasonable?
	T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x2B4, 0);

	st->MMU.sqrtCycles = st->nds_timer + 26;
	st->MMU.sqrtResult = ret;
	st->MMU.sqrtRunning = true;
	NDS_Reschedule(st);
}

static void execdiv(vio2sf_state *st)
{
	int64_t num, den;
	int64_t res, mod;
	uint8_t mode = st->MMU_new.div.mode;
	st->MMU_new.div.busy = 1;
	st->MMU_new.div.div0 = 0;

	switch (mode)
	{
		case 0: // 32/32
			num = T1ReadLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x290);
			den = T1ReadLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x298);
			st->MMU.divCycles = st->nds_timer + 36;
			break;
		case 1: // 64/32
		case 3: // gbatek says this is same as mode 1
			num = T1ReadQuad(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x290);
			den = T1ReadLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x298);
			st->MMU.divCycles = st->nds_timer + 68;
			break;
		case 2: // 64/64
		default:
			num = T1ReadQuad(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x290);
			den = T1ReadQuad(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x298);
			st->MMU.divCycles = st->nds_timer + 68;
	}

	if (!den)
	{
		res = num < 0 ? 1 : -1;
		mod = num;

		// the DIV0 flag in DIVCNT is set only if the full 64bit DIV_DENOM value is zero, even in 32bit mode
		if (!T1ReadQuad(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x298))
			st->MMU_new.div.div0 = 1;
	}
	else
	{
		res = num / den;
		mod = num % den;
	}

	T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x2A0, 0);
	T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x2A4, 0);
	T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x2A8, 0);
	T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x2AC, 0);

	st->MMU.divResult = res;
	st->MMU.divMod = mod;
	st->MMU.divRunning = true;
	NDS_Reschedule(st);
}

DSI_TSC::DSI_TSC()
{
	for (unsigned i = 0; i < ARRAY_SIZE(this->registers); ++i)
		this->registers[i] = 0x00;
	this->reset_command();
}

void DSI_TSC::reset_command()
{
	this->state = 0;
	this->readcount = 0;
	this->read_flag = 1;
}

uint16_t DSI_TSC::write16(uint16_t val)
{
	uint16_t ret;
	switch (state)
	{
		case 0:
			this->reg_selection = (val >> 1) & 0x7F;
			this->read_flag = val & 1;
			this->state = 1;
			return this->read16();
		case 1:
			if (!this->read_flag)
				this->registers[this->reg_selection] = val & 0xFF;
			ret = this->read16();
			++this->reg_selection;
			this->reg_selection &= 0x7F;
			return ret;
	}
	return 0;
}

uint16_t DSI_TSC::read16()
{
	uint8_t page = registers[0];
	switch (page)
	{
		case 3: // page 3
			switch (this->reg_selection)
			{
				case 9:
					return 0x40;
				case 14:
					return 0x02;
			}
	} // switch(page)

	// unknown page or register
	return 0xFF;
}

// TODO:
// NAND flash support (used in Made in Ore/WarioWare D.I.Y.)
void FASTCALL MMU_writeToGCControl(vio2sf_state *st, int PROCNUM, uint32_t val)
{
	int TEST_PROCNUM = PROCNUM;
	nds_dscard &card = st->MMU.dscard[TEST_PROCNUM];

	memcpy(&card.command[0], &st->MMU.MMU_MEM[TEST_PROCNUM][0x40][0x1A8], 8);

	card.blocklen = 0;
	st->slot1_device.write32(st, PROCNUM, 0xFFFFFFFF, val); // Special case for some flashcarts
	if (card.blocklen == 0x01020304)
		return;

	if (!(val & 0x80000000))
	{
		card.address = 0;
		card.transfer_count = 0;

		val &= 0x7F7FFFFF;
		T1WriteLong(st->MMU.MMU_MEM[TEST_PROCNUM][0x40], 0x1A4, val);
		return;
	}

	uint32_t shift = (val >> 24) & 7;
	if (shift == 7)
		card.transfer_count = 1;
	else if (!shift)
		card.transfer_count = 0;
	else
		card.transfer_count = (0x100 << shift) / 4;

	switch (card.mode)
	{
		case CardMode_Normal:
			break;

		case CardMode_KEY1:
			// TODO
			//INFO("Cartridge: KEY1 mode unsupported.\n");

			card.address = 0;
			card.transfer_count = 0;

			val &= 0x7F7FFFFF;
			T1WriteLong(st->MMU.MMU_MEM[TEST_PROCNUM][0x40], 0x1A4, val);
			return;
		case CardMode_KEY2:
			//INFO("Cartridge: KEY2 mode unsupported.\n");
			break;
	}

	switch (card.command[0])
	{
		case 0x9F: // Dummy
			card.address = 0;
			card.transfer_count = 0x800;
			break;

		case 0x3C: // Switch to KEY1 mode
			card.mode = CardMode_KEY1;
			break;

		default:
			// fall through to the special slot1 handler
			st->slot1_device.write32(st, TEST_PROCNUM, REG_GCROMCTRL, val);
	}

	if (!card.transfer_count)
	{
		val &= 0x7F7FFFFF;
		T1WriteLong(st->MMU.MMU_MEM[TEST_PROCNUM][0x40], 0x1A4, val);
		return;
	}

	val |= 0x00800000;
	T1WriteLong(st->MMU.MMU_MEM[TEST_PROCNUM][0x40], 0x1A4, val);

	// Launch DMA if start flag was set to "DS Cart"
	//fprintf(stderr, "triggering card dma\n");
	triggerDma(st, EDMAMode_Card);
}

uint32_t MMU_readFromGC(vio2sf_state *st, int PROCNUM)
{
	int TEST_PROCNUM = PROCNUM;

	nds_dscard& card = st->MMU.dscard[TEST_PROCNUM];
	uint32_t val = 0;

	if (!card.transfer_count)
		return 0;

	switch (card.command[0])
	{
		case 0x9F: // Dummy
			val = 0xFFFFFFFF;
			break;

		case 0x3C: // Switch to KEY1 mode
			val = 0xFFFFFFFF;
			break;

		default:
			val = st->slot1_device.read32(st, TEST_PROCNUM, REG_GCDATAIN);
	}

	card.address += 4; // increment address

	--card.transfer_count; // update transfer counter
	if (card.transfer_count) // if transfer is not ended
		return val; // return data

	// transfer is done
	T1WriteLong(st->MMU.MMU_MEM[TEST_PROCNUM][0x40], 0x1A4, T1ReadLong(st->MMU.MMU_MEM[TEST_PROCNUM][0x40], 0x1A4) & 0x7F7FFFFF);

	// if needed, throw irq for the end of transfer
	if (st->MMU.AUX_SPI_CNT & 0x4000)
		NDS_makeIrq(st, TEST_PROCNUM, IRQ_BIT_GC_TRANSFER_COMPLETE);

	return val;
}

static void REG_IF_WriteByte(vio2sf_state *st, int PROCNUM, uint32_t addr, uint8_t val)
{
	// the following bits are generated from logic and should not be affected here
	// Bit 21    NDS9 only: Geometry Command FIFO
	// arm9: IF &= ~0x00200000;
	// arm7: IF &= ~0x00000000;
	// UPDATE IN setIF() ALSO!!!!!!!!!!!!!!!!
	// UPDATE IN mmu_loadstate ALSO!!!!!!!!!!!!
	if (addr == 2)
	{
		if (PROCNUM == ARMCPU_ARM9)
			val &= ~0x20;
		else
			val &= ~0x00;
	}

	// ZERO 01-dec-2010 : I am no longer sure this approach is correct.. it proved to be wrong for IPC fifo.......
	// it seems as if IF bits should always be cached (only the user can clear them)

	st->MMU.reg_IF_bits[PROCNUM] &= ~(val << (addr << 3));
	NDS_Reschedule(st);
}

static void REG_IF_WriteWord(vio2sf_state *st, int PROCNUM, uint32_t addr, uint16_t val)
{
	REG_IF_WriteByte(st, PROCNUM, addr, val & 0xFF);
	REG_IF_WriteByte(st, PROCNUM, addr + 1, (val >> 8) & 0xFF);
}

static void REG_IF_WriteLong(vio2sf_state *st, int PROCNUM, uint32_t val)
{
	REG_IF_WriteByte(st, PROCNUM, 0, val & 0xFF);
	REG_IF_WriteByte(st, PROCNUM, 1, (val >> 8) & 0xFF);
	REG_IF_WriteByte(st, PROCNUM, 2, (val >> 16) & 0xFF);
	REG_IF_WriteByte(st, PROCNUM, 3, (val >> 24) & 0xFF);
}

template<int PROCNUM> uint32_t MMU_struct::gen_IF()
{
	return this->reg_IF_bits[PROCNUM];
}

static inline void MMU_IPCSync(vio2sf_state *st, uint8_t proc, uint32_t val)
{
	uint32_t sync_l = T1ReadLong(st->MMU.MMU_MEM[proc][0x40], 0x180) & 0xFFFF;
	uint32_t sync_r = T1ReadLong(st->MMU.MMU_MEM[proc ^ 1][0x40], 0x180) & 0xFFFF;

	sync_l = (sync_l & 0x000F) | (val & 0x0F00);
	sync_r = (sync_r & 0x6F00) | ((val >> 8) & 0x000F);

	sync_l |= val & 0x6000;

	T1WriteLong(st->MMU.MMU_MEM[proc][0x40], 0x180, sync_l);
	T1WriteLong(st->MMU.MMU_MEM[proc ^ 1][0x40], 0x180, sync_r);

	if ((sync_l & IPCSYNC_IRQ_SEND) && (sync_r & IPCSYNC_IRQ_RECV))
		NDS_makeIrq(st, proc ^ 1, IRQ_BIT_IPCSYNC);

	NDS_Reschedule(st);
}

static inline uint16_t read_timer(vio2sf_state *st, int proc, int timerIndex)
{
	// chained timers are always up to date
	if (st->MMU.timerMODE[proc][timerIndex] == 0xFFFF)
		return st->MMU.timer[proc][timerIndex];

	// sometimes a timer will be read when it is not enabled.
	// we should have the value cached
	if (!st->MMU.timerON[proc][timerIndex])
		return st->MMU.timer[proc][timerIndex];

	// for unchained timers, we do not keep the timer up to date. its value will need to be calculated here
	int32_t diff = (st->nds.timerCycle[proc][timerIndex] - st->nds_timer) & 0xFFFFFFFF;
	assert(diff >= 0);
	if (diff < 0)
		fprintf(stderr, "NEW EMULOOP BAD NEWS PLEASE REPORT: TIME READ DIFF < 0 (%d) (%d) (%d)\n", diff, timerIndex, st->MMU.timerMODE[proc][timerIndex]);

	int32_t units = diff / (1 << st->MMU.timerMODE[proc][timerIndex]);
	int32_t ret;

	if (units == 65536)
		ret = 0; // I'm not sure why this is happening...
	// whichever instruction setup this counter should advance nds_timer (I think?) and the division should truncate down to 65535 immediately
	else if (units > 65536)
	{
		fprintf(stderr, "NEW EMULOOP BAD NEWS PLEASE REPORT: UNITS %d:%d = %d\n", proc, timerIndex, units);
		ret = 0;
	}
	else
		ret = 65535 - units;

	return ret & 0xFFFF;
}

static inline void write_timer(vio2sf_state *st, int proc, int timerIndex, uint16_t val)
{
	if (val & 0x80)
		st->MMU.timer[proc][timerIndex] = st->MMU.timerReload[proc][timerIndex];
	else if (st->MMU.timerON[proc][timerIndex])
		// read the timer value one last time
		st->MMU.timer[proc][timerIndex] = read_timer(st, proc, timerIndex);

	st->MMU.timerON[proc][timerIndex] = val & 0x80;

	switch (val & 7)
	{
		case 0:
			st->MMU.timerMODE[proc][timerIndex] = 1;
			break;
		case 1:
			st->MMU.timerMODE[proc][timerIndex] = 7;
			break;
		case 2:
			st->MMU.timerMODE[proc][timerIndex] = 9;
			break;
		case 3:
			st->MMU.timerMODE[proc][timerIndex] = 11;
			break;
		default:
			st->MMU.timerMODE[proc][timerIndex] = 0xFFFF;
	}

	int remain = 65536 - st->MMU.timerReload[proc][timerIndex];
	st->nds.timerCycle[proc][timerIndex] = st->nds_timer + (remain << st->MMU.timerMODE[proc][timerIndex]);

	T1WriteWord(st->MMU.MMU_MEM[proc][0x40], 0x102 + timerIndex * 4, val);
	NDS_RescheduleTimers(st);
}

uint32_t TGXSTAT::read32()
{
	uint32_t ret = 0;

	ret |= this->tb | (this->tr << 1);

	ret |= this->sb << 14; // stack busy
	ret |= this->se << 15;
	ret |= 255 << 16;

	ret |= (this->gxfifo_irq & 0x3) << 30; // user's irq flags

	//fprintf(stderr, "vc=%03d Returning gxstat read: %08X\n",nds.VCount,ret);

	return ret;
}

void TGXSTAT::write32(uint32_t val)
{
	this->gxfifo_irq = (val >> 30) & 3;
	if (BIT15(val))
	{
		// Writing "1" to Bit15 does reset the Error Flag (Bit15),
		// and additionally resets the Projection Stack Pointer (Bit13)
		//mtxStack[0].position = 0;
		this->se = 0; // clear stack error flag
	}
	//fprintf(stderr, "gxstat write: %08X while gxfifo.size=%d\n",val,gxFIFO.size);
}

// this could be inlined...
void MMU_struct_new::write_dma(int proc, int size, uint32_t _adr, uint32_t val)
{
	//fprintf(stderr, "%08lld -- write_dma: %d %d %08X %08X\n",nds_timer,proc,size,_adr,val);
	uint32_t adr = _adr - _REG_DMA_CONTROL_MIN;
	uint32_t chan = adr / 12;
	uint32_t regnum = (adr - chan * 12) >> 2;

	this->dma[proc][chan].regs[regnum]->write(size, adr, val);
}

// this could be inlined...
uint32_t MMU_struct_new::read_dma(int proc, int size, uint32_t _adr)
{
	uint32_t adr = _adr - _REG_DMA_CONTROL_MIN;
	uint32_t chan = adr / 12;
	uint32_t regnum = (adr - chan * 12) >> 2;

	uint32_t temp = this->dma[proc][chan].regs[regnum]->read(size, adr);
	//fprintf(stderr, "%08lld --  read_dma: %d %d %08X = %08X\n",nds_timer,proc,size,_adr,temp);

	return temp;
}

MMU_struct_new::MMU_struct_new(vio2sf_state *_st)
: dma{{_st, _st, _st, _st}, {_st, _st, _st, _st}}, st(_st)
{
	for (int i = 0; i < 2; ++i)
		for (int j = 0; j < 4; ++j)
		{
			dma[i][j].procnum = i;
			dma[i][j].chan = j;
		}
}

void DmaController::write32(uint32_t val)
{
	//fprintf(stderr, "dma %d,%d WRITE %08X\n",procnum,chan,val);
	this->wordcount = val & 0x1FFFFF;
	uint8_t wasEnable = this->enable;
	uint32_t valhi = val >> 16;
	this->dar = static_cast<EDMADestinationUpdate>((valhi >> 5) & 3);
	this->sar = static_cast<EDMASourceUpdate>((valhi >> 7) & 3);
	this->repeatMode = BIT9(valhi);
	this->bitWidth = static_cast<EDMABitWidth>(BIT10(valhi));
	this->_startmode = (valhi >> 11) & 7;
	if (this->procnum == ARMCPU_ARM7)
		this->_startmode &= 6;
	this->irq = BIT14(valhi);
	this->enable = BIT15(valhi);

	// make sure we don't get any old triggers
	if (!wasEnable && this->enable)
		this->triggered = false;

	if (this->enable)
	{
		// address registers are reloaded from user's settings whenever dma is enabled
		// this is tested well by contra4 classic games, which use this to hdma scroll registers
		// specifically in the fit-screen mode.
		this->saddr = this->saddr_user;
		this->daddr = this->daddr_user;
	}

	//fprintf(stderr, "dma %d,%d set to startmode %d with wordcount set to: %08X\n",procnum,chan,_startmode,wordcount);
	// analyze enabling and startmode.
	// note that we only do this if the dma was freshly enabled.
	// we should probably also only be latching these other regs in that case too..
	// but for now just this one will do (otherwise the dma repeat stop procedure (in this case the ff4 title menu load with gamecard dma) will fail)
	//if(!running) enable = userEnable;

	// if we were previously in a triggered mode, and were already enabled,
	// then don't re-trigger now. this is rather confusing..
	// we really only want to auto-trigger gxfifo and immediate modes.
	// but we don't know what mode we're in yet.
	// so this is our workaround
	// (otherwise the dma repeat stop procedure (in this case the ff4 title menu load with gamecard dma) will fail)
	bool doNotStart = false;
	if (this->startmode != EDMAMode_Immediate && wasEnable)
		doNotStart = true;

	// this dma may need to trigger now, so give it a chance
	//if(!(wasRepeatMode && !repeatMode)) //this was an older test
	if (!doNotStart)
		this->doSchedule();
}

void DmaController::exec()
{
	// this function runs when the DMA ends. the dma start actually queues this event after some kind of guess as to how long the DMA should take

	// we'll need to unfreeze the arm9 bus now
	if (this->procnum == ARMCPU_ARM9)
		st->nds.freezeBus &= ~(1 << (this->chan + 1));

	this->dmaCheck = false;

	if (this->running)
	{
		this->doStop();
		return;
	}

	if (this->enable)
	{
		// analyze startmode (this only gets latched when a dma begins)
		if (this->procnum == ARMCPU_ARM9)
			this->startmode = static_cast<EDMAMode>(this->_startmode);
		else
		{
			// arm7 startmode analysis:
			static const EDMAMode lookup[] = { EDMAMode_Immediate, EDMAMode_VBlank, EDMAMode_Card, EDMAMode7_Wifi };
			// arm7 has a slightly different startmode encoding
			this->startmode = lookup[this->_startmode >> 1];
			if (this->startmode == EDMAMode7_Wifi && (this->chan == 1 || this->chan == 3))
				this->startmode = EDMAMode7_GBASlot;
		}

		// make it run, if it is triggered
		// but first, scan for triggering conditions
		switch (this->startmode)
		{
			case EDMAMode_Immediate:
				this->triggered = true;
				break;
			default:
				break;
		}

		if (this->triggered)
		{
			this->running = true;
			this->paused = false;
			if (this->procnum == ARMCPU_ARM9)
				this->doCopy<ARMCPU_ARM9>();
			else
				this->doCopy<ARMCPU_ARM7>();
		}
	}
}

template<int PROCNUM> void DmaController::doCopy()
{
	// generate a copy count depending on various copy mode's behavior
	uint32_t todo = this->wordcount;
	if (PROCNUM == ARMCPU_ARM9)
		if (!todo)
			todo = 0x200000; // according to gbatek.. we've verified this behaviour on the arm7
	if (this->startmode == EDMAMode_MemDisplay)
	{
		todo = 128; // this is a hack. maybe an alright one though. it should be 4 words at a time. this is a whole scanline

		// apparently this dma turns off after it finishes a frame
		if (st->nds.VCount == 191)
			this->enable = 0;
	}
	if (this->startmode == EDMAMode_Card)
		todo *= 0x80;

	// determine how we're going to copy
	bool bogarted = false;
	uint32_t sz = this->bitWidth == EDMABitWidth_16 ? 2 : 4;
	uint32_t dstinc = 0, srcinc = 0;
	switch (this->dar)
	{
		case EDMADestinationUpdate_Increment:
			dstinc = sz;
			break;
		case EDMADestinationUpdate_Decrement:
			dstinc = -static_cast<int32_t>(sz);
			break;
		case EDMADestinationUpdate_Fixed:
			dstinc = 0;
			break;
		case EDMADestinationUpdate_IncrementReload:
			dstinc = sz;
	}
	switch (this->sar)
	{
		case EDMASourceUpdate_Increment:
			srcinc = sz;
			break;
		case EDMASourceUpdate_Decrement:
			srcinc = -static_cast<int32_t>(sz);
			break;
		case EDMASourceUpdate_Fixed:
			srcinc = 0;
			break;
		case EDMASourceUpdate_Invalid:
			bogarted = true;
	}

	// need to figure out what to do about this
	if (bogarted)
	{
		fprintf(stderr, "YOUR GAME IS BOGARTED!!! PLEASE REPORT!!!\n");
		assert(false);
		return;
	}

	uint32_t src = this->saddr;
	uint32_t dst = this->daddr;

	// if these do not use MMU_AT_DMA and the corresponding code in the read/write routines,
	// then danny phantom title screen will be filled with a garbage char which is made by
	// dmaing from 0x00000000 to 0x06000000
	// TODO - these might be losing out a lot by not going through the templated version anymore.
	// we might make another function to do just the raw copy op which can use them with checks
	// outside the loop
	int time_elapsed = 0;
	for (int32_t i = todo; i > 0; --i)
	{
		if (sz == 4)
		{
			time_elapsed += _MMU_accesstime<PROCNUM, MMU_AT_DMA, 32, MMU_AD_READ, true>(st, src, true);
			time_elapsed += _MMU_accesstime<PROCNUM, MMU_AT_DMA, 32, MMU_AD_WRITE, true>(st, dst, true);
			uint32_t temp = _MMU_read32(st, procnum, MMU_AT_DMA, src);
			_MMU_write32(st, procnum, MMU_AT_DMA, dst, temp);
		}
		else
		{
			time_elapsed += _MMU_accesstime<PROCNUM, MMU_AT_DMA, 16, MMU_AD_READ, true>(st, src, true);
			time_elapsed += _MMU_accesstime<PROCNUM, MMU_AT_DMA, 16, MMU_AD_WRITE, true>(st, dst, true);
			uint16_t temp = _MMU_read16(st, procnum, MMU_AT_DMA, src);
			_MMU_write16(st, procnum, MMU_AT_DMA, dst, temp);
		}
		dst += dstinc;
		src += srcinc;
	}

	// reschedule an event for the end of this dma, and figure out how much it cost us
	this->doSchedule();

	// zeromus, check it
	if (this->wordcount > todo)
		this->nextEvent += todo / 4; // TODO - surely this is a gross simplification
	// apparently moon has very, very tight timing (i didnt spy it using waitbyloop swi...)
	// so lets bump this down a bit for now,
	// (i think this code is in nintendo libraries)

	// write back the addresses
	this->saddr = src;
	if (this->dar != EDMADestinationUpdate_IncrementReload) // but dont write back dst if we were supposed to reload
		this->daddr = dst;

	// do wordcount accounting
	if (this->startmode == EDMAMode_Card)
		todo /= 0x80; // divide this funky one back down before subtracting it

	if (!this->repeatMode)
		this->wordcount -= todo;
}

void triggerDma(vio2sf_state *st, EDMAMode mode)
{
	MACRODO2(0, {
		int i = X;
		MACRODO4(0, {
			int j = X;
			st->MMU_new.dma[i][j].tryTrigger(mode);
		});
	});
}

void DmaController::tryTrigger(EDMAMode mode)
{
	if (this->startmode != mode)
		return;
	if (!this->enable)
		return;

	// hmm dont trigger it if its already running!
	// but paused things need triggers to continue
	if (this->running && !this->paused)
		return;
	this->triggered = true;
	this->doSchedule();
}

void DmaController::doSchedule()
{
	this->dmaCheck = true;
	this->nextEvent = st->nds_timer;
	NDS_RescheduleDMA(st);
}

void DmaController::doPause()
{
	this->triggered = false;
	this->paused = true;
}

void DmaController::doStop()
{
	this->running = false;
	if (!this->repeatMode)
		this->enable = false;
	if (this->irq)
		NDS_makeIrq(st, this->procnum, IRQ_BIT_DMA_0 + this->chan);
}

uint32_t DmaController::read32()
{
	uint32_t ret = 0;
	ret |= this->enable << 31;
	ret |= this->irq << 30;
	ret |= this->_startmode << 27;
	ret |= this->bitWidth << 26;
	ret |= this->repeatMode << 25;
	ret |= this->sar << 23;
	ret |= this->dar << 21;
	ret |= this->wordcount;
	//fprintf(stderr, "dma %d,%d READ  %08X\n",procnum,chan,ret);
	return ret;
}

// ================================================================================================== ARM9 *
// =========================================================================================================
// =========================================================================================================
// ================================================= MMU write 08
void FASTCALL _MMU_ARM9_write08(vio2sf_state *st, uint32_t adr, uint8_t val)
{
	adr &= 0x0FFFFFFF;

	if (adr < 0x02000000)
	{
		T1WriteByte(st->MMU.ARM9_ITCM, adr & 0x7FFF, val);
		return;
	}

	if (adr >= 0x08000000 && adr < 0x0A010000)
		return;

	// block 8bit writes to OAM and palette memory
	if ((adr & 0x0F000000) == 0x07000000)
		return;
	if ((adr & 0x0F000000) == 0x05000000)
		return;

	if ((adr >> 24) == 4)
	{
		if (st->MMU_new.is_dma(adr))
		{
			st->MMU_new.write_dma(ARMCPU_ARM9, 8, adr, val);
			return;
		}

		switch (adr)
		{
			case REG_SQRTCNT:
				fprintf(stderr, "ERROR 8bit SQRTCNT WRITE\n");
				return;
			case REG_SQRTCNT + 1:
				fprintf(stderr, "ERROR 8bit SQRTCNT1 WRITE\n");
				return;
			case REG_SQRTCNT + 2:
				fprintf(stderr, "ERROR 8bit SQRTCNT2 WRITE\n");
				return;
			case REG_SQRTCNT + 3:
				fprintf(stderr, "ERROR 8bit SQRTCNT3 WRITE\n");
				return;

#if 1
			case REG_DIVCNT:
				fprintf(stderr, "ERROR 8bit DIVCNT WRITE\n");
				return;
			case REG_DIVCNT + 1:
				fprintf(stderr, "ERROR 8bit DIVCNT+1 WRITE\n");
				return;
			case REG_DIVCNT + 2:
				fprintf(stderr, "ERROR 8bit DIVCNT+2 WRITE\n");
				return;
			case REG_DIVCNT + 3:
				fprintf(stderr, "ERROR 8bit DIVCNT+3 WRITE\n");
				return;
#endif

			case REG_IF:
				REG_IF_WriteByte(st, ARMCPU_ARM9, 0, val);
				break;
			case REG_IF + 1:
				REG_IF_WriteByte(st, ARMCPU_ARM9, 1, val);
				break;
			case REG_IF + 2:
				REG_IF_WriteByte(st, ARMCPU_ARM9, 2, val);
				break;
			case REG_IF + 3:
				REG_IF_WriteByte(st, ARMCPU_ARM9, 3, val);
				break;

			case REG_VRAMCNTA:
			case REG_VRAMCNTB:
			case REG_VRAMCNTC:
			case REG_VRAMCNTD:
			case REG_VRAMCNTE:
			case REG_VRAMCNTF:
			case REG_VRAMCNTG:
			case REG_WRAMCNT:
			case REG_VRAMCNTH:
			case REG_VRAMCNTI:
				MMU_VRAMmapControl(st, adr - REG_VRAMCNTA, val);
		}

		st->MMU.MMU_MEM[ARMCPU_ARM9][adr >> 20][adr & st->MMU.MMU_MASK[ARMCPU_ARM9][adr >> 20]] = val;
		return;
	}

	// Removed the &0xFF as they are implicit with the adr&0x0FFFFFFF [shash]
	st->MMU.MMU_MEM[ARMCPU_ARM9][adr >> 20][adr & st->MMU.MMU_MASK[ARMCPU_ARM9][adr >> 20]] = val;
}

// ================================================= MMU ARM9 write 16
void FASTCALL _MMU_ARM9_write16(vio2sf_state *st, uint32_t adr, uint16_t val)
{
	adr &= 0x0FFFFFFE;

	if (adr < 0x02000000)
	{
		T1WriteWord(st->MMU.ARM9_ITCM, adr & 0x7FFF, val);
		return;
	}

	if (adr >= 0x08000000 && adr < 0x0A010000)
		return;

	if ((adr >> 24) == 4)
	{
		if (st->MMU_new.is_dma(adr))
		{
			st->MMU_new.write_dma(ARMCPU_ARM9, 16, adr, val);
			return;
		}

		switch (adr >> 4)
		{
			// toon table
			case 0x0400038:
			case 0x0400039:
			case 0x040003A:
			case 0x040003B:
				reinterpret_cast<uint16_t *>(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40])[(adr & 0xFFF) >> 1] = val;
				return;
		}
		// Address is an IO register
		switch (adr)
		{
			case REG_DIVCNT:
				st->MMU_new.div.write16(val);
				execdiv(st);
				return;

#if 1
			case REG_DIVNUMER:
			case REG_DIVNUMER + 2:
			case REG_DIVNUMER + 4:
				fprintf(stderr, "DIV: 16 write NUMER %08X. PLEASE REPORT! \n", val);
				break;
			case REG_DIVDENOM:
			case REG_DIVDENOM + 2:
			case REG_DIVDENOM + 4:
				fprintf(stderr, "DIV: 16 write DENOM %08X. PLEASE REPORT! \n", val);
				break;
#endif
			case REG_SQRTCNT:
				st->MMU_new.sqrt.write16(val);
				execsqrt(st);
				return;

			case REG_EXMEMCNT:
			{
				uint16_t remote_proc = T1ReadWord(st->MMU.MMU_MEM[ARMCPU_ARM7][0x40], 0x204);
				T1WriteWord(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x204, val);
				T1WriteWord(st->MMU.MMU_MEM[ARMCPU_ARM7][0x40], 0x204, (val & 0xFF80) | (remote_proc & 0x7F));
				return;
			}

			case REG_VRAMCNTA:
			case REG_VRAMCNTC:
			case REG_VRAMCNTE:
			case REG_VRAMCNTG:
			case REG_VRAMCNTH:
				MMU_VRAMmapControl(st, adr - REG_VRAMCNTA, val & 0xFF);
				MMU_VRAMmapControl(st, adr - REG_VRAMCNTA + 1, val >> 8);
				break;

			case REG_IME:
				NDS_Reschedule(st);
				st->MMU.reg_IME[ARMCPU_ARM9] = val & 0x01;
				T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x208, val);
				return;
			case REG_IE:
				NDS_Reschedule(st);
				st->MMU.reg_IE[ARMCPU_ARM9] = (st->MMU.reg_IE[ARMCPU_ARM9] & 0xFFFF0000) | val;
				return;
			case REG_IE + 2:
				NDS_Reschedule(st);
				st->MMU.reg_IE[ARMCPU_ARM9] = (st->MMU.reg_IE[ARMCPU_ARM9] & 0xFFFF) | (val << 16);
				return;
			case REG_IF:
				REG_IF_WriteWord(st, ARMCPU_ARM9, 0, val);
				return;
			case REG_IF + 2:
				REG_IF_WriteWord(st, ARMCPU_ARM9, 2, val);
				return;

			case REG_IPCSYNC:
				MMU_IPCSync(st, ARMCPU_ARM9, val);
				return;

			case REG_IPCFIFOCNT:
				IPC_FIFOcnt(st, ARMCPU_ARM9, val);
				return;
			case REG_TM0CNTL:
			case REG_TM1CNTL:
			case REG_TM2CNTL:
			case REG_TM3CNTL:
				st->MMU.timerReload[ARMCPU_ARM9][(adr >> 2) & 3] = val;
				return;
			case REG_TM0CNTH:
			case REG_TM1CNTH:
			case REG_TM2CNTH:
			case REG_TM3CNTH:
			{
				int timerIndex	= ((adr - 2) >> 2) & 0x3;
				write_timer(st, ARMCPU_ARM9, timerIndex, val);
				return;
			}

			case REG_GCROMCTRL:
				MMU_writeToGCControl(st, ARMCPU_ARM9, (T1ReadLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x1A4) & 0xFFFF0000) | val);
				return;
			case REG_GCROMCTRL + 2:
				MMU_writeToGCControl(st, ARMCPU_ARM9, (T1ReadLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x1A4) & 0xFFFF) | (val << 16));
				return;
		}

		T1WriteWord(st->MMU.MMU_MEM[ARMCPU_ARM9][adr >> 20], adr & st->MMU.MMU_MASK[ARMCPU_ARM9][adr >> 20], val);
		return;
	}

	bool unmapped, restricted;
	adr = MMU_LCDmap(st, ARMCPU_ARM9, adr, unmapped, restricted);
	if (unmapped)
		return;

	// Removed the &0xFF as they are implicit with the adr&0x0FFFFFFF [shash]
	T1WriteWord(st->MMU.MMU_MEM[ARMCPU_ARM9][adr >> 20], adr & st->MMU.MMU_MASK[ARMCPU_ARM9][adr >> 20], val);
}

// ================================================= MMU ARM9 write 32
void FASTCALL _MMU_ARM9_write32(vio2sf_state *st, uint32_t adr, uint32_t val)
{
	adr &= 0x0FFFFFFC;

	if (adr < 0x02000000)
	{
		T1WriteLong(st->MMU.ARM9_ITCM, adr & 0x7FFF, val);
		return;
	}

	if (adr >= 0x08000000 && adr < 0x0A010000)
		return;

	if ((adr >> 24) == 4)
	{
		if (st->MMU_new.is_dma(adr))
		{
			st->MMU_new.write_dma(ARMCPU_ARM9, 32, adr, val);
			return;
		}

		switch (adr)
		{
			case REG_SQRTCNT:
				st->MMU_new.sqrt.write16(val & 0xFFFF);
				return;
			case REG_DIVCNT:
				st->MMU_new.div.write16(val & 0xFFFF);
				return;

			case REG_VRAMCNTA:
			case REG_VRAMCNTE:
				MMU_VRAMmapControl(st, adr - REG_VRAMCNTA, val & 0xFF);
				MMU_VRAMmapControl(st, adr - REG_VRAMCNTA + 1, (val >> 8) & 0xFF);
				MMU_VRAMmapControl(st, adr - REG_VRAMCNTA + 2, (val >> 16) & 0xFF);
				MMU_VRAMmapControl(st, adr - REG_VRAMCNTA + 3, (val >> 24) & 0xFF);
				break;
			case REG_VRAMCNTH:
				MMU_VRAMmapControl(st, adr - REG_VRAMCNTA, val & 0xFF);
				MMU_VRAMmapControl(st, adr - REG_VRAMCNTA + 1, (val >> 8) & 0xFF);
				break;

			case REG_IME:
				NDS_Reschedule(st);
				st->MMU.reg_IME[ARMCPU_ARM9] = val & 0x01;
				T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x208, val);
				return;

			case REG_IE:
				NDS_Reschedule(st);
				st->MMU.reg_IE[ARMCPU_ARM9] = val;
				return;

			case REG_IF:
				REG_IF_WriteLong(st, ARMCPU_ARM9, val);
				return;

			case REG_TM0CNTL:
			case REG_TM1CNTL:
			case REG_TM2CNTL:
			case REG_TM3CNTL:
			{
				int timerIndex = (adr >> 2) & 0x3;
				st->MMU.timerReload[ARMCPU_ARM9][timerIndex] = val & 0xFFFF;
				T1WriteWord(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], adr & 0xFFF, val & 0xFFFF);
				write_timer(st, ARMCPU_ARM9, timerIndex, val >> 16);
				return;
			}

			case REG_DIVNUMER:
				T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x290, val);
				execdiv(st);
				return;
			case REG_DIVNUMER + 4:
				T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x294, val);
				execdiv(st);
				return;

			case REG_DIVDENOM:
			{
				T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x298, val);
				execdiv(st);
				return;
			}
			case REG_DIVDENOM + 4:
			{
				T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x29C, val);
				execdiv(st);
				return;
			}

			case REG_SQRTPARAM:
			{
				T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x2B8, val);
				execsqrt(st);
				return;
			}
			case REG_SQRTPARAM + 4:
			{
				T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x2BC, val);
				execsqrt(st);
				return;
			}

			case REG_IPCSYNC:
				MMU_IPCSync(st, ARMCPU_ARM9, val);
				return;
			case REG_IPCFIFOCNT:
				IPC_FIFOcnt(st, ARMCPU_ARM9, val & 0xFFFF);
				return;
			case REG_IPCFIFOSEND:
				IPC_FIFOsend(st, ARMCPU_ARM9, val);
				return;

			case REG_GCROMCTRL:
				MMU_writeToGCControl(st, ARMCPU_ARM9, val);
				return;
			case REG_DISPA_DISPCAPCNT:
				T1WriteLong(st->MMU.ARM9_REG, 0x64, val);
				return;

			case REG_GCDATAIN:
				st->slot1_device.write32(st, ARMCPU_ARM9, REG_GCDATAIN, val);
				return;
		}

		T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][adr >> 20], adr & st->MMU.MMU_MASK[ARMCPU_ARM9][adr >> 20], val);
		return;
	}

	bool unmapped, restricted;
	adr = MMU_LCDmap(st, ARMCPU_ARM9, adr, unmapped, restricted);
	if (unmapped)
		return;

	// Removed the &0xFF as they are implicit with the adr&0x0FFFFFFF [shash]
	T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM9][adr >> 20], adr & st->MMU.MMU_MASK[ARMCPU_ARM9][adr >> 20], val);
}

// ================================================= MMU ARM9 read 08
uint8_t FASTCALL _MMU_ARM9_read08(vio2sf_state *st, uint32_t adr)
{
	adr &= 0x0FFFFFFF;

	if (adr<0x02000000)
		return T1ReadByte(st->MMU.ARM9_ITCM, adr & 0x7FFF);

	if (adr >= 0x08000000 && adr < 0x0A010000)
		return 0;

	if ((adr >> 24) == 4)
	{
		//Address is an IO register

		if (st->MMU_new.is_dma(adr))
			return st->MMU_new.read_dma(ARMCPU_ARM9, 8, adr) & 0xFF;

		switch (adr)
		{
			case REG_IF:
				return st->MMU.gen_IF<ARMCPU_ARM9>() & 0xFF;
			case REG_IF + 1:
				return (st->MMU.gen_IF<ARMCPU_ARM9>() >> 8) & 0xFF;
			case REG_IF + 2:
				return (st->MMU.gen_IF<ARMCPU_ARM9>() >> 16) & 0xFF;
			case REG_IF + 3:
				return (st->MMU.gen_IF<ARMCPU_ARM9>() >> 24) & 0xFF;

			case REG_WRAMCNT:
				return st->MMU.WRAMCNT;

			case REG_SQRTCNT:
				return st->MMU_new.sqrt.read16() & 0xFF;
			case REG_SQRTCNT + 1:
				return (st->MMU_new.sqrt.read16() >> 8) & 0xFF;

			// sqrtcnt isnt big enough for these to exist. but they'd probably return 0 so its ok
			case REG_SQRTCNT + 2:
				fprintf(stderr, "ERROR 8bit SQRTCNT+2 READ\n");
				return 0;
			case REG_SQRTCNT + 3:
				fprintf(stderr, "ERROR 8bit SQRTCNT+3 READ\n");
				return 0;

			// Nostalgia's options menu requires that these work
			case REG_DIVCNT:
				return st->MMU_new.div.read16() & 0xFF;
			case REG_DIVCNT + 1:
				return (st->MMU_new.div.read16() >> 8) & 0xFF;

			// divcnt isnt big enough for these to exist. but they'd probably return 0 so its ok
			case REG_DIVCNT + 2:
				fprintf(stderr, "ERROR 8bit DIVCNT+2 READ\n");
				return 0;
			case REG_DIVCNT + 3:
				fprintf(stderr, "ERROR 8bit DIVCNT+3 READ\n");
				return 0;
		}
	}

	bool unmapped, restricted;
	adr = MMU_LCDmap(st, ARMCPU_ARM9, adr, unmapped, restricted);
	if (unmapped)
		return 0;

	return st->MMU.MMU_MEM[ARMCPU_ARM9][(adr >> 20) & 0xFF][adr & st->MMU.MMU_MASK[ARMCPU_ARM9][(adr >> 20) & 0xFF]];
}

// ================================================= MMU ARM9 read 16
uint16_t FASTCALL _MMU_ARM9_read16(vio2sf_state *st, uint32_t adr)
{
	adr &= 0x0FFFFFFE;

	if (adr < 0x02000000)
		return T1ReadWord_guaranteedAligned(st->MMU.ARM9_ITCM, adr & 0x7FFE);

	if (adr >= 0x08000000 && adr < 0x0A010000)
		return 0;

	if ((adr >> 24) == 4)
	{
		if (st->MMU_new.is_dma(adr))
			return st->MMU_new.read_dma(ARMCPU_ARM9, 16, adr) & 0xFFFF;

		// Address is an IO register
		switch (adr)
		{
			case REG_SQRTCNT:
				return st->MMU_new.sqrt.read16();
			// sqrtcnt isnt big enough for this to exist. but it'd probably return 0 so its ok
			case REG_SQRTCNT + 2:
				fprintf(stderr, "ERROR 16bit SQRTCNT+2 READ\n");
				return 0;

			case REG_DIVCNT:
				return st->MMU_new.div.read16();
			// divcnt isnt big enough for this to exist. but it'd probably return 0 so its ok
			case REG_DIVCNT + 2:
				fprintf(stderr, "ERROR 16bit DIVCNT+2 READ\n");
				return 0;

			case REG_IME:
				return st->MMU.reg_IME[ARMCPU_ARM9] & 0xFFFF;

			// WRAMCNT is readable but VRAMCNT is not, so just return WRAM's value
			case REG_VRAMCNTG:
				return st->MMU.WRAMCNT << 8;

			case REG_IE:
				return st->MMU.reg_IE[ARMCPU_ARM9] & 0xFFFF;
			case REG_IE + 2:
				return (st->MMU.reg_IE[ARMCPU_ARM9] >> 16) & 0xFFFF;

			case REG_IF:
				return st->MMU.gen_IF<ARMCPU_ARM9>() & 0xFFFF;
			case REG_IF + 2:
				return (st->MMU.gen_IF<ARMCPU_ARM9>() >> 16) & 0xFFFF;

			case REG_TM0CNTL:
			case REG_TM1CNTL:
			case REG_TM2CNTL:
			case REG_TM3CNTL:
				return read_timer(st, ARMCPU_ARM9, (adr & 0xF) >> 2);

			case REG_AUXSPICNT:
				return st->MMU.AUX_SPI_CNT;
		}

		return T1ReadWord_guaranteedAligned(st->MMU.MMU_MEM[ARMCPU_ARM9][adr >> 20], adr & st->MMU.MMU_MASK[ARMCPU_ARM9][adr >> 20]);
	}

	// Removed the &0xFF as they are implicit with the adr&0x0FFFFFFF
	return T1ReadWord_guaranteedAligned(st->MMU.MMU_MEM[ARMCPU_ARM9][adr >> 20], adr & st->MMU.MMU_MASK[ARMCPU_ARM9][adr >> 20]);
}

// ================================================= MMU ARM9 read 32
uint32_t FASTCALL _MMU_ARM9_read32(vio2sf_state *st, uint32_t adr)
{
	adr &= 0x0FFFFFFC;

	if (adr < 0x02000000)
		return T1ReadLong_guaranteedAligned(st->MMU.ARM9_ITCM, adr & 0x7FFC);

	if (adr >= 0x08000000 && adr < 0x0A010000)
		return 0;

	// Address is an IO register
	if ((adr >> 24) == 4)
	{
		if (st->MMU_new.is_dma(adr))
			return st->MMU_new.read_dma(ARMCPU_ARM9, 32, adr);

		switch (adr)
		{
			case REG_DSIMODE:
				if (!st->nds.Is_DSI())
					break;
				return 1;
			case 0x04004008:
				if (!st->nds.Is_DSI())
					break;
				return 0x8000;

			// WRAMCNT is readable but VRAMCNT is not, so just return WRAM's value
			case REG_VRAMCNTE:
				return st->MMU.WRAMCNT << 24;

			// despite these being 16bit regs,
			// Dolphin Island Underwater Adventures uses this amidst seemingly reasonable divs so we're going to emulate it.
			// well, it's pretty reasonable to read them as 32bits though, isnt it?
			case REG_DIVCNT:
				return st->MMU_new.div.read16();
			case REG_SQRTCNT:
				return st->MMU_new.sqrt.read16(); // I guess we'll do this also

			case REG_IME:
				return st->MMU.reg_IME[ARMCPU_ARM9];
			case REG_IE:
				return st->MMU.reg_IE[ARMCPU_ARM9];

			case REG_IF:
				return st->MMU.gen_IF<ARMCPU_ARM9>();

			case REG_IPCFIFORECV:
				return IPC_FIFOrecv(st, ARMCPU_ARM9);
			case REG_TM0CNTL:
			case REG_TM1CNTL:
			case REG_TM2CNTL:
			case REG_TM3CNTL:
			{
				uint32_t val = T1ReadWord(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], (adr + 2) & 0xFFF);
				return st->MMU.timer[ARMCPU_ARM9][(adr & 0xF) >> 2] | (val << 16);
			}

			case REG_GCDATAIN:
				return MMU_readFromGC(st, ARMCPU_ARM9);
		}
		return T1ReadLong_guaranteedAligned(st->MMU.MMU_MEM[ARMCPU_ARM9][adr >> 20], adr & st->MMU.MMU_MASK[ARMCPU_ARM9][adr >> 20]);
	}

	// Removed the &0xFF as they are implicit with the adr&0x0FFFFFFF [zeromus, inspired by shash]
	return T1ReadLong_guaranteedAligned(st->MMU.MMU_MEM[ARMCPU_ARM9][adr >> 20], adr & st->MMU.MMU_MASK[ARMCPU_ARM9][adr >> 20]);
}

// ================================================================================================== ARM7 *
// =========================================================================================================
// =========================================================================================================
// ================================================= MMU ARM7 write 08
void FASTCALL _MMU_ARM7_write08(vio2sf_state *st, uint32_t adr, uint8_t val)
{
	adr &= 0x0FFFFFFF;

	if (adr < 0x02000000)
		return; // can't write to bios or entire area below main memory

	if (adr >= 0x08000000 && adr < 0x0A010000)
		return;

	if (adr >= 0x04000400 && adr < 0x04000520)
	{
		SPU_WriteByte(st, adr, val);
		return;
	}

	if ((adr & 0xFFFF0000) == 0x04800000)
		/* is wifi hardware, dont intermix with regular hardware registers */
		// 8-bit writes to wifi I/O and RAM are ignored
		// Reference: http://nocash.emubase.de/gbatek.htm#dswifiiomap
		return;

	if ((adr >> 24) == 4)
	{
		if (st->MMU_new.is_dma(adr))
		{
			st->MMU_new.write_dma(ARMCPU_ARM7, 8, adr, val);
			return;
		}

		switch (adr)
		{
			case REG_IF:
				REG_IF_WriteByte(st, ARMCPU_ARM7, 0, val);
				break;
			case REG_IF + 1:
				REG_IF_WriteByte(st, ARMCPU_ARM7, 1, val);
				break;
			case REG_IF + 2:
				REG_IF_WriteByte(st, ARMCPU_ARM7, 2, val);
				break;
			case REG_IF + 3:
				REG_IF_WriteByte(st, ARMCPU_ARM7, 3, val);
				break;

			case REG_POSTFLG:
				// The NDS7 register can be written to only from code executed in BIOS.
				if (st->NDS_ARM7.instruct_adr > 0x3FFF)
					return;

				// hack for patched firmwares
				if (val == 1)
				{
					if (_MMU_ARM7_read08(st, REG_POSTFLG))
						break;
					_MMU_write32<ARMCPU_ARM9>(st, 0x27FFE24, st->gameInfo.header.ARM9exe);
					_MMU_write32<ARMCPU_ARM7>(st, 0x27FFE34, st->gameInfo.header.ARM7exe);
				}
				break;

			case REG_HALTCNT:
				//fprintf(stderr, "halt 0x%02X\n", val);
				switch (val)
				{
					case 0xC0:
						NDS_Sleep(st);
						break;
					case 0x80:
						armcpu_Wait4IRQ(&st->NDS_ARM7);
				}
				break;
		}
		st->MMU.MMU_MEM[ARMCPU_ARM7][adr >> 20][adr & st->MMU.MMU_MASK[ARMCPU_ARM7][adr >> 20]] = val;
		return;
	}

	// Removed the &0xFF as they are implicit with the adr&0x0FFFFFFF [shash]
	st->MMU.MMU_MEM[ARMCPU_ARM7][adr >> 20][adr & st->MMU.MMU_MASK[ARMCPU_ARM7][adr >> 20]] = val;
}

// ================================================= MMU ARM7 write 16
void FASTCALL _MMU_ARM7_write16(vio2sf_state *st, uint32_t adr, uint16_t val)
{
	adr &= 0x0FFFFFFE;

	if (adr < 0x02000000)
		return; // can't write to bios or entire area below main memory

	if (adr >= 0x08000000 && adr < 0x0A010000)
		return;

	if (adr >= 0x04000400 && adr < 0x04000520)
	{
		SPU_WriteWord(st, adr, val);
		return;
	}

	if ((adr >> 24) == 4)
	{
		if (st->MMU_new.is_dma(adr))
		{
			st->MMU_new.write_dma(ARMCPU_ARM7, 16, adr, val);
			return;
		}

		// Address is an IO register
		switch (adr)
		{
			case REG_DISPA_VCOUNT:
				if (st->nds.VCount >= 202 && st->nds.VCount <= 212)
				{
					fprintf(stderr, "VCOUNT set to %i (previous value %i)\n", val, st->nds.VCount);
					st->nds.VCount = val;
				}
				else
					fprintf(stderr, "Attempt to set VCOUNT while not within 202-212 (%i), ignored\n", st->nds.VCount);
				return;

			case REG_EXMEMCNT:
			{
				uint16_t remote_proc = T1ReadWord(st->MMU.MMU_MEM[ARMCPU_ARM9][0x40], 0x204);
				T1WriteWord(st->MMU.MMU_MEM[ARMCPU_ARM7][0x40], 0x204, (val & 0x7F) | (remote_proc & 0xFF80));
				return;
			}

			case REG_SPICNT:
			{
				bool reset_firmware = true;

				if (((st->MMU.SPI_CNT >> 8) & 0x3) == 1 && ((val >> 8) & 0x3) == 1 && BIT11(st->MMU.SPI_CNT))
					// select held
					reset_firmware = false;

				//MMU.fw.com == 0; // reset fw device communication
				if (reset_firmware)
					// reset fw device communication
					fw_reset_com(&st->MMU.fw);
				st->MMU.SPI_CNT = val;

				T1WriteWord(st->MMU.MMU_MEM[ARMCPU_ARM7][(REG_SPICNT >> 20) & 0xff], REG_SPICNT & 0xfff, val);
				return;
			}

			case REG_SPIDATA:
			{
				if (val)
					st->MMU.SPI_CMD = val;

				uint16_t spicnt = T1ReadWord(st->MMU.MMU_MEM[ARMCPU_ARM7][(REG_SPICNT >> 20) & 0xff], REG_SPICNT & 0xfff);

				switch ((spicnt >> 8) & 0x3)
				{
					case 0:
					{
						if (!st->MMU.powerMan_CntRegWritten)
						{
							st->MMU.powerMan_CntReg = val & 0xFF;
							st->MMU.powerMan_CntRegWritten = true;
						}
						else
						{
							uint16_t reg = st->MMU.powerMan_CntReg & 0x7F;
							reg &= 0x7;
							if (reg == 5 || reg == 6 || reg == 7)
								reg = 4;

							// (let's start with emulating a DS lite, since it is the more complex case)
							if (st->MMU.powerMan_CntReg & 0x80)
								// read
								val = st->MMU.powerMan_Reg[reg];
							else
							{
								// write
								st->MMU.powerMan_Reg[reg] = val & 0xFF;

								static const uint32_t PM_SYSTEM_PWR = BIT(6); /*!< \brief  Turn the power *off* if set */

								// our totally pathetic register handling, only the one thing we've wanted so far
								if (st->MMU.powerMan_Reg[0] & PM_SYSTEM_PWR)
								{
									fprintf(stderr, "SYSTEM POWERED OFF VIA ARM7 SPI POWER DEVICE\n");
									st->execute = false;
								}
							}

							st->MMU.powerMan_CntRegWritten = false;
						}
						break;
					}

					case 1: /* firmware memory device */
						if (spicnt & 0x3) /* check SPI baudrate (must be 4mhz) */
						{
							T1WriteWord(st->MMU.MMU_MEM[ARMCPU_ARM7][(REG_SPIDATA >> 20) & 0xff], REG_SPIDATA & 0xfff, 0);
							break;
						}
						T1WriteWord(st->MMU.MMU_MEM[ARMCPU_ARM7][(REG_SPIDATA >> 20) & 0xff], REG_SPIDATA & 0xfff, fw_transfer(&st->MMU.fw, val & 0xFF));
						return;

					case 2:
					{
						if (st->nds.Is_DSI())
						{
							// pass data to TSC
							val = st->MMU_new.dsi_tsc.write16(val);

							// apply reset command if appropriate
							if (!BIT11(st->MMU.SPI_CNT))
								st->MMU_new.dsi_tsc.reset_command();

							break;
						}

						int channel = (st->MMU.SPI_CMD & 0x70) >> 4;
						//fprintf(stderr, "%08X\n",channel);
						switch (channel)
						{
							case TSC_MEASURE_TEMP1:
								if (spicnt & 0x800)
								{
									if (st->partie)
									{
										val = 1632;
										st->partie = 0;
										break;
									}
									val = 716 >> 5;
									st->partie = 1;
									break;
								}
								val = 1632;
								st->partie = 1;
								break;
							case TSC_MEASURE_TEMP2:
								if(spicnt & 0x800)
								{
									if(st->partie)
									{
										val = 776;
										st->partie = 0;
										break;
									}
									val = 865 >> 5;
									st->partie = 1;
									break;
								}
								val = 776;
								st->partie = 1;
								break;
							case TSC_MEASURE_Y:
								if (st->MMU.SPI_CNT & (1 << 11))
								{
									if (st->partie)
									{
										st->partie = 0;
										break;
									}
									st->partie = 1;
									break;
								}
								st->partie = 1;
								break;
							case TSC_MEASURE_Z1: // Z1
								if (spicnt & 0x800)
								{
									if (st->partie)
									{
										val = (val << 3) & 0x7FF;
										st->partie = 0;
										break;
									}
									val >>= 5;
									st->partie = 1;
									break;
								}
								val = (val << 3) & 0x7FF;
								st->partie = 1;
								break;
							case TSC_MEASURE_Z2: // Z2
								if (spicnt & 0x800)
								{
									if (st->partie)
									{
										val = (val << 3) & 0x7FF;
										st->partie = 0;
										break;
									}
									val >>= 5;
									st->partie = 1;
									break;
								}
								val = (val << 3) & 0x7FF;
								st->partie = 1;
								break;
							case TSC_MEASURE_X:
								if (spicnt & 0x800)
								{
									if (st->partie)
									{
										st->partie = 0;
										break;
									}
									st->partie = 1;
									break;
								}
								st->partie = 1;
						}
					}
				}

				T1WriteWord(st->MMU.MMU_MEM[ARMCPU_ARM7][(REG_SPIDATA >> 20) & 0xff], REG_SPIDATA & 0xfff, val);
				return;
			}

			/* NOTICE: Perhaps we have to use gbatek-like reg names instead of libnds-like ones ...*/

			case REG_IME:
				NDS_Reschedule(st);
				st->MMU.reg_IME[ARMCPU_ARM7] = val & 0x01;
				T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM7][0x40], 0x208, val);
				return;
			case REG_IE:
				NDS_Reschedule(st);
				st->MMU.reg_IE[ARMCPU_ARM7] = (st->MMU.reg_IE[ARMCPU_ARM7] & 0xFFFF0000) | val;
				return;
			case REG_IE + 2:
				NDS_Reschedule(st);
				st->MMU.reg_IE[ARMCPU_ARM7] = (st->MMU.reg_IE[ARMCPU_ARM7] & 0xFFFF) | (val << 16);
				return;

			case REG_IF:
				REG_IF_WriteWord(st, ARMCPU_ARM7, 0, val);
				return;
			case REG_IF + 2:
				REG_IF_WriteWord(st, ARMCPU_ARM7, 2, val);
				return;

			case REG_IPCSYNC:
				MMU_IPCSync(st, ARMCPU_ARM7, val);
				return;

			case REG_IPCFIFOCNT:
				IPC_FIFOcnt(st, ARMCPU_ARM7, val);
				return;
			case REG_TM0CNTL:
			case REG_TM1CNTL:
			case REG_TM2CNTL:
			case REG_TM3CNTL:
				st->MMU.timerReload[ARMCPU_ARM7][(adr >> 2) & 3] = val;
				return;
			case REG_TM0CNTH:
			case REG_TM1CNTH:
			case REG_TM2CNTH:
			case REG_TM3CNTH:
			{
				int timerIndex	= ((adr - 2) >> 2) & 0x3;
				write_timer(st, ARMCPU_ARM7, timerIndex, val);
				return;
			}

			case REG_GCROMCTRL:
				MMU_writeToGCControl(st, ARMCPU_ARM7, (T1ReadLong(st->MMU.MMU_MEM[ARMCPU_ARM7][0x40], 0x1A4) & 0xFFFF0000) | val);
				return;
			case REG_GCROMCTRL + 2:
				MMU_writeToGCControl(st, ARMCPU_ARM7, (T1ReadLong(st->MMU.MMU_MEM[ARMCPU_ARM7][0x40], 0x1A4) & 0xFFFF) | (val << 16));
				return;
		}

		T1WriteWord(st->MMU.MMU_MEM[ARMCPU_ARM7][adr >> 20], adr & st->MMU.MMU_MASK[ARMCPU_ARM7][adr >> 20], val);
		return;
	}

	// Removed the &0xFF as they are implicit with the adr&0x0FFFFFFF [shash]
	T1WriteWord(st->MMU.MMU_MEM[ARMCPU_ARM7][adr >> 20], adr & st->MMU.MMU_MASK[ARMCPU_ARM7][adr >> 20], val);
}

// ================================================= MMU ARM7 write 32
void FASTCALL _MMU_ARM7_write32(vio2sf_state *st, uint32_t adr, uint32_t val)
{
	adr &= 0x0FFFFFFC;

	if (adr < 0x02000000)
		return; // can't write to bios or entire area below main memory

	if (adr >= 0x08000000 && adr < 0x0A010000)
		return;

	if (adr >= 0x04000400 && adr < 0x04000520)
	{
		SPU_WriteLong(st, adr, val);
		return;
	}

	if ((adr >> 24) == 4)
	{
		if (st->MMU_new.is_dma(adr))
		{
			st->MMU_new.write_dma(ARMCPU_ARM7, 32, adr, val);
			return;
		}

		switch (adr)
		{
			case REG_IME:
				NDS_Reschedule(st);
				st->MMU.reg_IME[ARMCPU_ARM7] = val & 0x01;
				T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM7][0x40], 0x208, val);
				return;

			case REG_IE:
				NDS_Reschedule(st);
				st->MMU.reg_IE[ARMCPU_ARM7] = val;
				return;

			case REG_IF:
				REG_IF_WriteLong(st, ARMCPU_ARM7, val);
				return;

			case REG_TM0CNTL:
			case REG_TM1CNTL:
			case REG_TM2CNTL:
			case REG_TM3CNTL:
			{
				int timerIndex = (adr >> 2) & 0x3;
				st->MMU.timerReload[ARMCPU_ARM7][timerIndex] = val & 0xFFFF;
				T1WriteWord(st->MMU.MMU_MEM[ARMCPU_ARM7][0x40], adr & 0xFFF, val & 0xFFFF);
				write_timer(st, ARMCPU_ARM7, timerIndex, val >> 16);
				return;
			}

			case REG_IPCSYNC:
				MMU_IPCSync(st, ARMCPU_ARM7, val);
				return;
			case REG_IPCFIFOCNT:
				IPC_FIFOcnt(st, ARMCPU_ARM7, val & 0xFFFF);
				return;
			case REG_IPCFIFOSEND:
				IPC_FIFOsend(st, ARMCPU_ARM7, val);
				return;

			case REG_GCROMCTRL:
				MMU_writeToGCControl(st, ARMCPU_ARM7, val);
				return;

			case REG_GCDATAIN:
				st->slot1_device.write32(st, ARMCPU_ARM7, REG_GCDATAIN,val);
				return;
		}
		T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM7][adr >> 20], adr & st->MMU.MMU_MASK[ARMCPU_ARM7][adr >> 20], val);
		return;
	}

	// Removed the &0xFF as they are implicit with the adr&0x0FFFFFFF [shash]
	T1WriteLong(st->MMU.MMU_MEM[ARMCPU_ARM7][adr >> 20], adr & st->MMU.MMU_MASK[ARMCPU_ARM7][adr >> 20], val);
}

// ================================================= MMU ARM7 read 08
uint8_t FASTCALL _MMU_ARM7_read08(vio2sf_state *st, uint32_t adr)
{
	adr &= 0x0FFFFFFF;

	if (adr < 0x4000)
	{
		// How accurate is this? our R[15] may not be exactly what the hardware uses (may use something less by up to 0x08)
		// This may be inaccurate at the very edge cases.
		if (st->NDS_ARM7.instruct_adr > 0x3FFF)
			return 0xFF;
	}

	if (adr >= 0x08000000 && adr < 0x0A010000)
		return 0;

	if (adr >= 0x04000400 && adr < 0x04000520)
		return SPU_ReadByte(st, adr);

	if ((adr >> 24) == 4)
	{
		if (st->MMU_new.is_dma(adr))
			return st->MMU_new.read_dma(ARMCPU_ARM7, 8, adr) & 0xFF;

		// Address is an IO register

		switch (adr)
		{
			case REG_IF:
				return st->MMU.gen_IF<ARMCPU_ARM7>() & 0xFF;
			case REG_IF + 1:
				return (st->MMU.gen_IF<ARMCPU_ARM7>() >> 8) & 0xFF;
			case REG_IF + 2:
				return (st->MMU.gen_IF<ARMCPU_ARM7>() >> 16) & 0xFF;
			case REG_IF + 3:
				return (st->MMU.gen_IF<ARMCPU_ARM7>() >> 24) & 0xFF;

			case REG_WRAMSTAT:
				return st->MMU.WRAMCNT;
		}

		return st->MMU.MMU_MEM[ARMCPU_ARM7][adr >> 20][adr & st->MMU.MMU_MASK[ARMCPU_ARM7][adr >> 20]];
	}

	return st->MMU.MMU_MEM[ARMCPU_ARM7][adr >> 20][adr & st->MMU.MMU_MASK[ARMCPU_ARM7][adr >> 20]];
}

// ================================================= MMU ARM7 read 16
uint16_t FASTCALL _MMU_ARM7_read16(vio2sf_state *st, uint32_t adr)
{
	adr &= 0x0FFFFFFE;

	if (adr < 0x4000)
	{
		if (st->NDS_ARM7.instruct_adr > 0x3FFF)
			return 0xFFFF;
	}

	if (adr >= 0x08000000 && adr < 0x0A010000)
		return 0;

	if (adr >= 0x04000400 && adr < 0x04000520)
		return SPU_ReadWord(st, adr);

	if ((adr >> 24) == 4)
	{
		// Address is an IO register

		if (st->MMU_new.is_dma(adr))
			return st->MMU_new.read_dma(ARMCPU_ARM7, 16, adr) & 0xFFFF;

		switch (adr)
		{
			case REG_IME:
				return st->MMU.reg_IME[ARMCPU_ARM7] & 0xFFFF;

			case REG_IE:
				return st->MMU.reg_IE[ARMCPU_ARM7] & 0xFFFF;
			case REG_IE + 2:
				return (st->MMU.reg_IE[ARMCPU_ARM7] >> 16) & 0xFFFF;

			case REG_IF:
				return st->MMU.gen_IF<ARMCPU_ARM7>() & 0xFFFF;
			case REG_IF + 2:
				return (st->MMU.gen_IF<ARMCPU_ARM7>() >> 16) & 0xFFFF;

			case REG_TM0CNTL:
			case REG_TM1CNTL:
			case REG_TM2CNTL:
			case REG_TM3CNTL:
				return read_timer(st, ARMCPU_ARM7, (adr & 0xF) >> 2);

			case REG_VRAMSTAT:
				// make sure WRAMSTAT is stashed and then fallthrough to return the value from memory. i know, gross.
				T1WriteByte(st->MMU.MMU_MEM[ARMCPU_ARM7][0x40], 0x241, st->MMU.WRAMCNT);
				break;
		}
		return T1ReadWord_guaranteedAligned(st->MMU.MMU_MEM[ARMCPU_ARM7][adr >> 20], adr & st->MMU.MMU_MASK[ARMCPU_ARM7][adr >> 20]);
	}

	/* Returns data from memory */
	// Removed the &0xFF as they are implicit with the adr&0x0FFFFFFF
	return T1ReadWord_guaranteedAligned(st->MMU.MMU_MEM[ARMCPU_ARM7][adr >> 20], adr & st->MMU.MMU_MASK[ARMCPU_ARM7][adr >> 20]);
}

// ================================================= MMU ARM7 read 32
uint32_t FASTCALL _MMU_ARM7_read32(vio2sf_state *st, uint32_t adr)
{
	adr &= 0x0FFFFFFC;

	if (adr < 0x4000)
	{
		if (st->NDS_ARM7.instruct_adr > 0x3FFF)
			return 0xFFFFFFFF;
	}

	if (adr >= 0x08000000 && adr < 0x0A010000)
		return 0;

	if (adr >= 0x04000400 && adr < 0x04000520)
		return SPU_ReadLong(st, adr);

	if ((adr >> 24) == 4)
	{
		// Address is an IO register

		if (st->MMU_new.is_dma(adr))
			return st->MMU_new.read_dma(ARMCPU_ARM7, 32, adr);

		switch (adr)
		{
			case REG_IME:
				return st->MMU.reg_IME[ARMCPU_ARM7];
			case REG_IE:
				return st->MMU.reg_IE[ARMCPU_ARM7];
			case REG_IF:
				return st->MMU.gen_IF<ARMCPU_ARM7>();
			case REG_IPCFIFORECV:
				return IPC_FIFOrecv(st, ARMCPU_ARM7);
			case REG_TM0CNTL:
			case REG_TM1CNTL:
			case REG_TM2CNTL:
			case REG_TM3CNTL:
			{
				uint32_t val = T1ReadWord(st->MMU.MMU_MEM[ARMCPU_ARM7][0x40], (adr + 2) & 0xFFF);
				return st->MMU.timer[ARMCPU_ARM7][(adr & 0xF) >> 2] | (val << 16);
			}
			case REG_GCROMCTRL:
				break;
			case REG_GCDATAIN:
				return MMU_readFromGC(st, ARMCPU_ARM7);

			case REG_VRAMSTAT:
				// make sure WRAMSTAT is stashed and then fallthrough return the value from memory. i know, gross.
				T1WriteByte(st->MMU.MMU_MEM[ARMCPU_ARM7][0x40], 0x241, st->MMU.WRAMCNT);
				break;
		}

		return T1ReadLong_guaranteedAligned(st->MMU.MMU_MEM[ARMCPU_ARM7][adr >> 20], adr & st->MMU.MMU_MASK[ARMCPU_ARM7][adr >> 20]);
	}

	// Returns data from memory
	// Removed the &0xFF as they are implicit with the adr&0x0FFFFFFF [zeromus, inspired by shash]
	return T1ReadLong_guaranteedAligned(st->MMU.MMU_MEM[ARMCPU_ARM7][adr >> 20], adr & st->MMU.MMU_MASK[ARMCPU_ARM7][adr >> 20]);
}

// =========================================================================================================

// these templates needed to be instantiated manually
template uint32_t MMU_struct::gen_IF<ARMCPU_ARM9>();
template uint32_t MMU_struct::gen_IF<ARMCPU_ARM7>();

// formerly inline functions
uint8_t _MMU_read08(vio2sf_state *st, int PROCNUM, MMU_ACCESS_TYPE AT, uint32_t addr)
{
	// special handling for DMA: read 0 from TCM
	if (PROCNUM == ARMCPU_ARM9 && AT == MMU_AT_DMA)
	{
		if (addr < 0x02000000)
			return 0; // itcm
		if ((addr & ~0x3FFF) == st->MMU.DTCMRegion)
			return 0; // dtcm
	}

	if (PROCNUM == ARMCPU_ARM9 && (addr & ~0x3FFF) == st->MMU.DTCMRegion)
		// Returns data from DTCM (ARM9 only)
		return T1ReadByte(st->MMU.ARM9_DTCM, addr & 0x3FFF);

	if ((addr & 0x0F000000) == 0x02000000)
		return T1ReadByte(st->MMU.MAIN_MEM, addr & st->_MMU_MAIN_MEM_MASK);

	if (PROCNUM == ARMCPU_ARM9)
		return _MMU_ARM9_read08(st, addr);
	else
		return _MMU_ARM7_read08(st, addr);
}

uint16_t _MMU_read16(vio2sf_state *st, int PROCNUM, MMU_ACCESS_TYPE AT, uint32_t addr)
{
	// special handling for DMA: read 0 from TCM
	if (PROCNUM == ARMCPU_ARM9 && AT == MMU_AT_DMA)
	{
		if (addr < 0x02000000)
			return 0; // itcm
		if ((addr & ~0x3FFF) == st->MMU.DTCMRegion)
			return 0; // dtcm
	}

	// special handling for execution from arm9, since we spend so much time in there
	if (PROCNUM == ARMCPU_ARM9 && AT == MMU_AT_CODE)
	{
		if ((addr & 0x0F000000) == 0x02000000)
			return T1ReadWord_guaranteedAligned(st->MMU.MAIN_MEM, addr & st->_MMU_MAIN_MEM_MASK16);

		if (addr < 0x02000000)
			return T1ReadWord_guaranteedAligned(st->MMU.ARM9_ITCM, addr&0x7FFE);

		goto dunno;
	}

	if (PROCNUM == ARMCPU_ARM9 && (addr & ~0x3FFF) == st->MMU.DTCMRegion)
		// Returns data from DTCM (ARM9 only)
		return T1ReadWord_guaranteedAligned(st->MMU.ARM9_DTCM, addr & 0x3FFE);

	if ((addr & 0x0F000000) == 0x02000000)
		return T1ReadWord_guaranteedAligned(st->MMU.MAIN_MEM, addr & st->_MMU_MAIN_MEM_MASK16);

dunno:
	if (PROCNUM == ARMCPU_ARM9)
		return _MMU_ARM9_read16(st, addr);
	else
		return _MMU_ARM7_read16(st, addr);
}

uint32_t _MMU_read32(vio2sf_state *st, int PROCNUM, MMU_ACCESS_TYPE AT, uint32_t addr)
{
	// special handling for DMA: read 0 from TCM
	if (PROCNUM == ARMCPU_ARM9 && AT == MMU_AT_DMA)
	{
		if (addr < 0x02000000)
			return 0; // itcm
		if ((addr & ~0x3FFF) == st->MMU.DTCMRegion)
			return 0; // dtcm
	}

	//s pecial handling for execution from arm9, since we spend so much time in there
	if (PROCNUM == ARMCPU_ARM9 && AT == MMU_AT_CODE)
	{
		if ((addr & 0x0F000000) == 0x02000000)
			return T1ReadLong_guaranteedAligned(st->MMU.MAIN_MEM, addr & st->_MMU_MAIN_MEM_MASK32);

		if (addr < 0x02000000)
			return T1ReadLong_guaranteedAligned(st->MMU.ARM9_ITCM, addr&0x7FFC);

		// what happens when we execute from DTCM? nocash makes it look like we get 0xFFFFFFFF but i can't seem to verify it
		// historically, desmume would fall through to its old memory map struct
		// which would return unused memory (0)
		// it seems the hardware returns 0 or something benign because in actuality 0xFFFFFFFF is an undefined opcode
		// and we know our handling for that is solid

		goto dunno;
	}

	// special handling for execution from arm7. try reading from main memory first
	if (PROCNUM == ARMCPU_ARM7 && (addr & 0x0F000000) == 0x02000000)
		return T1ReadLong_guaranteedAligned(st->MMU.MAIN_MEM, addr & st->_MMU_MAIN_MEM_MASK32);

	// for other arm9 cases, we have to check from dtcm first because it is patched on top of the main memory range
	if (PROCNUM == ARMCPU_ARM9)
	{
		if ((addr & ~0x3FFF) == st->MMU.DTCMRegion)
			// Returns data from DTCM (ARM9 only)
			return T1ReadLong_guaranteedAligned(st->MMU.ARM9_DTCM, addr & 0x3FFC);

		if ((addr & 0x0F000000) == 0x02000000)
			return T1ReadLong_guaranteedAligned(st->MMU.MAIN_MEM, addr & st->_MMU_MAIN_MEM_MASK32);
	}

dunno:
	if (PROCNUM == ARMCPU_ARM9)
		return _MMU_ARM9_read32(st, addr);
	else
		return _MMU_ARM7_read32(st, addr);
}

void _MMU_write08(vio2sf_state *st, int PROCNUM, MMU_ACCESS_TYPE AT, uint32_t addr, uint8_t val)
{
	// special handling for DMA: discard writes to TCM
	if (PROCNUM == ARMCPU_ARM9 && AT == MMU_AT_DMA)
	{
		if (addr < 0x02000000)
			return; // itcm
		if ((addr & ~0x3FFF) == st->MMU.DTCMRegion)
			return; // dtcm
	}

	if (PROCNUM == ARMCPU_ARM9 && (addr & ~0x3FFF) == st->MMU.DTCMRegion)
	{
		T1WriteByte(st->MMU.ARM9_DTCM, addr & 0x3FFF, val);
		return;
	}

	if ((addr & 0x0F000000) == 0x02000000)
	{
		T1WriteByte( st->MMU.MAIN_MEM, addr & st->_MMU_MAIN_MEM_MASK, val);
		return;
	}

	if (PROCNUM == ARMCPU_ARM9)
		_MMU_ARM9_write08(st, addr, val);
	else
		_MMU_ARM7_write08(st, addr, val);
}

void _MMU_write16(vio2sf_state *st, int PROCNUM, MMU_ACCESS_TYPE AT, uint32_t addr, uint16_t val)
{
	// special handling for DMA: discard writes to TCM
	if (PROCNUM == ARMCPU_ARM9 && AT == MMU_AT_DMA)
	{
		if (addr < 0x02000000)
			return; // itcm
		if ((addr & ~0x3FFF) == st->MMU.DTCMRegion)
			return; // dtcm
	}

	if (PROCNUM == ARMCPU_ARM9 && (addr & ~0x3FFF) == st->MMU.DTCMRegion)
	{
		T1WriteWord(st->MMU.ARM9_DTCM, addr & 0x3FFE, val);
		return;
	}

	if ((addr & 0x0F000000) == 0x02000000)
	{
		T1WriteWord( st->MMU.MAIN_MEM, addr & st->_MMU_MAIN_MEM_MASK16, val);
		return;
	}

	if (PROCNUM == ARMCPU_ARM9)
		_MMU_ARM9_write16(st, addr, val);
	else
		_MMU_ARM7_write16(st, addr, val);
}

void _MMU_write32(vio2sf_state *st, int PROCNUM, MMU_ACCESS_TYPE AT, uint32_t addr, uint32_t val)
{
	// special handling for DMA: discard writes to TCM
	if (PROCNUM == ARMCPU_ARM9 && AT == MMU_AT_DMA)
	{
		if (addr < 0x02000000)
			return; // itcm
		if ((addr & ~0x3FFF) == st->MMU.DTCMRegion)
			return; // dtcm
	}

	if (PROCNUM == ARMCPU_ARM9 && (addr & ~0x3FFF) == st->MMU.DTCMRegion)
	{
		T1WriteLong(st->MMU.ARM9_DTCM, addr & 0x3FFC, val);
		return;
	}

	if ((addr & 0x0F000000) == 0x02000000)
	{
		T1WriteLong( st->MMU.MAIN_MEM, addr & st->_MMU_MAIN_MEM_MASK32, val);
		return;
	}

	if (PROCNUM == ARMCPU_ARM9)
		_MMU_ARM9_write32(st, addr, val);
	else
		_MMU_ARM7_write32(st, addr, val);
}

template<int PROCNUM, MMU_ACCESS_TYPE AT, int READSIZE, MMU_ACCESS_DIRECTION DIRECTION, bool TIMING> uint32_t _MMU_accesstime(vio2sf_state *st, uint32_t addr, bool /*sequential*/)
{
	static const int MC = 1; // cached or tcm memory speed
	static const int M32 = PROCNUM == ARMCPU_ARM9 ? 2 : 1; // access through 32-bit bus
	static const int M16 = M32 * (READSIZE > 16 ? 2 : 1); // access through 16-bit bus
	static const int MSLW = M16 * 8; // this needs tuning

	if (PROCNUM == ARMCPU_ARM9 && AT == MMU_AT_CODE && addr < 0x02000000)
		return MC; // ITCM

#ifdef ACCOUNT_FOR_DATA_TCM_SPEED
	if (TIMING && PROCNUM == ARMCPU_ARM9 && AT == MMU_AT_DATA && (addr & ~0x3FFF) == MMU.DTCMRegion)
		return MC; // DTCM
#endif

	// for now, assume the cache is always enabled for all of main memory
	if (AT != MMU_AT_DMA && TIMING && PROCNUM == ARMCPU_ARM9 && (addr & 0x0F000000) == 0x02000000)
	{
#ifdef ENABLE_CACHE_CONTROLLER_EMULATION
		bool cached = false;
		if (AT == MMU_AT_CODE)
			cached = st->MMU_timing.arm9codeCache.Cached<DIRECTION>(addr);
		if (AT == MMU_AT_DATA)
			cached = st->MMU_timing.arm9dataCache.Cached<DIRECTION>(addr);
		if (cached)
			return MC;
		uint32_t c;
		if (sequential && AT == MMU_AT_DATA)
			c = M16; // bonus for sequential data access
		else if (DIRECTION == MMU_AD_READ)
			c = M16 * 5;
		else
			c = M16 * 2; // should be 4, but write buffer isn't emulated yet.
		if (DIRECTION == MMU_AD_READ)
			// cache miss while reading means it has to fill a whole cache line
			// by reading 32 bytes...
			c += 8 * M32 * 2;
		return c;
#elif defined(ACCOUNT_FOR_NON_SEQUENTIAL_ACCESS)
		// this is the closest approximation I could find
		// to the with-cache-controller timing
		// that doesn't do any actual caching logic.
		return sequential ? MC : M16;
#endif
	}

	static const TWaitState MMU_WAIT[] =
	{
		// ITCM, ITCM, MAIN, SWI, REG, VMEM, LCD, OAM,  ROM,  ROM,  RAM,   U,  U,  U,  U, BIOS
#define X    MC,   MC,  M16, M32, M32,  M16, M16, M32, MSLW, MSLW, MSLW, M32,M32,M32,M32,  M32,
		// duplicate it 16 times (this was somehow faster than using a mask of 0xF)
		X X X X  X X X X  X X X X  X X X X
#undef X
	};

	uint32_t c = MMU_WAIT[addr >> 24];

#ifdef ACCOUNT_FOR_NON_SEQUENTIAL_ACCESS
	if (TIMING && !sequential)
	{
		//if(c != MC || PROCNUM==ARMCPU_ARM7) // check not needed anymore because ITCM/DTCM return earlier
		{
			c += PROCNUM == ARMCPU_ARM9 ? 6 : 1;
		}
	}
#endif

	return c;
}

template<int PROCNUM, int READSIZE, MMU_ACCESS_DIRECTION DIRECTION, bool TIMING> uint32_t MMU_memAccessCycles(vio2sf_state *st, uint32_t addr)
{
	if (TIMING)
		return st->MMU_timing.armDataFetch<PROCNUM>().template Fetch<READSIZE, DIRECTION, true>(st, addr & (~((READSIZE >> 3) - 1)));
	else
		return st->MMU_timing.armDataFetch<PROCNUM>().template Fetch<READSIZE, DIRECTION, false>(st, addr & (~((READSIZE >> 3) - 1)));
}

template<int PROCNUM, int READSIZE> uint32_t MMU_codeFetchCycles(vio2sf_state *st, uint32_t addr)
{
	if (USE_TIMING(st))
		return st->MMU_timing.armCodeFetch<PROCNUM>().template Fetch<READSIZE, MMU_AD_READ, true>(st, addr & (~((READSIZE >> 3) - 1)));
	else
		return st->MMU_timing.armCodeFetch<PROCNUM>().template Fetch<READSIZE, MMU_AD_READ, false>(st, addr & (~((READSIZE >> 3) - 1)));
}

// Instantiate these
template uint32_t MMU_codeFetchCycles<0, 32>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_codeFetchCycles<1, 16>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_codeFetchCycles<1, 32>(vio2sf_state *st, uint32_t addr);

template uint32_t MMU_aluMemAccessCycles<0, 8, MMU_AD_READ>(vio2sf_state *st, uint32_t aluCycles, uint32_t addr);
template uint32_t MMU_aluMemAccessCycles<0, 8, MMU_AD_WRITE>(vio2sf_state *st, uint32_t aluCycles, uint32_t addr);
template uint32_t MMU_aluMemAccessCycles<0, 16, MMU_AD_READ>(vio2sf_state *st, uint32_t aluCycles, uint32_t addr);
template uint32_t MMU_aluMemAccessCycles<0, 16, MMU_AD_WRITE>(vio2sf_state *st, uint32_t aluCycles, uint32_t addr);
template uint32_t MMU_aluMemAccessCycles<0, 32, MMU_AD_READ>(vio2sf_state *st, uint32_t aluCycles, uint32_t addr);
template uint32_t MMU_aluMemAccessCycles<0, 32, MMU_AD_WRITE>(vio2sf_state *st, uint32_t aluCycles, uint32_t addr);

template uint32_t MMU_aluMemAccessCycles<1, 8, MMU_AD_READ>(vio2sf_state *st, uint32_t aluCycles, uint32_t addr);
template uint32_t MMU_aluMemAccessCycles<1, 8, MMU_AD_WRITE>(vio2sf_state *st, uint32_t aluCycles, uint32_t addr);
template uint32_t MMU_aluMemAccessCycles<1, 16, MMU_AD_READ>(vio2sf_state *st, uint32_t aluCycles, uint32_t addr);
template uint32_t MMU_aluMemAccessCycles<1, 16, MMU_AD_WRITE>(vio2sf_state *st, uint32_t aluCycles, uint32_t addr);
template uint32_t MMU_aluMemAccessCycles<1, 32, MMU_AD_READ>(vio2sf_state *st, uint32_t aluCycles, uint32_t addr);
template uint32_t MMU_aluMemAccessCycles<1, 32, MMU_AD_WRITE>(vio2sf_state *st, uint32_t aluCycles, uint32_t addr);

template uint32_t MMU_memAccessCycles<0, 8, MMU_AD_READ, false>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<0, 8, MMU_AD_READ, true>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<0, 8, MMU_AD_WRITE, false>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<0, 8, MMU_AD_WRITE, true>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<0, 16, MMU_AD_READ, false>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<0, 16, MMU_AD_READ, true>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<0, 16, MMU_AD_WRITE, false>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<0, 16, MMU_AD_WRITE, true>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<0, 32, MMU_AD_READ, false>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<0, 32, MMU_AD_READ, true>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<0, 32, MMU_AD_WRITE, false>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<0, 32, MMU_AD_WRITE, true>(vio2sf_state *st, uint32_t addr);

template uint32_t MMU_memAccessCycles<1, 8, MMU_AD_READ, false>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<1, 8, MMU_AD_READ, true>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<1, 8, MMU_AD_WRITE, false>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<1, 8, MMU_AD_WRITE, true>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<1, 16, MMU_AD_READ, false>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<1, 16, MMU_AD_READ, true>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<1, 16, MMU_AD_WRITE, false>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<1, 16, MMU_AD_WRITE, true>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<1, 32, MMU_AD_READ, false>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<1, 32, MMU_AD_READ, true>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<1, 32, MMU_AD_WRITE, false>(vio2sf_state *st, uint32_t addr);
template uint32_t MMU_memAccessCycles<1, 32, MMU_AD_WRITE, true>(vio2sf_state *st, uint32_t addr);
