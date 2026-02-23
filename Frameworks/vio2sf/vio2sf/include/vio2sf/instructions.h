/*	Copyright (C) 2006 yopyop
	Copyright (C) 2012 DeSmuME team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

struct armcpu_t;

typedef uint32_t (FASTCALL *OpFunc)(armcpu_t *, uint32_t i);
extern const OpFunc arm_instructions_set[2][4096];
extern const char *arm_instruction_names[4096];
extern const OpFunc thumb_instructions_set[2][1024];
extern const char *thumb_instruction_names[1024];
