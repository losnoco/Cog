/*
	Copyright (C) 2010-2011 DeSmuME team

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

#include "vio2sf.h"

extern SLOT1INTERFACE slot1Retail;

//SLOT1INTERFACE slot1_device = slot1Retail; // default for frontends that dont even configure this

void init_slot1(vio2sf_state *st)
{
	st->slot1_device = slot1Retail;
}