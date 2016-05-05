/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2015 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000 Simon White
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

#ifndef MMU_H
#define MMU_H

#include <stdint.h>

#include "sidendian.h"
#include "sidmemory.h"
#include "EventScheduler.h"

#include "Banks/SystemRAMBank.h"
#include "Banks/SystemROMBanks.h"
#include "Banks/ZeroRAMBank.h"

#include "sidcxx11.h"

#include <string.h>

namespace libsidplayfp
{

class Bank;
class IOBank;

/**
 * The C64 MMU chip.
 */
class MMU final : public PLA, public sidmemory
{
private:
    EventScheduler &eventScheduler;

    /// CPU port signals
    bool loram, hiram, charen;

    /// CPU read memory mapping in 4k chunks
    Bank* cpuReadMap[16];

    /// CPU write memory mapping in 4k chunks
    Bank* cpuWriteMap[16];

    /// IO region handler
    IOBank* ioBank;

    /// Kernal ROM
    KernalRomBank kernalRomBank;

    /// BASIC ROM
    BasicRomBank basicRomBank;

    /// Character ROM
    CharacterRomBank characterRomBank;

    /// RAM
    SystemRAMBank ramBank;

    /// RAM bank 0
    ZeroRAMBank zeroRAMBank;

private:
    void setCpuPort(uint8_t state) override;
    uint8_t getLastReadByte() const override { return 0; }
    event_clock_t getPhi2Time() const override { return eventScheduler.getTime(EVENT_CLOCK_PHI2); }

    void updateMappingPHI2();

public:
    MMU(EventScheduler &eventScheduler, IOBank* ioBank);
    ~MMU() {}

    void reset();

    void setRoms(const uint8_t* kernal, const uint8_t* basic, const uint8_t* character)
    {
        kernalRomBank.set(kernal);
        basicRomBank.set(basic);
        characterRomBank.set(character);
    }

    // RAM access methods
    uint8_t readMemByte(uint_least16_t addr) override { return ramBank.peek(addr); }
    uint_least16_t readMemWord(uint_least16_t addr) override { return endian_little16(ramBank.ram+addr); }

    void writeMemByte(uint_least16_t addr, uint8_t value) override { ramBank.poke(addr, value); }
    void writeMemWord(uint_least16_t addr, uint_least16_t value) override { endian_little16(ramBank.ram+addr, value); }

    void fillRam(uint_least16_t start, uint8_t value, unsigned int size) override
    {
        memset(ramBank.ram+start, value, size);
    }
    void fillRam(uint_least16_t start, const uint8_t* source, unsigned int size) override
    {
        memcpy(ramBank.ram+start, source, size);
    }

    // SID specific hacks
    void installResetHook(uint_least16_t addr) override { kernalRomBank.installResetHook(addr); }

    void installBasicTrap(uint_least16_t addr) override { basicRomBank.installTrap(addr); }

    void setBasicSubtune(uint8_t tune) override { basicRomBank.setSubtune(tune); }

    /**
     * Access memory as seen by CPU.
     *
     * @param addr the address where to read from
     * @return value at address
     */
    uint8_t cpuRead(uint_least16_t addr) const { return cpuReadMap[addr >> 12]->peek(addr); }

    /**
     * Access memory as seen by CPU.
     *
     * @param addr the address where to write
     * @param data the value to write
     */
    void cpuWrite(uint_least16_t addr, uint8_t data) { cpuWriteMap[addr >> 12]->poke(addr, data); }
};

}

#endif
