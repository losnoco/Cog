/*
 * openmpt123_stdio.hpp
 * --------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_STDIO_HPP
#define OPENMPT123_STDIO_HPP

#include "openmpt123_config.hpp"

#include "openmpt123_exception.hpp"

#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/string/types.hpp"

#include <cstdio>

#if MPT_OS_DJGPP
#include <fcntl.h>
#include <io.h>
#elif MPT_OS_WINDOWS
#include <fcntl.h>
#include <io.h>
#endif
#include <stdio.h>

namespace openmpt123 {

enum class FILE_mode {
	text,
	binary,
};

#if MPT_OS_DJGPP

class FILE_mode_guard {
private:
	FILE * file;
	int old_mode;
public:
	FILE_mode_guard( FILE * file, FILE_mode new_mode )
		: file(file)
		, old_mode(-1)
	{
		switch (new_mode) {
			case FILE_mode::text:
				fflush( file );
				old_mode = setmode( fileno( file ), O_TEXT );
				if ( old_mode == -1 ) {
					throw exception( MPT_USTRING("failed to set TEXT mode on file descriptor") );
				}
				break;
			case FILE_mode::binary:
				fflush( file );
				old_mode = setmode( fileno( file ), O_BINARY );
				if ( old_mode == -1 ) {
					throw exception( MPT_USTRING("failed to set binary mode on file descriptor") );
				}
				break;
		}
	}
	FILE_mode_guard( const FILE_mode_guard & ) = delete;
	FILE_mode_guard( FILE_mode_guard && ) = default;
	FILE_mode_guard & operator=( const FILE_mode_guard & ) = delete;
	FILE_mode_guard & operator=( FILE_mode_guard && ) = default;
	~FILE_mode_guard() {
		if ( old_mode != -1 ) {
			fflush( file );
			old_mode = setmode( fileno( file ), old_mode );
		}
	}
};

#elif MPT_OS_WINDOWS

class FILE_mode_guard {
private:
	FILE * file;
	int old_mode;
public:
	FILE_mode_guard( FILE * file, FILE_mode new_mode )
		: file(file)
		, old_mode(-1)
	{
		switch (new_mode) {
			case FILE_mode::text:
				fflush( file );
				#if defined(UNICODE) && MPT_LIBC_MS_AT_LEAST(MPT_LIBC_MS_VER_UCRT)
					old_mode = _setmode( _fileno( file ), _O_U8TEXT );
				#else
					old_mode = _setmode( _fileno( file ), _O_TEXT );
				#endif
				if ( old_mode == -1 ) {
					throw exception( MPT_USTRING("failed to set TEXT mode on file descriptor") );
				}
				break;
			case FILE_mode::binary:
				fflush( file );
				old_mode = _setmode( _fileno( file ), _O_BINARY );
				if ( old_mode == -1 ) {
					throw exception( MPT_USTRING("failed to set binary mode on file descriptor") );
				}
				break;
		}
	}
	FILE_mode_guard( const FILE_mode_guard & ) = delete;
	FILE_mode_guard( FILE_mode_guard && ) = default;
	FILE_mode_guard & operator=( const FILE_mode_guard & ) = delete;
	FILE_mode_guard & operator=( FILE_mode_guard && ) = default;
	~FILE_mode_guard() {
		if ( old_mode != -1 ) {
			fflush( file );
			old_mode = _setmode( _fileno( file ), old_mode );
		}
	}
};

#else

class FILE_mode_guard {
public:
	FILE_mode_guard( FILE * /* file */, FILE_mode /* new_mode */ ) {
		return;
	}
	FILE_mode_guard( const FILE_mode_guard & ) = delete;
	FILE_mode_guard( FILE_mode_guard && ) = default;
	FILE_mode_guard & operator=( const FILE_mode_guard & ) = delete;
	FILE_mode_guard & operator=( FILE_mode_guard && ) = default;
	~FILE_mode_guard() = default;
};

#endif

} // namespace openmpt123

#endif // OPENMPT123_STDIO_HPP
