/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2014 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include <string>
#include <iostream>
#include <fstream>
#include <memory>

#include "sidplayfp/sidplayfp.h"
#include "sidplayfp/SidTune.h"
#include "sidplayfp/sidbuilder.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

/*
 * Adjust these paths to point to existing ROM dumps
 */
#define KERNAL_PATH "/usr/lib/vice/C64/kernal"
#define BASIC_PATH "/usr/lib/vice/C64/basic"
#define CHARGEN_PATH "/usr/lib/vice/C64/chargen"

void loadRom(const char* path, char* buffer)
{
    std::ifstream is(path, std::ios::binary);
    if (!is.is_open())
    {
        std::cout << "File " << path << " not found" << std::endl;
        exit(-1);
    }
    is.read(buffer, 8192);
    is.close();
}

int main(int argc, char* argv[])
{
    sidplayfp m_engine;

    char kernal[8192];
    char basic[8192];
    char chargen[4096];

    loadRom(KERNAL_PATH, kernal);
    loadRom(BASIC_PATH, basic);
    loadRom(CHARGEN_PATH, chargen);

    m_engine.setRoms((const uint8_t*)kernal, (const uint8_t*)basic, (const uint8_t*)chargen);

    std::string name(PC64_TESTSUITE);

    if (argc > 1)
    {
        name.append(argv[1]).append(".prg");
    }
    else
    {
        name.append(" start.prg");
    }

    std::auto_ptr<SidTune> tune(new SidTune(name.c_str()));

    if (!tune->getStatus())
    {
        std::cerr << "Error: " << tune->statusString() << std::endl;
        return -1;
    }

    tune->selectSong(0);

    if (!m_engine.load(tune.get()))
    {
        std::cerr << m_engine.error() << std::endl;
        return -1;
    }

    for (;;)
    {
        m_engine.play(0, 0);
    }
}
