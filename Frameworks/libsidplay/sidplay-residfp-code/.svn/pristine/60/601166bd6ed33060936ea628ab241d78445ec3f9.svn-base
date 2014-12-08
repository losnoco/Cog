/*
 *  Copyright (C) 2014 Leandro Nini
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

#ifndef DATAPARSER_H
#define DATAPARSER_H

#include <sstream>
#include <cstring>

#include "types.h"

class dataParser
{
public:
    class parseError {};

private:
    template<typename T>
    static T convertString(const TCHAR* data)
    {
        T value;

        SID_STRINGTREAM stream(data);
        stream >> std::boolalpha >> value;
        if (stream.fail()) {
            throw parseError();
        }
        return value;
    }

public:
    static double parseDouble(const TCHAR* data) { return convertString<double>(data); }
    static int parseInt(const TCHAR* data) { return convertString<int>(data); }
    static bool parseBool(const TCHAR* data) { return convertString<bool>(data); }
};

#endif // DATAPARSER_H
