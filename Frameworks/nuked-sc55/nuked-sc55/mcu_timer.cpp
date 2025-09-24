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
#include <stdint.h>
#include <string.h>
#include "mcu.h"
#include "mcu_timer.h"

enum {
    REG_TCR = 0x00,
    REG_TCSR = 0x01,
    REG_FRCH = 0x02,
    REG_FRCL = 0x03,
    REG_OCRAH = 0x04,
    REG_OCRAL = 0x05,
    REG_OCRBH = 0x06,
    REG_OCRBL = 0x07,
    REG_ICRH = 0x08,
    REG_ICRL = 0x09,
};

void TIMER_Reset(struct sc55_state *st)
{
    st->timer_cycles = 0;
    st->timer_tempreg = 0;
    memset(st->frt, 0, sizeof(st->frt));
    memset(&st->timer, 0, sizeof(st->timer));
}

void TIMER_Write(struct sc55_state *st, uint32_t address, uint8_t data)
{
    uint32_t t = (address >> 4) - 1;
    if (t > 2)
        return;
    address &= 0x0f;
    frt_t *timer = &st->frt[t];
    switch (address)
    {
    case REG_TCR:
        timer->tcr = data;
        break;
    case REG_TCSR:
        timer->tcsr &= ~0xf;
        timer->tcsr |= data & 0xf;
        if ((data & 0x10) == 0 && (timer->status_rd & 0x10) != 0)
        {
            timer->tcsr &= ~0x10;
            timer->status_rd &= ~0x10;
            MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_FRT0_FOVI + t * 4, 0);
        }
        if ((data & 0x20) == 0 && (timer->status_rd & 0x20) != 0)
        {
            timer->tcsr &= ~0x20;
            timer->status_rd &= ~0x20;
            MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_FRT0_OCIA + t * 4, 0);
        }
        if ((data & 0x40) == 0 && (timer->status_rd & 0x40) != 0)
        {
            timer->tcsr &= ~0x40;
            timer->status_rd &= ~0x40;
            MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_FRT0_OCIB + t * 4, 0);
        }
        break;
    case REG_FRCH:
    case REG_OCRAH:
    case REG_OCRBH:
    case REG_ICRH:
        st->timer_tempreg = data;
        break;
    case REG_FRCL:
        timer->frc = (st->timer_tempreg << 8) | data;
        break;
    case REG_OCRAL:
        timer->ocra = (st->timer_tempreg << 8) | data;
        break;
    case REG_OCRBL:
        timer->ocrb = (st->timer_tempreg << 8) | data;
        break;
    case REG_ICRL:
        timer->icr = (st->timer_tempreg << 8) | data;
        break;
    }
}

uint8_t TIMER_Read(struct sc55_state *st, uint32_t address)
{
    uint32_t t = (address >> 4) - 1;
    if (t > 2)
        return 0xff;
    address &= 0x0f;
    frt_t *timer = &st->frt[t];
    switch (address)
    {
    case REG_TCR:
        return timer->tcr;
    case REG_TCSR:
    {
        uint8_t ret = timer->tcsr;
        timer->status_rd |= timer->tcsr & 0xf0;
        //timer->status_rd |= 0xf0;
        return ret;
    }
    case REG_FRCH:
        st->timer_tempreg = timer->frc & 0xff;
        return timer->frc >> 8;
    case REG_OCRAH:
        st->timer_tempreg = timer->ocra & 0xff;
        return timer->ocra >> 8;
    case REG_OCRBH:
        st->timer_tempreg = timer->ocrb & 0xff;
        return timer->ocrb >> 8;
    case REG_ICRH:
        st->timer_tempreg = timer->icr & 0xff;
        return timer->icr >> 8;
    case REG_FRCL:
    case REG_OCRAL:
    case REG_OCRBL:
    case REG_ICRL:
        return st->timer_tempreg;
    }
    return 0xff;
}

void TIMER2_Write(struct sc55_state *st, uint32_t address, uint8_t data)
{
    switch (address)
    {
    case DEV_TMR_TCR:
        st->timer.tcr = data;
        break;
    case DEV_TMR_TCSR:
        st->timer.tcsr &= ~0xf;
        st->timer.tcsr |= data & 0xf;
        if ((data & 0x20) == 0 && (st->timer.status_rd & 0x20) != 0)
        {
            st->timer.tcsr &= ~0x20;
            st->timer.status_rd &= ~0x20;
            MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_TIMER_OVI, 0);
        }
        if ((data & 0x40) == 0 && (st->timer.status_rd & 0x40) != 0)
        {
            st->timer.tcsr &= ~0x40;
            st->timer.status_rd &= ~0x40;
            MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_TIMER_CMIA, 0);
        }
        if ((data & 0x80) == 0 && (st->timer.status_rd & 0x80) != 0)
        {
            st->timer.tcsr &= ~0x80;
            st->timer.status_rd &= ~0x80;
            MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_TIMER_CMIB, 0);
        }
        break;
    case DEV_TMR_TCORA:
        st->timer.tcora = data;
        break;
    case DEV_TMR_TCORB:
        st->timer.tcorb = data;
        break;
    case DEV_TMR_TCNT:
        st->timer.tcnt = data;
        break;
    }
}

uint8_t TIMER_Read2(struct sc55_state *st, uint32_t address)
{
    switch (address)
    {
    case DEV_TMR_TCR:
        return st->timer.tcr;
    case DEV_TMR_TCSR:
    {
        uint8_t ret = st->timer.tcsr;
        st->timer.status_rd |= st->timer.tcsr & 0xe0;
        return ret;
    }
    case DEV_TMR_TCORA:
        return st->timer.tcora;
    case DEV_TMR_TCORB:
        return st->timer.tcorb;
    case DEV_TMR_TCNT:
        return st->timer.tcnt;
    }
    return 0xff;
}

void TIMER_Clock(struct sc55_state *st, uint64_t cycles)
{
    uint32_t i;
    while (st->timer_cycles*2 < cycles) // FIXME
    {
        for (i = 0; i < 3; i++)
        {
            frt_t *timer = &st->frt[i];
            //uint32_t offset = 0x10 * i;

            switch (timer->tcr & 3)
            {
            case 0: // o / 4
                if (st->timer_cycles & 3)
                    continue;
                break;
            case 1: // o / 8
                if (st->timer_cycles & 7)
                    continue;
                break;
            case 2: // o / 32
                if (st->timer_cycles & 31)
                    continue;
                break;
            case 3: // ext (o / 2)
                if (st->mcu_mk1)
                {
                    if (st->timer_cycles & 3)
                        continue;
                }
                else
                {
                    if (st->timer_cycles & 1)
                        continue;
                }
                break;
            }

            uint32_t value = timer->frc;
            uint32_t matcha = value == timer->ocra;
            uint32_t matchb = value == timer->ocrb;
            if ((timer->tcsr & 1) != 0 && matcha) // CCLRA
                value = 0;
            else
                value++;
            uint32_t of = (value >> 16) & 1;
            value &= 0xffff;
            timer->frc = value;

            // flags
            if (of)
                timer->tcsr |= 0x10;
            if (matcha)
                timer->tcsr |= 0x20;
            if (matchb)
                timer->tcsr |= 0x40;
            if ((timer->tcr & 0x10) != 0 && (timer->tcsr & 0x10) != 0)
                MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_FRT0_FOVI + i * 4, 1);
            if ((timer->tcr & 0x20) != 0 && (timer->tcsr & 0x20) != 0)
                MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_FRT0_OCIA + i * 4, 1);
            if ((timer->tcr & 0x40) != 0 && (timer->tcsr & 0x40) != 0)
                MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_FRT0_OCIB + i * 4, 1);
        }

        int32_t timer_step = 0;

        switch (st->timer.tcr & 7)
        {
        case 0:
        case 4:
            break;
        case 1: // o / 8
            if ((st->timer_cycles & 7) == 0)
                timer_step = 1;
            break;
        case 2: // o / 64
            if ((st->timer_cycles & 63) == 0)
                timer_step = 1;
            break;
        case 3: // o / 1024
            if ((st->timer_cycles & 1023) == 0)
                timer_step = 1;
            break;
        case 5:
        case 6:
        case 7: // ext (o / 2)
            if (st->mcu_mk1)
            {
                if ((st->timer_cycles & 3) == 0)
                    timer_step = 1;
            }
            else
            {
                if ((st->timer_cycles & 1) == 0)
                    timer_step = 1;
            }
            break;
        }
        if (timer_step)
        {
            uint32_t value = st->timer.tcnt;
            uint32_t matcha = value == st->timer.tcora;
            uint32_t matchb = value == st->timer.tcorb;
            if ((st->timer.tcr & 24) == 8 && matcha)
                value = 0;
            else if ((st->timer.tcr & 24) == 16 && matchb)
                value = 0;
            else
                value++;
            uint32_t of = (value >> 8) & 1;
            value &= 0xff;
            st->timer.tcnt = value;

            // flags
            if (of)
                st->timer.tcsr |= 0x20;
            if (matcha)
                st->timer.tcsr |= 0x40;
            if (matchb)
                st->timer.tcsr |= 0x80;
            if ((st->timer.tcr & 0x20) != 0 && (st->timer.tcsr & 0x20) != 0)
                MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_TIMER_OVI, 1);
            if ((st->timer.tcr & 0x40) != 0 && (st->timer.tcsr & 0x40) != 0)
                MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_TIMER_CMIA, 1);
            if ((st->timer.tcr & 0x80) != 0 && (st->timer.tcsr & 0x80) != 0)
                MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_TIMER_CMIB, 1);
        }

        st->timer_cycles++;
    }
}
