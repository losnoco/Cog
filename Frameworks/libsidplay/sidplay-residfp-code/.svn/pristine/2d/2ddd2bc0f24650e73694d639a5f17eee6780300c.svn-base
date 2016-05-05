/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2015 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000-2001 Simon White
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SIDDEFS_H
#define SIDDEFS_H

/* DLL building support on win32 hosts */
#ifndef SID_EXTERN
#  ifdef DLL_EXPORT      /* defined by libtool (if required) */
#    define SID_EXTERN __declspec(dllexport)
#  endif
#  ifdef SID_DLL_IMPORT  /* define if linking with this dll */
#    define SID_EXTERN __declspec(dllimport)
#  endif
#  ifndef SID_EXTERN     /* static linking or !_WIN32 */
#    if defined(__GNUC__) && (__GNUC__ >= 4)
#      define SID_EXTERN __attribute__ ((visibility("default")))
#    else
#      define SID_EXTERN
#    endif
#  endif
#endif

/* Deprecated attributes */
#if defined(_MSCVER)
#  define SID_DEPRECATED __declspec(deprecated)
#elif defined(__GNUC__)
#  define SID_DEPRECATED __attribute__ ((deprecated))
#else
#  define SID_DEPRECATED
#endif

/* Unused attributes */
#if defined(__GNUC__)
#  define SID_UNUSED __attribute__ ((unused))
#else
#  define SID_UNUSED
#endif

#endif /* SIDDEFS_H */
