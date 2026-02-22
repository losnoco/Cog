/*
	Copyright (C) 2009-2012 DeSmuME team

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

#include "version.h"
#include "types.h"

#define DESMUME_NAME "DeSmuME"

#if defined(__x86_64__) || defined(__LP64) || defined(__IA64__) || defined(_M_X64) || defined(_WIN64)
# define DESMUME_PLATFORM_STRING " x64"
#elif defined(__i386__) || defined(_M_IX86) || defined(_WIN32)
# define DESMUME_PLATFORM_STRING " x86"
#elif defined(__arm__)
# define DESMUME_PLATFORM_STRING " ARM"
#elif defined(__thumb__)
# define DESMUME_PLATFORM_STRING " ARM-Thumb"
#elif defined(__ppc64__)
# define DESMUME_PLATFORM_STRING " PPC64"
#elif defined(__ppc__) || defined(_M_PPC)
# define DESMUME_PLATFORM_STRING " PPC"
#else
# define DESMUME_PLATFORM_STRING ""
#endif

#ifndef ENABLE_SSE2
# ifndef ENABLE_SSE
#  define DESMUME_CPUEXT_STRING " NOSSE"
# else
#  define DESMUME_CPUEXT_STRING " NOSSE2"
# endif
#else
# define DESMUME_CPUEXT_STRING ""
#endif

#define DESMUME_SUBVERSION_STRING " svn 4608"

#ifdef HAVE_JIT
# define DESMUME_JIT "-JIT"
#else
# define DESMUME_JIT ""
#endif

#define DESMUME_NAME_AND_VERSION DESMUME_NAME " 0.9.9" DESMUME_SUBVERSION_STRING DESMUME_PLATFORM_STRING DESMUME_JIT DESMUME_CPUEXT_STRING

const char *EMU_DESMUME_NAME_AND_VERSION() { return DESMUME_NAME_AND_VERSION; }
