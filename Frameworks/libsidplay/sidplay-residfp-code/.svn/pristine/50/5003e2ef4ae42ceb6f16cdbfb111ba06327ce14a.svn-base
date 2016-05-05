/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2016 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef MOS6510_H
#define MOS6510_H

#include <stdint.h>
#include <cstdio>

#include "flags.h"
#include "EventCallback.h"
#include "EventScheduler.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

class EventContext;

namespace libsidplayfp
{

#ifdef DEBUG
class MOS6510;

namespace MOS6510Debug
{
    void DumpState(event_clock_t time, MOS6510 &cpu);
}
#endif

/*
 * Define this to get correct emulation of SHA/SHX/SHY/SHS instructions
 * (see VICE CPU tests).
 * This will slow down the emulation a bit with no real benefit
 * for SID playing so we keep it disabled.
 */
//#define CORRECT_SH_INSTRUCTIONS


/**
 * Cycle-exact 6502/6510 emulation core.
 *
 * Code is based on work by Simon A. White <sidplay2@yahoo.com>.
 * Original Java port by Ken Händel. Later on, it has been hacked to
 * improve compatibility with Lorenz suite on VICE's test suite.
 *
 * @author alankila
 */
class MOS6510
{
#ifdef DEBUG
    friend void MOS6510Debug::DumpState(event_clock_t time, MOS6510 &cpu);
#endif

private:
    /**
     * IRQ/NMI magic limit values.
     * Need to be larger than about 0x103 << 3,
     * but can't be min/max for Integer type.
     */
    static const int MAX = 65536;

    /// Stack page location
    static const uint8_t SP_PAGE = 0x01;

public:
    /// Status register interrupt bit.
    static const int SR_INTERRUPT = 2;

private:
    struct ProcessorCycle
    {
        void (MOS6510::*func)();
        bool nosteal;
        ProcessorCycle() :
            func(0),
            nosteal(false) {}
    };

private:
    /// Event scheduler
    EventScheduler &eventScheduler;

    /// Current instruction and subcycle within instruction
    int cycleCount;

    /// When IRQ was triggered. -MAX means "during some previous instruction", MAX means "no IRQ"
    int interruptCycle;

    /// IRQ asserted on CPU
    bool irqAssertedOnPin;

    /// NMI requested?
    bool nmiFlag;

    /// RST requested?
    bool rstFlag;

    /// RDY pin state (stop CPU on read)
    bool rdy;

    /// Address Low summer carry
    bool adl_carry;

#ifdef CORRECT_SH_INSTRUCTIONS
    /// The RDY pin state during last throw away read.
    bool rdyOnThrowAwayRead;
#endif

    /// Status register
    Flags flags;

    // Data regarding current instruction
    uint_least16_t Register_ProgramCounter;
    uint_least16_t Cycle_EffectiveAddress;
    uint_least16_t Cycle_Pointer;

    uint8_t Cycle_Data;
    uint8_t Register_StackPointer;
    uint8_t Register_Accumulator;
    uint8_t Register_X;
    uint8_t Register_Y;

#ifdef DEBUG
    // Debug info
    uint_least16_t instrStartPC;
    uint_least16_t instrOperand;

    FILE *m_fdbg;

    bool dodump;
#endif

    /// Table of CPU opcode implementations
    struct ProcessorCycle instrTable[0x101 << 3];

private:
    /// Represents an instruction subcycle that writes
    EventCallback<MOS6510> m_nosteal;

    /// Represents an instruction subcycle that reads
    EventCallback<MOS6510> m_steal;

    void eventWithoutSteals();
    void eventWithSteals();

    inline void Initialise();

    // Declare Interrupt Routines
    inline void IRQLoRequest();
    inline void IRQHiRequest();
    inline void interruptsAndNextOpcode();
    inline void calculateInterruptTriggerCycle();

    // Declare Instruction Routines
    inline void fetchNextOpcode();
    inline void throwAwayFetch();
    inline void throwAwayRead();
    inline void FetchDataByte();
    inline void FetchLowAddr();
    inline void FetchLowAddrX();
    inline void FetchLowAddrY();
    inline void FetchHighAddr();
    inline void FetchHighAddrX();
    inline void FetchHighAddrX2();
    inline void FetchHighAddrY();
    inline void FetchHighAddrY2();
    inline void FetchLowEffAddr();
    inline void FetchHighEffAddr();
    inline void FetchHighEffAddrY();
    inline void FetchHighEffAddrY2();
    inline void FetchLowPointer();
    inline void FetchLowPointerX();
    inline void FetchHighPointer();
    inline void FetchEffAddrDataByte();
    inline void PutEffAddrDataByte();
    inline void PushLowPC();
    inline void PushHighPC();
    inline void PushSR();
    inline void PopLowPC();
    inline void PopHighPC();
    inline void PopSR();
    inline void brkPushLowPC();
    inline void WasteCycle();

    // Delcare Instruction Operation Routines
    inline void adc_instr();
    inline void alr_instr();
    inline void anc_instr();
    inline void and_instr();
    inline void ane_instr();
    inline void arr_instr();
    inline void asl_instr();
    inline void asla_instr();
    inline void aso_instr();
    inline void axa_instr();
    inline void axs_instr();
    inline void bcc_instr();
    inline void bcs_instr();
    inline void beq_instr();
    inline void bit_instr();
    inline void bmi_instr();
    inline void bne_instr();
    inline void branch_instr(bool condition);
    inline void fix_branch();
    inline void bpl_instr();
    inline void brk_instr();
    inline void bvc_instr();
    inline void bvs_instr();
    inline void clc_instr();
    inline void cld_instr();
    inline void cli_instr();
    inline void clv_instr();
    inline void cmp_instr();
    inline void cpx_instr();
    inline void cpy_instr();
    inline void dcm_instr();
    inline void dec_instr();
    inline void dex_instr();
    inline void dey_instr();
    inline void eor_instr();
    inline void inc_instr();
    inline void ins_instr();
    inline void inx_instr();
    inline void iny_instr();
    inline void jmp_instr();
    inline void las_instr();
    inline void lax_instr();
    inline void lda_instr();
    inline void ldx_instr();
    inline void ldy_instr();
    inline void lse_instr();
    inline void lsr_instr();
    inline void lsra_instr();
    inline void oal_instr();
    inline void ora_instr();
    inline void pha_instr();
    inline void pla_instr();
    inline void plp_instr();
    inline void rla_instr();
    inline void rol_instr();
    inline void rola_instr();
    inline void ror_instr();
    inline void rora_instr();
    inline void rra_instr();
    inline void rti_instr();
    inline void rts_instr();
    inline void sbx_instr();
    inline void say_instr();
    inline void sbc_instr();
    inline void sec_instr();
    inline void sed_instr();
    inline void sei_instr();
    inline void shs_instr();
    inline void sta_instr();
    inline void stx_instr();
    inline void sty_instr();
    inline void tax_instr();
    inline void tay_instr();
    inline void tsx_instr();
    inline void txa_instr();
    inline void txs_instr();
    inline void tya_instr();
    inline void xas_instr();
    inline void sh_instr(uint8_t offset);

    void illegal_instr();

    // Declare Arithmetic Operations
    inline void doADC();
    inline void doSBC();

    inline void doJSR();

    inline void buildInstructionTable();

protected:
    MOS6510(EventScheduler &scheduler);
    ~MOS6510() {}

    /**
     * Get data from system environment.
     *
     * @param address
     * @return data byte CPU requested
     */
    virtual uint8_t cpuRead(uint_least16_t addr) =0;

    /**
     * Write data to system environment.
     *
     * @param address
     * @param data
     */
    virtual void cpuWrite(uint_least16_t addr, uint8_t data) =0;

public:
#ifdef PC64_TESTSUITE
    virtual void loadFile(const char *file) =0;
#endif

    void reset();

    static const char *credits();

    void debug(bool enable, FILE *out);
    void setRDY(bool newRDY);

    // Non-standard functions
    void triggerRST();
    void triggerNMI();
    void triggerIRQ();
    void clearIRQ();
};

}

#endif // MOS6510_H
