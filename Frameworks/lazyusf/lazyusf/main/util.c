/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - util.c                                                  *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2012 CasualJames                                        *
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

/**
 * Provides common utilities to the rest of the code:
 *  -String functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>

#include "usf/usf.h"

#include "rom.h"
#include "util.h"
#include "osal/preproc.h"

/**********************
   Byte swap utilities
 **********************/
void swap_buffer(void *buffer, size_t length, size_t count)
{
    size_t i;
    if (length == 2)
    {
        unsigned short *pun = (unsigned short *)buffer;
        for (i = 0; i < count; i++)
            pun[i] = m64p_swap16(pun[i]);
    }
    else if (length == 4)
    {
        unsigned int *pun = (unsigned int *)buffer;
        for (i = 0; i < count; i++)
            pun[i] = m64p_swap32(pun[i]);
    }
    else if (length == 8)
    {
        unsigned long long *pun = (unsigned long long *)buffer;
        for (i = 0; i < count; i++)
            pun[i] = m64p_swap64(pun[i]);
    }
}

void to_little_endian_buffer(void *buffer, size_t length, size_t count)
{
    #ifdef M64P_BIG_ENDIAN
    swap_buffer(buffer, length, count);
    #endif
}

void to_big_endian_buffer(void *buffer, size_t length, size_t count)
{
    #ifndef M64P_BIG_ENDIAN
    swap_buffer(buffer, length, count);
    #endif
}

/**********************
    String utilities
 **********************/
char *trim(char *str)
{
    char *start = str, *end = str + strlen(str);

    while (start < end && isspace(*start))
        start++;

    while (end > start && isspace(*(end-1)))
        end--;

    memmove(str, start, end - start);
    str[end - start] = '\0';

    return str;
}

int string_to_int(const char *str, int *result)
{
    char *endptr;
    long int n;
    if (*str == '\0' || isspace(*str))
        return 0;
    errno = 0;
    n = strtol(str, &endptr, 10);
    if (*endptr != '\0' || errno != 0 || n < INT_MIN || n > INT_MAX)
        return 0;
    *result = (int)n;
    return 1;
}

static unsigned char char2hex(char c)
{
    c = tolower(c);
    if(c >= '0' && c <= '9')
        return c - '0';
    else if(c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else
        return 0xFF;
}

int parse_hex(const char *str, unsigned char *output, size_t output_size)
{
    size_t i, j;
    for (i = 0; i < output_size; i++)
    {
        output[i] = 0;
        for (j = 0; j < 2; j++)
        {
            unsigned char h = char2hex(*str++);
            if (h == 0xFF)
                return 0;

            output[i] = (output[i] << 4) | h;
        }
    }

    if (*str != '\0')
        return 0;

    return 1;
}

char *formatstr(const char *fmt, ...)
{
	int size = 128, ret;
	char *str = (char *)malloc(size), *newstr;
	va_list args;

	/* There are two implementations of vsnprintf we have to deal with:
	 * C99 version: Returns the number of characters which would have been written
	 *              if the buffer had been large enough, and -1 on failure.
	 * Windows version: Returns the number of characters actually written,
	 *                  and -1 on failure or truncation.
	 * NOTE: An implementation equivalent to the Windows one appears in glibc <2.1.
	 */
	while (str != NULL)
	{
		va_start(args, fmt);
		ret = vsnprintf(str, size, fmt, args);
		va_end(args);

		// Successful result?
		if (ret >= 0 && ret < size)
			return str;

		// Increment the capacity of the buffer
		if (ret >= size)
			size = ret + 1; // C99 version: We got the needed buffer size
		else
			size *= 2; // Windows version: Keep guessing

		newstr = (char *)realloc(str, size);
		if (newstr == NULL)
			free(str);
		str = newstr;
	}

	return NULL;
}

