/*
 * Copyright (C) 2021, 2024 nukeykt
 *
 *  Redistribution and use of this code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *   - Redistributions may not be sold, nor may they be used in a commercial
 *     product or activity.
 *
 *   - Redistributions that are modified from the original source must include the
 *     complete source code, including the source code for all components used by a
 *     binary built from the modified sources. However, as a special exception, the
 *     source code distributed need not include anything that is normally distributed
 *     (in either source or binary form) with the major components (compiler, kernel,
 *     and so on) of the operating system on which the executable runs, unless that
 *     component itself accompanies the executable.
 *
 *   - Redistributions must reproduce the above copyright notice, this list of
 *     conditions and the following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include <stdint.h>

#include "state.h"

#include "mcu_interrupt.h"

void MCU_ErrorTrap(struct sc55_state *st);

uint8_t MCU_Read(struct sc55_state *st, uint32_t address);
uint16_t MCU_Read16(struct sc55_state *st, uint32_t address);
uint32_t MCU_Read32(struct sc55_state *st, uint32_t address);
void MCU_Write(struct sc55_state *st, uint32_t address, uint8_t value);
void MCU_Write16(struct sc55_state *st, uint32_t address, uint16_t value);

inline uint32_t MCU_GetAddress(uint8_t page, uint16_t address) {
    return (page << 16) + address;
}

inline uint8_t MCU_ReadCode(struct sc55_state *st) {
    return MCU_Read(st, MCU_GetAddress(st->mcu.cp, st->mcu.pc));
}

inline uint8_t MCU_ReadCodeAdvance(struct sc55_state *st) {
    uint8_t ret = MCU_ReadCode(st);
    st->mcu.pc++;
    return ret;
}

inline void MCU_SetRegisterByte(struct sc55_state *st, uint8_t reg, uint8_t val)
{
    st->mcu.r[reg] = val;
}

inline uint32_t MCU_GetVectorAddress(struct sc55_state *st, uint32_t vector)
{
    return MCU_Read32(st, vector * 4);
}

inline uint32_t MCU_GetPageForRegister(struct sc55_state *st, uint32_t reg)
{
    if (reg >= 6)
        return st->mcu.tp;
    else if (reg >= 4)
        return st->mcu.ep;
    return st->mcu.dp;
}

inline void MCU_ControlRegisterWrite(struct sc55_state *st, uint32_t reg, uint32_t siz, uint32_t data)
{
    if (siz)
    {
        if (reg == 0)
        {
            st->mcu.sr = data;
            st->mcu.sr &= sr_mask;
        }
        else if (reg == 5) // FIXME: undocumented
        {
            st->mcu.dp = data & 0xff;
        }
        else if (reg == 4) // FIXME: undocumented
        {
            st->mcu.ep = data & 0xff;
        }
        else if (reg == 3) // FIXME: undocumented
        {
            st->mcu.br = data & 0xff;
        }
        else
        {
            MCU_ErrorTrap(st);
        }
    }
    else
    {
        if (reg == 1)
        {
            st->mcu.sr &= ~0xff;
            st->mcu.sr |= data & 0xff;
            st->mcu.sr &= sr_mask;
        }
        else if (reg == 3)
        {
            st->mcu.br = data;
        }
        else if (reg == 4)
        {
            st->mcu.ep = data;
        }
        else if (reg == 5)
        {
            st->mcu.dp = data;
        }
        else if (reg == 7)
        {
            st->mcu.tp = data;
        }
        else
        {
            MCU_ErrorTrap(st);
        }
    }
}

inline uint32_t MCU_ControlRegisterRead(struct sc55_state *st, uint32_t reg, uint32_t siz)
{
    uint32_t ret = 0;
    if (siz)
    {
        if (reg == 0)
        {
            ret = st->mcu.sr & sr_mask;
        }
        else if (reg == 5) // FIXME: undocumented
        {
            ret = st->mcu.dp | (st->mcu.dp << 8);
        }
        else if (reg == 4) // FIXME: undocumented
        {
            ret = st->mcu.ep | (st->mcu.ep << 8);
        }
        else if (reg == 3) // FIXME: undocumented
        {
            ret = st->mcu.br | (st->mcu.br << 8);;
        }
        else
        {
            MCU_ErrorTrap(st);
        }
        ret &= 0xffff;
    }
    else
    {
        if (reg == 1)
        {
            ret = st->mcu.sr & sr_mask;
        }
        else if (reg == 3)
        {
            ret = st->mcu.br;
        }
        else if (reg == 4)
        {
            ret = st->mcu.ep;
        }
        else if (reg == 5)
        {
            ret = st->mcu.dp;
        }
        else if (reg == 7)
        {
            ret = st->mcu.tp;
        }
        else
        {
            MCU_ErrorTrap(st);
        }
        ret &= 0xff;
    }
    return ret;
}

inline void MCU_SetStatus(struct sc55_state *st, uint32_t condition, uint32_t mask)
{
    if (condition)
        st->mcu.sr |= mask;
    else
        st->mcu.sr &= ~mask;
}

inline void MCU_PushStack(struct sc55_state *st, uint16_t data)
{
    if (st->mcu.r[7] & 1)
        MCU_Interrupt_Exception(st, EXCEPTION_SOURCE_ADDRESS_ERROR);
    st->mcu.r[7] -= 2;
    MCU_Write16(st, st->mcu.r[7], data);
}

inline uint16_t MCU_PopStack(struct sc55_state *st)
{
    uint16_t ret;
    if (st->mcu.r[7] & 1)
        MCU_Interrupt_Exception(st, EXCEPTION_SOURCE_ADDRESS_ERROR);
    ret = MCU_Read16(st, st->mcu.r[7]);
    st->mcu.r[7] += 2;
    return ret;
}

uint8_t MCU_ReadP0(struct sc55_state *st);
uint8_t MCU_ReadP1(struct sc55_state *st);
void MCU_WriteP0(struct sc55_state *st, uint8_t data);
void MCU_WriteP1(struct sc55_state *st, uint8_t data);
void MCU_GA_SetGAInt(struct sc55_state *st, int line, int value);

void MCU_EncoderTrigger(struct sc55_state *st, int dir);

void MCU_PostSample(struct sc55_state *st, int *sample);
void MCU_PostUART(struct sc55_state *st, uint8_t data);
