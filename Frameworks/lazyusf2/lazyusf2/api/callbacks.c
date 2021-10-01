/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-core - api/callbacks.c                                    *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2009 Richard Goedeken                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* This file contains the Core functions for handling callbacks to the
 * front-end application
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "usf/usf.h"

#include "usf/usf_internal.h"

#include "m64p_types.h"
#include "callbacks.h"

void DebugMessage(usf_state_t * state, int level, const char *message, ...)
{
  va_list args;
  size_t len;

  if (
#ifdef DEBUG_INFO
      1 ||
#endif
      level > 1 )
  {
#ifdef DEBUG_INFO
      if (state->debug_log)
      {
        char buffer[1024];
        va_start(args, message);
        vsprintf(buffer, message, args);
        va_end(args);
        fprintf(state->debug_log, "%s\n", buffer);
      }
      if ( level > 1 )
#endif
    return;
  }

  len = strlen( state->error_message );

  if ( len )
    state->error_message[ len++ ] = '\n';

  va_start(args, message);
#if _MSC_VER >= 1300
  vsprintf_s(state->error_message + len, 16384 - len, message, args);
#else
  vsprintf(state->error_message + len, message, args);
#endif
  va_end(args);

  state->last_error = state->error_message;
  state->stop = 1;
}
