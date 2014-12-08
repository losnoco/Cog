/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2014 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef PSIDDRV_H
#define PSIDDRV_H

#include <stdint.h>

class SidTuneInfo;
class sidmemory;

namespace libsidplayfp
{

class psiddrv
{
private:
    const SidTuneInfo *m_tuneInfo;
    const char *m_errorString;

    uint8_t *reloc_driver;
    int      reloc_size;

    uint_least16_t m_driverAddr;
    uint_least16_t m_driverLength;

private:
    /**
     * Get required I/O map to reach address
     *
     * @param addr a 16-bit effective address
     * @return a default bank-select value for $01
     */
    uint8_t iomap(uint_least16_t addr) const;

public:
    psiddrv(const SidTuneInfo *tuneInfo) :
        m_tuneInfo(tuneInfo) {}

    /**
     * Relocate the driver.
     */
    bool drvReloc();

    /**
     * Install the driver.
     * Must be called after the tune has been placed in memory.
     *
     * @param mem the c64 memory interface
     * @param video the PAL/NTSC switch value, 0: NTSC, 1: PAL
     */
    void install(sidmemory *mem, uint8_t video) const;

    /**
     * Get a detailed error message.
     *
     * @return a pointer to the string
     */
    const char* errorString() const { return m_errorString; }

    uint_least16_t driverAddr() const { return m_driverAddr; }
    uint_least16_t driverLength() const { return m_driverLength; }
};

}

#endif
