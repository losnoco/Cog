/*
	Copyright (C) 2006 yopyop
	Copyright (C) 2006-2010 DeSmuME team

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

#include <vio2sf/armcpu.h>

const uint32_t CP15_ACCESS_WRITE = 0;
const uint32_t CP15_ACCESS_READ = 2;
const uint32_t CP15_ACCESS_EXECUTE = 4;
const uint32_t CP15_ACCESS_WRITEUSR = CP15_ACCESS_WRITE;
const uint32_t CP15_ACCESS_WRITESYS = 1;
const uint32_t CP15_ACCESS_READUSR = CP15_ACCESS_READ;
const uint32_t CP15_ACCESS_READSYS = 3;
const uint32_t CP15_ACCESS_EXECUSR = CP15_ACCESS_EXECUTE;
const uint32_t CP15_ACCESS_EXECSYS = 5;

struct armcp15_t
{
public:
	uint32_t IDCode;
	uint32_t cacheType;
	uint32_t TCMSize;
	uint32_t ctrl;
	uint32_t DCConfig;
	uint32_t ICConfig;
	uint32_t writeBuffCtrl;
	uint32_t und;
	uint32_t DaccessPerm;
	uint32_t IaccessPerm;
	uint32_t protectBaseSize0;
	uint32_t protectBaseSize1;
	uint32_t protectBaseSize2;
	uint32_t protectBaseSize3;
	uint32_t protectBaseSize4;
	uint32_t protectBaseSize5;
	uint32_t protectBaseSize6;
	uint32_t protectBaseSize7;
	uint32_t cacheOp;
	uint32_t DcacheLock;
	uint32_t IcacheLock;
	uint32_t ITCMRegion;
	uint32_t DTCMRegion;
	uint32_t processID;
	uint32_t RAM_TAG;
	uint32_t testState;
	uint32_t cacheDbg;
	/* calculated bitmasks for the regions to decide rights uppon */
	/* calculation is done in the MCR instead of on mem access for performance */
	uint32_t regionWriteMask_USR[8];
	uint32_t regionWriteMask_SYS[8];
	uint32_t regionReadMask_USR[8];
	uint32_t regionReadMask_SYS[8];
	uint32_t regionExecuteMask_USR[8];
	uint32_t regionExecuteMask_SYS[8];
	uint32_t regionWriteSet_USR[8];
	uint32_t regionWriteSet_SYS[8];
	uint32_t regionReadSet_USR[8];
	uint32_t regionReadSet_SYS[8];
	uint32_t regionExecuteSet_USR[8];
	uint32_t regionExecuteSet_SYS[8];

	armcpu_t *cpu;

	void setSingleRegionAccess(uint32_t dAccess, uint32_t iAccess, unsigned char num, uint32_t mask, uint32_t set);
	void maskPrecalc();

public:
	armcp15_t() : IDCode(0), cacheType(0), TCMSize(0), ctrl(0), DCConfig(0), ICConfig(0), writeBuffCtrl(0), und(0), DaccessPerm(0), IaccessPerm(0), protectBaseSize0(0), protectBaseSize1(0), protectBaseSize2(0),
		protectBaseSize3(0), protectBaseSize4(0), protectBaseSize5(0), protectBaseSize6(0), protectBaseSize7(0), cacheOp(0), DcacheLock(0), IcacheLock(0), ITCMRegion(0), DTCMRegion(0), processID(0), RAM_TAG(0),
		testState(0), cacheDbg(0), cpu(nullptr)
	{
		memset(&this->regionWriteMask_USR[0], 0, sizeof(this->regionWriteMask_USR));
		memset(&this->regionWriteMask_SYS[0], 0, sizeof(this->regionWriteMask_SYS));
		memset(&this->regionReadMask_USR[0], 0, sizeof(this->regionReadMask_USR));
		memset(&this->regionReadMask_SYS[0], 0, sizeof(this->regionReadMask_SYS));
		memset(&this->regionExecuteMask_USR[0], 0, sizeof(this->regionExecuteMask_USR));
		memset(&this->regionExecuteMask_SYS[0], 0, sizeof(this->regionExecuteMask_SYS));
		memset(&this->regionWriteSet_USR[0], 0, sizeof(this->regionWriteSet_USR));
		memset(&this->regionWriteSet_SYS[0], 0, sizeof(this->regionWriteSet_SYS));
		memset(&this->regionReadSet_USR[0], 0, sizeof(this->regionReadSet_USR));
		memset(&this->regionReadSet_SYS[0], 0, sizeof(this->regionReadSet_SYS));
		memset(&this->regionExecuteSet_USR[0], 0, sizeof(this->regionExecuteSet_USR));
		memset(&this->regionExecuteSet_SYS[0], 0, sizeof(this->regionExecuteSet_SYS));
	}
	bool reset(armcpu_t *c);
	bool dataProcess(uint8_t CRd, uint8_t CRn, uint8_t CRm, uint8_t opcode1, uint8_t opcode2);
	bool load(uint8_t CRd, uint8_t adr);
	bool store(uint8_t CRd, uint8_t adr);
	bool moveCP2ARM(uint32_t *R, uint8_t CRn, uint8_t CRm, uint8_t opcode1, uint8_t opcode2);
	bool moveARM2CP(uint32_t val, uint8_t CRn, uint8_t CRm, uint8_t opcode1, uint8_t opcode2);
	bool isAccessAllowed(uint32_t address,uint32_t access);
};

void maskPrecalc(vio2sf_state *st);
