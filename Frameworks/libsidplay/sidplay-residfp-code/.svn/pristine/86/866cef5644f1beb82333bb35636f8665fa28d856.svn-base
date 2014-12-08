/*
 *  Copyright (C) 2010-2014 Leandro Nini
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

#include "iniHandler.h"

#include <cstdlib>

#include <fstream>
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#  include <windows.h>
#endif

iniHandler::iniHandler() :
    changed(false)
{}

iniHandler::~iniHandler()
{
    close();
}

SID_STRING iniHandler::parseSection(const SID_STRING &buffer)
{
    const size_t pos = buffer.find(']');

    if (pos == SID_STRING::npos)
    {
        throw parseError();
    }

    return buffer.substr(1, pos-1);
}

iniHandler::stringPair_t iniHandler::parseKey(const SID_STRING &buffer)
{
    const size_t pos = buffer.find('=');

    if (pos == SID_STRING::npos)
    {
        throw parseError();
    }

    const SID_STRING key = buffer.substr(0, buffer.find_last_not_of(' ', pos-1) + 1);
    const size_t vpos = buffer.find_first_not_of(' ', pos+1);
    const SID_STRING value = (vpos == SID_STRING::npos) ? TEXT("") : buffer.substr(vpos);
    return make_pair(key, value);
}

bool iniHandler::open(const TCHAR *fName)
{
    fileName.assign(fName);

    SID_WIFSTREAM iniFile(fName);

    if (!iniFile.is_open())
    {
        // Try creating new file
#ifdef _WIN32
        const HANDLE h = CreateFile(fName, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h != INVALID_HANDLE_VALUE)
        {
            CloseHandle(h);
            return true;
        }
#else
        SID_WOFSTREAM newIniFile(fName);
        return newIniFile.is_open();
#endif
        return false;
    }

    SID_STRING buffer;

    while (getline(iniFile, buffer))
    {
        if (buffer.empty())
            continue;

        switch (buffer.at(0))
        {
        case ';':
        case '#':
            // Comments
            if (!sections.empty())
            {
                sections_t::reference lastSect(sections.back());
                lastSect.second.push_back(make_pair(SID_STRING(), buffer));
            }
            break;

        case '[':
            try
            {
                const SID_STRING section = parseSection(buffer);
                const keys_t keys;
                sections.push_back(make_pair(section, keys));
            }
            catch (parseError const &e) {}

            break;

        default:
            try
            {
                if (!sections.empty()) //FIXME add a default section?
                {
                    sections_t::reference lastSect(sections.back());
                    lastSect.second.push_back(parseKey(buffer));
                }
            }
            catch (parseError const &e) {}

            break;
        }
    }

    return !iniFile.bad();
}

void iniHandler::close()
{
    if (changed)
    {
        write(fileName.c_str());
    }

    sections.clear();
    changed = false;
}

bool iniHandler::setSection(const TCHAR *section)
{
    curSection = std::find_if(sections.begin(), sections.end(), compare<keyPair_t>(section));
    return (curSection != sections.end());
}

const TCHAR *iniHandler::getValue(const TCHAR *key) const
{
    keys_t::const_iterator keyIt = std::find_if((*curSection).second.begin(), (*curSection).second.end(), compare<stringPair_t>(key));
    return (keyIt != (*curSection).second.end()) ? keyIt->second.c_str() : 0;
}

void iniHandler::addSection(const TCHAR *section)
{
    const keys_t keys;
    curSection = sections.insert(curSection, make_pair(section, keys));
    changed = true;
}

void iniHandler::addValue(const TCHAR *key, const TCHAR *value)
{
    (*curSection).second.push_back(make_pair(SID_STRING(key), SID_STRING(value)));
    changed = true;
}

bool iniHandler::write(const TCHAR *fName)
{
    SID_WOFSTREAM iniFile(fName);

#ifdef _WIN32
    // On Windows XP it seems that opening an ofstream sets the read-only attribute
    SetFileAttributes(fName, GetFileAttributes(fName) & ~FILE_ATTRIBUTE_READONLY);
#endif

    if (!iniFile.is_open())
    {
        return false;
    }

    for (sections_t::iterator section = sections.begin(); section != sections.end(); ++section)
    {
        iniFile << "[" << (*section).first << "]" << std::endl;

        for (keys_t::iterator entry = (*section).second.begin(); entry != (*section).second.end(); ++entry)
        {
            const SID_STRING key = (*entry).first;
            if (!key.empty())
                iniFile << key << " = ";
            iniFile << (*entry).second << std::endl;
        }
        iniFile << std::endl;
    }

    return true;
}
