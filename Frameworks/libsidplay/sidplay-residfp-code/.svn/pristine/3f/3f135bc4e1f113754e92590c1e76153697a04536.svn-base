/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2015 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2010 Antti Lankila
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

#ifndef SYSTEMROMBANKS_H
#define SYSTEMROMBANKS_H

#include <stdint.h>
#include <cstring>

#include "Bank.h"
#include "c64/CPU/opcodes.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

/**
 * ROM bank base class.
 * N must be a power of two.
 */
template <int N>
class romBank : public Bank
{
protected:
    /// The ROM array
    uint8_t rom[N];

protected:
    /**
     * Set value at memory address.
     */
    void setVal(uint_least16_t address, uint8_t val) { rom[address & (N-1)]=val; }

    /**
     * Return value from memory address.
     */
    uint8_t getVal(uint_least16_t address) const { return rom[address & (N-1)]; }

    /**
     * Return pointer to memory address.
     */
    void* getPtr(uint_least16_t address) const { return (void*)&rom[address & (N-1)]; }

public:
    /**
     * Copy content from source buffer.
     */
    void set(const uint8_t* source) { if (source != nullptr) memcpy(rom, source, N); }

    /**
     * Writing to ROM is a no-op.
     */
    void poke(uint_least16_t, uint8_t) override {}

    /**
     * Read from ROM.
     */
    uint8_t peek(uint_least16_t address) override { return rom[address & (N-1)]; }
};

/**
 * Kernal ROM
 *
 * Located at $E000-$FFFF
 */
class KernalRomBank final : public romBank<0x2000>
{
private:
    uint8_t resetVectorLo;  // 0xfffc
    uint8_t resetVectorHi;  // 0xfffd

public:
    void set(const uint8_t* kernal)
    {
        romBank<0x2000>::set(kernal);

        if (kernal == nullptr)
        {
            // IRQ entry point
            setVal(0xffa0, PHAn); // Save regs
            setVal(0xffa1, TXAn);
            setVal(0xffa2, PHAn);
            setVal(0xffa3, TYAn);
            setVal(0xffa4, PHAn);
            setVal(0xffa5, JMPi); // Jump to IRQ routine
            setVal(0xffa6, 0x14);
            setVal(0xffa7, 0x03);

            // Halt
            setVal(0xea39, 0x02);

            // Hardware vectors
            setVal(0xfffa, 0x39); // NMI vector
            setVal(0xfffb, 0xea);
            setVal(0xfffc, 0x39); // RESET vector
            setVal(0xfffd, 0xea);
            setVal(0xfffe, 0xa0); // IRQ/BRK vector
            setVal(0xffff, 0xff);
        }

        // Backup Reset Vector
        resetVectorLo = getVal(0xfffc);
        resetVectorHi = getVal(0xfffd);
    }

    void reset()
    {
        // Restore original Reset Vector
        setVal(0xfffc, resetVectorLo);
        setVal(0xfffd, resetVectorHi);
    }

    /**
     * Change the RESET vector.
     *
     * @param addr the new addres to point to
     */
    void installResetHook(uint_least16_t addr)
    {
        setVal(0xfffc, endian_16lo8(addr));
        setVal(0xfffd, endian_16hi8(addr));
    }
};

/**
 * BASIC ROM
 *
 * Located at $A000-$BFFF
 */
class BasicRomBank final : public romBank<0x2000>
{
private:
    uint8_t trap[3];
    uint8_t subTune[11];

public:
    void set(const uint8_t* basic)
    {
        romBank<0x2000>::set(basic);

        // Backup BASIC Warm Start
        memcpy(trap, getPtr(0xa7ae), sizeof(trap));

        memcpy(subTune, getPtr(0xbf53), sizeof(subTune));
    }

    void reset()
    {
        // Restore original BASIC Warm Start
        memcpy(getPtr(0xa7ae), trap, sizeof(trap));

        memcpy(getPtr(0xbf53), subTune, sizeof(subTune));
    }

    /**
     * Set BASIC Warm Start address.
     *
     * @param addr
     */
    void installTrap(uint_least16_t addr)
    {
        setVal(0xa7ae, JMPw);
        setVal(0xa7af, endian_16lo8(addr));
        setVal(0xa7b0, endian_16hi8(addr));
    }

    void setSubtune(uint8_t tune)
    {
        setVal(0xbf53, LDAb);
        setVal(0xbf54, tune);
        setVal(0xbf55, STAa);
        setVal(0xbf56, 0x0c);
        setVal(0xbf57, 0x03);
        setVal(0xbf58, JSRw);
        setVal(0xbf59, 0x2c);
        setVal(0xbf5a, 0xa8);
        setVal(0xbf5b, JMPw);
        setVal(0xbf5c, 0xb1);
        setVal(0xbf5d, 0xa7);
    }
};

/**
 * Character ROM
 *
 * Located at $D000-$DFFF
 */
class CharacterRomBank final : public romBank<0x1000> {};

}

#endif
