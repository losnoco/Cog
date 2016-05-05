/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2015 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2009-2014 VICE Project
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

#ifndef ZERORAMBANK_H
#define ZERORAMBANK_H

#include <stdint.h>

#include "Bank.h"
#include "SystemRAMBank.h"

#include "Event.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

/**
 * Interface to PLA functions.
 */
class PLA
{
public:
    virtual void setCpuPort(uint8_t state) =0;
    virtual uint8_t getLastReadByte() const =0;
    virtual event_clock_t getPhi2Time() const =0;

protected:
    ~PLA() {}
};

/**
 * Unused data port bits emulation, as investigated by groepaz:
 *
 *  - There are 2 different unused bits, 1) the output bits, 2) the input bits
 *  - The output bits can be (re)set when the data-direction is set to output
 *    for those bits and the output bits will not drop-off to 0.
 *  - When the data-direction for the unused bits is set to output then the
 *    unused input bits can be (re)set by writing to them, when set to 1 the
 *    drop-off timer will start which will cause the unused input bits to drop
 *    down to 0 in a certain amount of time.
 *  - When an unused input bit already had the drop-off timer running, and is
 *    set to 1 again, the drop-off timer will restart.
 *  - when a an unused bit changes from output to input, and the current output
 *    bit is 1, the drop-off timer will restart again
 */
template <int Bit>
class dataBit
{
private:
    /**
     * $01 bits 6 and 7 fall-off cycles (1->0), average is about 350 msec for a 6510
     * and about 1500 msec for a 8500.
     *
     *  NOTE: fall-off cycles are heavily chip- and temperature dependent. as a
     *        consequence it is very hard to find suitable realistic values that
     *        always work and we can only tweak them based on testcases. (unless we
     *        want to make it configurable or emulate temperature over time =))
     *
     *        it probably makes sense to tweak the values for a warmed up CPU, since
     *        this is likely how (old) programs were coded and tested :)
     *
     *  NOTE: the unused bits of the 6510 seem to be much more temperature dependant
     *        and the fall-off time decreases quicker and more drastically than on a
     *        8500
     *
     * cpuports.prg from the lorenz testsuite will fail when the falloff takes more
     * than 1373 cycles. this suggests that he tested on a well warmed up c64 :)
     * he explicitly delays by ~1280 cycles and mentions capacitance, so he probably
     * even was aware of what happens.
     */
    //@{
    static event_clock_t const C64_CPU6510_DATA_PORT_FALL_OFF_CYCLES = 350000;
    static event_clock_t const C64_CPU8500_DATA_PORT_FALL_OFF_CYCLES = 1500000; // Curently unused
    //@}

private:
    /// Cycle that should invalidate the bit.
    event_clock_t dataSetClk;

    /// Indicates if the bit is in the process of falling off.
    bool isFallingOff;

    /// Value of the bit.
    uint8_t dataSet;

public:
    void reset()
    {
        isFallingOff = false;
        dataSet = 0;
    }

    uint8_t readBit(event_clock_t phi2time)
    {
        if (isFallingOff && dataSetClk < phi2time)
        {
            // discharge the "capacitor"
            reset();
        }
        return dataSet;
    }

    void writeBit(event_clock_t phi2time, uint8_t value)
    {
        dataSetClk = phi2time + C64_CPU6510_DATA_PORT_FALL_OFF_CYCLES;
        dataSet = value & (1 << Bit);
        isFallingOff = true;
    }
};

/**
 * Area backed by RAM, including cpu port addresses 0 and 1.
 *
 * This is bit of a fake. We know that the CPU port is an internal
 * detail of the CPU, and therefore CPU should simply pay the price
 * for reading/writing to $00/$01.
 *
 * However, that would slow down all accesses, which is suboptimal. Therefore
 * we install this little hook to the 4k 0 region to deal with this.
 *
 * Implementation based on VICE code.
 */
class ZeroRAMBank final : public Bank
{
private:
    // not emulated
    static bool const tape_sense = false;

private:
    PLA &pla;

    /// C64 RAM area
    SystemRAMBank &ramBank;

    /// Unused bits of the data port.
    //@{
    dataBit<6> dataBit6;
    dataBit<7> dataBit7;
    //@}

    /// Value written to processor port.
    //@{
    uint8_t dir;
    uint8_t data;
    //@}

    /// Value read from processor port.
    uint8_t dataRead;

    /// State of processor port pins.
    uint8_t procPortPins;

private:
    void updateCpuPort()
    {
        // Update data pins for which direction is OUTPUT
        procPortPins = (procPortPins & ~dir) | (data & dir);

        dataRead = (data | ~dir) & (procPortPins | 0x17);

        pla.setCpuPort((data | ~dir) & 0x07);

        if ((dir & 0x20) == 0)
        {
            dataRead &= ~0x20;
        }
        if (tape_sense && (dir & 0x10) == 0)
        {
            dataRead &= ~0x10;
        }
    }

public:
    ZeroRAMBank(PLA &pla, SystemRAMBank &ramBank) :
        pla(pla),
        ramBank(ramBank)
    {}

    void reset()
    {
        dataBit6.reset();
        dataBit7.reset();

        dir = 0;
        data = 0x3f;
        dataRead = 0x3f;
        procPortPins = 0x3f;

        updateCpuPort();
    }

    uint8_t peek(uint_least16_t address) override
    {
        switch (address)
        {
        case 0:
            return dir;
        case 1:
        {
            uint8_t retval = dataRead;

            // for unused bits in input mode, the value comes from the "capacitor"

            // set real value of bit 6
            if (!(dir & 0x40))
            {
                retval &= ~0x40;
                retval |= dataBit6.readBit(pla.getPhi2Time());
            }

            // set real value of bit 7
            if (!(dir & 0x80))
            {
                retval &= ~0x80;
                retval |= dataBit7.readBit(pla.getPhi2Time());
            }

            return retval;
        }
        default:
            return ramBank.peek(address);
        }
    }

    void poke(uint_least16_t address, uint8_t value) override
    {
        switch (address)
        {
        case 0:
            // when switching an unused bit from output (where it contained a
            // stable value) to input mode (where the input is floating), some
            // of the charge is transferred to the floating input

            if (dir != value)
            {
                // check if bit 6 has flipped from 1 to 0
                if ((dir & 0x40) && !(value & 0x40))
                    dataBit6.writeBit(pla.getPhi2Time(), data);

                // check if bit 7 has flipped from 1 to 0
                if ((dir & 0x80) && !(value & 0x80))
                    dataBit7.writeBit(pla.getPhi2Time(), data);

                dir = value;
                updateCpuPort();
            }

            value = pla.getLastReadByte();
            break;
        case 1:
            // when writing to an unused bit that is output, charge the "capacitor",
            // otherwise don't touch it

            if (dir & 0x40)
                dataBit6.writeBit(pla.getPhi2Time(), value);

            if (dir & 0x80)
                dataBit7.writeBit(pla.getPhi2Time(), value);

            if (data != value)
            {
                data = value;
                updateCpuPort();
            }

            value = pla.getLastReadByte();
            break;
        default:
            break;
        }

        ramBank.poke(address, value);
    }
};

}

#endif
