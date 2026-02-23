/*
The MIT License

Copyright (C) 2009-2012 DeSmuME team

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

// don't use emufile for files bigger than 2GB! you have been warned! some day this will be fixed.

#pragma once

#include <vector>
#include <algorithm>
#include <string>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cmath>
#include <vio2sf/types.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
# include <io.h>
#else
# include <unistd.h>
#endif

class EMUFILE
{
protected:
	bool failbit;

public:
	EMUFILE() : failbit(false) { }
	virtual ~EMUFILE() { }

	bool fail(bool unset = false)
	{
		bool ret = failbit;
		if (unset)
			unfail();
		return ret;
	}
	void unfail() { failbit = false; }

	size_t fread(void *ptr, size_t bytes)
	{
		return _fread(ptr, bytes);
	}

	// virtuals
	virtual size_t _fread(void *ptr, size_t bytes) = 0;

	virtual int fseek(int offset, int origin) = 0;

	virtual size_t ftell() = 0;
	virtual size_t size() = 0;
};

// todo - handle read-only specially?
class EMUFILE_MEMORY : public EMUFILE
{
protected:
	std::vector<uint8_t> *vec;
	bool ownvec;
	int32_t pos, len;

	void reserve(uint32_t amt)
	{
		if (this->vec->size() < amt)
			this->vec->resize(amt);
	}
public:
	EMUFILE_MEMORY(std::vector<uint8_t> *underlying) : vec(underlying), ownvec(false), pos(0), len((int32_t)underlying->size()) { }
	EMUFILE_MEMORY(uint32_t preallocate) : vec(new std::vector<uint8_t>()), ownvec(true), pos(0), len(0)
	{
		this->vec->resize(preallocate);
		this->len = preallocate;
	}
	EMUFILE_MEMORY() : vec(new std::vector<uint8_t>()), ownvec(true), pos(0), len(0)
	{
		this->vec->reserve(1024);
	}
	EMUFILE_MEMORY(void *Buf, int32_t Size) : vec(new std::vector<uint8_t>()), ownvec(true), pos(0), len(Size)
	{
		this->vec->resize(Size);
		if (Size)
			memcpy(&(*this->vec)[0], Buf, Size);
	}

	~EMUFILE_MEMORY()
	{
		if (this->ownvec)
			delete this->vec;
	}

	uint8_t *buf()
	{
		if (!this->size())
			this->reserve(1);
		return &(*this->vec)[0];
	}

	virtual size_t _fread(void *ptr, size_t bytes);

	virtual int fseek(int offset, int origin)
	{
		// work differently for read-only...?
		switch (origin)
		{
			case SEEK_SET:
				this->pos = offset;
				break;
			case SEEK_CUR:
				this->pos += offset;
				break;
			case SEEK_END:
				this->pos = (int32_t) (this->size() + offset);
				break;
			default:
				assert(false);
		}
		this->reserve(pos);
		return 0;
	}

	virtual size_t ftell()
	{
		return this->pos;
	}

	virtual size_t size()
	{
		return this->len;
	}
};

class EMUFILE_FILE : public EMUFILE
{
protected:
	FILE *fp;
	std::string fname;
	char mode[16];
private:
	void open(const char *fn, const char *Mode)
	{
		this->fp = fopen(fn, Mode);
		if (!this->fp)
			this->failbit = true;
		this->fname = fn;
		strcpy(this->mode, Mode);
	}
public:
	EMUFILE_FILE(const std::string &fn, const char *Mode)
	{
		this->open(fn.c_str(), Mode);
	}

	virtual ~EMUFILE_FILE()
	{
		if (this->fp)
			fclose(this->fp);
	}

	virtual size_t _fread(void *ptr, size_t bytes)
	{
		size_t ret = ::fread(ptr, 1, bytes, this->fp);
		if (ret < bytes)
			this->failbit = true;
		return ret;
	}

	virtual int fseek(int offset, int origin)
	{
		return ::fseek(this->fp, offset, origin);
	}

	virtual size_t ftell()
	{
		return ::ftell(this->fp);
	}

	virtual size_t size()
	{
		int oldpos = (int32_t)(this->ftell());
		this->fseek(0, SEEK_END);
		int len = (int)(this->ftell());
		this->fseek(oldpos, SEEK_SET);
		return len;
	}
};
