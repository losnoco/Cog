/*
 *  Copyright (C) 2014 Leandro Nini
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef SID_FSTREAM_H
#define SID_FSTREAM_H

#include <iostream>

#if defined(_WIN32) && defined (UNICODE)

#  include <windows.h>
#  include <io.h>
#  include <fcntl.h>

#  ifdef __GLIBCXX__
#    include <ext/stdio_filebuf.h>

static int getOflag(std::ios_base::openmode mode)
{
    int flags = (mode & std::ios_base::binary) ? _O_BINARY : 0;

    if ((mode & std::ios_base::in) && (mode & std::ios_base::out))
            flags |= _O_CREAT | _O_TRUNC | _O_RDWR;
    else if (mode & std::ios_base::in)
            flags |= _O_RDONLY;
    else if (mode & std::ios_base::out)
            flags |= _O_CREAT | _O_TRUNC | _O_WRONLY;

    return flags;
}

template<typename T>
class sid_stream_base
{
protected:
    __gnu_cxx::stdio_filebuf<T> filebuf;

    sid_stream_base(int fd, std::ios_base::openmode mode) :
        filebuf(fd, mode)
    {}

public:
    bool is_open() { return filebuf.is_open(); }

    void close() { filebuf.close(); }
};

class sid_ifstream : public sid_stream_base<char>, public std::istream
{
public:
    sid_ifstream(const TCHAR* filename, ios_base::openmode mode = ios_base::in) :
        sid_stream_base(_wopen(filename, getOflag(mode|ios_base::in)), mode|ios_base::in),
        std::istream(&filebuf)
    {}
};

class sid_ofstream : public sid_stream_base<char>, public std::ostream
{
public:
    sid_ofstream(const TCHAR* filename, ios_base::openmode mode = ios_base::out) :
        sid_stream_base(_wopen(filename, getOflag(mode|ios_base::out)), mode|ios_base::out),
        std::ostream(&filebuf)
    {}
};

class sid_wifstream : public sid_stream_base<TCHAR>, public std::wistream
{
public:
    sid_wifstream(const TCHAR* filename, ios_base::openmode mode = ios_base::in) :
        sid_stream_base(_wopen(filename, getOflag(mode|ios_base::in)), mode|ios_base::in),
        std::wistream(&filebuf)
    {}
};

class sid_wofstream : public sid_stream_base<TCHAR>, public std::wostream
{
public:
    sid_wofstream(const TCHAR* filename, ios_base::openmode mode = ios_base::out) :
        sid_stream_base(_wopen(filename, getOflag(mode|ios_base::out)), mode|ios_base::out),
        std::wostream(&filebuf)
    {}
};

#  else // _MSC_VER
#    define sid_wifstream std::wifstream
#    define sid_wofstream std::wofstream
#    define sid_ifstream std::ifstream
#    define sid_ofstream std::ofstream
#  endif // __GLIBCXX__

#else
#  define sid_wifstream std::ifstream
#  define sid_wofstream std::ofstream
#  define sid_ifstream std::ifstream
#  define sid_ofstream std::ofstream
#endif // _WIN32

#endif // SID_FSTREAM_H
