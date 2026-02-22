/*
	Copyright 2006 yopyop
	Copyright 2007 shash
	Copyright 2007-2012 DeSmuME team

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

#include <cstring>
#include "vio2sf.h"

// ========================================================= IPC FIFO
void IPC_FIFOinit(vio2sf_state *st, uint8_t proc)
{
	memset(&st->ipc_fifo[proc], 0, sizeof(IPC_FIFO));
	T1WriteWord(st->MMU.MMU_MEM[proc][0x40], 0x184, 0x00000101);
}

void IPC_FIFOsend(vio2sf_state *st, uint8_t proc, uint32_t val)
{
	uint16_t cnt_l = T1ReadWord(st->MMU.MMU_MEM[proc][0x40], 0x184);
	if (!(cnt_l & IPCFIFOCNT_FIFOENABLE))
		return; // FIFO disabled
	uint8_t proc_remote = proc ^ 1;

	if (st->ipc_fifo[proc].size > 15)
	{
		cnt_l |= IPCFIFOCNT_FIFOERROR;
		T1WriteWord(st->MMU.MMU_MEM[proc][0x40], 0x184, cnt_l);
		return;
	}

	uint16_t cnt_r = T1ReadWord(st->MMU.MMU_MEM[proc_remote][0x40], 0x184);

	cnt_l &= 0xBFFC; // clear send empty bit & full
	cnt_r &= 0xBCFF; // set recv empty bit & full
	st->ipc_fifo[proc].buf[st->ipc_fifo[proc].tail] = val;
	++st->ipc_fifo[proc].tail;
	++st->ipc_fifo[proc].size;
	if (st->ipc_fifo[proc].tail > 15)
		st->ipc_fifo[proc].tail = 0;

	if (st->ipc_fifo[proc].size > 15)
	{
		cnt_l |= IPCFIFOCNT_SENDFULL; // set send full bit
		cnt_r |= IPCFIFOCNT_RECVFULL; // set recv full bit
	}

	T1WriteWord(st->MMU.MMU_MEM[proc][0x40], 0x184, cnt_l);
	T1WriteWord(st->MMU.MMU_MEM[proc_remote][0x40], 0x184, cnt_r);

	if (cnt_r & IPCFIFOCNT_RECVIRQEN)
		NDS_makeIrq(st, proc_remote, IRQ_BIT_IPCFIFO_RECVNONEMPTY);

	NDS_Reschedule(st);
}

uint32_t IPC_FIFOrecv(vio2sf_state *st, uint8_t proc)
{
	uint16_t cnt_l = T1ReadWord(st->MMU.MMU_MEM[proc][0x40], 0x184);
	if (!(cnt_l & IPCFIFOCNT_FIFOENABLE))
		return 0; // FIFO disabled
	uint8_t proc_remote = proc ^ 1;

	uint32_t val = 0;

	if (!st->ipc_fifo[proc_remote].size) // remote FIFO error
	{
		cnt_l |= IPCFIFOCNT_FIFOERROR;
		T1WriteWord(st->MMU.MMU_MEM[proc][0x40], 0x184, cnt_l);
		return 0;
	}

	uint16_t cnt_r = T1ReadWord(st->MMU.MMU_MEM[proc_remote][0x40], 0x184);

	cnt_l &= 0xBCFF; // clear send full bit & empty
	cnt_r &= 0xBFFC; // set recv full bit & empty

	val = st->ipc_fifo[proc_remote].buf[st->ipc_fifo[proc_remote].head];
	++st->ipc_fifo[proc_remote].head;
	--st->ipc_fifo[proc_remote].size;
	if (st->ipc_fifo[proc_remote].head > 15)
		st->ipc_fifo[proc_remote].head = 0;

	if (!st->ipc_fifo[proc_remote].size) // FIFO empty
	{
		cnt_l |= IPCFIFOCNT_RECVEMPTY;
		cnt_r |= IPCFIFOCNT_SENDEMPTY;

		if (cnt_r & IPCFIFOCNT_SENDIRQEN)
			NDS_makeIrq(st, proc_remote, IRQ_BIT_IPCFIFO_SENDEMPTY);
	}

	T1WriteWord(st->MMU.MMU_MEM[proc][0x40], 0x184, cnt_l);
	T1WriteWord(st->MMU.MMU_MEM[proc_remote][0x40], 0x184, cnt_r);

	NDS_Reschedule(st);

	return val;
}

void IPC_FIFOcnt(vio2sf_state *st, uint8_t proc, uint16_t val)
{
	uint16_t cnt_l = T1ReadWord(st->MMU.MMU_MEM[proc][0x40], 0x184);
	uint16_t cnt_r = T1ReadWord(st->MMU.MMU_MEM[proc^1][0x40], 0x184);

	if (val & IPCFIFOCNT_FIFOERROR)
		// at least SPP uses this, maybe every retail game
		cnt_l &= ~IPCFIFOCNT_FIFOERROR;

	if (val & IPCFIFOCNT_SENDCLEAR)
	{
		st->ipc_fifo[proc].head = 0;
		st->ipc_fifo[proc].tail = 0;
		st->ipc_fifo[proc].size = 0;

		cnt_l |= IPCFIFOCNT_SENDEMPTY;
		cnt_r |= IPCFIFOCNT_RECVEMPTY;

		cnt_l &= ~IPCFIFOCNT_SENDFULL;
		cnt_r &= ~IPCFIFOCNT_RECVFULL;
	}

	cnt_l &= ~IPCFIFOCNT_WRITEABLE;
	cnt_l |= val & IPCFIFOCNT_WRITEABLE;

	// IPCFIFOCNT_SENDIRQEN may have been set (and/or the fifo may have been cleared) so we may need to trigger this irq
	// (this approach is used by libnds fifo system on occasion in fifoInternalSend, and began happening frequently for value32 with r4326)
	if ((cnt_l & IPCFIFOCNT_SENDIRQEN) && (cnt_l & IPCFIFOCNT_SENDEMPTY))
		NDS_makeIrq(st, proc, IRQ_BIT_IPCFIFO_SENDEMPTY);

	// IPCFIFOCNT_RECVIRQEN may have been set so we may need to trigger this irq
	if ((cnt_l & IPCFIFOCNT_RECVIRQEN) && !(cnt_l & IPCFIFOCNT_RECVEMPTY))
		NDS_makeIrq(st, proc, IRQ_BIT_IPCFIFO_RECVNONEMPTY);

	T1WriteWord(st->MMU.MMU_MEM[proc][0x40], 0x184, cnt_l);
	T1WriteWord(st->MMU.MMU_MEM[proc^1][0x40], 0x184, cnt_r);

	NDS_Reschedule(st);
}
