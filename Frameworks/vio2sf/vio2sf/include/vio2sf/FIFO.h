/*
	Copyright 2006 yopyop
	Copyright 2007 shash
	Copyright 2007-2011 DeSmuME team

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

#include <vio2sf/types.h>

struct vio2sf_state;

//=================================================== IPC FIFO
struct IPC_FIFO
{
	uint32_t buf[16];

	uint8_t head;
	uint8_t tail;
	uint8_t size;
};

extern void IPC_FIFOinit(vio2sf_state *st, uint8_t proc);
extern void IPC_FIFOsend(vio2sf_state *st, uint8_t proc, uint32_t val);
extern uint32_t IPC_FIFOrecv(vio2sf_state *st, uint8_t proc);
extern void IPC_FIFOcnt(vio2sf_state *st, uint8_t proc, uint16_t val);
