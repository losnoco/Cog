/*
	Copyright (C) 2006 yopyop
	Copyright (C) 2006-2011 DeSmuME team

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

#include <cstdlib>
#include "vio2sf.h"

bool armcp15_t::reset(armcpu_t *c)
{
	//fprintf(stderr, "CP15 Reset\n");
	this->cpu = c;
	this->IDCode = 0x41059461;
	this->cacheType = 0x0F0D2112;
	this->TCMSize = 0x00140180;
	this->ctrl = 0x00012078;
	this->DCConfig = 0x0;
	this->ICConfig = 0x0;
	this->writeBuffCtrl = 0x0;
	this->und = 0x0;
	this->DaccessPerm = 0x22222222;
	this->IaccessPerm = 0x22222222;
	this->protectBaseSize0 = 0x0;
	this->protectBaseSize1 = 0x0;
	this->protectBaseSize2 = 0x0;
	this->protectBaseSize3 = 0x0;
	this->protectBaseSize4 = 0x0;
	this->protectBaseSize5 = 0x0;
	this->protectBaseSize6 = 0x0;
	this->protectBaseSize7 = 0x0;
	this->cacheOp = 0x0;
	this->DcacheLock = 0x0;
	this->IcacheLock = 0x0;
	this->ITCMRegion = 0x0C;
	this->DTCMRegion = 0x0080000A;
	this->processID = 0;

	this->cpu->st->MMU.ARM9_RW_MODE = BIT7(this->ctrl);
	this->cpu->intVector = 0xFFFF0000 * BIT13(this->ctrl);
	this->cpu->LDTBit = !BIT15(this->ctrl); // TBit

	/* preset calculated regionmasks */
	for (uint8_t i = 0; i < 8; ++i)
	{
		this->regionWriteMask_USR[i] = 0;
		this->regionWriteMask_SYS[i] = 0;
		this->regionReadMask_USR[i] = 0;
		this->regionReadMask_SYS[i] = 0;
		this->regionExecuteMask_USR[i] = 0;
		this->regionExecuteMask_SYS[i] = 0;
		this->regionWriteSet_USR[i] = 0;
		this->regionWriteSet_SYS[i] = 0;
		this->regionReadSet_USR[i] = 0;
		this->regionReadSet_SYS[i] = 0;
		this->regionExecuteSet_USR[i] = 0;
		this->regionExecuteSet_SYS[i] = 0;
	}

	return true;
}

static inline uint32_t ACCESSTYPE(uint32_t val, unsigned char n) { return (val >> (4 * n)) & 0x0F; }
static inline uint32_t SIZEIDENTIFIER(uint32_t val) { return (val >> 1) & 0x1F; }
static inline uint32_t SIZEBINARY(uint32_t val) { return 1 << (SIZEIDENTIFIER(val) + 1); }
static inline uint32_t MASKFROMREG(uint32_t val) { return ~((SIZEBINARY(val) - 1) | 0x3F); }
static inline uint32_t SETFROMREG(uint32_t val) { return val & MASKFROMREG(val); }
/* sets the precalculated regions to mask,set for the affected accesstypes */
void armcp15_t::setSingleRegionAccess(uint32_t dAccess, uint32_t iAccess, unsigned char num, uint32_t mask, uint32_t set)
{

	switch (ACCESSTYPE(dAccess, num))
	{
		case 4: /* UNP */
		case 7: /* UNP */
		case 8: /* UNP */
		case 9: /* UNP */
		case 10: /* UNP */
		case 11: /* UNP */
		case 12: /* UNP */
		case 13: /* UNP */
		case 14: /* UNP */
		case 15: /* UNP */
		case 0: /* no access at all */
			this->regionWriteMask_USR[num] = 0;
			this->regionWriteSet_USR[num] = 0xFFFFFFFF;
			this->regionReadMask_USR[num] = 0;
			this->regionReadSet_USR[num] = 0xFFFFFFFF;
			this->regionWriteMask_SYS[num] = 0;
			this->regionWriteSet_SYS[num] = 0xFFFFFFFF;
			this->regionReadMask_SYS[num] = 0;
			this->regionReadSet_SYS[num] = 0xFFFFFFFF;
			break;
		case 1: /* no access at USR, all to sys */
			this->regionWriteMask_USR[num] = 0;
			this->regionWriteSet_USR[num] = 0xFFFFFFFF;
			this->regionReadMask_USR[num] = 0;
			this->regionReadSet_USR[num] = 0xFFFFFFFF;
			this->regionWriteMask_SYS[num] = mask;
			this->regionWriteSet_SYS[num] = set;
			this->regionReadMask_SYS[num] = mask;
			this->regionReadSet_SYS[num] = set;
			break;
		case 2: /* read at USR, all to sys */
			this->regionWriteMask_USR[num] = 0;
			this->regionWriteSet_USR[num] = 0xFFFFFFFF;
			this->regionReadMask_USR[num] = mask;
			this->regionReadSet_USR[num] = set;
			this->regionWriteMask_SYS[num] = mask;
			this->regionWriteSet_SYS[num] = set;
			this->regionReadMask_SYS[num] = mask;
			this->regionReadSet_SYS[num] = set;
			break;
		case 3: /* all to USR, all to sys */
			this->regionWriteMask_USR[num] = mask;
			this->regionWriteSet_USR[num] = set;
			this->regionReadMask_USR[num] = mask;
			this->regionReadSet_USR[num] = set;
			this->regionWriteMask_SYS[num] = mask;
			this->regionWriteSet_SYS[num] = set;
			this->regionReadMask_SYS[num] = mask;
			this->regionReadSet_SYS[num] = set;
			break;
		case 5: /* no access at USR, read to sys */
			this->regionWriteMask_USR[num] = 0;
			this->regionWriteSet_USR[num] = 0xFFFFFFFF;
			this->regionReadMask_USR[num] = 0;
			this->regionReadSet_USR[num] = 0xFFFFFFFF;
			this->regionWriteMask_SYS[num] = 0;
			this->regionWriteSet_SYS[num] = 0xFFFFFFFF;
			this->regionReadMask_SYS[num] = mask;
			this->regionReadSet_SYS[num] = set;
			break;
		case 6: /* read at USR, read to sys */
			this->regionWriteMask_USR[num] = 0;
			this->regionWriteSet_USR[num] = 0xFFFFFFFF;
			this->regionReadMask_USR[num] = mask;
			this->regionReadSet_USR[num] = set;
			this->regionWriteMask_SYS[num] = 0;
			this->regionWriteSet_SYS[num] = 0xFFFFFFFF;
			this->regionReadMask_SYS[num] = mask;
			this->regionReadSet_SYS[num] = set;
	}
	switch (ACCESSTYPE(iAccess, num))
	{
		case 4: /* UNP */
		case 7: /* UNP */
		case 8: /* UNP */
		case 9: /* UNP */
		case 10: /* UNP */
		case 11: /* UNP */
		case 12: /* UNP */
		case 13: /* UNP */
		case 14: /* UNP */
		case 15: /* UNP */
		case 0: /* no access at all */
			this->regionExecuteMask_USR[num] = 0;
			this->regionExecuteSet_USR[num] = 0xFFFFFFFF;
			this->regionExecuteMask_SYS[num] = 0;
			this->regionExecuteSet_SYS[num] = 0xFFFFFFFF;
			break;
		case 1:
			this->regionExecuteMask_USR[num] = 0;
			this->regionExecuteSet_USR[num] = 0xFFFFFFFF;
			this->regionExecuteMask_SYS[num] = mask;
			this->regionExecuteSet_SYS[num] = set;
			break;
		case 2:
		case 3:
		case 6:
			this->regionExecuteMask_USR[num] = mask;
			this->regionExecuteSet_USR[num] = set;
			this->regionExecuteMask_SYS[num] = mask;
			this->regionExecuteSet_SYS[num] = set;
	}
}

/* precalculate region masks/sets from cp15 register */
void armcp15_t::maskPrecalc()
{
#define precalc(num) \
{ \
	uint32_t mask = 0, set = 0xFFFFFFFF; /* (x & 0) == 0xFF..FF is allways false (disabled) */ \
	if (BIT_N(this->protectBaseSize##num, 0)) /* if region is enabled */ \
	{ \
		/* reason for this define: naming includes var */ \
		mask = MASKFROMREG(this->protectBaseSize##num); \
		set = SETFROMREG(this->protectBaseSize##num); \
		if (SIZEIDENTIFIER(this->protectBaseSize##num) == 0x1F) \
		{ \
			/* for the 4GB region, u32 suffers wraparound */ \
			mask = 0; \
			set = 0; /* (x & 0) == 0  is allways true (enabled) */ \
		} \
	} \
	this->setSingleRegionAccess(this->DaccessPerm, this->IaccessPerm, num, mask, set); \
}
	precalc(0);
	precalc(1);
	precalc(2);
	precalc(3);
	precalc(4);
	precalc(5);
	precalc(6);
	precalc(7);
#undef precalc
}

bool armcp15_t::moveCP2ARM(uint32_t *R, uint8_t CRn, uint8_t CRm, uint8_t opcode1, uint8_t opcode2)
{
	if (!this->cpu)
	{
		fprintf(stderr, "ERROR: cp15 don\'t allocated\n");
		return false;
	}
	if (this->cpu->CPSR.bits.mode == USR)
		return false;

	switch (CRn)
	{
		case 0:
			if (!opcode1 && !CRm)
			{
				switch (opcode2)
				{
					case 1:
						*R = this->cacheType;
						return true;
					case 2:
						*R = this->TCMSize;
						return true;
					default:
						*R = this->IDCode;
						return true;
				}
			}
			return false;
		case 1:
			if (!opcode1 && !opcode2 && !CRm)
			{
				*R = this->ctrl;
				return true;
			}
			return false;
		case 2:
			if (!opcode1 && !CRm)
			{
				switch (opcode2)
				{
					case 0:
						*R = this->DCConfig;
						return true;
					case 1:
						*R = this->ICConfig;
						return true;
					default:
						return false;
				}
			}
			return false;
		case 3:
			if (!opcode1 && ~static_cast<int8_t>(opcode2) && !CRm)
			{
				*R = this->writeBuffCtrl;
				return true;
			}
			return false;
		case 5:
			if (!opcode1 && !CRm)
			{
				switch (opcode2)
				{
					case 2:
						*R = this->DaccessPerm;
						return true;
					case 3:
						*R = this->IaccessPerm;
						return true;
					default:
						return false;
				}
			}
			return false;
		case 6:
			if (!opcode1 && !opcode2)
			{
				switch (CRm)
				{
					case 0:
						*R = this->protectBaseSize0;
						return true;
					case 1:
						*R = this->protectBaseSize1;
						return true;
					case 2:
						*R = this->protectBaseSize2;
						return true;
					case 3:
						*R = this->protectBaseSize3;
						return true;
					case 4:
						*R = this->protectBaseSize4;
						return true;
					case 5:
						*R = this->protectBaseSize5;
						return true;
					case 6:
						*R = this->protectBaseSize6;
						return true;
					case 7:
						*R = this->protectBaseSize7;
						return true;
					default:
						return false;
				}
			}
			return false;
		case 9:
			if (!opcode1)
			{
				switch (CRm)
				{
					case 0:
						switch (opcode2)
						{
							case 0:
								*R = this->DcacheLock;
								return true;
							case 1:
								*R = this->IcacheLock;
								return true;
							default:
								return false;
						}
					case 1:
						switch (opcode2)
						{
							case 0:
								*R = this->DTCMRegion;
								return true;
							case 1:
								*R = this->ITCMRegion;
								return true;
							default:
								return false;
						}
				}
			}
			return false;
		default:
			return false;
	}
}

bool armcp15_t::moveARM2CP(uint32_t val, uint8_t CRn, uint8_t CRm, uint8_t opcode1, uint8_t opcode2)
{
	if (!this->cpu)
	{
		fprintf(stderr, "ERROR: cp15 don\'t allocated\n");
		return false;
	}
	if (this->cpu->CPSR.bits.mode == USR)
		return false;

	switch (CRn)
	{
		case 1:
			if (!opcode1 && !opcode2 && !CRm)
			{
				// On the NDS bit0,2,7,12..19 are R/W, Bit3..6 are always set, all other bits are always zero.
				this->ctrl = (val & 0x000FF085) | 0x00000078;
				this->cpu->st->MMU.ARM9_RW_MODE = BIT7(val);
				// zero 31-jan-2010: change from 0x0FFF0000 to 0xFFFF0000 per gbatek
				this->cpu->intVector = 0xFFFF0000 * BIT13(val);
				this->cpu->LDTBit = !BIT15(val); // TBit
				return true;
			}
			return false;
		case 2:
			if (!opcode1 && !CRm)
			{
				switch (opcode2)
				{
					case 0:
						this->DCConfig = val;
						return true;
					case 1:
						this->ICConfig = val;
						return true;
					default:
						return false;
				}
			}
			return false;
		case 3:
			if (!opcode1 && !opcode2 && !CRm)
			{
				this->writeBuffCtrl = val;
				return true;
			}
			return false;
		case 5:
			if (!opcode1 && !CRm)
			{
				switch (opcode2)
				{
					case 2:
						this->DaccessPerm = val;
						this->maskPrecalc();
						return true;
					case 3:
						this->IaccessPerm = val;
						this->maskPrecalc();
						return true;
					default:
						return false;
				}
			}
			return false;
		case 6:
			if (!opcode1 && !opcode2)
			{
				switch (CRm)
				{
					case 0:
						this->protectBaseSize0 = val;
						this->maskPrecalc();
						return true;
					case 1:
						this->protectBaseSize1 = val;
						this->maskPrecalc();
						return true;
					case 2:
						this->protectBaseSize2 = val;
						this->maskPrecalc();
						return true;
					case 3:
						this->protectBaseSize3 = val;
						this->maskPrecalc();
						return true;
					case 4:
						this->protectBaseSize4 = val;
						this->maskPrecalc();
						return true;
					case 5:
						this->protectBaseSize5 = val;
						this->maskPrecalc();
						return true;
					case 6:
						this->protectBaseSize6 = val;
						this->maskPrecalc();
						return true;
					case 7:
						this->protectBaseSize7 = val;
						this->maskPrecalc();
						return true;
					default:
						return false;
				}
			}
			return false;
		case 7:
			if (!CRm && !opcode1 && opcode2 == 4)
			{
				this->cpu->waitIRQ = true;
				this->cpu->halt_IE_and_IF = true;
				// IME set deliberately omitted: only SWI sets IME to 1
				return true;
			}
			return false;
		case 9:
			if (!opcode1)
			{
				switch (CRm)
				{
					case 0:
						switch (opcode2)
						{
							case 0:
								this->DcacheLock = val;
								return true;
							case 1:
								this->IcacheLock = val;
								return true;
							default:
								return false;
						}
					case 1:
						switch (opcode2)
						{
							case 0:
								this->cpu->st->MMU.DTCMRegion = this->DTCMRegion = val & 0x0FFFF000;
								return true;
							case 1:
								this->ITCMRegion = val;
								// ITCM base is not writeable!
								this->cpu->st->MMU.ITCMRegion = 0;
								return true;
							default:
								return false;
						}
				}
			}
			return false;
		default:
			return false;
	}
}

/* precalculate region masks/sets from cp15 register ----- JIT */
#define cp15 (st->cp15)
void maskPrecalc(vio2sf_state *st)
{
#define precalc(num) \
{ \
	uint32_t mask = 0, set = 0xFFFFFFFF; /* (x & 0) == 0xFF..FF is allways false (disabled) */ \
	if (BIT_N(cp15.protectBaseSize##num, 0)) /* if region is enabled */ \
	{ \
		/* reason for this define: naming includes var */ \
		mask = MASKFROMREG(cp15.protectBaseSize##num); \
		set = SETFROMREG(cp15.protectBaseSize##num); \
		if (SIZEIDENTIFIER(cp15.protectBaseSize##num) == 0x1F) \
		{ \
			/* for the 4GB region, u32 suffers wraparound */ \
			mask = 0; \
			set = 0; /* (x & 0) == 0  is allways true (enabled) */ \
		} \
	} \
	cp15.setSingleRegionAccess(cp15.DaccessPerm, cp15.IaccessPerm, num, mask, set); \
}
	precalc(0);
	precalc(1);
	precalc(2);
	precalc(3);
	precalc(4);
	precalc(5);
	precalc(6);
	precalc(7);
#undef precalc
}
#undef cp15
