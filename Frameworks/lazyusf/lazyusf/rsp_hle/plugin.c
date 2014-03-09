/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - plugin.c                                        *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Bobby Smiles                                       *
 *   Copyright (C) 2009 Richard Goedeken                                   *
 *   Copyright (C) 2002 Hacktarux                                          *
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

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "../usf.h"
#include "../main.h"
#include "../usf_internal.h"

#include "hle.h"

/* Global functions needed by HLE core */
void HleVerboseMessage(void* user_defined, const char *message, ...)
{
    /* discard verbose message */
}

void HleErrorMessage(void* user_defined, const char *message, ...)
{
    usf_state_t* state;
    va_list ap;
    size_t len;

    state = (usf_state_t*)user_defined;
    len = strlen( state->error_message );

    if ( len )
        state->error_message[ len++ ] = '\n';

    va_start( ap, message );
    vsprintf( state->error_message + len, message, ap );
    va_end( ap );

    state->last_error = state->error_message;
    StopEmulation( state );
}

void HleWarnMessage(void* user_defined, const char *message, ...)
{
    usf_state_t* state;
    va_list ap;
    size_t len;
    
    state = (usf_state_t*)user_defined;
    len = strlen( state->error_message );
    
    if ( len )
        state->error_message[ len++ ] = '\n';
    
    va_start( ap, message );
    vsprintf( state->error_message + len, message, ap );
    va_end( ap );
    
    state->last_error = state->error_message;
    StopEmulation( state );
}

void HleCheckInterrupts(void* user_defined)
{
    CheckInterrupts((usf_state_t*)user_defined);
}

void HleProcessDlistList(void* user_defined)
{
    /* disabled */
}

void HleProcessAlistList(void* user_defined)
{
    /* disabled */
}

void HleProcessRdpList(void* user_defined)
{
    /* disabled */
}

void HleShowCFB(void* user_defined)
{
    /* disabled */
}
