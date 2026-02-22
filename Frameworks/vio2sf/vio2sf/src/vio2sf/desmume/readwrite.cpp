/*
	Copyright (C) 2006-2009 DeSmuME team

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

#include "readwrite.h"
#include "types.h"

int read8le(uint8_t *Bufo, EMUFILE *is)
{
	if (is->_fread(Bufo, 1) != 1)
		return 0;
	return 1;
}

int read32le(uint32_t *Bufo, EMUFILE *fp)
{
	uint32_t buf;
	if (fp->_fread(&buf, 4) < 4)
		return 0;
#ifdef LOCAL_LE
	*Bufo = buf;
#else
	*Bufo = ((buf & 0xFF) << 24) | ((buf & 0xFF00) << 8) | ((buf & 0xFF0000) >> 8) | ((buf & 0xFF000000) >> 24);
#endif
	return 1;
}

int read16le(uint16_t *Bufo, EMUFILE *is)
{
	uint16_t buf;
	if (is->_fread(&buf, 2) != 2)
		return 0;
#ifdef LOCAL_LE
	*Bufo = buf;
#else
	*Bufo = LE_TO_LOCAL_16(buf);
#endif
	return 1;
}

int read64le(uint64_t *Bufo, EMUFILE *is)
{
	uint64_t buf;
	if (is->_fread(&buf, 8) != 8)
		return 0;
#ifdef LOCAL_LE
	*Bufo = buf;
#else
	*Bufo = LE_TO_LOCAL_64(buf);
#endif
	return 1;
}

int readbool(bool *b, EMUFILE *is)
{
	uint32_t temp = 0;
	int ret = read32le(&temp, is);
	*b = !!temp;
	return ret;
}

int readbuffer(std::vector<uint8_t> &vec, EMUFILE *is)
{
	uint32_t size;
	if (read32le(&size, is) != 1)
		return 0;
	vec.resize(size);
	if (size > 0)
		is->fread(&vec[0], size);
	return 1;
}
