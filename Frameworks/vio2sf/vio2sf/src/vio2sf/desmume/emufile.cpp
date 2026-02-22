/*
The MIT License

Copyright (C) 2009-2010 DeSmuME team

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "emufile.h"

size_t EMUFILE_MEMORY::_fread(void *ptr, size_t bytes)
{
	uint32_t remain = this->len - this->pos;
	uint32_t todo = std::min<uint32_t>(remain, bytes);
	if (!len)
	{
		this->failbit = true;
		return 0;
	}
	if (todo <= 4)
	{
		uint8_t *src = this->buf() + this->pos;
		uint8_t *dst = static_cast<uint8_t *>(ptr);
		for (uint32_t i = 0; i < todo; ++i)
			*dst++ = *src++;
	}
	else
		memcpy(ptr, this->buf() + this->pos, todo);
	this->pos += todo;
	if (todo < bytes)
		this->failbit = true;
	return todo;
}
