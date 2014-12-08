/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2013-2014 Leandro Nini
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

#include "utils.h"

#include <cstdlib>

#ifdef _WIN32
#  include <windows.h>
#  include <shlobj.h>

#ifdef UNICODE
#  define _tgetenv _wgetenv
#else
#  define _tgetenv getenv
#endif

SID_STRING utils::getPath()
{
    SID_STRING returnPath;

    TCHAR szPath[MAX_PATH];

    if (SHGetFolderPath(NULL, CSIDL_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szPath)!=S_OK)
    {
        TCHAR *path = _tgetenv(TEXT("USERPROFILE"));
        if (!path)
            throw error();
        returnPath.append(path).append(TEXT("\\Application Data"));
    }
    else
    {
        returnPath.append(szPath);
    }

    return returnPath;
}

SID_STRING utils::getDataPath() { return getPath(); }

SID_STRING utils::getConfigPath() { return getPath(); }

#else

SID_STRING utils::getPath(const char* id, const char* def)
{
    SID_STRING returnPath;

    char *path = getenv(id);
    if (!path)
    {
        path = getenv("HOME");
        if (!path)
            throw error();
        returnPath.append(path).append(def);
    }
    else
        returnPath.append(path);

    return returnPath;
}

SID_STRING utils::getDataPath() { return getPath("XDG_DATA_HOME", "/.local/share"); }

SID_STRING utils::getConfigPath() { return getPath("XDG_CONFIG_HOME", "/.config"); }

#endif
