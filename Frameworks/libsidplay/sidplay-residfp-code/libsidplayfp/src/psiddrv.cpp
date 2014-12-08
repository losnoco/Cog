/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2014 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2001 Simon White
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


// --------------------------------------------------------
// The code here is use to support the PSID Version 2NG
// (proposal B) file format for player relocation support.
// --------------------------------------------------------
#include "psiddrv.h"

#include "sidplayfp/SidTuneInfo.h"

#include "sidendian.h"
#include "sidmemory.h"
#include "reloc65.h"
#include "c64/CPU/mos6510.h"

namespace libsidplayfp
{

// Error Strings
const char ERR_PSIDDRV_NO_SPACE[]  = "ERROR: No space to install psid driver in C64 ram";
const char ERR_PSIDDRV_RELOC[]     = "ERROR: Failed whilst relocating psid driver";

uint8_t psid_driver[] = {
#  include "psiddrv.bin"
};

const uint8_t POWERON[] =
{
#  include "poweron.bin"
};


/**
 * Copy in power on settings. These were created by running
 * the kernel reset routine and storing the useful values
 * from $0000-$03ff. Format is:
 * - offset byte (bit 7 indicates presence rle byte)
 * - rle count byte (bit 7 indicates compression used)
 * - data (single byte) or quantity represented by uncompressed count
 * all counts and offsets are 1 less than they should be
 */
void copyPoweronPattern(sidmemory *mem)
{
    mem->fillRam(0, static_cast<uint8_t>(0), 0x3ff);

    uint_least16_t addr = 0;
    for (unsigned int i = 0; i < sizeof(POWERON);)
    {
        uint8_t off   = POWERON[i++];
        uint8_t count = 0;
        bool compressed = false;

        // Determine data count/compression
        if (off & 0x80)
        {
            // fixup offset
            off  &= 0x7f;
            count = POWERON[i++];
            if (count & 0x80)
            {
                // fixup count
                count &= 0x7f;
                compressed = true;
            }
        }

        // Fix count off by ones (see format details)
        count++;
        addr += off;

        // Extract compressed data
        if (compressed)
        {
            const uint8_t data = POWERON[i++];
            while (count-- > 0)
            {
                mem->writeMemByte(addr++, data);
            }
        }
        // Extract uncompressed data
        else
        {
            while (count-- > 0)
            {
                mem->writeMemByte(addr++, POWERON[i++]);
            }
        }
    }
}

uint8_t psiddrv::iomap(uint_least16_t addr) const
{
    // Force Real C64 Compatibility
    if (m_tuneInfo->compatibility() == SidTuneInfo::COMPATIBILITY_R64
        || m_tuneInfo->compatibility() == SidTuneInfo::COMPATIBILITY_BASIC
        || addr == 0)
    {
        // Special case, converted to 0x37 by the psid driver
        return 0;
    }

    if (addr < 0xa000)
        return 0x37;  // Basic-ROM, Kernal-ROM, I/O
    if (addr < 0xd000)
        return 0x36;  // Kernal-ROM, I/O
    if (addr >= 0xe000)
        return 0x35;  // I/O only

    return 0x34;  // RAM only
}

bool psiddrv::drvReloc()
{
    const int startlp = m_tuneInfo->loadAddr() >> 8;
    const int endlp   = (m_tuneInfo->loadAddr() + (m_tuneInfo->c64dataLen() - 1)) >> 8;

    uint_least8_t relocStartPage = m_tuneInfo->relocStartPage();
    uint_least8_t relocPages = m_tuneInfo->relocPages();

    if (m_tuneInfo->compatibility() == SidTuneInfo::COMPATIBILITY_BASIC)
    {
        // The psiddrv is only used for initialisation and to
        // autorun basic tunes as running the kernel falls
        // into a manual load/run mode
        relocStartPage = 0x04;
        relocPages     = 0x03;
    }

    // Check for free space in tune
    if (relocStartPage == 0xff)
        relocPages = 0;
    // Check if we need to find the reloc addr
    else if (relocStartPage == 0)
    {
        relocPages = 0;
        // find area where to dump the driver in.
        // It's only 1 block long, so any free block we can find will do.
        for (int i = 4; i < 0xd0; i ++)
        {
            if (i >= startlp && i <= endlp)
                continue;

            if (i >= 0xa0 && i <= 0xbf)
                continue;

            relocStartPage = i;
            relocPages = 1;
            break;
        }
    }

    if (relocPages < 1)
    {
        m_errorString = ERR_PSIDDRV_NO_SPACE;
        return false;
    }

    // Place psid driver into ram
    const uint_least16_t relocAddr = relocStartPage << 8;

    reloc_driver = psid_driver;
    reloc_size   = sizeof(psid_driver);

    reloc65 relocator;
    relocator.setReloc(reloc65::TEXT, relocAddr - 10);
    relocator.setExtract(reloc65::TEXT);
    if (!relocator.reloc(&reloc_driver, &reloc_size))
    {
        m_errorString = ERR_PSIDDRV_RELOC;
        return false;
    }

    // Adjust size to not included initialisation data.
    reloc_size -= 10;

    m_driverAddr   = relocAddr;
    m_driverLength = (uint_least16_t)reloc_size;
    // Round length to end of page
    m_driverLength += 0xff;
    m_driverLength &= 0xff00;

    return true;
}

void psiddrv::install(sidmemory *mem, uint8_t video) const
{
    if (m_tuneInfo->compatibility() >= SidTuneInfo::COMPATIBILITY_R64)
    {
        copyPoweronPattern(mem);
    }

    mem->writeMemByte(0x02a6, video);

    mem->installResetHook(endian_little16(reloc_driver));

    // If not a basic tune then the psiddrv must install
    // interrupt hooks and trap programs trying to restart basic
    if (m_tuneInfo->compatibility() == SidTuneInfo::COMPATIBILITY_BASIC)
    {
        // Install hook to set subtune number for basic
        mem->setBasicSubtune((uint8_t)(m_tuneInfo->currentSong() - 1));
        mem->installBasicTrap(0xbf53);
    }
    else
    {
        // Only install irq handle for RSID tunes
        mem->fillRam(0x0314, &reloc_driver[2], m_tuneInfo->compatibility() == SidTuneInfo::COMPATIBILITY_R64 ? 2 : 6);

        // Experimental restart basic trap
        const uint_least16_t addr = endian_little16(&reloc_driver[8]);
        mem->installBasicTrap(0xffe1);
        mem->writeMemWord(0x0328, addr);
    }

    int pos = m_driverAddr;

    // Install driver to ram
    mem->fillRam(pos, &reloc_driver[10], reloc_size);

    // Set song number
    mem->writeMemByte(pos, (uint8_t) (m_tuneInfo->currentSong() - 1));
    pos++;

    // Set speed flag
    mem->writeMemByte(pos, m_tuneInfo->songSpeed() == SidTuneInfo::SPEED_VBI ? 0 : 1);
    pos++;

    // Set init address
    mem->writeMemWord(pos, m_tuneInfo->compatibility() == SidTuneInfo::COMPATIBILITY_BASIC ?
                     0xbf55 : m_tuneInfo->initAddr());
    pos += 2;

    // Set play address
    mem->writeMemWord(pos, m_tuneInfo->playAddr());
    pos += 2;

    // Set init address io bank value
    mem->writeMemByte(pos, iomap(m_tuneInfo->initAddr()));
    pos++;

    // Set play address io bank value
    mem->writeMemByte(pos, iomap(m_tuneInfo->playAddr()));
    pos++;

    // Set PAL/NTSC flag
    mem->writeMemByte(pos, video);
    pos++;

    // Add the required tune speed
    switch (m_tuneInfo->clockSpeed())
    {
    case SidTuneInfo::CLOCK_PAL:
        mem->writeMemByte(pos, 1);
        break;
    case SidTuneInfo::CLOCK_NTSC:
        mem->writeMemByte(pos, 0);
        break;
    default: // UNKNOWN or ANY
        mem->writeMemByte(pos, video);
        break;
    }
    pos++;

    // Set default processor register flags on calling init
    mem->writeMemByte(pos, m_tuneInfo->compatibility() >= SidTuneInfo::COMPATIBILITY_R64 ? 0 : 1 << MOS6510::SR_INTERRUPT);
}

}
