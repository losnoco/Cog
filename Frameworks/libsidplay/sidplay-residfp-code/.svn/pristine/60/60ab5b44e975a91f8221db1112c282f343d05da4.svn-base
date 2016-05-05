/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2016 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2009-2014 VICE Project
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

// References below are from:
//     The MOS 6567/6569 video controller (VIC-II)
//     and its application in the Commodore 64
//     http://www.uni-mainz.de/~bauec002/VIC-Article.gz
//
// MOS 6572 info taken from http://solidstate.com.ar/wp/?p=200

#include "mos656x.h"

#include <cstring>

#include "sidendian.h"

namespace libsidplayfp
{

/// Cycle # at which the VIC takes the bus in a bad line (BA goes low).
const unsigned int VICII_FETCH_CYCLE = 11;

const unsigned int VICII_SCREEN_TEXTCOLS = 40;

const MOS656X::model_data_t MOS656X::modelData[] =
{
    {262, 64, &MOS656X::clockOldNTSC},  // Old NTSC (MOS6567R56A)
    {263, 65, &MOS656X::clockNTSC},     // NTSC-M   (MOS6567R8)
    {312, 63, &MOS656X::clockPAL},      // PAL-B    (MOS6569R1, MOS6569R3)
    {312, 65, &MOS656X::clockNTSC},     // PAL-N    (MOS6572)
};

const char *MOS656X::credits()
{
    return
            "MOS6567/6569/6572 (VIC II) Emulation:\n"
            "\tCopyright (C) 2001 Simon White\n"
            "\tCopyright (C) 2007-2010 Antti Lankila\n"
            "\tCopyright (C) 2009-2014 VICE Project\n"
            "\tCopyright (C) 2011-2016 Leandro Nini\n";
}


MOS656X::MOS656X(EventScheduler &scheduler) :
    Event("VIC Raster"),
    eventScheduler(scheduler),
    sprites(regs),
    badLineStateChangeEvent("Update AEC signal", *this, &MOS656X::badLineStateChange),
    rasterYIRQEdgeDetectorEvent("RasterY changed", *this, &MOS656X::rasterYIRQEdgeDetector)
{
    chip(MOS6569);
}

void MOS656X::reset()
{
    irqFlags            = 0;
    irqMask             = 0;
    yscroll             = 0;
    rasterY             = maxRasters - 1;
    lineCycle           = 0;
    areBadLinesEnabled  = false;
    isBadLine           = false;
    rasterYIRQCondition = false;
    rasterClk           = 0;
    vblanking           = false;
    lpAsserted          = false;

    memset(regs, 0, sizeof(regs));

    lp.reset();
    sprites.reset();

    eventScheduler.cancel(*this);
    eventScheduler.schedule(*this, 0, EVENT_CLOCK_PHI1);
}

void MOS656X::chip(model_t model)
{
    maxRasters    = modelData[model].rasterLines;
    cyclesPerLine = modelData[model].cyclesPerLine;
    clock         = modelData[model].clock;

    lp.setScreenSize(maxRasters, cyclesPerLine);

    reset();
}

uint8_t MOS656X::read(uint_least8_t addr)
{
    addr &= 0x3f;

    // Sync up timers
    sync();

    switch (addr)
    {
    case 0x11:
        // Control register 1
        return (regs[addr] & 0x7f) | ((rasterY & 0x100) >> 1);
    case 0x12:
        // Raster counter
        return rasterY & 0xFF;
    case 0x13:
        return lp.getX();
    case 0x14:
        return lp.getY();
    case 0x19:
        // Interrupt Pending Register
        return irqFlags | 0x70;
    case 0x1a:
        // Interrupt Mask Register
        return irqMask | 0xf0;
    default:
        // for addresses < $20 read from register directly
        if (addr < 0x20)
            return regs[addr];
        // for addresses < $2f set bits of high nibble to 1
        if (addr < 0x2f)
            return regs[addr] | 0xf0;
        // for addresses >= $2f return $ff
        return 0xff;
    }
}

void MOS656X::write(uint_least8_t addr, uint8_t data)
{
    addr &= 0x3f;

    regs[addr] = data;

    // Sync up timers
    sync();

    switch (addr)
    {
    case 0x11: // Control register 1
    {
        const unsigned int oldYscroll = yscroll;
        yscroll = data & 0x7;

        // This is the funniest part... handle bad line tricks.
        const bool wasBadLinesEnabled = areBadLinesEnabled;

        if (rasterY == FIRST_DMA_LINE && lineCycle == 0)
        {
            areBadLinesEnabled = readDEN();
        }

        if (oldRasterY() == FIRST_DMA_LINE && readDEN())
        {
            areBadLinesEnabled = true;
        }

        if ((oldYscroll != yscroll || areBadLinesEnabled != wasBadLinesEnabled)
            && rasterY >= FIRST_DMA_LINE
            && rasterY <= LAST_DMA_LINE)
        {
            // Check whether bad line state has changed.
            const bool wasBadLine = (wasBadLinesEnabled && (oldYscroll == (rasterY & 7)));
            const bool nowBadLine = (areBadLinesEnabled && (yscroll == (rasterY & 7)));

            if (nowBadLine != wasBadLine)
            {
                const bool oldBadLine = isBadLine;

                if (wasBadLine)
                {
                    if (lineCycle < VICII_FETCH_CYCLE)
                    {
                        isBadLine = false;
                    }
                }
                else
                {
                    if (lineCycle >= VICII_FETCH_CYCLE
                        && lineCycle < VICII_FETCH_CYCLE + VICII_SCREEN_TEXTCOLS + 3)
                    {
                        isBadLine = true;
                    }
                    else if (lineCycle <= VICII_FETCH_CYCLE + VICII_SCREEN_TEXTCOLS + 6)
                    {
                        // Bad line has been generated after fetch interval, but
                        // before the raster counter is incremented.
                        isBadLine = true;
                    }
                }

                if (isBadLine != oldBadLine)
                        eventScheduler.schedule(badLineStateChangeEvent, 0, EVENT_CLOCK_PHI1);
            }
        }
    }
        // fall-through

    case 0x12: // Raster counter
        // check raster Y irq condition changes at the next PHI1
        eventScheduler.schedule(rasterYIRQEdgeDetectorEvent, 0, EVENT_CLOCK_PHI1);
        break;

    case 0x17:
        sprites.lineCrunch(data, lineCycle);
        break;

    case 0x19:
        // VIC Interrupt Flag Register
        irqFlags &= (~data & 0x0f) | 0x80;
        handleIrqState();
        break;

    case 0x1a:
        // IRQ Mask Register
        irqMask = data & 0x0f;
        handleIrqState();
        break;
    }
}

void MOS656X::handleIrqState()
{
    // signal an IRQ unless we already signaled it
    if ((irqFlags & irqMask & 0x0f) != 0)
    {
        if ((irqFlags & 0x80) == 0)
        {
            interrupt(true);
            irqFlags |= 0x80;
        }
    }
    else
    {
        if ((irqFlags & 0x80) != 0)
        {
            interrupt(false);
            irqFlags &= 0x7f;
        }
    }
}

void MOS656X::event()
{
    const event_clock_t cycles = eventScheduler.getTime(rasterClk, eventScheduler.phase());

    event_clock_t delay;

    if (cycles)
    {
        // Update x raster
        rasterClk += cycles;
        lineCycle += cycles;
        lineCycle %= cyclesPerLine;

        delay = (this->*clock)();
    }
    else
        delay = 1;

    eventScheduler.schedule(*this, delay - eventScheduler.phase(), EVENT_CLOCK_PHI1);
}

event_clock_t MOS656X::clockPAL()
{
    event_clock_t delay = 1;

    switch (lineCycle)
    {
    case 0:
        checkVblank();
        endDma<2>();
        break;

    case 1:
        vblank();
        startDma<5>();

        // No sprites before next compulsory cycle
        if (!sprites.isDma(0xf8))
           delay = 10;
        break;

    case 2:
        endDma<3>();
        break;

    case 3:
        startDma<6>();
        break;

    case 4:
        endDma<4>();
        break;

    case 5:
        startDma<7>();
        break;

    case 6:
        endDma<5>();

        delay = sprites.isDma(0xc0) ? 2 : 4;
        break;

    case 7:
        break;

    case 8:
        endDma<6>();

        delay = 2;
        break;

    case 9:
        break;

    case 10:
        endDma<7>();
        break;

    case 11:
        startBadline();

        delay = 3;
        break;

    case 12:
        delay = 2;
        break;

    case 13:
        break;

    case 14:
        sprites.updateMc();
        break;

    case 15:
        sprites.updateMcBase();

        delay = 39;
        break;

    case 54:
        sprites.checkDma(rasterY, regs);
        startDma<0>();
        break;

    case 55:
        sprites.checkDma(rasterY, regs);    // Phi1
        sprites.checkExp();                 // Phi2
        startDma<0>();
        break;

    case 56:
        startDma<1>();
        break;

    case 57:
        sprites.checkDisplay();

        // No sprites before next compulsory cycle
        if (!sprites.isDma(0x1f))
            delay = 6;
        break;

    case 58:
        startDma<2>();
        break;

    case 59:
        endDma<0>();
        break;

    case 60:
        startDma<3>();
        break;

    case 61:
        endDma<1>();
        break;

    case 62:
        startDma<4>();
        break;

    default:
        delay = 54 - lineCycle;
    }

    return delay;
}

event_clock_t MOS656X::clockNTSC()
{
    event_clock_t delay = 1;

    switch (lineCycle)
    {
    case 0:
        checkVblank();
        startDma<5>();
        break;

    case 1:
        vblank();
        endDma<3>();

        // No sprites before next compulsory cycle
        if (!sprites.isDma(0xf8))
            delay = 10;
        break;

    case 2:
        startDma<6>();
        break;

    case 3:
        endDma<4>();
        break;

    case 4:
        startDma<7>();
        break;

    case 5:
        endDma<5>();

        delay = sprites.isDma(0xc0) ? 2 : 4;
        break;

    case 6:
        break;

    case 7:
        endDma<6>();

        delay = 2;
        break;

    case 8:
        break;

    case 9:
        endDma<7>();

        delay = 2;
        break;

    case 10:
        break;

    case 11:
        startBadline();

        delay = 3;
        break;

    case 12:
        delay = 2;
        break;

    case 13:
        break;

    case 14:
        sprites.updateMc();
        break;

    case 15:
        sprites.updateMcBase();

        delay = 40;
        break;

    case 55:
        sprites.checkDma(rasterY, regs);    // Phi1
        sprites.checkExp();                 // Phi2
        startDma<0>();
        break;

    case 56:
        sprites.checkDma(rasterY, regs);
        startDma<0>();
        break;

    case 57:
        startDma<1>();
        break;

    case 58:
        sprites.checkDisplay();

        // No sprites before next compulsory cycle
        if (!sprites.isDma(0x1f))
            delay = 7;
        break;

    case 59:
        startDma<2>();
        break;

    case 60:
        endDma<0>();
        break;

    case 61:
        startDma<3>();
        break;

    case 62:
        endDma<1>();
        break;

    case 63:
        startDma<4>();
        break;

    case 64:
        endDma<2>();
        break;

    default:
        delay = 55 - lineCycle;
    }

    return delay;
}

event_clock_t MOS656X::clockOldNTSC()
{
    event_clock_t delay = 1;

    switch (lineCycle)
    {
    case 0:
        checkVblank();
        endDma<2>();
        break;

    case 1:
        vblank();
        startDma<5>();

        // No sprites before next compulsory cycle
        if (!sprites.isDma(0xf8))
           delay = 10;
        break;

    case 2:
        endDma<3>();
        break;

    case 3:
        startDma<6>();
        break;

    case 4:
        endDma<4>();
        break;

    case 5:
        startDma<7>();
        break;

    case 6:
        endDma<5>();

        delay = sprites.isDma(0xc0) ? 2 : 4;
        break;

    case 7:
        break;

    case 8:
        endDma<6>();

        delay = 2;
        break;

    case 9:
        break;

    case 10:
        endDma<7>();
        break;

    case 11:
        startBadline();

        delay = 3;
        break;

    case 12:
        delay = 2;
        break;

    case 13:
        break;

    case 14:
        sprites.updateMc();
        break;

    case 15:
        sprites.updateMcBase();

        delay = 40;
        break;

    case 55:
        sprites.checkDma(rasterY, regs);    // Phi1
        sprites.checkExp();                 // Phi2
        startDma<0>();
        break;

    case 56:
        sprites.checkDma(rasterY, regs);
        startDma<0>();
        break;

    case 57:
        sprites.checkDisplay();
        startDma<1>();

        // No sprites before next compulsory cycle
        delay = (!sprites.isDma(0x1f)) ? 7 : 2;
        break;

    case 58:
        break;

    case 59:
        startDma<2>();
        break;

    case 60:
        endDma<0>();
        break;

    case 61:
        startDma<3>();
        break;

    case 62:
        endDma<1>();
        break;

    case 63:
        startDma<4>();
        break;

    default:
        delay = 55 - lineCycle;
    }

    return delay;
}

void MOS656X::triggerLightpen()
{
    // Synchronise simulation
    sync();

    lpAsserted = true;

    if (lp.trigger(lineCycle, rasterY))
    {
        activateIRQFlag(IRQ_LIGHTPEN);
    }
}

void MOS656X::clearLightpen()
{
    lpAsserted = false;
}

}
