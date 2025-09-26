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
#include <stdio.h>
#include <string.h>

#include "mcu.h"
#include "mcu_opcodes.h"
#include "mcu_interrupt.h"
#include "mcu_timer.h"
#include "pcm.h"
#include "lcd.h"
#include "submcu.h"

#if __linux__
#include <unistd.h>
#include <limits.h>
#endif

const char* rs_name[ROM_SET_COUNT] = {
    "SC-55mk2",
    "SC-55st",
    "SC-55mk1",
    "CM-300/SCC-1",
    "JV-880",
    "SCB-55",
    "RLP-3237",
    "SC-155",
    "SC-155mk2"
};

static const int ROM_SET_N_FILES = 6;

const char* roms[ROM_SET_COUNT][ROM_SET_N_FILES] =
{
    "rom1.bin",
    "rom2.bin",
    "waverom1.bin",
    "waverom2.bin",
    "rom_sm.bin",
    "",

    "rom1.bin",
    "rom2_st.bin",
    "waverom1.bin",
    "waverom2.bin",
    "rom_sm.bin",
    "",

    "sc55_rom1.bin",
    "sc55_rom2.bin",
    "sc55_waverom1.bin",
    "sc55_waverom2.bin",
    "sc55_waverom3.bin",
    "",

    "cm300_rom1.bin",
    "cm300_rom2.bin",
    "cm300_waverom1.bin",
    "cm300_waverom2.bin",
    "cm300_waverom3.bin",
    "",

    "jv880_rom1.bin",
    "jv880_rom2.bin",
    "jv880_waverom1.bin",
    "jv880_waverom2.bin",
    "jv880_waverom_expansion.bin",
    "jv880_waverom_pcmcard.bin",

    "scb55_rom1.bin",
    "scb55_rom2.bin",
    "scb55_waverom1.bin",
    "scb55_waverom2.bin",
    "",
    "",

    "rlp3237_rom1.bin",
    "rlp3237_rom2.bin",
    "rlp3237_waverom1.bin",
    "",
    "",
    "",

    "sc155_rom1.bin",
    "sc155_rom2.bin",
    "sc155_waverom1.bin",
    "sc155_waverom2.bin",
    "sc155_waverom3.bin",
    "",

    "rom1.bin",
    "rom2.bin",
    "waverom1.bin",
    "waverom2.bin",
    "rom_sm.bin",
    "",
};

void MCU_ErrorTrap(struct sc55_state *st)
{
    printf("%.2x %.4x\n", st->mcu.cp, st->mcu.pc);
}

uint8_t RCU_Read(struct sc55_state *st)
{
    (void)st;
    return 0;
}

enum {
    ANALOG_LEVEL_RCU_LOW = 0,
    ANALOG_LEVEL_RCU_HIGH = 0,
    ANALOG_LEVEL_SW_0 = 0,
    ANALOG_LEVEL_SW_1 = 0x155,
    ANALOG_LEVEL_SW_2 = 0x2aa,
    ANALOG_LEVEL_SW_3 = 0x3ff,
    ANALOG_LEVEL_BATTERY = 0x2a0,
};

uint16_t MCU_SC155Sliders(struct sc55_state *st, uint32_t index)
{
    (void)st;
    // 0 - 1/9
    // 1 - 2/10
    // 2 - 3/11
    // 3 - 4/12
    // 4 - 5/13
    // 5 - 6/14
    // 6 - 7/15
    // 7 - 8/16
    // 8 - ALL
    return 0x0;
}

uint16_t MCU_AnalogReadPin(struct sc55_state *st, uint32_t pin)
{
    if (st->mcu_cm300)
        return 0;
    if (st->mcu_jv880)
    {
        if (pin == 1)
            return ANALOG_LEVEL_BATTERY;
        return 0x3ff;
    }
    if (0)
    {
READ_RCU:
        uint8_t rcu = RCU_Read(st);
        if (rcu & (1 << pin))
            return ANALOG_LEVEL_RCU_HIGH;
        else
            return ANALOG_LEVEL_RCU_LOW;
    }
    if (st->mcu_mk1)
    {
        if (st->mcu_sc155 && (st->dev_register[DEV_P9DR] & 1) != 0)
        {
            return MCU_SC155Sliders(st, pin);
        }
        if (pin == 7)
        {
            if (st->mcu_sc155 && (st->dev_register[DEV_P9DR] & 2) != 0)
                return MCU_SC155Sliders(st, 8);
            else
                return ANALOG_LEVEL_BATTERY;
        }
        else
            goto READ_RCU;
    }
    else
    {
        if (st->mcu_sc155 && (st->io_sd & 16) != 0)
        {
            return MCU_SC155Sliders(st, pin);
        }
        if (pin == 7)
        {
            if (st->mcu_mk1)
                return ANALOG_LEVEL_BATTERY;
            switch ((st->io_sd >> 2) & 3)
            {
            case 0: // Battery voltage
                return ANALOG_LEVEL_BATTERY;
            case 1: // NC
                if (st->mcu_sc155)
                    return MCU_SC155Sliders(st, 8);
                return 0;
            case 2: // SW
                switch (st->sw_pos)
                {
                case 0:
                default:
                    return ANALOG_LEVEL_SW_0;
                case 1:
                    return ANALOG_LEVEL_SW_1;
                case 2:
                    return ANALOG_LEVEL_SW_2;
                case 3:
                    return ANALOG_LEVEL_SW_3;
                }
            case 3: // RCU
                goto READ_RCU;
            }
        }
        else
            goto READ_RCU;
    }

    // unreachable
    return 0;
}

void MCU_AnalogSample(struct sc55_state *st, int channel)
{
    int value = MCU_AnalogReadPin(st, channel);
    int dest = (channel << 1) & 6;
    st->dev_register[DEV_ADDRAH + dest] = value >> 2;
    st->dev_register[DEV_ADDRAL + dest] = (value << 6) & 0xc0;
}

void MCU_DeviceWrite(struct sc55_state *st, uint32_t address, uint8_t data)
{
    address &= 0x7f;
    if (address >= 0x10 && address < 0x40)
    {
        TIMER_Write(st, address, data);
        return;
    }
    if (address >= 0x50 && address < 0x55)
    {
        TIMER2_Write(st, address, data);
        return;
    }
    switch (address)
    {
    case DEV_P1DDR: // P1DDR
        break;
    case DEV_P5DDR:
        break;
    case DEV_P6DDR:
        break;
    case DEV_P7DDR:
        break;
    case DEV_SCR:
        break;
    case DEV_WCR:
        break;
    case DEV_P9DDR:
        break;
    case DEV_RAME: // RAME
        break;
    case DEV_P1CR: // P1CR
        break;
    case DEV_DTEA:
        break;
    case DEV_DTEB:
        break;
    case DEV_DTEC:
        break;
    case DEV_DTED:
        break;
    case DEV_SMR:
        break;
    case DEV_BRR:
        break;
    case DEV_IPRA:
        break;
    case DEV_IPRB:
        break;
    case DEV_IPRC:
        break;
    case DEV_IPRD:
        break;
    case DEV_PWM1_DTR:
        break;
    case DEV_PWM1_TCR:
        break;
    case DEV_PWM2_DTR:
        break;
    case DEV_PWM2_TCR:
        break;
    case DEV_PWM3_DTR:
        break;
    case DEV_PWM3_TCR:
        break;
    case DEV_P7DR:
        break;
    case DEV_TMR_TCNT:
        break;
    case DEV_TMR_TCR:
        break;
    case DEV_TMR_TCSR:
        break;
    case DEV_TMR_TCORA:
        break;
    case DEV_TDR:
        break;
    case DEV_ADCSR:
    {
        st->dev_register[address] &= ~0x7f;
        st->dev_register[address] |= data & 0x7f;
        if ((data & 0x80) == 0 && st->adf_rd)
        {
            st->dev_register[address] &= ~0x80;
            MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_ANALOG, 0);
        }
        if ((data & 0x40) == 0)
            MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_ANALOG, 0);
        return;
    }
    case DEV_SSR:
    {
        if ((data & 0x80) == 0 && (st->ssr_rd & 0x80) != 0)
        {
            st->dev_register[address] &= ~0x80;
            st->uart_tx_delay = st->mcu.cycles + 3000;
            MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_UART_TX, 0);
        }
        if ((data & 0x40) == 0 && (st->ssr_rd & 0x40) != 0)
        {
            st->uart_rx_delay = st->mcu.cycles + 3000;
            st->dev_register[address] &= ~0x40;
            MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_UART_RX, 0);
        }
        if ((data & 0x20) == 0 && (st->ssr_rd & 0x20) != 0)
        {
            st->dev_register[address] &= ~0x20;
        }
        if ((data & 0x10) == 0 && (st->ssr_rd & 0x10) != 0)
        {
            st->dev_register[address] &= ~0x10;
        }
        break;
    }
    default:
        address += 0;
        break;
    }
    st->dev_register[address] = data;
}

uint8_t MCU_DeviceRead(struct sc55_state *st, uint32_t address)
{
    address &= 0x7f;
    if (address >= 0x10 && address < 0x40)
    {
        return TIMER_Read(st, address);
    }
    if (address >= 0x50 && address < 0x55)
    {
        return TIMER_Read2(st, address);
    }
    switch (address)
    {
    case DEV_ADDRAH:
    case DEV_ADDRAL:
    case DEV_ADDRBH:
    case DEV_ADDRBL:
    case DEV_ADDRCH:
    case DEV_ADDRCL:
    case DEV_ADDRDH:
    case DEV_ADDRDL:
        return st->dev_register[address];
    case DEV_ADCSR:
        st->adf_rd = (st->dev_register[address] & 0x80) != 0;
        return st->dev_register[address];
    case DEV_SSR:
        st->ssr_rd = st->dev_register[address];
        return st->dev_register[address];
    case DEV_RDR:
        return st->uart_rx_byte;
    case 0x00:
        return 0xff;
    case DEV_P7DR:
    {
        if (!st->mcu_jv880) return 0xff;

        uint8_t data = 0xff;
        uint32_t button_pressed = st->mcu_button_pressed;

        if (st->io_sd == 0b11111011)
            data &= ((button_pressed >> 0) & 0b11111) ^ 0xFF;
        if (st->io_sd == 0b11110111)
            data &= ((button_pressed >> 5) & 0b11111) ^ 0xFF;
        if (st->io_sd == 0b11101111)
            data &= ((button_pressed >> 10) & 0b1111) ^ 0xFF;

        data |= 0b10000000;
        return data;
    }
    case DEV_P9DR:
    {
        int cfg = 0;
        if (!st->mcu_mk1)
            cfg = st->mcu_sc155 ? 0 : 2; // bit 1: 0 - SC-155mk2 (???), 1 - SC-55mk2

        int dir = st->dev_register[DEV_P9DDR];

        int val = cfg & (dir ^ 0xff);
        val |= st->dev_register[DEV_P9DR] & dir;
        return val;
    }
    case DEV_SCR:
    case DEV_TDR:
    case DEV_SMR:
        return st->dev_register[address];
    case DEV_IPRC:
    case DEV_IPRD:
    case DEV_DTEC:
    case DEV_DTED:
    case DEV_FRT2_TCSR:
    case DEV_FRT1_TCSR:
    case DEV_FRT1_TCR:
    case DEV_FRT1_FRCH:
    case DEV_FRT1_FRCL:
    case DEV_FRT3_TCSR:
    case DEV_FRT3_OCRAH:
    case DEV_FRT3_OCRAL:
        return st->dev_register[address];
    }
    return st->dev_register[address];
}

void MCU_DeviceReset(struct sc55_state *st)
{
    // dev_register[0x00] = 0x03;
    // dev_register[0x7c] = 0x87;
    st->dev_register[DEV_RAME] = 0x80;
    st->dev_register[DEV_SSR] = 0x80;
}

void MCU_UpdateAnalog(struct sc55_state *st, uint64_t cycles)
{
    int ctrl = st->dev_register[DEV_ADCSR];
    int isscan = (ctrl & 16) != 0;

    if (ctrl & 0x20)
    {
        if (st->analog_end_time == 0)
            st->analog_end_time = cycles + 200;
        else if (st->analog_end_time < cycles)
        {
            if (isscan)
            {
                int base = ctrl & 4;
                for (int i = 0; i <= (ctrl & 3); i++)
                    MCU_AnalogSample(st, base + i);
                st->analog_end_time = cycles + 200;
            }
            else
            {
                MCU_AnalogSample(st, ctrl & 7);
                st->dev_register[DEV_ADCSR] &= ~0x20;
                st->analog_end_time = 0;
            }
            st->dev_register[DEV_ADCSR] |= 0x80;
            if (ctrl & 0x40)
                MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_ANALOG, 1);
        }
    }
    else
        st->analog_end_time = 0;
}

uint8_t MCU_Read(struct sc55_state *st, uint32_t address)
{
    uint32_t address_rom = address & 0x3ffff;
    if (address & 0x80000 && !st->mcu_jv880)
        address_rom |= 0x40000;
    uint8_t page = (address >> 16) & 0xf;
    address &= 0xffff;
    uint8_t ret = 0xff;
    switch (page)
    {
    case 0:
        if (!(address & 0x8000))
            ret = st->rom1[address & 0x7fff];
        else
        {
            if (!st->mcu_mk1)
            {
                uint16_t base = st->mcu_jv880 ? 0xf000 : 0xe000;
                if (address >= base && address < (base | 0x400))
                {
                    ret = PCM_Read(st, address & 0x3f);
                }
                else if (!st->mcu_scb55 && address >= 0xec00 && address < 0xf000)
                {
                    ret = SM_SysRead(st, address & 0xff);
                }
                else if (address >= 0xff80)
                {
                    ret = MCU_DeviceRead(st, address & 0x7f);
                }
                else if (address >= 0xfb80 && address < 0xff80
                    && (st->dev_register[DEV_RAME] & 0x80) != 0)
                    ret = st->ram[(address - 0xfb80) & 0x3ff];
                else if (address >= 0x8000 && address < 0xe000)
                {
                    ret = st->sram[address & 0x7fff];
                }
                else if (address == (base | 0x402))
                {
                    ret = st->ga_int_trigger;
                    st->ga_int_trigger = 0;
                    MCU_Interrupt_SetRequest(st, st->mcu_jv880 ? INTERRUPT_SOURCE_IRQ0 : INTERRUPT_SOURCE_IRQ1, 0);
                }
                else
                {
                    printf("Unknown read %x\n", address);
                    ret = 0xff;
                }
                //
                // e402:2-0 irq source
                //
            }
            else
            {
                if (address >= 0xe000 && address < 0xe040)
                {
                    ret = PCM_Read(st, address & 0x3f);
                }
                else if (address >= 0xff80)
                {
                    ret = MCU_DeviceRead(st, address & 0x7f);
                }
                else if (address >= 0xfb80 && address < 0xff80
                    && (st->dev_register[DEV_RAME] & 0x80) != 0)
                {
                    ret = st->ram[(address - 0xfb80) & 0x3ff];
                }
                else if (address >= 0x8000 && address < 0xe000)
                {
                    ret = st->sram[address & 0x7fff];
                }
                else if (address >= 0xf000 && address < 0xf100)
                {
                    st->io_sd = address & 0xff;

                    if (st->mcu_cm300)
                        return 0xff;

                    LCD_Enable(st, (st->io_sd & 8) != 0);

                    uint8_t data = 0xff;
                    uint32_t button_pressed = st->mcu_button_pressed;

                    if ((st->io_sd & 1) == 0)
                        data &= ((button_pressed >> 0) & 255) ^ 255;
                    if ((st->io_sd & 2) == 0)
                        data &= ((button_pressed >> 8) & 255) ^ 255;
                    if ((st->io_sd & 4) == 0)
                        data &= ((button_pressed >> 16) & 255) ^ 255;
                    if ((st->io_sd & 8) == 0)
                        data &= ((button_pressed >> 24) & 255) ^ 255;
                    return data;
                }
                else if (address == 0xf106)
                {
                    ret = st->ga_int_trigger;
                    st->ga_int_trigger = 0;
                    MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_IRQ1, 0);
                }
                else
                {
                    printf("Unknown read %x\n", address);
                    ret = 0xff;
                }
                //
                // f106:2-0 irq source
                //
            }
        }
        break;
#if 0
    case 3:
        ret = st->rom2[address | 0x30000];
        break;
    case 4:
        ret = st->rom2[address];
        break;
    case 10:
        ret = st->rom2[address | 0x60000]; // FIXME
        break;
    case 1:
        ret = st->rom2[address | 0x10000];
        break;
#endif
    case 1:
        ret = st->rom2[address_rom & st->rom2_mask];
        break;
    case 2:
        ret = st->rom2[address_rom & st->rom2_mask];
        break;
    case 3:
        ret = st->rom2[address_rom & st->rom2_mask];
        break;
    case 4:
        ret = st->rom2[address_rom & st->rom2_mask];
        break;
    case 8:
        if (!st->mcu_jv880)
            ret = st->rom2[address_rom & st->rom2_mask];
        else
            ret = 0xff;
        break;
    case 9:
        if (!st->mcu_jv880)
            ret = st->rom2[address_rom & st->rom2_mask];
        else
            ret = 0xff;
        break;
    case 14:
    case 15:
        if (!st->mcu_jv880)
            ret = st->rom2[address_rom & st->rom2_mask];
        else
            ret = st->cardram[address & 0x7fff]; // FIXME
        break;
    case 10:
    case 11:
        if (!st->mcu_mk1)
            ret = st->sram[address & 0x7fff]; // FIXME
        else
            ret = 0xff;
        break;
    case 12:
    case 13:
        if (st->mcu_jv880)
            ret = st->nvram[address & 0x7fff]; // FIXME
        else
            ret = 0xff;
        break;
    case 5:
        if (st->mcu_mk1)
            ret = st->sram[address & 0x7fff]; // FIXME
        else
            ret = 0xff;
        break;
    default:
        ret = 0x00;
        break;
    }
    return ret;
}

uint16_t MCU_Read16(struct sc55_state *st, uint32_t address)
{
    address &= ~1;
    uint8_t b0, b1;
    b0 = MCU_Read(st, address);
    b1 = MCU_Read(st, address+1);
    return (b0 << 8) + b1;
}

uint32_t MCU_Read32(struct sc55_state *st, uint32_t address)
{
    address &= ~3;
    uint8_t b0, b1, b2, b3;
    b0 = MCU_Read(st, address);
    b1 = MCU_Read(st, address+1);
    b2 = MCU_Read(st, address+2);
    b3 = MCU_Read(st, address+3);
    return (b0 << 24) + (b1 << 16) + (b2 << 8) + b3;
}

void MCU_Write(struct sc55_state *st, uint32_t address, uint8_t value)
{
    uint8_t page = (address >> 16) & 0xf;
    address &= 0xffff;
    if (page == 0)
    {
        if (address & 0x8000)
        {
            if (!st->mcu_mk1)
            {
                uint16_t base = st->mcu_jv880 ? 0xf000 : 0xe000;
                if (address >= (base | 0x400) && address < (base | 0x800))
                {
                    if (address == (base | 0x404) || address == (base | 0x405))
                        LCD_Write(st, address & 1, value);
                    else if (address == (base | 0x401))
                    {
                        st->io_sd = value;
                        LCD_Enable(st, (value & 1) == 0);
                    }
                    else if (address == (base | 0x402))
                        st->ga_int_enable = (value << 1);
                    else
                        printf("Unknown write %x %x\n", address, value);
                    //
                    // e400: always 4?
                    // e401: SC0-6?
                    // e402: enable/disable IRQ?
                    // e403: always 1?
                    // e404: LCD
                    // e405: LCD
                    // e406: 0 or 40
                    // e407: 0, e406 continuation?
                    //
                }
                else if (address >= (base | 0x000) && address < (base | 0x400))
                {
                    PCM_Write(st, address & 0x3f, value);
                }
                else if (!st->mcu_scb55 && address >= 0xec00 && address < 0xf000)
                {
                    SM_SysWrite(st, address & 0xff, value);
                }
                else if (address >= 0xff80)
                {
                    MCU_DeviceWrite(st, address & 0x7f, value);
                }
                else if (address >= 0xfb80 && address < 0xff80
                    && (st->dev_register[DEV_RAME] & 0x80) != 0)
                {
                    st->ram[(address - 0xfb80) & 0x3ff] = value;
                }
                else if (address >= 0x8000 && address < 0xe000)
                {
                    st->sram[address & 0x7fff] = value;
                }
                else
                {
                    printf("Unknown write %x %x\n", address, value);
                }
            }
            else
            {
                if (address >= 0xe000 && address < 0xe040)
                {
                    PCM_Write(st, address & 0x3f, value);
                }
                else if (address >= 0xff80)
                {
                    MCU_DeviceWrite(st, address & 0x7f, value);
                }
                else if (address >= 0xfb80 && address < 0xff80
                    && (st->dev_register[DEV_RAME] & 0x80) != 0)
                {
                    st->ram[(address - 0xfb80) & 0x3ff] = value;
                }
                else if (address >= 0x8000 && address < 0xe000)
                {
                    st->sram[address & 0x7fff] = value;
                }
                else if (address >= 0xf000 && address < 0xf100)
                {
                    st->io_sd = address & 0xff;
                    LCD_Enable(st, (st->io_sd & 8) != 0);
                }
                else if (address == 0xf105)
                {
                    LCD_Write(st, 0, value);
                    st->ga_lcd_counter = 500;
                }
                else if (address == 0xf104)
                {
                    LCD_Write(st, 1, value);
                    st->ga_lcd_counter = 500;
                }
                else if (address == 0xf107)
                {
                    st->io_sd = value;
                }
                else
                {
                    printf("Unknown write %x %x\n", address, value);
                }
            }
        }
        else if (st->mcu_jv880 && address >= 0x6196 && address <= 0x6199)
        {
            // nop: the jv880 rom writes into the rom at 002E77-002E7D
        }
        else
        {
            printf("Unknown write %x %x\n", address, value);
        }
    }
    else if (page == 5 && st->mcu_mk1)
    {
        st->sram[address & 0x7fff] = value; // FIXME
    }
    else if (page == 10 && !st->mcu_mk1)
    {
        st->sram[address & 0x7fff] = value; // FIXME
    }
    else if (page == 12 && st->mcu_jv880)
    {
        st->nvram[address & 0x7fff] = value; // FIXME
    }
    else if (page == 14 && st->mcu_jv880)
    {
        st->cardram[address & 0x7fff] = value; // FIXME
    }
    else
    {
        printf("Unknown write %x %x\n", (page << 16) | address, value);
    }
}

void MCU_Write16(struct sc55_state *st, uint32_t address, uint16_t value)
{
    address &= ~1;
    MCU_Write(st, address, value >> 8);
    MCU_Write(st, address + 1, value & 0xff);
}

void MCU_ReadInstruction(struct sc55_state *st)
{
    uint8_t operand = MCU_ReadCodeAdvance(st);

    MCU_Operand_Table[operand](st, operand);

    if (st->mcu.sr & STATUS_T)
    {
        MCU_Interrupt_Exception(st, EXCEPTION_SOURCE_TRACE);
    }
}

void MCU_Init(struct sc55_state *st)
{
    memset(&st->mcu, 0, sizeof(mcu_t));
}

void MCU_Reset(struct sc55_state *st)
{
    st->mcu.r[0] = 0;
    st->mcu.r[1] = 0;
    st->mcu.r[2] = 0;
    st->mcu.r[3] = 0;
    st->mcu.r[4] = 0;
    st->mcu.r[5] = 0;
    st->mcu.r[6] = 0;
    st->mcu.r[7] = 0;

    st->mcu.pc = 0;

    st->mcu.sr = 0x700;

    st->mcu.cp = 0;
    st->mcu.dp = 0;
    st->mcu.ep = 0;
    st->mcu.tp = 0;
    st->mcu.br = 0;

    uint32_t reset_address = MCU_GetVectorAddress(st, VECTOR_RESET);
    st->mcu.cp = (reset_address >> 16) & 0xff;
    st->mcu.pc = reset_address & 0xffff;

    st->mcu.exception_pending = -1;

    MCU_DeviceReset(st);

    if (st->mcu_mk1)
    {
        st->ga_int_enable = 255;
    }
}

void MCU_PostUART(struct sc55_state *st, uint8_t data)
{
    st->uart_buffer[st->uart_write_ptr] = data;
    st->uart_write_ptr = (st->uart_write_ptr + 1) % uart_buffer_size;
}

void MCU_UpdateUART_RX(struct sc55_state *st)
{
    if ((st->dev_register[DEV_SCR] & 16) == 0) // RX disabled
        return;
    if (st->uart_write_ptr == st->uart_read_ptr) // no byte
        return;

    if (st->dev_register[DEV_SSR] & 0x40)
        return;

    if (st->mcu.cycles < st->uart_rx_delay)
        return;

    st->uart_rx_byte = st->uart_buffer[st->uart_read_ptr];
    st->uart_read_ptr = (st->uart_read_ptr + 1) % uart_buffer_size;
    st->dev_register[DEV_SSR] |= 0x40;
    MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_UART_RX, (st->dev_register[DEV_SCR] & 0x40) != 0);
}

// dummy TX
void MCU_UpdateUART_TX(struct sc55_state *st)
{
    if ((st->dev_register[DEV_SCR] & 32) == 0) // TX disabled
        return;

    if (st->dev_register[DEV_SSR] & 0x80)
        return;

    if (st->mcu.cycles < st->uart_tx_delay)
        return;

    st->dev_register[DEV_SSR] |= 0x80;
    MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_UART_TX, (st->dev_register[DEV_SCR] & 0x80) != 0);

    // printf("tx:%x\n", dev_register[DEV_TDR]);
}

static void MCU_Work(struct sc55_state *st) {
	sc55_push_lcd lcdCallback = st->lcdCallback;
	void *lcdContext = st->lcdContext;
	const int port = st->port;

    while (st->sample_buffer_requested && st->sample_buffer_count < st->audio_buffer_size)
    {
        if (st->pcm.config_reg_3c & 0x40)
            st->sample_write_ptr &= ~3;
        else
            st->sample_write_ptr &= ~1;

        if (st->sample_buffer_count > 0)
        {
            if (st->sample_buffer_count >= st->sample_buffer_requested)
            {
                st->sample_buffer_requested = 0;
                break;
            }
        }

        if (!st->mcu.ex_ignore)
            MCU_Interrupt_Handle(st);
        else
            st->mcu.ex_ignore = 0;

        if (!st->mcu.sleep)
            MCU_ReadInstruction(st);

        st->mcu.cycles += 12; // FIXME: assume 12 cycles per instruction

        // if (mcu.cycles % 24000000 == 0)
        //     printf("seconds: %i\n", (int)(mcu.cycles / 24000000));

		uint32_t samplesLast = st->sample_buffer_count;
        PCM_Update(st, st->mcu.cycles);
		if (samplesLast != st->sample_buffer_count)
		{
			uint32_t samplesDone = st->sample_buffer_count - samplesLast;
			st->sample_counter += samplesDone;
		}

		if (lcdCallback && memcmp(&st->lcd.state, &st->lcd.lastState, sizeof(st->lcd.state)) != 0)
		{
			st->lcd.lastState = st->lcd.state;

			uint64_t lcdClock = (st->sample_counter * 1000 + st->sample_rate / 2) / st->sample_rate;
			lcdCallback(lcdContext, port, &st->lcd.state, sizeof(st->lcd.state), lcdClock);
		}

        TIMER_Clock(st, st->mcu.cycles);

        if (!st->mcu_mk1 && !st->mcu_jv880 && !st->mcu_scb55)
            SM_Update(st, st->mcu.cycles);
        else
        {
            MCU_UpdateUART_RX(st);
            MCU_UpdateUART_TX(st);
        }

        MCU_UpdateAnalog(st, st->mcu.cycles);

        if (st->mcu_mk1)
        {
            if (st->ga_lcd_counter)
            {
                st->ga_lcd_counter--;
                if (st->ga_lcd_counter == 0)
                {
                    MCU_GA_SetGAInt(st, 1, 0);
                    MCU_GA_SetGAInt(st, 1, 1);
                }
            }
        }
    }
}

void MCU_PatchROM(struct sc55_state *st)
{
    //st->rom2[0x1333] = 0x11;
    //st->rom2[0x1334] = 0x19;
    //st->rom1[0x622d] = 0x19;
}

uint8_t MCU_ReadP0(struct sc55_state *st)
{
    (void)st;
    return 0xff;
}

uint8_t MCU_ReadP1(struct sc55_state *st)
{
    uint8_t data = 0xff;
    uint32_t button_pressed = st->mcu_button_pressed;

    if ((st->mcu_p0_data & 1) == 0)
        data &= ((button_pressed >> 0) & 255) ^ 255;
    if ((st->mcu_p0_data & 2) == 0)
        data &= ((button_pressed >> 8) & 255) ^ 255;
    if ((st->mcu_p0_data & 4) == 0)
        data &= ((button_pressed >> 16) & 255) ^ 255;
    if ((st->mcu_p0_data & 8) == 0)
        data &= ((button_pressed >> 24) & 255) ^ 255;

    return data;
}

void MCU_WriteP0(struct sc55_state *st, uint8_t data)
{
    st->mcu_p0_data = data;
}

void MCU_WriteP1(struct sc55_state *st, uint8_t data)
{
    st->mcu_p1_data = data;
}

void unscramble(uint8_t *src, uint8_t *dst, int len)
{
    for (int i = 0; i < len; i++)
    {
        int address = i & ~0xfffff;
        static const int aa[] = {
            2, 0, 3, 4, 1, 9, 13, 10, 18, 17, 6, 15, 11, 16, 8, 5, 12, 7, 14, 19
        };
        for (int j = 0; j < 20; j++)
        {
            if (i & (1 << j))
                address |= 1<<aa[j];
        }
        uint8_t srcdata = src[address];
        uint8_t data = 0;
        static const int dd[] = {
            2, 0, 4, 5, 7, 6, 3, 1
        };
        for (int j = 0; j < 8; j++)
        {
            if (srcdata & (1 << dd[j]))
                data |= 1<<j;
        }
        dst[i] = data;
    }
}

int MCU_OpenAudio(struct sc55_state *st, int pageSize, int pageNum)
{
    st->audio_page_size = (pageSize/2)*2; // must be even
    st->audio_buffer_size = st->audio_page_size*pageNum;

    st->sample_rate = (st->mcu_mk1 || st->mcu_jv880) ? 64000 : 66207;
    
    st->sample_buffer = (short*)calloc(st->audio_buffer_size, sizeof(short));
    if (!st->sample_buffer)
    {
        printf("Cannot allocate audio buffer.\n");
        return 0;
    }
    st->sample_read_ptr = 0;
    st->sample_write_ptr = 0;
    st->sample_buffer_count = 0;
    
    return 1;
}

void MCU_CloseAudio(struct sc55_state *st)
{
    if (st->sample_buffer) {
        free(st->sample_buffer);
        st->sample_buffer = NULL;
    }
}

void MCU_PostSample(struct sc55_state *st, int *sample)
{
    if(st->sample_buffer_count >= st->audio_buffer_size)
        return;

    sample[0] >>= 15;
    if (sample[0] > INT16_MAX)
        sample[0] = INT16_MAX;
    else if (sample[0] < INT16_MIN)
        sample[0] = INT16_MIN;
    sample[1] >>= 15;
    if (sample[1] > INT16_MAX)
        sample[1] = INT16_MAX;
    else if (sample[1] < INT16_MIN)
        sample[1] = INT16_MIN;
    st->sample_buffer[st->sample_write_ptr + 0] = sample[0];
    st->sample_buffer[st->sample_write_ptr + 1] = sample[1];
    st->sample_write_ptr = (st->sample_write_ptr + 2) % st->audio_buffer_size;
    st->sample_buffer_count++;
}

void MCU_GA_SetGAInt(struct sc55_state *st, int line, int value)
{
    // guesswork
    if (value && !st->ga_int[line] && (st->ga_int_enable & (1 << line)) != 0)
        st->ga_int_trigger = line;
    st->ga_int[line] = value;

    if (st->mcu_jv880)
        MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_IRQ0, st->ga_int_trigger != 0);
    else
        MCU_Interrupt_SetRequest(st, INTERRUPT_SOURCE_IRQ1, st->ga_int_trigger != 0);
}

void MCU_EncoderTrigger(struct sc55_state *st, int dir)
{
    if (!st->mcu_jv880) return;
    MCU_GA_SetGAInt(st, dir == 0 ? 3 : 4, 0);
    MCU_GA_SetGAInt(st, dir == 0 ? 3 : 4, 1);
}

void MIDI_Reset(struct sc55_state *st, enum ResetType resetType)
{
    const unsigned char gmReset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
    const unsigned char gsReset[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
    
    if (resetType == ResetType::GS_RESET)
    {
        for (size_t i = 0; i < sizeof(gsReset); i++)
        {
            MCU_PostUART(st, gsReset[i]);
        }
    }
    else  if (resetType == ResetType::GM_RESET)
    {
        for (size_t i = 0; i < sizeof(gmReset); i++)
        {
            MCU_PostUART(st, gmReset[i]);
        }
    }

}

struct sc55_state * sc55_init(int port, enum ResetType resetType, sc55_read_rom readCallback, void *readContext)
{
    struct sc55_state *st = (struct sc55_state *) calloc(1, sizeof(*st));
    if (!st)
        return NULL;

    // static init
    st->sw_pos = 3;
    st->rom2_mask = ROM2_SIZE - 1;
    st->lcd.lcd_width = 741;
    st->lcd.lcd_height = 268;
    st->lcd.lcd_col1 = 0x000000;
    st->lcd.lcd_col2 = 0x0050c8;

    st->lcd.state.lcd_enable = 1;

	st->port = port;

    const int pageSize = 512;
    const int pageNum = 8;
    bool autodetect = true;

    st->romset = ROM_SET_MK2;

    if (autodetect)
    {
        bool good = true;
        for (size_t i = 0; i < ROM_SET_COUNT; i++)
        {
            good = true;
            for (size_t j = 0; j < 5; j++)
            {
                if (roms[i][j][0] == '\0')
                    continue;
                int r = readCallback(readContext, roms[i][j], NULL, NULL);
                if (r < 0)
                {
                    good = false;
                    break;
                }
            }
            if (good)
            {
                st->romset = (int)i;
                break;
            }
        }
        if (!good) {
            free(st);
            return nullptr;
        }
        printf("ROM set autodetect: %s\n", rs_name[st->romset]);
    }

    st->mcu_mk1 = false;
    st->mcu_cm300 = false;
    st->mcu_st = false;
    st->mcu_jv880 = false;
    st->mcu_scb55 = false;
    st->mcu_sc155 = false;
    switch (st->romset)
    {
        case ROM_SET_MK2:
        case ROM_SET_SC155MK2:
            if (st->romset == ROM_SET_SC155MK2)
                st->mcu_sc155 = true;
            break;
        case ROM_SET_ST:
            st->mcu_st = true;
            break;
        case ROM_SET_MK1:
        case ROM_SET_SC155:
            st->mcu_mk1 = true;
            st->mcu_st = false;
            if (st->romset == ROM_SET_SC155)
                st->mcu_sc155 = true;
            break;
        case ROM_SET_CM300:
            st->mcu_mk1 = true;
            st->mcu_cm300 = true;
            break;
        case ROM_SET_JV880:
            st->mcu_jv880 = true;
            st->rom2_mask /= 2; // rom is half the size
            st->lcd.lcd_width = 820;
            st->lcd.lcd_height = 100;
            st->lcd.lcd_col1 = 0x000000;
            st->lcd.lcd_col2 = 0x78b500;
            break;
        case ROM_SET_SCB55:
        case ROM_SET_RLP3237:
            st->mcu_scb55 = true;
            break;
    }

    /*if (LCD_SetBack(st, "back.data", readCallback, readContext) != 0)
    {
        free(st);
        return nullptr;
    }*/

    memset(&st->mcu, 0, sizeof(mcu_t));

    uint32_t size = ROM1_SIZE;
    if (readCallback(readContext, roms[st->romset][0], st->rom1, &size) < 0 || size != ROM1_SIZE)
    {
        fprintf(stderr, "FATAL ERROR: Failed to read the mcu ROM1.\n");
        fflush(stderr);
        free(st);
        return nullptr;
    }

    uint32_t rom2_read = ROM2_SIZE;
    int readErr = readCallback(readContext, roms[st->romset][1], st->rom2, &rom2_read);

    if (!readErr && (rom2_read == ROM2_SIZE || rom2_read == ROM2_SIZE / 2))
    {
        st->rom2_mask = rom2_read - 1;
    }
    else
    {
        fprintf(stderr, "FATAL ERROR: Failed to read the mcu ROM2.\n");
        fflush(stderr);
        free(st);
        return nullptr;
    }

    uint8_t *tempbuf = nullptr;

    tempbuf = (uint8_t *) malloc(0x800000);
    if (!tempbuf) {
        free(st);
        return nullptr;
    }

    if (st->mcu_mk1)
    {
        size = 0x100000;
        if (readCallback(readContext, roms[st->romset][2], tempbuf, &size) < 0 || size != 0x100000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom1.\n");
            fflush(stderr);
            free(tempbuf);
            free(st);
            return nullptr;
        }

        unscramble(tempbuf, st->waverom1, 0x100000);

        if (readCallback(readContext, roms[st->romset][3], tempbuf, &size) < 0 || size != 0x100000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom2.\n");
            fflush(stderr);
            free(tempbuf);
            free(st);
            return nullptr;
        }

        unscramble(tempbuf, st->waverom2, 0x100000);

        if (readCallback(readContext, roms[st->romset][4], tempbuf, &size) < 0 || size != 0x100000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom3.\n");
            fflush(stderr);
            free(tempbuf);
            free(st);
            return nullptr;
        }

        unscramble(tempbuf, st->waverom3, 0x100000);
    }
    else if (st->mcu_jv880)
    {
        size = 0x200000;
        if (readCallback(readContext, roms[st->romset][2], tempbuf, &size) < 0 || size != 0x200000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom1.\n");
            fflush(stderr);
            free(tempbuf);
            free(st);
            return nullptr;
        }

        unscramble(tempbuf, st->waverom1, 0x200000);

        if (readCallback(readContext, roms[st->romset][3], tempbuf, &size) < 0 || size != 0x200000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom2.\n");
            fflush(stderr);
            free(tempbuf);
            free(st);
            return nullptr;
        }

        unscramble(tempbuf, st->waverom2, 0x200000);
        
        size = 0x800000;
        if (readCallback(readContext, roms[st->romset][4], tempbuf, &size) == 0)
            unscramble(tempbuf, st->waverom_exp, 0x800000);
        else
            printf("WaveRom EXP not found, skipping it.\n");
        
        size = 0x200000;
        if (readCallback(readContext, roms[st->romset][5], tempbuf, &size) == 0)
            unscramble(tempbuf, st->waverom_card, 0x200000);
        else
            printf("WaveRom PCM not found, skipping it.\n");
    }
    else
    {
        size = 0x200000;
        if (readCallback(readContext, roms[st->romset][2], tempbuf, &size) < 0 || size != 0x200000)
        {
            fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom1.\n");
            fflush(stderr);
            free(tempbuf);
            free(st);
            return nullptr;
        }

        unscramble(tempbuf, st->waverom1, 0x200000);

        if (roms[st->romset][3][0])
        {
            size = 0x100000;
            if (readCallback(readContext, roms[st->romset][3], tempbuf, &size) < 0 || size != 0x100000)
            {
                fprintf(stderr, "FATAL ERROR: Failed to read the WaveRom2.\n");
                fflush(stderr);
                free(tempbuf);
                free(st);
                return nullptr;
            }

            unscramble(tempbuf, st->mcu_scb55 ? st->waverom3 : st->waverom2, 0x100000);
        }

        if (roms[st->romset][4][0])
        {
            size = ROMSM_SIZE;
            if (readCallback(readContext, roms[st->romset][4], st->sm_rom, &size) < 0 || size != ROMSM_SIZE)
            {
                fprintf(stderr, "FATAL ERROR: Failed to read the sub mcu ROM.\n");
                fflush(stderr);
                free(tempbuf);
                free(st);
                return nullptr;
            }
        }
    }

    free(tempbuf);
    tempbuf = nullptr;

    if (!MCU_OpenAudio(st, pageSize, pageNum))
    {
        fprintf(stderr, "FATAL ERROR: Failed to open the audio stream.\n");
        fflush(stderr);
        free(st);
        return nullptr;
    }

    LCD_Init(st);
    MCU_Init(st);
    MCU_PatchROM(st);
    MCU_Reset(st);
    SM_Reset(st);
    PCM_Reset(st);

    if (resetType != ResetType::NONE) MIDI_Reset(st, resetType);

    return st;
}

void sc55_free(struct sc55_state *st)
{
    MCU_CloseAudio(st);
    //LCD_UnInit(st);

    free(st);
}

static void sc55_read_samples(struct sc55_state *st, short *buffer, uint32_t count)
{
    while(count && st->sample_buffer_count)
    {
        size_t countToDo = (st->audio_buffer_size - st->sample_read_ptr) / 2;
        if (countToDo > st->sample_buffer_count)
            countToDo = st->sample_buffer_count;
        if (countToDo > count)
            countToDo = count;
        memcpy(buffer, &st->sample_buffer[st->sample_read_ptr], countToDo * sizeof(short) * 2);
		buffer += countToDo * 2;
        st->sample_read_ptr = (st->sample_read_ptr + countToDo * 2) % st->audio_buffer_size;
        st->sample_buffer_count -= countToDo;
    }
}

void sc55_render(struct sc55_state *st, short *buffer, uint32_t count)
{
	sc55_render_with_lcd(st, buffer, count, NULL, NULL);
}

void sc55_render_with_lcd(struct sc55_state *st, short *buffer, uint32_t count, sc55_push_lcd lcdCallback, void *lcdContext)
{
	st->lcdCallback = lcdCallback;
	st->lcdContext = lcdContext;

    uint32_t countDone = 0;

    while (countDone < count)
    {
        uint32_t countToDo = count - countDone;

        if (st->sample_buffer_count)
        {
            if (st->sample_buffer_count >= countToDo)
            {
                sc55_read_samples(st, buffer, count);
				break;
            }
            else
            {
                uint32_t savedCount = st->sample_buffer_count;
                sc55_read_samples(st, buffer, savedCount);
                buffer += savedCount * 2;
                countDone += savedCount;
            }
        }

        countToDo = count - countDone;
        if (countToDo)
        {
            if (countToDo > st->audio_page_size)
                countToDo = st->audio_page_size;
            st->sample_buffer_requested = countToDo;
            MCU_Work(st);
        }
    }

	// Clear these
	st->lcdCallback = NULL;
	st->lcdContext = NULL;
}

void sc55_write_uart(struct sc55_state *st, const uint8_t *data, uint32_t count)
{
	for (uint32_t i = 0; i < count; ++i)
	{
		MCU_PostUART(st, data[i]);
	}
}

uint32_t sc55_get_sample_rate(struct sc55_state *st)
{
	return st->sample_rate;
}

void sc55_spin(struct sc55_state *st, uint32_t count)
{
	st->lcdCallback = NULL;
	st->lcdContext = NULL;

	while(count > 0)
	{
		uint32_t countToDo = st->audio_page_size;
		if(countToDo > count)
			countToDo = count;
		st->sample_buffer_requested = countToDo;
		MCU_Work(st);
		st->sample_buffer_count = 0;
		st->sample_counter = 0;
		count -= countToDo;
	}
}

uint32_t sc55_lcd_state_size()
{
	return sizeof(lcd_state_t);
}

void sc55_lcd_clear(void *lcdState, size_t stateSize, uint32_t width, uint32_t height)
{
	if (stateSize != sizeof(lcd_state_t)) return;

	memset(lcdState, 0, stateSize);

	struct lcd_state_t *st = (struct lcd_state_t *)lcdState;
	st->lcd_width = width;
	st->lcd_height = height;
}

void sc55_lcd_get_size(const struct sc55_state *st, uint32_t *width, uint32_t *height)
{
	if (width) *width = st->lcd.lcd_width;
	if (height) *height = st->lcd.lcd_height;
}

void sc55_lcd_render_screen(const lcd_background_t lcd_background, lcd_buffer_t lcd_buffer, const void *lcdState, size_t stateSize)
{
	if (stateSize != sizeof(lcd_state_t)) return;

	const struct lcd_state_t *state = (const struct lcd_state_t *)lcdState;

	LCD_Update(state, lcd_background, lcd_buffer);
}
