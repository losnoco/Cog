/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2015 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2009-2014 VICE Project
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

#include "mos6526.h"

#include <cstring>

#include "sidendian.h"

namespace libsidplayfp
{

enum
{
    PRA     = 0,
    PRB     = 1,
    DDRA    = 2,
    DDRB    = 3,
    TAL     = 4,
    TAH     = 5,
    TBL     = 6,
    TBH     = 7,
    TOD_TEN = 8,
    TOD_SEC = 9,
    TOD_MIN = 10,
    TOD_HR  = 11,
    SDR     = 12,
    ICR     = 13,
    IDR     = 13,
    CRA     = 14,
    CRB     = 15
};

void TimerA::underFlow()
{
    parent.underflowA();
}

void TimerA::serialPort()
{
    parent.handleSerialPort();
}

void TimerB::underFlow()
{
    parent.underflowB();
}

void InterruptSource6526A::trigger(uint8_t interruptMask)
{
    InterruptSource::trigger(interruptMask);

    if (interruptMasked() && interruptTriggered())
    {
        triggerInterrupt();
        parent.interrupt(true);
    }
}

uint8_t InterruptSource6526A::clear()
{
    if (!interruptTriggered())
    {
        parent.interrupt(false);
    }

    return InterruptSource::clear();
}

void InterruptSource6526::trigger(uint8_t interruptMask)
{
    InterruptSource::trigger(interruptMask);

    if (interruptMasked() && interruptTriggered())
    {
        schedule();
    }
}

uint8_t InterruptSource6526::clear()
{
    if (scheduled)
    {
        eventScheduler.cancel(*this);
        scheduled = false;
    }

    if (!interruptTriggered())
    {
        parent.interrupt(false);
    }

    return InterruptSource::clear();
}

void InterruptSource6526::event()
{
    triggerInterrupt();
    parent.interrupt(true);

    scheduled = false;
}

void InterruptSource6526::reset()
{
    InterruptSource::reset();

    scheduled = false;
}

const char *MOS6526::credits()
{
    return
            "MOS6526/6526A (CIA) Emulation:\n"
            "\tCopyright (C) 2001-2004 Simon White\n"
            "\tCopyright (C) 2007-2010 Antti S. Lankila\n"
            "\tCopyright (C) 2009-2014 VICE Project\n"
            "\tCopyright (C) 2011-2015 Leandro Nini\n";
}

MOS6526::MOS6526(EventScheduler &scheduler) :
    eventScheduler(scheduler),
    pra(regs[PRA]),
    prb(regs[PRB]),
    ddra(regs[DDRA]),
    ddrb(regs[DDRB]),
    timerA(scheduler, *this),
    timerB(scheduler, *this),
    interruptSource(new InterruptSource6526(scheduler, *this)),
    tod(scheduler, *this, regs),
    serialPort(interruptSource.get()),
    bTickEvent("CIA B counts A", *this, &MOS6526::bTick)
{
    reset();
}

void MOS6526::handleSerialPort()
{
    if (regs[CRA] & 0x40)
    {
        serialPort.handle(regs[SDR]);
    }
}

void MOS6526::reset()
{
    memset(regs, 0, sizeof(regs));

    serialPort.reset();

    // Reset timers
    timerA.reset();
    timerB.reset();

    // Reset interruptSource
    interruptSource->reset();

    // Reset tod
    tod.reset();

    triggerScheduled = false;

    eventScheduler.cancel(bTickEvent);
}

uint8_t MOS6526::read(uint_least8_t addr)
{
    addr &= 0x0f;

    timerA.syncWithCpu();
    timerA.wakeUpAfterSyncWithCpu();
    timerB.syncWithCpu();
    timerB.wakeUpAfterSyncWithCpu();

    switch (addr)
    {
    case PRA: // Simulate a serial port
        return (regs[PRA] | ~regs[DDRA]);
    case PRB:
    {
        uint8_t data = regs[PRB] | ~regs[DDRB];
        // Timers can appear on the port
        if (regs[CRA] & 0x02)
        {
            data &= 0xbf;
            if (timerA.getPb(regs[CRA]))
                data |= 0x40;
        }
        if (regs[CRB] & 0x02)
        {
            data &= 0x7f;
            if (timerB.getPb(regs[CRB]))
                data |= 0x80;
        }
        return data;
    }
    case TAL:
        return endian_16lo8(timerA.getTimer());
    case TAH:
        return endian_16hi8(timerA.getTimer());
    case TBL:
        return endian_16lo8(timerB.getTimer());
    case TBH:
        return endian_16hi8(timerB.getTimer());
    case TOD_TEN:
    case TOD_SEC:
    case TOD_MIN:
    case TOD_HR:
        return tod.read(addr - TOD_TEN);
    case IDR:
        return interruptSource->clear();
    case CRA:
        return (regs[CRA] & 0xee) | (timerA.getState() & 1);
    case CRB:
        return (regs[CRB] & 0xee) | (timerB.getState() & 1);
    default:
        return regs[addr];
    }
}

void MOS6526::write(uint_least8_t addr, uint8_t data)
{
    addr &= 0x0f;

    timerA.syncWithCpu();
    timerB.syncWithCpu();

    const uint8_t oldData = regs[addr];
    regs[addr] = data;

    switch (addr)
    {
    case PRA:
    case DDRA:
        portA();
        break;
    case PRB:
    case DDRB:
        portB();
        break;
    case TAL:
        timerA.latchLo(data);
        break;
    case TAH:
        timerA.latchHi(data);
        break;
    case TBL:
        timerB.latchLo(data);
        break;
    case TBH:
        timerB.latchHi(data);
        break;
    case TOD_TEN:
    case TOD_SEC:
    case TOD_MIN:
    case TOD_HR:
        tod.write(addr - TOD_TEN, data);
        break;
    case SDR:
        if (regs[CRA] & 0x40)
            serialPort.setBuffered();
        break;
    case ICR:
        interruptSource->set(data);
        break;
    case CRA:
        if ((data & 1) && !(oldData & 1))
        {
            // Reset the underflow flipflop for the data port
            timerA.setPbToggle(true);
        }
        timerA.setControlRegister(data);
        break;
    case CRB:
        if ((data & 1) && !(oldData & 1))
        {
            // Reset the underflow flipflop for the data port
            timerB.setPbToggle(true);
        }
        timerB.setControlRegister(data | (data & 0x40) >> 1);
        break;
    }

    timerA.wakeUpAfterSyncWithCpu();
    timerB.wakeUpAfterSyncWithCpu();
}

void MOS6526::bTick()
{
    timerB.cascade();
}

void MOS6526::underflowA()
{
    interruptSource->trigger(InterruptSource::INTERRUPT_UNDERFLOW_A);

    if ((regs[CRB] & 0x41) == 0x41)
    {
        if (timerB.started())
        {
            eventScheduler.schedule(bTickEvent, 0, EVENT_CLOCK_PHI2);
        }
    }
}

void MOS6526::underflowB()
{
    interruptSource->trigger(InterruptSource::INTERRUPT_UNDERFLOW_B);
}

void MOS6526::todInterrupt()
{
    interruptSource->trigger(InterruptSource::INTERRUPT_ALARM);
}

}
