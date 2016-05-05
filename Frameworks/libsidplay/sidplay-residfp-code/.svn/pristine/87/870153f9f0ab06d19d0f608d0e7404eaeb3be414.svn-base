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

#ifndef C64_H
#define C64_H

#include <stdint.h>
#include <cstdio>

#include <map>

#include "Banks/IOBank.h"
#include "Banks/ColorRAMBank.h"
#include "Banks/DisconnectedBusBank.h"
#include "Banks/SidBank.h"
#include "Banks/ExtraSidBank.h"

#include "EventScheduler.h"

#include "c64/c64env.h"
#include "c64/c64cpu.h"
#include "c64/c64cia.h"
#include "c64/c64vic.h"
#include "c64/mmu.h"

#include "sidcxx11.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

namespace libsidplayfp
{

class c64sid;
class sidmemory;

#ifdef PC64_TESTSUITE
class testEnv
{
public:
    virtual ~testEnv() {}
    virtual void load(const char *) =0;
};
#endif

/**
 * Commodore 64 emulation core.
 *
 * It consists of the following chips:
 * - CPU 6510
 * - VIC-II 6567/6569/6572
 * - CIA 6526
 * - SID 6581/8580
 * - PLA 7700/82S100
 * - Color RAM 2114
 * - System RAM 4164-20/50464-150
 * - Character ROM 2332
 * - Basic ROM 2364
 * - Kernal ROM 2364
 */
class c64 final : private c64env
{
public:
    typedef enum
    {
        PAL_B = 0     ///< PAL C64
        ,NTSC_M       ///< NTSC C64
        ,OLD_NTSC_M   ///< Old NTSC C64
        ,PAL_N        ///< C64 Drean
    } model_t;

private:
    typedef std::map<int, ExtraSidBank*> sidBankMap_t;

private:
    /// System clock frequency
    double cpuFrequency;

    /// Number of sources asserting IRQ
    int irqCount;

    /// BA state
    bool oldBAState;

    /// System event context
    EventScheduler eventScheduler;

    /// CPU
    c64cpu cpu;

    /// CIA1
    c64cia1 cia1;

    /// CIA2
    c64cia2 cia2;

    /// VIC II
    c64vic vic;

    /// Color RAM
    ColorRAMBank colorRAMBank;

    /// SID
    SidBank sidBank;

    /// Extra SIDs
    sidBankMap_t extraSidBanks;

    /// I/O Area #1 and #2
    DisconnectedBusBank disconnectedBusBank;

    /// I/O Area
    IOBank ioBank;

    /// MMU chip
    MMU mmu;

private:
    static double getCpuFreq(model_t model);

private:
    /**
     * Access memory as seen by CPU.
     *
     * @param addr the address where to read from
     * @return value at address
     */
    uint8_t cpuRead(uint_least16_t addr) override { return mmu.cpuRead(addr); }

    /**
     * Access memory as seen by CPU.
     *
     * @param addr the address where to write to
     * @param data the value to write
     */
    void cpuWrite(uint_least16_t addr, uint8_t data) override { mmu.cpuWrite(addr, data); }

    /**
     * IRQ trigger signal.
     *
     * Calls permitted any time, but normally originated by chips at PHI1.
     *
     * @param state
     */
    inline void interruptIRQ(bool state) override;

    /**
     * NMI trigger signal.
     *
     * Calls permitted any time, but normally originated by chips at PHI1.
     */
    inline void interruptNMI() override { cpu.triggerNMI(); }

    /**
     * Reset signal.
     */
    inline void interruptRST() override { cpu.triggerRST(); }

    /**
     * BA signal.
     *
     * Calls permitted during PHI1.
     *
     * @param state
     */
    inline void setBA(bool state) override;

    inline void lightpen(bool state) override;

#ifdef PC64_TESTSUITE
    testEnv *m_env;

    void loadFile(const char *file) override
    {
        m_env->load(file);
    }
#endif

    void resetIoBank();

public:
    c64();
    ~c64() {}

#ifdef PC64_TESTSUITE
    void setTestEnv(testEnv *env)
    {
        m_env = env;
    }
#endif

    /**
     * Get C64's event scheduler
     *
     * @return the scheduler
     */
    //@{
    EventScheduler *getEventScheduler() { return &eventScheduler; }
    const EventScheduler &getEventScheduler() const { return eventScheduler; }
    //@}

    void debug(bool enable, FILE *out) { cpu.debug(enable, out); }

    void reset();
    void resetCpu() { cpu.reset(); }

    /**
     * Set the c64 model.
     */
    void setModel(model_t model);

    void setRoms(const uint8_t* kernal, const uint8_t* basic, const uint8_t* character)
    {
        mmu.setRoms(kernal, basic, character);
    }

    /**
     * Get the CPU clock speed.
     *
     * @return the speed in Hertz
     */
    double getMainCpuSpeed() const { return cpuFrequency; }

    /**
     * Set the base SID.
     *
     * @param s the sid emu to set
     */
    void setBaseSid(c64sid *s);

    /**
     * Add an extra SID.
     *
     * @param s the sid emu to set
     * @param sidAddress
     *            base address (e.g. 0xd420)
     *
     * @return false if address is unsupported
     */
    bool addExtraSid(c64sid *s, int address);

    /**
     * Remove all the SIDs.
     */
    void clearSids();

    /**
     * Get the components credits
     */
    //@{
    const char* cpuCredits() const { return cpu.credits(); }
    const char* ciaCredits() const { return cia1.credits(); }
    const char* vicCredits() const { return vic.credits(); }
    //@}

    sidmemory& getMemInterface() { return mmu; }

    uint_least16_t getCia1TimerA() const { return cia1.getTimerA(); }
};

void c64::interruptIRQ(bool state)
{
    if (state)
    {
        if (irqCount == 0)
            cpu.triggerIRQ();

        irqCount ++;
    }
    else
    {
        irqCount --;
        if (irqCount == 0)
             cpu.clearIRQ();
    }
}

void c64::setBA(bool state)
{
    // only react to changes in state
    if (state == oldBAState)
        return;

    oldBAState = state;

    // Signal changes in BA to interested parties
    cpu.setRDY(state);
}

void c64::lightpen(bool state)
{
    if (state)
        vic.triggerLightpen();
    else
        vic.clearLightpen();
}

}

#endif // C64_H
