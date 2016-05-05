/*
 *  Copyright (C) 2010-2015 Leandro Nini
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

#include "iniParser.h"

#include "sidcxx11.h"

#include <fstream>

namespace libsidplayfp
{

class parseError {};

std::string iniParser::parseSection(const std::string &buffer)
{
    const size_t pos = buffer.find(']');

    if (pos == std::string::npos)
    {
        throw parseError();
    }

    return buffer.substr(1, pos-1);
}

iniParser::keys_t::value_type iniParser::parseKey(const std::string &buffer)
{
    const size_t pos = buffer.find('=');

    if (pos == std::string::npos)
    {
        throw parseError();
    }

    const std::string key = buffer.substr(0, buffer.find_last_not_of(' ', pos-1) + 1);
    const std::string value = buffer.substr(pos + 1);
    return make_pair(key, value);
}

bool iniParser::open(const char *fName)
{
    std::ifstream iniFile(fName);

    if (iniFile.fail())
    {
        return false;
    }

    sections_t::iterator mIt;

    while (iniFile.good())
    {
        std::string buffer;
        getline(iniFile, buffer);

        if (buffer.empty())
            continue;

        switch (buffer.at(0))
        {
        case ';':
        case '#':
            // skip comments
            break;
        case '[':
            try
            {
                const std::string section = parseSection(buffer);
                const keys_t keys;
                std::pair<sections_t::iterator, bool> it = sections.insert(make_pair(section, keys));
                mIt = it.first;
            }
            catch (parseError const &) {};
            break;
        default:
            try
            {
                (*mIt).second.insert(parseKey(buffer));
            }
            catch (parseError const &) {};
            break;
        }
    }

    return true;
}

void iniParser::close()
{
    sections.clear();
}

bool iniParser::setSection(const char *section)
{
    curSection = sections.find(std::string(section));
    return (curSection != sections.end());
}

const char *iniParser::getValue(const char *key)
{
    keys_t::const_iterator keyIt = (*curSection).second.find(std::string(key));
    return (keyIt != (*curSection).second.end()) ? keyIt->second.c_str() : nullptr;
}

}
