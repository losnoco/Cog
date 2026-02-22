/*
	Copyright (C) 2008-2009 DeSmuME team

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

#include <iostream>
#include <vector>
#include <cstdio>
#include <vio2sf/types.h>
#include <vio2sf/emufile.h>

int read8le(uint8_t *Bufo, EMUFILE *is);
int read16le(uint16_t *Bufo, EMUFILE *is);
inline int read16le(int16_t *Bufo, EMUFILE *is) { return read16le(reinterpret_cast<uint16_t *>(Bufo), is); }
int read32le(uint32_t *Bufo, EMUFILE *is);
inline int read32le(int32_t *Bufo, EMUFILE *is) { return read32le(reinterpret_cast<uint32_t *>(Bufo), is); }
int read64le(uint64_t *Bufo, EMUFILE *is);
inline int read_double_le(double *Bufo, EMUFILE *is)
{
	uint64_t temp;
	int ret = read64le(&temp,is);
	*Bufo = u64_to_double(temp);
	return ret;
}
int read16le(uint16_t *Bufo, std::istream *is);

int readbool(bool *b, EMUFILE *is);

int readbuffer(std::vector<uint8_t> &vec, EMUFILE *is);
