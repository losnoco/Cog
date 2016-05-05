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

#ifndef INIPARSER_H
#define INIPARSER_H

#include <string>
#include <map>
#include <utility>

namespace libsidplayfp
{

class iniParser
{
private:
    typedef std::map<std::string, std::string> keys_t;
    typedef std::map<std::string, keys_t> sections_t;

private:
    sections_t sections;

    sections_t::const_iterator curSection;

private:
    std::string parseSection(const std::string &buffer);

    keys_t::value_type parseKey(const std::string &buffer);

public:
    bool open(const char *fName);
    void close();

    bool setSection(const char *section);
    const char *getValue(const char *key);
};

}

#endif // INIPARSER_H
