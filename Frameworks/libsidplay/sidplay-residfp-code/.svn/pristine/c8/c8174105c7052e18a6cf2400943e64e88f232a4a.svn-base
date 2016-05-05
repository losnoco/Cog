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

#include "mos6510.h"

#include "Event.h"
#include "sidendian.h"

#include "opcodes.h"

#ifdef DEBUG
#  include <cstdio>
#  include "mos6510debug.h"
#endif


#ifdef PC64_TESTSUITE
#  include <cstdlib>
#endif

namespace libsidplayfp
{

#ifdef PC64_TESTSUITE

/**
 * CHR$ conversion table (0x01 = no output)
 */
static const char CHRtab[256] =
{
  0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x0d,0x01,0x01,
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x20,0x21,0x01,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
  0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
  0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
  0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x5b,0x24,0x5d,0x20,0x20,
  // alternative: CHR$(92=0x5c) => ISO Latin-1(0xa3)
  0x2d,0x23,0x7c,0x2d,0x2d,0x2d,0x2d,0x7c,0x7c,0x5c,0x5c,0x2f,0x5c,0x5c,0x2f,0x2f,
  0x5c,0x23,0x5f,0x23,0x7c,0x2f,0x58,0x4f,0x23,0x7c,0x23,0x2b,0x7c,0x7c,0x26,0x5c,
  // 0x80-0xFF
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x20,0x7c,0x23,0x2d,0x2d,0x7c,0x23,0x7c,0x23,0x2f,0x7c,0x7c,0x2f,0x5c,0x5c,0x2d,
  0x2f,0x2d,0x2d,0x7c,0x7c,0x7c,0x7c,0x2d,0x2d,0x2d,0x2f,0x5c,0x5c,0x2f,0x2f,0x23,
  0x20,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
  0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x2b,0x7c,0x7c,0x26,0x5c,
  0x20,0x7c,0x23,0x2d,0x2d,0x7c,0x23,0x7c,0x23,0x2f,0x7c,0x7c,0x2f,0x5c,0x5c,0x2d,
  0x2f,0x2d,0x2d,0x7c,0x7c,0x7c,0x7c,0x2d,0x2d,0x2d,0x2f,0x5c,0x5c,0x2f,0x2f,0x23
};
static char filetmp[0x100];
static int  filepos = 0;
#endif // PC64_TESTSUITE


/**
 * Magic value for lxa and ane undocumented instructions.
 * Magic may be EE, EF, FE or FF, but most emulators seem to use EE.
 * Based on tests on a couple of chips at
 * http://visual6502.org/wiki/index.php?title=6502_Opcode_8B_(XAA,_ANE)
 * the value of magic for the MOS 6510 is FF.
 * However the Lorentz test suite assumes this to be EE.
 */
const uint8_t magic =
#ifdef PC64_TESTSUITE
    0xee
#else
    0xff
#endif
;
//-------------------------------------------------------------------------//

/**
 * When AEC signal is high, no stealing is possible.
 */
void MOS6510::eventWithoutSteals()
{
    const ProcessorCycle &instr = instrTable[cycleCount++];
    (this->*(instr.func)) ();
    eventScheduler.schedule(m_nosteal, 1);
}

/**
 * When AEC signal is low, steals permitted.
 */
void MOS6510::eventWithSteals()
{
    if (instrTable[cycleCount].nosteal)
    {
        const ProcessorCycle &instr = instrTable[cycleCount++];
        (this->*(instr.func)) ();
        eventScheduler.schedule(m_steal, 1);
    }
    else
    {
#ifdef CORRECT_SH_INSTRUCTIONS
        // Save rdy state for SH* instructions
        if (instrTable[cycleCount].func == &MOS6510::throwAwayRead)
        {
            rdyOnThrowAwayRead = false;
        }
#endif
        // Even while stalled, the CPU can still process first clock of
        // interrupt delay, but only the first one.
        if (interruptCycle == cycleCount)
        {
            interruptCycle --;
        }
    }
}


/**
 * Handle bus access signals. When RDY line is asserted, the CPU
 * will pause when executing the next read operation.
 *
 * @param newRDY new state for RDY signal
 */
void MOS6510::setRDY(bool newRDY)
{
    rdy = newRDY;

    if (rdy)
    {
        eventScheduler.cancel(m_steal);
        eventScheduler.schedule(m_nosteal, 0, EVENT_CLOCK_PHI2);
    }
    else
    {
        eventScheduler.cancel(m_nosteal);
        eventScheduler.schedule(m_steal, 0, EVENT_CLOCK_PHI2);
    }
}


/**
 * Push P on stack, decrement S.
 */
void MOS6510::PushSR()
{
    const uint_least16_t addr = endian_16(SP_PAGE, Register_StackPointer);
    cpuWrite(addr, flags.get());
    Register_StackPointer--;
}

/**
 * increment S, Pop P off stack.
 */
void MOS6510::PopSR()
{
    // Get status register off stack
    Register_StackPointer++;
    const uint_least16_t addr = endian_16(SP_PAGE, Register_StackPointer);
    flags.set(cpuRead(addr));
    flags.setB(true);

    calculateInterruptTriggerCycle();
}


//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Interrupt Routines                                                      //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

/**
 * This forces the CPU to abort whatever it is doing and immediately
 * enter the RST interrupt handling sequence. The implementation is
 * not compatible: instructions actually get aborted mid-execution.
 * However, there is no possible way to trigger this signal from
 * programs, so it's OK.
 */
void MOS6510::triggerRST()
{
    Initialise();
    cycleCount = BRKn << 3;
    rstFlag = true;
    calculateInterruptTriggerCycle();
}

/**
 * Trigger NMI interrupt on the CPU. Calling this method
 * flags that CPU must enter the NMI routine at earliest
 * opportunity. There is no way to cancel NMI request once
 * given.
 */
void MOS6510::triggerNMI()
{
    nmiFlag = true;
    calculateInterruptTriggerCycle();

    /* maybe process 1 clock of interrupt delay. */
    if (!rdy)
    {
        eventScheduler.cancel(m_steal);
        eventScheduler.schedule(m_steal, 0, EVENT_CLOCK_PHI2);
    }
}

/**
 * Pull IRQ line low on CPU.
 */
void MOS6510::triggerIRQ()
{
    irqAssertedOnPin = true;
    calculateInterruptTriggerCycle();

    /* maybe process 1 clock of interrupt delay. */
    if (!rdy && interruptCycle == cycleCount)
    {
        eventScheduler.cancel(m_steal);
        eventScheduler.schedule(m_steal, 0, EVENT_CLOCK_PHI2);
    }
}

/**
 * Inform CPU that IRQ is no longer pulled low.
 */
void MOS6510::clearIRQ()
{
    irqAssertedOnPin = false;
    calculateInterruptTriggerCycle();
}

void MOS6510::interruptsAndNextOpcode()
{
    if (cycleCount > interruptCycle + 2)
    {
#ifdef DEBUG
        if (dodump)
        {
            const event_clock_t cycles = eventScheduler.getTime(EVENT_CLOCK_PHI2);
            fprintf(m_fdbg, "****************************************************\n");
            fprintf(m_fdbg, " interrupt (%d)\n", static_cast<int>(cycles));
            fprintf(m_fdbg, "****************************************************\n");
            MOS6510Debug::DumpState(cycles, *this);
        }
#endif
        cpuRead(Register_ProgramCounter);
        cycleCount = BRKn << 3;
        flags.setB(false);
        interruptCycle = MAX;
    }
    else
    {
        fetchNextOpcode();
    }
}

void MOS6510::fetchNextOpcode()
{
#ifdef DEBUG
    if (dodump)
    {
        MOS6510Debug::DumpState(eventScheduler.getTime(EVENT_CLOCK_PHI2), *this);
    }

    instrStartPC = Register_ProgramCounter;
#endif

#ifdef CORRECT_SH_INSTRUCTIONS
    rdyOnThrowAwayRead = true;
#endif
    cycleCount = cpuRead(Register_ProgramCounter) << 3;
    Register_ProgramCounter++;

    if (!rstFlag && !nmiFlag && !(!flags.getI() && irqAssertedOnPin))
    {
        interruptCycle = MAX;
    }
    if (interruptCycle != MAX)
    {
        interruptCycle = -MAX;
    }
}

/**
 * Evaluate when to execute an interrupt. Calling this method can also
 * result in the decision that no interrupt at all needs to be scheduled.
 */
void MOS6510::calculateInterruptTriggerCycle()
{
    /* Interrupt cycle not going to trigger? */
    if (interruptCycle == MAX)
    {
        if (rstFlag || nmiFlag || (!flags.getI() && irqAssertedOnPin))
        {
            interruptCycle = cycleCount;
        }
    }
}

void MOS6510::IRQLoRequest()
{
    endian_16lo8(Register_ProgramCounter, cpuRead(Cycle_EffectiveAddress));
}

void MOS6510::IRQHiRequest()
{
    endian_16hi8(Register_ProgramCounter, cpuRead(Cycle_EffectiveAddress + 1));
}

/**
 * Read the next opcode byte from memory (and throw it away)
 */
void MOS6510::throwAwayFetch()
{
    cpuRead(Register_ProgramCounter);
}

/**
 * Issue throw-away read and fix address.
 * Some people use these to ACK CIA IRQs.
 */
void MOS6510::throwAwayRead()
{
    cpuRead(Cycle_EffectiveAddress);
    if (adl_carry)
        Cycle_EffectiveAddress += 0x100;
}

/**
 * Fetch value, increment PC.
 *
 * Addressing Modes:
 * - Immediate
 * - Relative
 */
void MOS6510::FetchDataByte()
{
    Cycle_Data = cpuRead(Register_ProgramCounter);
    if (flags.getB())
    {
        Register_ProgramCounter++;
    }

#ifdef DEBUG
    instrOperand = Cycle_Data;
#endif
}

/**
 * Fetch low address byte, increment PC.
 *
 * Addressing Modes:
 * - Stack Manipulation
 * - Absolute
 * - Zero Page
 * - Zero Page Indexed
 * - Absolute Indexed
 * - Absolute Indirect
 */
void MOS6510::FetchLowAddr()
{
    Cycle_EffectiveAddress = cpuRead(Register_ProgramCounter);
    Register_ProgramCounter++;

#ifdef DEBUG
    instrOperand = Cycle_EffectiveAddress;
#endif
}

/**
 * Read from address, add index register X to it.
 *
 * Addressing Modes:
 * - Zero Page Indexed
 */
void MOS6510::FetchLowAddrX()
{
    FetchLowAddr();
    Cycle_EffectiveAddress = (Cycle_EffectiveAddress + Register_X) & 0xFF;
}

/**
 * Read from address, add index register Y to it.
 *
 * Addressing Modes:
 * - Zero Page Indexed
 */
void MOS6510::FetchLowAddrY()
{
    FetchLowAddr();
    Cycle_EffectiveAddress = (Cycle_EffectiveAddress + Register_Y) & 0xFF;
}

/**
 * Fetch high address byte, increment PC (Absolute Addressing).
 * Low byte must have been obtained first!
 *
 * Addressing Modes:
 * - Absolute
 */
void MOS6510::FetchHighAddr()
{   // Get the high byte of an address from memory
    endian_16hi8(Cycle_EffectiveAddress, cpuRead(Register_ProgramCounter));
    Register_ProgramCounter++;

#ifdef DEBUG
    endian_16hi8(instrOperand, endian_16hi8(Cycle_EffectiveAddress));
#endif
}

/**
 * Fetch high byte of address, add index register X to low address byte,
 * increment PC.
 *
 * Addressing Modes:
 * - Absolute Indexed
 */
void MOS6510::FetchHighAddrX()
{
    Cycle_EffectiveAddress += Register_X;
    adl_carry = Cycle_EffectiveAddress > 0xff;
    FetchHighAddr();
}

/**
 * Same as #FetchHighAddrX except doesn't worry about page crossing.
 */
void MOS6510::FetchHighAddrX2()
{
    FetchHighAddrX();
    if (!adl_carry)
        cycleCount++;
}

/**
 * Fetch high byte of address, add index register Y to low address byte,
 * increment PC.
 *
 * Addressing Modes:
 * - Absolute Indexed
 */
void MOS6510::FetchHighAddrY()
{
    Cycle_EffectiveAddress += Register_Y;
    adl_carry = Cycle_EffectiveAddress > 0xff;
    FetchHighAddr();
}

/**
 * Same as #FetchHighAddrY except doesn't worry about page crossing.
 */
void MOS6510::FetchHighAddrY2()
{
    FetchHighAddrY();
    if (!adl_carry)
        cycleCount++;
}

/**
 * Fetch pointer address low, increment PC.
 *
 * Addressing Modes:
 * - Absolute Indirect
 * - Indirect indexed (post Y)
 */
void MOS6510::FetchLowPointer()
{
    Cycle_Pointer = cpuRead(Register_ProgramCounter);
    Register_ProgramCounter++;

#ifdef DEBUG
    instrOperand = Cycle_Pointer;
#endif
}

/**
 * Add X to it.
 *
 * Addressing Modes:
 * - Indexed Indirect (pre X)
 */
void MOS6510::FetchLowPointerX()
{
    endian_16lo8(Cycle_Pointer, (Cycle_Pointer + Register_X) & 0xFF);
}

/**
 * Fetch pointer address high, increment PC.
 *
 * Addressing Modes:
 * - Absolute Indirect
 */
void MOS6510::FetchHighPointer()
{
    endian_16hi8(Cycle_Pointer, cpuRead(Register_ProgramCounter));
    Register_ProgramCounter++;

#ifdef DEBUG
    endian_16hi8(instrOperand, endian_16hi8(Cycle_Pointer));
#endif
}

/**
 * Fetch effective address low.
 *
 * Addressing Modes:
 * - Indirect
 * - Indexed Indirect (pre X)
 * - Indirect indexed (post Y)
 */
void MOS6510::FetchLowEffAddr()
{
    Cycle_EffectiveAddress = cpuRead(Cycle_Pointer);
}

/**
 * Fetch effective address high.
 *
 * Addressing Modes:
 * - Indirect
 * - Indexed Indirect (pre X)
 */
void MOS6510::FetchHighEffAddr()
{
    endian_16lo8(Cycle_Pointer, (Cycle_Pointer + 1) & 0xff);
    endian_16hi8(Cycle_EffectiveAddress, cpuRead(Cycle_Pointer));
}

/**
 * Fetch effective address high, add Y to low byte of effective address.
 *
 * Addressing Modes:
 * - Indirect indexed (post Y)
 */
void MOS6510::FetchHighEffAddrY()
{
    Cycle_EffectiveAddress += Register_Y;
    adl_carry = Cycle_EffectiveAddress > 0xff;
    FetchHighEffAddr();
}


/**
 * Same as #FetchHighEffAddrY except doesn't worry about page crossing.
 */
void MOS6510::FetchHighEffAddrY2()
{
    FetchHighEffAddrY();
    if (!adl_carry)
        cycleCount++;
}

//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Common Data Accessing Routines                                          //
// Data Accessing operations as described in 64doc by John West and        //
// Marko Makela                                                            //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

void MOS6510::FetchEffAddrDataByte()
{
    Cycle_Data = cpuRead(Cycle_EffectiveAddress);
}

/**
 * Write Cycle_Data to effective address.
 */
void MOS6510::PutEffAddrDataByte()
{
    cpuWrite(Cycle_EffectiveAddress, Cycle_Data);
}

/**
 * Push Program Counter Low Byte on stack, decrement S.
 */
void MOS6510::PushLowPC()
{
    const uint_least16_t addr = endian_16(SP_PAGE, Register_StackPointer);
    cpuWrite(addr, endian_16lo8(Register_ProgramCounter));
    Register_StackPointer--;
}

/**
 * Push Program Counter High Byte on stack, decrement S.
 */
void MOS6510::PushHighPC()
{
    const uint_least16_t addr = endian_16(SP_PAGE, Register_StackPointer);
    cpuWrite(addr, endian_16hi8(Register_ProgramCounter));
    Register_StackPointer--;
}

/**
 * Increment stack and pull program counter low byte from stack.
 */
void MOS6510::PopLowPC()
{
    Register_StackPointer++;
    const uint_least16_t addr = endian_16(SP_PAGE, Register_StackPointer);
    endian_16lo8(Cycle_EffectiveAddress, cpuRead(addr));
}

/**
 * Increment stack and pull program counter high byte from stack.
 */
void MOS6510::PopHighPC()
{
    Register_StackPointer++;
    const uint_least16_t addr = endian_16(SP_PAGE, Register_StackPointer);
    endian_16hi8(Cycle_EffectiveAddress, cpuRead(addr));
}

void MOS6510::WasteCycle() {}

void MOS6510::brkPushLowPC()
{
    PushLowPC();
    if (rstFlag)
    {
        /* rst = %10x */
        Cycle_EffectiveAddress = 0xfffc;
    }
    else if (nmiFlag)
    {
        /* nmi = %01x */
        Cycle_EffectiveAddress = 0xfffa;
    }
    else
    {
        /* irq = %11x */
        Cycle_EffectiveAddress = 0xfffe;
    }

    rstFlag = false;
    nmiFlag = false;
    calculateInterruptTriggerCycle();
}

//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Common Instruction Opcodes                                              //
// See and 6510 Assembly Book for more information on these instructions   //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

void MOS6510::brk_instr()
{
    PushSR();
    flags.setB(true);
    flags.setI(true);
}

void MOS6510::cld_instr()
{
    flags.setD(false);
    interruptsAndNextOpcode();
}

void MOS6510::cli_instr()
{
    flags.setI(false);
    calculateInterruptTriggerCycle();
    interruptsAndNextOpcode();
}

void MOS6510::jmp_instr()
{
    doJSR();
    interruptsAndNextOpcode();
}

void MOS6510::doJSR()
{
    Register_ProgramCounter = Cycle_EffectiveAddress;

#ifdef PC64_TESTSUITE
    // trap handlers
    if (Register_ProgramCounter == 0xffd2)
    {
        // Print character
        const char ch = CHRtab[Register_Accumulator];
        switch (ch)
        {
        case 0:
            break;
        case 1:
            fprintf(stderr, " ");
            break;
        case 0xd:
            fprintf(stderr, "\n");
            filepos = 0;
            break;
        default:
            filetmp[filepos++] = ch;
            fprintf(stderr, "%c", ch);
        }
    }
    else if (Register_ProgramCounter == 0xe16f)
    {
        // Load
        filetmp[filepos] = '\0';
        loadFile(filetmp);
    }
    else if (Register_ProgramCounter == 0x8000
        || Register_ProgramCounter == 0xa474)
    {
        // Stop
        exit(0);
    }
#endif // PC64_TESTSUITE
}

void MOS6510::pha_instr()
{
    const uint_least16_t addr = endian_16(SP_PAGE, Register_StackPointer);
    cpuWrite(addr, Register_Accumulator);
    Register_StackPointer--;
}

/**
 * RTI does not delay the IRQ I flag change as it is set 3 cycles before
 * the end of the opcode, and thus the 6510 has enough time to call the
 * interrupt routine as soon as the opcode ends, if necessary.
 */
void MOS6510::rti_instr()
{
#ifdef DEBUG
    if (dodump)
        fprintf (m_fdbg, "****************************************************\n\n");
#endif
    Register_ProgramCounter = Cycle_EffectiveAddress;
    interruptsAndNextOpcode();
}

void MOS6510::rts_instr()
{
    cpuRead(Cycle_EffectiveAddress);
    Register_ProgramCounter = Cycle_EffectiveAddress;
    Register_ProgramCounter++;
}

void MOS6510::sed_instr()
{
    flags.setD(true);
    interruptsAndNextOpcode();
}

void MOS6510::sei_instr()
{
    flags.setI(true);
    interruptsAndNextOpcode();
    if (!rstFlag && !nmiFlag && interruptCycle != MAX)
        interruptCycle = MAX;
}

void MOS6510::sta_instr()
{
    Cycle_Data = Register_Accumulator;
    PutEffAddrDataByte();
}

void MOS6510::stx_instr()
{
    Cycle_Data = Register_X;
    PutEffAddrDataByte();
}

void MOS6510::sty_instr()
{
    Cycle_Data = Register_Y;
    PutEffAddrDataByte();
}



//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Common Instruction Undocumented Opcodes                                 //
// See documented 6502-nmo.opc by Adam Vardy for more details              //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

#if 0
// Not required - Operation performed By another method
//
// Undocumented - HLT crashes the microprocessor.  When this opcode is executed, program
// execution ceases.  No hardware interrupts will execute either.  The author
// has characterized this instruction as a halt instruction since this is the
// most straightforward explanation for this opcode's behaviour.  Only a reset
// will restart execution.  This opcode leaves no trace of any operation
// performed!  No registers affected.
void MOS6510::hlt_instr() {}
#endif

/**
 * Perform the SH* instructions.
 *
 * @param offset the index added to the address
 */
void MOS6510::sh_instr(uint8_t offset)
{
    const uint8_t tmp = Cycle_Data & (endian_16hi8(Cycle_EffectiveAddress - offset) + 1);

#ifdef CORRECT_SH_INSTRUCTIONS
    /*
     * When a DMA is going on (the CPU is halted by the VIC-II)
     * while the instruction sha/shx/shy executes then the last
     * term of the ANDing (ADH+1) drops off.
     *
     * http://sourceforge.net/p/vice-emu/bugs/578/
     */
    if (rdyOnThrowAwayRead)
    {
        Cycle_Data = tmp;
    }
#endif

    /*
     * When the addressing/indexing causes a page boundary crossing
     * the highbyte of the target address becomes equal to the value stored.
     */
    if (adl_carry)
        endian_16hi8(Cycle_EffectiveAddress, tmp);
    PutEffAddrDataByte();
}

/**
 * Undocumented - This opcode stores the result of A AND X AND ADH+1 in memory.
 */
void MOS6510::axa_instr()
{
    Cycle_Data = Register_X & Register_Accumulator;
    sh_instr(Register_Y);
}

/**
 * Undocumented - This opcode ANDs the contents of the Y register with ADH+1 and stores the
 * result in memory.
 */
void MOS6510::say_instr()
{
    Cycle_Data = Register_Y;
    sh_instr(Register_X);
}

/**
 * Undocumented - This opcode ANDs the contents of the X register with ADH+1 and stores the
 * result in memory.
 */
void MOS6510::xas_instr()
{
    Cycle_Data = Register_X;
    sh_instr(Register_Y);
}

/**
 * Undocumented - AXS ANDs the contents of the A and X registers (without changing the
 * contents of either register) and stores the result in memory.
 * AXS does not affect any flags in the processor status register.
 */
void MOS6510::axs_instr()
{
    Cycle_Data = Register_Accumulator & Register_X;
    PutEffAddrDataByte();
}


/**
 * BCD adding.
 */
void MOS6510::doADC()
{
    const unsigned int C      = flags.getC() ? 1 : 0;
    const unsigned int A      = Register_Accumulator;
    const unsigned int s      = Cycle_Data;
    const unsigned int regAC2 = A + s + C;

    if (flags.getD())
    {   // BCD mode
        unsigned int lo = (A & 0x0f) + (s & 0x0f) + C;
        unsigned int hi = (A & 0xf0) + (s & 0xf0);
        if (lo > 0x09)
            lo += 0x06;
        if (lo > 0x0f)
            hi += 0x10;

        flags.setZ(!(regAC2 & 0xff));
        flags.setN(hi & 0x80);
        flags.setV(((hi ^ A) & 0x80) && !((A ^ s) & 0x80));
        if (hi > 0x90)
            hi += 0x60;

        flags.setC(hi > 0xff);
        Register_Accumulator = (hi | (lo & 0x0f));
    }
    else
    {   // Binary mode
        flags.setC(regAC2 > 0xff);
        flags.setV(((regAC2 ^ A) & 0x80) && !((A ^ s) & 0x80));
        flags.setNZ(Register_Accumulator = regAC2 & 0xff);
    }
}

/**
 * BCD subtracting.
 */
void MOS6510::doSBC()
{
    const unsigned int C      = flags.getC() ? 0 : 1;
    const unsigned int A      = Register_Accumulator;
    const unsigned int s      = Cycle_Data;
    const unsigned int regAC2 = A - s - C;

    flags.setC(regAC2 < 0x100);
    flags.setV(((regAC2 ^ A) & 0x80) && ((A ^ s) & 0x80));
    flags.setNZ(regAC2);

    if (flags.getD())
    {   // BCD mode
        unsigned int lo = (A & 0x0f) - (s & 0x0f) - C;
        unsigned int hi = (A & 0xf0) - (s & 0xf0);
        if (lo & 0x10)
        {
             lo -= 0x06;
             hi -= 0x10;
        }
        if (hi & 0x100)
            hi -= 0x60;
        Register_Accumulator = (hi | (lo & 0x0f));
    }
    else
    {   // Binary mode
        Register_Accumulator = regAC2 & 0xff;
    }
}



//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Generic Instruction Addressing Routines                                 //
//-------------------------------------------------------------------------//


//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Generic Instruction Opcodes                                             //
// See and 6510 Assembly Book for more information on these instructions   //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

void MOS6510::adc_instr()
{
    doADC();
    interruptsAndNextOpcode();
}

void MOS6510::and_instr()
{
    flags.setNZ(Register_Accumulator &= Cycle_Data);
    interruptsAndNextOpcode();
}

/**
 * Undocumented - For a detailed explanation of this opcode look at:
 * http://visual6502.org/wiki/index.php?title=6502_Opcode_8B_(XAA,_ANE)
 */
void MOS6510::ane_instr()
{
    flags.setNZ(Register_Accumulator = (Register_Accumulator | magic) & Register_X & Cycle_Data);
    interruptsAndNextOpcode();
}

void MOS6510::asl_instr()
{
    PutEffAddrDataByte();
    flags.setC(Cycle_Data & 0x80);
    flags.setNZ(Cycle_Data <<= 1);
}

void MOS6510::asla_instr()
{
    flags.setC(Register_Accumulator & 0x80);
    flags.setNZ(Register_Accumulator <<= 1);
    interruptsAndNextOpcode();
}

void MOS6510::fix_branch()
{
    // Throw away read
    cpuRead(Cycle_EffectiveAddress);

    // Fix address
    Register_ProgramCounter += Cycle_Data < 0x80 ? 0x0100 : 0xff00;
}

void MOS6510::branch_instr(bool condition)
{
    // 2 cycles spent before arriving here. spend 0 - 2 cycles here;
    // - condition false: Continue immediately to FetchNextInstr.
    //
    // Otherwise read the byte following the opcode (which is already scheduled to occur on this cycle).
    // This effort is wasted. Then calculate address of the branch target. If branch is on same page,
    // then continue at that insn on next cycle (this delays IRQs by 1 clock for some reason, allegedly).
    //
    // If the branch is on different memory page, issue a spurious read with wrong high byte before
    // continuing at the correct address.
    if (condition)
    {
        // issue the spurious read for next insn here.
        cpuRead(Register_ProgramCounter);

        Cycle_EffectiveAddress = endian_16lo8(Register_ProgramCounter);
        Cycle_EffectiveAddress += Cycle_Data;
        adl_carry = (Cycle_EffectiveAddress > 0xff) != (Cycle_Data > 0x7f);
        endian_16hi8(Cycle_EffectiveAddress, endian_16hi8(Register_ProgramCounter));

        Register_ProgramCounter = Cycle_EffectiveAddress;

        // Check for page boundary crossing
        if (!adl_carry)
        {
            // Skip next throw away read
            cycleCount++;

            // Hack: delay the interrupt past this instruction.
            if (interruptCycle >> 3 == cycleCount >> 3)
                interruptCycle += 2;
        }
    }
    else
    {
        // branch not taken: skip the following spurious read insn and go to FetchNextInstr immediately.
        interruptsAndNextOpcode();
    }
}

void MOS6510::bcc_instr()
{
    branch_instr(!flags.getC());
}

void MOS6510::bcs_instr()
{
    branch_instr(flags.getC());
}

void MOS6510::beq_instr()
{
    branch_instr(flags.getZ());
}

void MOS6510::bit_instr()
{
    flags.setZ((Register_Accumulator & Cycle_Data) == 0);
    flags.setN(Cycle_Data & 0x80);
    flags.setV(Cycle_Data & 0x40);
    interruptsAndNextOpcode();
}

void MOS6510::bmi_instr()
{
    branch_instr(flags.getN());
}

void MOS6510::bne_instr()
{
    branch_instr(!flags.getZ());
}

void MOS6510::bpl_instr()
{
    branch_instr(!flags.getN());
}

void MOS6510::bvc_instr()
{
    branch_instr(!flags.getV());
}

void MOS6510::bvs_instr()
{
    branch_instr(flags.getV());
}

void MOS6510::clc_instr()
{
    flags.setC(false);
    interruptsAndNextOpcode();
}

void MOS6510::clv_instr()
{
    flags.setV(false);
    interruptsAndNextOpcode();
}

void MOS6510::cmp_instr()
{
    const uint_least16_t tmp = static_cast<uint_least16_t>(Register_Accumulator) - Cycle_Data;
    flags.setNZ(tmp);
    flags.setC(tmp < 0x100);
    interruptsAndNextOpcode();
}

void MOS6510::cpx_instr()
{
    const uint_least16_t tmp = static_cast<uint_least16_t>(Register_X) - Cycle_Data;
    flags.setNZ(tmp);
    flags.setC(tmp < 0x100);
    interruptsAndNextOpcode();
}

void MOS6510::cpy_instr()
{
    const uint_least16_t tmp = static_cast<uint_least16_t>(Register_Y) - Cycle_Data;
    flags.setNZ(tmp);
    flags.setC(tmp < 0x1009);
    interruptsAndNextOpcode();
}

void MOS6510::dec_instr()
{
    PutEffAddrDataByte();
    flags.setNZ(--Cycle_Data);
}

void MOS6510::dex_instr()
{
    flags.setNZ(--Register_X);
    interruptsAndNextOpcode();
}

void MOS6510::dey_instr()
{
    flags.setNZ(--Register_Y);
    interruptsAndNextOpcode();
}

void MOS6510::eor_instr()
{
    flags.setNZ(Register_Accumulator ^= Cycle_Data);
    interruptsAndNextOpcode();
}

void MOS6510::inc_instr()
{
    PutEffAddrDataByte();
    flags.setNZ(++Cycle_Data);
}

void MOS6510::inx_instr()
{
    flags.setNZ(++Register_X);
    interruptsAndNextOpcode();
}

void MOS6510::iny_instr()
{
    flags.setNZ(++Register_Y);
    interruptsAndNextOpcode();
}

void MOS6510::lda_instr()
{
    flags.setNZ(Register_Accumulator = Cycle_Data);
    interruptsAndNextOpcode();
}

void MOS6510::ldx_instr()
{
    flags.setNZ(Register_X = Cycle_Data);
    interruptsAndNextOpcode();
}

void MOS6510::ldy_instr()
{
    flags.setNZ(Register_Y = Cycle_Data);
    interruptsAndNextOpcode();
}

void MOS6510::lsr_instr()
{
    PutEffAddrDataByte();
    flags.setC(Cycle_Data & 0x01);
    flags.setNZ(Cycle_Data >>= 1);
}

void MOS6510::lsra_instr()
{
    flags.setC(Register_Accumulator & 0x01);
    flags.setNZ(Register_Accumulator >>= 1);
    interruptsAndNextOpcode();
}

void MOS6510::ora_instr()
{
    flags.setNZ(Register_Accumulator |= Cycle_Data);
    interruptsAndNextOpcode();
}

void MOS6510::pla_instr()
{
    Register_StackPointer++;
    const uint_least16_t addr = endian_16(SP_PAGE, Register_StackPointer);
    flags.setNZ(Register_Accumulator = cpuRead (addr));
}

void MOS6510::plp_instr()
{
    interruptsAndNextOpcode();
}

void MOS6510::rol_instr()
{
    const uint8_t newC = Cycle_Data & 0x80;
    PutEffAddrDataByte();
    Cycle_Data <<= 1;
    if (flags.getC())
        Cycle_Data |= 0x01;
    flags.setNZ(Cycle_Data);
    flags.setC(newC);
}

void MOS6510::rola_instr()
{
    const uint8_t newC = Register_Accumulator & 0x80;
    Register_Accumulator <<= 1;
    if (flags.getC())
        Register_Accumulator |= 0x01;
    flags.setNZ(Register_Accumulator);
    flags.setC(newC);
    interruptsAndNextOpcode();
}

void MOS6510::ror_instr()
{
    const uint8_t newC = Cycle_Data & 0x01;
    PutEffAddrDataByte();
    Cycle_Data >>= 1;
    if (flags.getC())
        Cycle_Data |= 0x80;
    flags.setNZ(Cycle_Data);
    flags.setC(newC);
}

void MOS6510::rora_instr()
{
    const uint8_t newC = Register_Accumulator & 0x01;
    Register_Accumulator >>= 1;
    if (flags.getC())
        Register_Accumulator |= 0x80;
    flags.setNZ(Register_Accumulator);
    flags.setC(newC);
    interruptsAndNextOpcode();
}

void MOS6510::sbx_instr()
{
    const unsigned int tmp = (Register_X & Register_Accumulator) - Cycle_Data;
    flags.setNZ(Register_X = tmp & 0xff);
    flags.setC(tmp < 0x100);
    interruptsAndNextOpcode();
}

void MOS6510::sbc_instr()
{
    doSBC();
    interruptsAndNextOpcode();
}

void MOS6510::sec_instr()
{
    flags.setC(true);
    interruptsAndNextOpcode();
}

void MOS6510::shs_instr()
{
    Register_StackPointer = Register_Accumulator & Register_X;
    Cycle_Data = Register_StackPointer;
    sh_instr(Register_Y);
}

void MOS6510::tax_instr()
{
    flags.setNZ(Register_X = Register_Accumulator);
    interruptsAndNextOpcode();
}

void MOS6510::tay_instr()
{
    flags.setNZ(Register_Y = Register_Accumulator);
    interruptsAndNextOpcode();
}

void MOS6510::tsx_instr()
{
    flags.setNZ(Register_X = Register_StackPointer);
    interruptsAndNextOpcode();
}

void MOS6510::txa_instr()
{
    flags.setNZ(Register_Accumulator = Register_X);
    interruptsAndNextOpcode();
}

void MOS6510::txs_instr()
{
    Register_StackPointer = Register_X;
    interruptsAndNextOpcode();
}

void MOS6510::tya_instr()
{
    flags.setNZ(Register_Accumulator = Register_Y);
    interruptsAndNextOpcode();
}

void MOS6510::illegal_instr()
{
    cycleCount --;
}


//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Generic Instruction Undocumented Opcodes                                //
// See documented 6502-nmo.opc by Adam Vardy for more details              //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

/**
 * Undocumented - This opcode ANDs the contents of the A register with an immediate value and
 * then LSRs the result.
 */
void MOS6510::alr_instr()
{
    Register_Accumulator &= Cycle_Data;
    flags.setC(Register_Accumulator & 0x01);
    flags.setNZ(Register_Accumulator >>= 1);
    interruptsAndNextOpcode();
}

/**
 * Undocumented - ANC ANDs the contents of the A register with an immediate value and then
 * moves bit 7 of A into the Carry flag.  This opcode works basically
 * identically to AND #immed. except that the Carry flag is set to the same
 * state that the Negative flag is set to.
 */
void MOS6510::anc_instr()
{
    flags.setNZ(Register_Accumulator &= Cycle_Data);
    flags.setC(flags.getN());
    interruptsAndNextOpcode();
}

/**
 * Undocumented - This opcode ANDs the contents of the A register with an immediate value and
 * then RORs the result. (Implementation based on that of Frodo C64 Emulator)
 */
void MOS6510::arr_instr()
{
    const uint8_t data = Cycle_Data & Register_Accumulator;
    Register_Accumulator = data >> 1;
    if (flags.getC())
        Register_Accumulator |= 0x80;

    if (flags.getD())
    {
        flags.setN(flags.getC());
        flags.setZ(Register_Accumulator == 0);
        flags.setV((data ^ Register_Accumulator) & 0x40);

        if ((data & 0x0f) + (data & 0x01) > 5)
            Register_Accumulator  = (Register_Accumulator & 0xf0) | ((Register_Accumulator + 6) & 0x0f);
        flags.setC(((data + (data & 0x10)) & 0x1f0) > 0x50);
        if (flags.getC())
            Register_Accumulator += 0x60;
    }
    else
    {
        flags.setNZ(Register_Accumulator);
        flags.setC(Register_Accumulator & 0x40);
        flags.setV((Register_Accumulator & 0x40) ^ ((Register_Accumulator & 0x20) << 1));
    }
    interruptsAndNextOpcode();
}

/**
 * Undocumented - This opcode ASLs the contents of a memory location and then ORs the result
 * with the accumulator.
 */
void MOS6510::aso_instr()
{
    PutEffAddrDataByte();
    flags.setC(Cycle_Data & 0x80);
    Cycle_Data <<= 1;
    flags.setNZ(Register_Accumulator |= Cycle_Data);
}

/**
 * Undocumented - This opcode DECs the contents of a memory location and then CMPs the result
 * with the A register.
 */
void MOS6510::dcm_instr()
{
    PutEffAddrDataByte();
    Cycle_Data--;
    const uint_least16_t tmp = static_cast<uint_least16_t>(Register_Accumulator) - Cycle_Data;
    flags.setNZ(tmp);
    flags.setC(tmp < 0x100);
}

/**
 * Undocumented - This opcode INCs the contents of a memory location and then SBCs the result
 * from the A register.
 */
void MOS6510::ins_instr()
{
    PutEffAddrDataByte();
    Cycle_Data++;
    doSBC();
}

/**
 * Undocumented - This opcode ANDs the contents of a memory location with the contents of the
 * stack pointer register and stores the result in the accumulator, the X
 * register, and the stack pointer. Affected flags: N Z.
 */
void MOS6510::las_instr()
{
    flags.setNZ(Cycle_Data &= Register_StackPointer);
    Register_Accumulator  = Cycle_Data;
    Register_X            = Cycle_Data;
    Register_StackPointer = Cycle_Data;
    interruptsAndNextOpcode();
}

/**
 * Undocumented - This opcode loads both the accumulator and the X register with the contents
 * of a memory location.
 */
void MOS6510::lax_instr()
{
    flags.setNZ(Register_Accumulator = Register_X = Cycle_Data);
    interruptsAndNextOpcode();
}

/**
 * Undocumented - LSE LSRs the contents of a memory location and then EORs the result with
 * the accumulator.
 */
void MOS6510::lse_instr()
{
    PutEffAddrDataByte();
    flags.setC(Cycle_Data & 0x01);
    Cycle_Data >>= 1;
    flags.setNZ(Register_Accumulator ^= Cycle_Data);
}

/**
 * Undocumented - This opcode ORs the A register with #xx (the "magic" value),
 * ANDs the result with an immediate value, and then stores the result in both A and X.
 */
void MOS6510::oal_instr()
{
    flags.setNZ(Register_X = (Register_Accumulator = (Cycle_Data & (Register_Accumulator | magic))));
    interruptsAndNextOpcode();
}

/**
 * Undocumented - RLA ROLs the contents of a memory location and then ANDs the result with
 * the accumulator.
 */
void MOS6510::rla_instr()
{
    const uint8_t newC = Cycle_Data & 0x80;
    PutEffAddrDataByte();
    Cycle_Data = Cycle_Data << 1;
    if (flags.getC())
        Cycle_Data |= 0x01;
    flags.setC(newC);
    flags.setNZ(Register_Accumulator &= Cycle_Data);
}

/**
 * Undocumented - RRA RORs the contents of a memory location and then ADCs the result with
 * the accumulator.
 */
void MOS6510::rra_instr()
{
    const uint8_t newC = Cycle_Data & 0x01;
    PutEffAddrDataByte();
    Cycle_Data >>= 1;
    if (flags.getC())
        Cycle_Data |= 0x80;
    flags.setC(newC);
    doADC();
}

//-------------------------------------------------------------------------//

/**
 * Create new CPU emu.
 *
 * @param context
 *            The Event Context
 */
MOS6510::MOS6510(EventScheduler &scheduler) :
    eventScheduler(scheduler),
#ifdef DEBUG
    m_fdbg(stdout),
#endif
    m_nosteal("CPU-nosteal", *this, &MOS6510::eventWithoutSteals),
    m_steal("CPU-steal", *this, &MOS6510::eventWithSteals)
{
    buildInstructionTable();

    // Intialise Processor Registers
    Register_Accumulator   = 0;
    Register_X             = 0;
    Register_Y             = 0;

    Cycle_EffectiveAddress = 0;
    Cycle_Data             = 0;
    #ifdef DEBUG
    dodump = false;
    #endif
    Initialise();
}

/**
 * Build up the processor instruction table.
 */
void MOS6510::buildInstructionTable()
{
    for (unsigned int i = 0; i < 0x100; i++)
    {
#if DEBUG > 1
        printf("Building Command %d[%02x]... ", i, i);
#endif

        /*
         * So: what cycles are marked as stealable? Rules are:
         *
         * - CPU performs either read or write at every cycle. Reads are
         *   always stealable. Writes are rare.
         *
         * - Every instruction begins with a sequence of reads. Writes,
         *   if any, are at the end for most instructions.
         */

        unsigned int buildCycle = i << 3;

        typedef enum { WRITE, READ } AccessMode;
        AccessMode access = WRITE;
        bool legalMode  = true;
        bool legalInstr = true;

        switch (i)
        {
        // Accumulator or Implied addressing
        case ASLn: case CLCn: case CLDn: case CLIn: case CLVn:  case DEXn:
        case DEYn: case INXn: case INYn: case LSRn: case NOPn_: case PHAn:
        case PHPn: case PLAn: case PLPn: case ROLn: case RORn:
        case SECn: case SEDn: case SEIn: case TAXn:  case TAYn:
        case TSXn: case TXAn: case TXSn: case TYAn:
            instrTable[buildCycle++].func = &MOS6510::throwAwayFetch;
            break;

        // Immediate and Relative Addressing Mode Handler
        case ADCb: case ANDb:  case ANCb_: case ANEb: case ASRb: case ARRb:
        case BCCr: case BCSr:  case BEQr:  case BMIr: case BNEr: case BPLr:
        case BRKn: case BVCr:  case BVSr:  case CMPb: case CPXb: case CPYb:
        case EORb: case LDAb:  case LDXb:  case LDYb: case LXAb: case NOPb_:
        case ORAb: case SBCb_: case SBXb:  case RTIn: case RTSn:
            instrTable[buildCycle++].func = &MOS6510::FetchDataByte;
            break;

        // Zero Page Addressing Mode Handler - Read & RMW
        case ADCz:  case ANDz: case BITz: case CMPz: case CPXz: case CPYz:
        case EORz:  case LAXz: case LDAz: case LDXz: case LDYz: case ORAz:
        case NOPz_: case SBCz:
        case ASLz: case DCPz: case DECz: case INCz: case ISBz: case LSRz:
        case ROLz: case RORz: case SREz: case SLOz: case RLAz: case RRAz:
            access = READ;
        case SAXz: case STAz: case STXz: case STYz:
            instrTable[buildCycle++].func = &MOS6510::FetchLowAddr;
            break;

        // Zero Page with X Offset Addressing Mode Handler
        // these issue extra reads on the 0 page, but we don't care about it
        // because there are no detectable effects from them. These reads
        // occur during the "wasted" cycle.
        case ADCzx: case ANDzx:  case CMPzx: case EORzx: case LDAzx: case LDYzx:
        case NOPzx_: case ORAzx: case SBCzx:
        case ASLzx: case DCPzx: case DECzx: case INCzx: case ISBzx: case LSRzx:
        case RLAzx:    case ROLzx: case RORzx: case RRAzx: case SLOzx: case SREzx:
            access = READ;
        case STAzx: case STYzx:
            instrTable[buildCycle++].func = &MOS6510::FetchLowAddrX;
            // operates on 0 page in read mode. Truly side-effect free.
            instrTable[buildCycle++].func = &MOS6510::WasteCycle;
            break;

        // Zero Page with Y Offset Addressing Mode Handler
        case LDXzy: case LAXzy:
            access = READ;
        case STXzy: case SAXzy:
            instrTable[buildCycle++].func = &MOS6510::FetchLowAddrY;
            // operates on 0 page in read mode. Truly side-effect free.
            instrTable[buildCycle++].func = &MOS6510::WasteCycle;
            break;

        // Absolute Addressing Mode Handler
        case ADCa: case ANDa: case BITa: case CMPa: case CPXa: case CPYa:
        case EORa: case LAXa: case LDAa: case LDXa: case LDYa: case NOPa:
        case ORAa: case SBCa:
        case ASLa: case DCPa: case DECa: case INCa: case ISBa: case LSRa:
        case ROLa: case RORa: case SLOa: case SREa: case RLAa: case RRAa:
            access = READ;
        case JMPw: case SAXa: case STAa: case STXa: case STYa:
            instrTable[buildCycle++].func = &MOS6510::FetchLowAddr;
            instrTable[buildCycle++].func = &MOS6510::FetchHighAddr;
            break;

        case JSRw:
            instrTable[buildCycle++].func = &MOS6510::FetchLowAddr;
            break;

        // Absolute With X Offset Addressing Mode Handler (Read)
        case ADCax: case ANDax:  case CMPax: case EORax: case LDAax:
        case LDYax: case NOPax_: case ORAax: case SBCax:
            access = READ;
            instrTable[buildCycle++].func = &MOS6510::FetchLowAddr;
            instrTable[buildCycle++].func = &MOS6510::FetchHighAddrX2;
            // this cycle is skipped if the address is already correct.
            // otherwise, it will be read and ignored.
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
            break;

        // Absolute X (RMW; no page crossing handled, always reads before writing)
        case ASLax: case DCPax: case DECax: case INCax: case ISBax:
        case LSRax: case RLAax: case ROLax: case RORax: case RRAax:
        case SLOax: case SREax:
            access = READ;
        case SHYax: case STAax:
            instrTable[buildCycle++].func = &MOS6510::FetchLowAddr;
            instrTable[buildCycle++].func = &MOS6510::FetchHighAddrX;
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
            break;

        // Absolute With Y Offset Addresing Mode Handler (Read)
        case ADCay: case ANDay: case CMPay: case EORay: case LASay:
        case LAXay: case LDAay: case LDXay: case ORAay: case SBCay:
            access = READ;
            instrTable[buildCycle++].func = &MOS6510::FetchLowAddr;
            instrTable[buildCycle++].func = &MOS6510::FetchHighAddrY2;
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
            break;

        // Absolute Y (No page crossing handled)
        case DCPay: case ISBay: case RLAay: case RRAay: case SLOay:
        case SREay:
            access = READ;
        case SHAay: case SHSay: case SHXay: case STAay:
            instrTable[buildCycle++].func = &MOS6510::FetchLowAddr;
            instrTable[buildCycle++].func = &MOS6510::FetchHighAddrY;
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
            break;

        // Absolute Indirect Addressing Mode Handler
        case JMPi:
            instrTable[buildCycle++].func = &MOS6510::FetchLowPointer;
            instrTable[buildCycle++].func = &MOS6510::FetchHighPointer;
            instrTable[buildCycle++].func = &MOS6510::FetchLowEffAddr;
            instrTable[buildCycle++].func = &MOS6510::FetchHighEffAddr;
            break;

        // Indexed with X Preinc Addressing Mode Handler
        case ADCix: case ANDix: case CMPix: case EORix: case LAXix: case LDAix:
        case ORAix: case SBCix:
        case DCPix: case ISBix: case SLOix: case SREix: case RLAix: case RRAix:
            access = READ;
        case SAXix: case STAix:
            instrTable[buildCycle++].func = &MOS6510::FetchLowPointer;
            instrTable[buildCycle++].func = &MOS6510::FetchLowPointerX;
            instrTable[buildCycle++].func = &MOS6510::FetchLowEffAddr;
            instrTable[buildCycle++].func = &MOS6510::FetchHighEffAddr;
            break;

        // Indexed with Y Postinc Addressing Mode Handler (Read)
        case ADCiy: case ANDiy: case CMPiy: case EORiy: case LAXiy:
        case LDAiy: case ORAiy: case SBCiy:
            access = READ;
            instrTable[buildCycle++].func = &MOS6510::FetchLowPointer;
            instrTable[buildCycle++].func = &MOS6510::FetchLowEffAddr;
            instrTable[buildCycle++].func = &MOS6510::FetchHighEffAddrY2;
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
            break;

        // Indexed Y (No page crossing handled)
        case DCPiy: case ISBiy: case RLAiy: case RRAiy: case SLOiy:
        case SREiy:
            access = READ;
        case SHAiy: case STAiy:
            instrTable[buildCycle++].func = &MOS6510::FetchLowPointer;
            instrTable[buildCycle++].func = &MOS6510::FetchLowEffAddr;
            instrTable[buildCycle++].func = &MOS6510::FetchHighEffAddrY;
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
            break;

        default:
            legalMode = false;
            break;
        }

        if (access == READ)
        {
            instrTable[buildCycle++].func = &MOS6510::FetchEffAddrDataByte;
        }

        //---------------------------------------------------------------------------------------
        // Addressing Modes Finished, other cycles are instruction dependent
        //---------------------------------------------------------------------------------------
        switch(i)
        {
        case ADCz:  case ADCzx: case ADCa: case ADCax: case ADCay: case ADCix:
        case ADCiy: case ADCb:
            instrTable[buildCycle++].func = &MOS6510::adc_instr;
            break;

        case ANCb_:
            instrTable[buildCycle++].func = &MOS6510::anc_instr;
            break;

        case ANDz:  case ANDzx: case ANDa: case ANDax: case ANDay: case ANDix:
        case ANDiy: case ANDb:
            instrTable[buildCycle++].func = &MOS6510::and_instr;
            break;

        case ANEb: // Also known as XAA
            instrTable[buildCycle++].func = &MOS6510::ane_instr;
            break;

        case ARRb:
            instrTable[buildCycle++].func = &MOS6510::arr_instr;
            break;

        case ASLn:
            instrTable[buildCycle++].func = &MOS6510::asla_instr;
            break;

        case ASLz: case ASLzx: case ASLa: case ASLax:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::asl_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
            break;

        case ASRb: // Also known as ALR
            instrTable[buildCycle++].func = &MOS6510::alr_instr;
            break;

        case BCCr:
            instrTable[buildCycle++].func = &MOS6510::bcc_instr;
            instrTable[buildCycle++].func = &MOS6510::fix_branch;
            break;

        case BCSr:
            instrTable[buildCycle++].func = &MOS6510::bcs_instr;
            instrTable[buildCycle++].func = &MOS6510::fix_branch;
            break;

        case BEQr:
            instrTable[buildCycle++].func = &MOS6510::beq_instr;
            instrTable[buildCycle++].func = &MOS6510::fix_branch;
            break;

        case BITz: case BITa:
            instrTable[buildCycle++].func = &MOS6510::bit_instr;
            break;

        case BMIr:
            instrTable[buildCycle++].func = &MOS6510::bmi_instr;
            instrTable[buildCycle++].func = &MOS6510::fix_branch;
            break;

        case BNEr:
            instrTable[buildCycle++].func = &MOS6510::bne_instr;
            instrTable[buildCycle++].func = &MOS6510::fix_branch;
            break;

        case BPLr:
            instrTable[buildCycle++].func = &MOS6510::bpl_instr;
            instrTable[buildCycle++].func = &MOS6510::fix_branch;
            break;

        case BRKn:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PushHighPC;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::brkPushLowPC;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::brk_instr;
            instrTable[buildCycle++].func = &MOS6510::IRQLoRequest;
            instrTable[buildCycle++].func = &MOS6510::IRQHiRequest;
            instrTable[buildCycle++].func = &MOS6510::fetchNextOpcode;
            break;

        case BVCr:
            instrTable[buildCycle++].func = &MOS6510::bvc_instr;
            instrTable[buildCycle++].func = &MOS6510::fix_branch;
            break;

        case BVSr:
            instrTable[buildCycle++].func = &MOS6510::bvs_instr;
            instrTable[buildCycle++].func = &MOS6510::fix_branch;
            break;

        case CLCn:
            instrTable[buildCycle++].func = &MOS6510::clc_instr;
            break;

        case CLDn:
            instrTable[buildCycle++].func = &MOS6510::cld_instr;
            break;

        case CLIn:
            instrTable[buildCycle++].func = &MOS6510::cli_instr;
            break;

        case CLVn:
            instrTable[buildCycle++].func = &MOS6510::clv_instr;
            break;

        case CMPz:  case CMPzx: case CMPa: case CMPax: case CMPay: case CMPix:
        case CMPiy: case CMPb:
            instrTable[buildCycle++].func = &MOS6510::cmp_instr;
            break;

        case CPXz: case CPXa: case CPXb:
            instrTable[buildCycle++].func = &MOS6510::cpx_instr;
            break;

        case CPYz: case CPYa: case CPYb:
            instrTable[buildCycle++].func = &MOS6510::cpy_instr;
            break;

        case DCPz: case DCPzx: case DCPa: case DCPax: case DCPay: case DCPix:
        case DCPiy: // Also known as DCM
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::dcm_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
            break;

        case DECz: case DECzx: case DECa: case DECax:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::dec_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
            break;

        case DEXn:
            instrTable[buildCycle++].func = &MOS6510::dex_instr;
            break;

        case DEYn:
            instrTable[buildCycle++].func = &MOS6510::dey_instr;
            break;

        case EORz:  case EORzx: case EORa: case EORax: case EORay: case EORix:
        case EORiy: case EORb:
            instrTable[buildCycle++].func = &MOS6510::eor_instr;
            break;
#if 0
        // HLT, also known as JAM
        case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52:
        case 0x62: case 0x72: case 0x92: case 0xb2: case 0xd2: case 0xf2:
        case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52:
        case 0x62: case 0x72: case 0x92: case 0xb2: case 0xd2: case 0xf2:
            instrTable[buildCycle++].func = hlt_instr;
            break;
#endif
        case INCz: case INCzx: case INCa: case INCax:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::inc_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
            break;

        case INXn:
            instrTable[buildCycle++].func = &MOS6510::inx_instr;
            break;

        case INYn:
            instrTable[buildCycle++].func = &MOS6510::iny_instr;
            break;

        case ISBz: case ISBzx: case ISBa: case ISBax: case ISBay: case ISBix:
        case ISBiy: // Also known as INS
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::ins_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
            break;

        case JSRw:
            instrTable[buildCycle++].func = &MOS6510::WasteCycle;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PushHighPC;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PushLowPC;
            instrTable[buildCycle++].func = &MOS6510::FetchHighAddr;
        case JMPw: case JMPi:
            instrTable[buildCycle++].func = &MOS6510::jmp_instr;
            break;

        case LASay:
            instrTable[buildCycle++].func = &MOS6510::las_instr;
            break;

        case LAXz: case LAXzy: case LAXa: case LAXay: case LAXix: case LAXiy:
            instrTable[buildCycle++].func = &MOS6510::lax_instr;
            break;

        case LDAz:  case LDAzx: case LDAa: case LDAax: case LDAay: case LDAix:
        case LDAiy: case LDAb:
            instrTable[buildCycle++].func = &MOS6510::lda_instr;
            break;

        case LDXz: case LDXzy: case LDXa: case LDXay: case LDXb:
            instrTable[buildCycle++].func = &MOS6510::ldx_instr;
            break;

        case LDYz: case LDYzx: case LDYa: case LDYax: case LDYb:
            instrTable[buildCycle++].func = &MOS6510::ldy_instr;
            break;

        case LSRn:
            instrTable[buildCycle++].func = &MOS6510::lsra_instr;
            break;

        case LSRz: case LSRzx: case LSRa: case LSRax:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::lsr_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
            break;

        case NOPn_: case NOPb_:
        case NOPz_: case NOPzx_: case NOPa: case NOPax_:
        // NOPb NOPz NOPzx - Also known as SKBn
        // NOPa NOPax      - Also known as SKWn
            break;

        case LXAb: // Also known as OAL
            instrTable[buildCycle++].func = &MOS6510::oal_instr;
            break;

        case ORAz:  case ORAzx: case ORAa: case ORAax: case ORAay: case ORAix:
        case ORAiy: case ORAb:
            instrTable[buildCycle++].func = &MOS6510::ora_instr;
            break;

        case PHAn:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::pha_instr;
            break;

        case PHPn:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PushSR;
            break;

        case PLAn:
            // should read the value at current stack register.
            // Truly side-effect free.
            instrTable[buildCycle++].func = &MOS6510::WasteCycle;
            instrTable[buildCycle++].func = &MOS6510::pla_instr;
            break;

        case PLPn:
            // should read the value at current stack register.
            // Truly side-effect free.
            instrTable[buildCycle++].func = &MOS6510::WasteCycle;
            instrTable[buildCycle++].func = &MOS6510::PopSR;
            instrTable[buildCycle++].func = &MOS6510::plp_instr;
            break;

        case RLAz: case RLAzx: case RLAix: case RLAa: case RLAax: case RLAay:
        case RLAiy:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::rla_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
            break;

        case ROLn:
            instrTable[buildCycle++].func = &MOS6510::rola_instr;
            break;

        case ROLz: case ROLzx: case ROLa: case ROLax:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::rol_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
            break;

        case RORn:
            instrTable[buildCycle++].func = &MOS6510::rora_instr;
            break;

        case RORz: case RORzx: case RORa: case RORax:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::ror_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
            break;

        case RRAa: case RRAax: case RRAay: case RRAz: case RRAzx: case RRAix:
        case RRAiy:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::rra_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
            break;

        case RTIn:
            // should read the value at current stack register.
            // Truly side-effect free.
            instrTable[buildCycle++].func = &MOS6510::WasteCycle;
            instrTable[buildCycle++].func = &MOS6510::PopSR;
            instrTable[buildCycle++].func = &MOS6510::PopLowPC;
            instrTable[buildCycle++].func = &MOS6510::PopHighPC;
            instrTable[buildCycle++].func = &MOS6510::rti_instr;
            break;

        case RTSn:
            // should read the value at current stack register.
            // Truly side-effect free.
            instrTable[buildCycle++].func = &MOS6510::WasteCycle;
            instrTable[buildCycle++].func = &MOS6510::PopLowPC;
            instrTable[buildCycle++].func = &MOS6510::PopHighPC;
            instrTable[buildCycle++].func = &MOS6510::rts_instr;
            break;

        case SAXz: case SAXzy: case SAXa: case SAXix: // Also known as AXS
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::axs_instr;
            break;

        case SBCz:  case SBCzx: case SBCa: case SBCax: case SBCay: case SBCix:
        case SBCiy: case SBCb_:
            instrTable[buildCycle++].func = &MOS6510::sbc_instr;
            break;

        case SBXb:
            instrTable[buildCycle++].func = &MOS6510::sbx_instr;
            break;

        case SECn:
            instrTable[buildCycle++].func = &MOS6510::sec_instr;
            break;

        case SEDn:
            instrTable[buildCycle++].func = &MOS6510::sed_instr;
            break;

        case SEIn:
            instrTable[buildCycle++].func = &MOS6510::sei_instr;
            break;

        case SHAay: case SHAiy: // Also known as AXA
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::axa_instr;
            break;

        case SHSay: // Also known as TAS
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::shs_instr;
            break;

        case SHXay: // Also known as XAS
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::xas_instr;
            break;

        case SHYax: // Also known as SAY
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::say_instr;
            break;

        case SLOz: case SLOzx: case SLOa: case SLOax: case SLOay: case SLOix:
        case SLOiy: // Also known as ASO
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::aso_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
            break;

        case SREz: case SREzx: case SREa: case SREax: case SREay: case SREix:
        case SREiy: // Also known as LSE
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::lse_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
            break;

        case STAz: case STAzx: case STAa: case STAax: case STAay: case STAix:
        case STAiy:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::sta_instr;
            break;

        case STXz: case STXzy: case STXa:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::stx_instr;
            break;

        case STYz: case STYzx: case STYa:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::sty_instr;
            break;

        case TAXn:
            instrTable[buildCycle++].func = &MOS6510::tax_instr;
            break;

        case TAYn:
            instrTable[buildCycle++].func = &MOS6510::tay_instr;
            break;

        case TSXn:
            instrTable[buildCycle++].func = &MOS6510::tsx_instr;
            break;

        case TXAn:
            instrTable[buildCycle++].func = &MOS6510::txa_instr;
            break;

        case TXSn:
            instrTable[buildCycle++].func = &MOS6510::txs_instr;
            break;

        case TYAn:
            instrTable[buildCycle++].func = &MOS6510::tya_instr;
            break;

        default:
            legalInstr = false;
            break;
        }

        // Missing an addressing mode or implementation makes opcode invalid.
        // These are normally called HLT instructions. In the hardware, the
        // CPU state machine locks up and will never recover.
        if (!(legalMode && legalInstr))
        {
            instrTable[buildCycle++].func = &MOS6510::illegal_instr;
        }

        // check for IRQ triggers or fetch next opcode...
        instrTable[buildCycle].func = &MOS6510::interruptsAndNextOpcode;

#if DEBUG > 1
        printf("Done [%d Cycles]\n", buildCycle - (i << 3));
#endif
    }
}

/**
 * Initialise CPU Emulation (Registers).
 */
void MOS6510::Initialise()
{
    // Reset stack
    Register_StackPointer = 0xFF;

    // Reset Cycle Count
    cycleCount = (BRKn << 3) + 6; // fetchNextOpcode

    // Reset Status Register
    flags.reset();

    // Set PC to some value
    Register_ProgramCounter = 0;

    // IRQs pending check
    irqAssertedOnPin = false;
    nmiFlag = false;
    rstFlag = false;
    interruptCycle = MAX;

    // Signals
    rdy = true;

    eventScheduler.schedule(m_nosteal, 0, EVENT_CLOCK_PHI2);
}

/**
 * Reset CPU Emulation.
 */
void MOS6510::reset()
{
    // Internal Stuff
    Initialise();

    // Set processor port to the default values
    cpuWrite(0, 0x2F);
    cpuWrite(1, 0x37);

    // Requires External Bits
    // Read from reset vector for program entry point
    endian_16lo8(Cycle_EffectiveAddress, cpuRead(0xFFFC));
    endian_16hi8(Cycle_EffectiveAddress, cpuRead(0xFFFD));
    Register_ProgramCounter = Cycle_EffectiveAddress;
}

/**
 * Module Credits.
 */
const char *MOS6510::credits()
{
    return
        "MOS6510 Cycle Exact Emulation\n"
        "\t(C) 2000 Simon A. White\n"
        "\t(C) 2008-2010 Antti S. Lankila\n"
        "\t(C) 2011-2016 Leandro Nini\n";
}

void MOS6510::debug(bool enable, FILE *out)
{
#ifdef DEBUG
    dodump = enable;
    if (!(out && enable))
        m_fdbg = stdout;
    else
        m_fdbg = out;
#endif
}

}
