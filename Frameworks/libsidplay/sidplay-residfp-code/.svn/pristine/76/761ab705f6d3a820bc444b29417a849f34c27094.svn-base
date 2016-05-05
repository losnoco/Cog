/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2012-2016 Leandro Nini
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

#ifndef C64CPU_H
#define C64CPU_H

#include "c64/c64env.h"
#include "CPU/mos6510.h"

#include "sidcxx11.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

namespace libsidplayfp
{

class c64cpu final : public MOS6510
{
private:
    c64env &m_env;

protected:
    uint8_t cpuRead(uint_least16_t addr) override { return m_env.cpuRead(addr); }
    void cpuWrite(uint_least16_t addr, uint8_t data) override { m_env.cpuWrite(addr, data); }

public:
    c64cpu (c64env &env) :
        MOS6510(env.scheduler()),
        m_env(env) {}

#ifdef PC64_TESTSUITE
    void loadFile(const char *file) override { m_env.loadFile(file); }
#endif
};

}

#endif // C64CPU_H

