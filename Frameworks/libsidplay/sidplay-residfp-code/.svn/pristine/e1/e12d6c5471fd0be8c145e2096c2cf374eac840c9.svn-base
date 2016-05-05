/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright 2013-2014 Leandro Nini
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

#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#if defined(HAVE_STRCASECMP) || defined (HAVE_STRNCASECMP)
#  include <strings.h>
#endif

#if defined(HAVE_STRICMP) || defined (HAVE_STRNICMP)
#  include <string.h>
#endif

#include <cctype>
#include <algorithm>
#include <string>


namespace stringutils
{
    /**
     * Compare two characters in a case insensitive way.
     */
    inline bool casecompare(char c1, char c2) { return (tolower(c1) == tolower(c2)); }

    /**
     * Compare two strings in a case insensitive way. 
     *
     * @return true if strings are equal.
     */
    inline bool equal(const std::string& s1, const std::string& s2)
    {
        return s1.size() == s2.size()
            && std::equal(s1.begin(), s1.end(), s2.begin(), casecompare);
    }

    /**
     * Compare two strings in a case insensitive way.
     *
     * @return true if strings are equal.
     */
    inline bool equal(const char* s1, const char* s2)
    {
#if defined(HAVE_STRCASECMP)
        return strcasecmp(s1, s2) == 0;
#elif defined(HAVE_STRICMP)
        return stricmp(s1, s2) == 0;
#else
        if (s1 == s2)
            return true;

        if (s1 == 0 || s2 == 0)
            return false;

        while ((*s1 != '\0') || (*s2 != '\0'))
        {
            if (!casecompare(*s1, *s2))
                return false;
            ++s1;
            ++s2;
        }

        return true;
#endif
    }

    /**
     * Compare first n characters of two strings in a case insensitive way.
     *
     * @return true if strings are equal.
     */
    inline bool equal(const char* s1, const char* s2, size_t n)
    {
#if defined(HAVE_STRNCASECMP)
        return strncasecmp(s1, s2, n) == 0;
#elif defined(HAVE_STRNICMP)
        return strnicmp(s1, s2, n) == 0;
#else
        if (s1 == s2 || n == 0)
            return true;

        if (s1 == 0 || s2 == 0)
            return false;

        while (n-- && ((*s1 != '\0') || (*s2 != '\0')))
        {
            if (!casecompare(*s1, *s2))
                return false;
            ++s1;
            ++s2;
        }

        return true;
#endif
    }
}

#endif
