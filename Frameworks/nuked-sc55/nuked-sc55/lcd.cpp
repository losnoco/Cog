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
#include <stdio.h>
#include <string.h>
#include "lcd.h"
#include "lcd_font.h"
#include "mcu.h"
#include "submcu.h"

void LCD_Enable(struct sc55_state *st, uint32_t enable)
{
    st->lcd.state.lcd_enable = enable;
}

/*bool LCD_QuitRequested(struct sc55_state *st)
{
    return st->lcd_quit_requested;
}*/

void LCD_Write(struct sc55_state *sc, uint32_t address, uint8_t data)
{
    struct lcd_state_t *st = &sc->lcd.state;
    if (address == 0)
    {
        if ((data & 0xe0) == 0x20)
        {
            st->LCD_DL = (data & 0x10) != 0;
            st->LCD_N = (data & 0x8) != 0;
            st->LCD_F = (data & 0x4) != 0;
        }
        else if ((data & 0xf8) == 0x8)
        {
            st->LCD_D = (data & 0x4) != 0;
            st->LCD_C = (data & 0x2) != 0;
            st->LCD_B = (data & 0x1) != 0;
        }
        else if ((data & 0xff) == 0x01)
        {
            st->LCD_DD_RAM = 0;
            st->LCD_ID = 1;
            memset(st->LCD_Data, 0x20, sizeof(st->LCD_Data));
        }
        else if ((data & 0xff) == 0x02)
        {
            st->LCD_DD_RAM = 0;
        }
        else if ((data & 0xfc) == 0x04)
        {
            st->LCD_ID = (data & 0x2) != 0;
            st->LCD_S = (data & 0x1) != 0;
        }
        else if ((data & 0xc0) == 0x40)
        {
            st->LCD_CG_RAM = (data & 0x3f);
            st->LCD_RAM_MODE = 0;
        }
        else if ((data & 0x80) == 0x80)
        {
            st->LCD_DD_RAM = (data & 0x7f);
            st->LCD_RAM_MODE = 1;
        }
        /*else
        {
            address += 0;
        }*/
    }
    else
    {
        if (!st->LCD_RAM_MODE)
        {
            st->LCD_CG[st->LCD_CG_RAM] = data & 0x1f;
            if (st->LCD_ID)
            {
                st->LCD_CG_RAM++;
            }
            else
            {
                st->LCD_CG_RAM--;
            }
            st->LCD_CG_RAM &= 0x3f;
        }
        else
        {
            if (st->LCD_N)
            {
                if (st->LCD_DD_RAM & 0x40)
                {
                    if ((st->LCD_DD_RAM & 0x3f) < 40)
                        st->LCD_Data[(st->LCD_DD_RAM & 0x3f) + 40] = data;
                }
                else
                {
                    if ((st->LCD_DD_RAM & 0x3f) < 40)
                        st->LCD_Data[st->LCD_DD_RAM & 0x3f] = data;
                }
            }
            else
            {
                if (st->LCD_DD_RAM < 80)
                    st->LCD_Data[st->LCD_DD_RAM] = data;
            }
            if (st->LCD_ID)
            {
                st->LCD_DD_RAM++;
            }
            else
            {
                st->LCD_DD_RAM--;
            }
            st->LCD_DD_RAM &= 0x7f;
        }
    }
    //printf("%i %.2x ", address, data);
    // if (data >= 0x20 && data <= 'z')
    //     printf("%c\n", data);
    //else
    //    printf("\n");
}

#if 0
const int button_map_sc55[][2] =
{
    SDL_SCANCODE_Q, MCU_BUTTON_POWER,
    SDL_SCANCODE_W, MCU_BUTTON_INST_ALL,
    SDL_SCANCODE_E, MCU_BUTTON_INST_MUTE,
    SDL_SCANCODE_R, MCU_BUTTON_PART_L,
    SDL_SCANCODE_T, MCU_BUTTON_PART_R,
    SDL_SCANCODE_Y, MCU_BUTTON_INST_L,
    SDL_SCANCODE_U, MCU_BUTTON_INST_R,
    SDL_SCANCODE_I, MCU_BUTTON_KEY_SHIFT_L,
    SDL_SCANCODE_O, MCU_BUTTON_KEY_SHIFT_R,
    SDL_SCANCODE_P, MCU_BUTTON_LEVEL_L,
    SDL_SCANCODE_LEFTBRACKET, MCU_BUTTON_LEVEL_R,
    SDL_SCANCODE_A, MCU_BUTTON_MIDI_CH_L,
    SDL_SCANCODE_S, MCU_BUTTON_MIDI_CH_R,
    SDL_SCANCODE_D, MCU_BUTTON_PAN_L,
    SDL_SCANCODE_F, MCU_BUTTON_PAN_R,
    SDL_SCANCODE_G, MCU_BUTTON_REVERB_L,
    SDL_SCANCODE_H, MCU_BUTTON_REVERB_R,
    SDL_SCANCODE_J, MCU_BUTTON_CHORUS_L,
    SDL_SCANCODE_K, MCU_BUTTON_CHORUS_R,
    SDL_SCANCODE_LEFT, MCU_BUTTON_PART_L,
    SDL_SCANCODE_RIGHT, MCU_BUTTON_PART_R,
};

const int button_map_jv880[][2] =
{
    SDL_SCANCODE_P, MCU_BUTTON_PREVIEW,
    SDL_SCANCODE_LEFT, MCU_BUTTON_CURSOR_L,
    SDL_SCANCODE_RIGHT, MCU_BUTTON_CURSOR_R,
    SDL_SCANCODE_TAB, MCU_BUTTON_DATA,
    SDL_SCANCODE_Q, MCU_BUTTON_TONE_SELECT,
    SDL_SCANCODE_A, MCU_BUTTON_PATCH_PERFORM,
    SDL_SCANCODE_W, MCU_BUTTON_EDIT,
    SDL_SCANCODE_E, MCU_BUTTON_SYSTEM,
    SDL_SCANCODE_R, MCU_BUTTON_RHYTHM,
    SDL_SCANCODE_T, MCU_BUTTON_UTILITY,
    SDL_SCANCODE_S, MCU_BUTTON_MUTE,
    SDL_SCANCODE_D, MCU_BUTTON_MONITOR,
    SDL_SCANCODE_F, MCU_BUTTON_COMPARE,
    SDL_SCANCODE_G, MCU_BUTTON_ENTER,
};
#endif

#if 0
int LCD_SetBack(struct sc55_state *st, const char *name, sc55_read_rom readCallback, void *readContext)
{
    struct lcd_t *state = &st->lcd;
    uint32_t size = sizeof(state->lcd_background);
    return readCallback(readContext, name, (uint8_t *)state->lcd_background, &size);
}
#endif

void LCD_Init(struct sc55_state *sc)
{
    struct lcd_t *lcd = &sc->lcd;
    struct lcd_state_t *st = &lcd->state;

    st->lcd_width = lcd->lcd_width;
    st->lcd_height = lcd->lcd_height;

    st->lcd_col1 = lcd->lcd_col1;
    st->lcd_col2 = lcd->lcd_col2;

    st->mcu_mk1 = (uint8_t) sc->mcu_mk1;
    st->mcu_cm300 = (uint8_t) sc->mcu_cm300;
    st->mcu_st = (uint8_t) sc->mcu_st;
    st->mcu_jv880 = (uint8_t) sc->mcu_jv880;
    st->mcu_scb55 = (uint8_t) sc->mcu_scb55;
    st->mcu_sc155 = (uint8_t) sc->mcu_sc155;
}

/*
void LCD_UnInit(struct sc55_state *st)
{
}
*/

void LCD_FontRenderStandard(const struct lcd_state_t *st, lcd_buffer_t lcd_buffer, int32_t x, int32_t y, uint8_t ch, bool overlay = false)
{
    const uint32_t lcd_col1 = st->lcd_col1;
    const uint32_t lcd_col2 = st->lcd_col2;

    const uint8_t* f;
    if (ch >= 16)
        f = &lcd_font[ch - 16][0];
    else
        f = &st->LCD_CG[(ch & 7) * 8];
    for (int i = 0; i < 7; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            uint32_t col;
            if (f[i] & (1<<(4-j)))
            {
                col = lcd_col1;
            }
            else
            {
                col = lcd_col2;
            }
            int xx = x + i * 6;
            int yy = y + j * 6;
            for (int ii = 0; ii < 5; ii++)
            {
                for (int jj = 0; jj < 5; jj++)
                {
                    if (overlay)
                        lcd_buffer[(xx+ii) * lcd_width_max + yy+jj] &= col;
                    else
                        lcd_buffer[(xx+ii) * lcd_width_max + yy+jj] = col;
                }
            }
        }
    }
}

void LCD_FontRenderLevel(const struct lcd_state_t *st, lcd_buffer_t lcd_buffer, int32_t x, int32_t y, uint8_t ch, uint8_t width = 5)
{
    const uint32_t lcd_col1 = st->lcd_col1;
    const uint32_t lcd_col2 = st->lcd_col2;

    const uint8_t* f;
    if (ch >= 16)
        f = &lcd_font[ch - 16][0];
    else
        f = &st->LCD_CG[(ch & 7) * 8];
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < width; j++)
        {
            uint32_t col;
            if (f[i] & (1<<(4-j)))
            {
                col = lcd_col1;
            }
            else
            {
                col = lcd_col2;
            }
            int xx = x + i * 11;
            int yy = y + j * 26;
            for (int ii = 0; ii < 9; ii++)
            {
                for (int jj = 0; jj < 24; jj++)
                {
                    lcd_buffer[(xx+ii) * lcd_width_max + yy+jj] = col;
                }
            }
        }
    }
}

static const uint8_t LR[2][12][11] =
{
    {
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,
    },
    {
        1,1,1,1,1,1,1,1,1,0,0,
        1,1,1,1,1,1,1,1,1,1,0,
        1,1,0,0,0,0,0,0,1,1,0,
        1,1,0,0,0,0,0,0,1,1,0,
        1,1,0,0,0,0,0,0,1,1,0,
        1,1,1,1,1,1,1,1,1,1,0,
        1,1,1,1,1,1,1,1,1,0,0,
        1,1,0,0,0,0,0,1,1,0,0,
        1,1,0,0,0,0,0,0,1,1,0,
        1,1,0,0,0,0,0,0,1,1,0,
        1,1,0,0,0,0,0,0,0,1,1,
        1,1,0,0,0,0,0,0,0,1,1,
    }
};

static const int LR_xy[2][2] = {
    { 70, 264 },
    { 232, 264 }
};


void LCD_FontRenderLR(const struct lcd_state_t *st, lcd_buffer_t lcd_buffer, uint8_t ch)
{
    const uint32_t lcd_col1 = st->lcd_col1;
    const uint32_t lcd_col2 = st->lcd_col2;

    const uint8_t* f;
    if (ch >= 16)
        f = &lcd_font[ch - 16][0];
    else
        f = &st->LCD_CG[(ch & 7) * 8];
    int col;
    if (f[0] & 1)
    {
        col = lcd_col1;
    }
    else
    {
        col = lcd_col2;
    }
    for (int f = 0; f < 2; f++)
    {
        for (int i = 0; i < 12; i++)
        {
            for (int j = 0; j < 11; j++)
            {
                if (LR[f][i][j])
                    lcd_buffer[(i+LR_xy[f][0]) * lcd_width_max + j+LR_xy[f][1]] = col;
            }
        }
    }
}

void LCD_Update(const struct lcd_state_t *st, const lcd_background_t lcd_background, lcd_buffer_t lcd_buffer)
{
    const uint32_t lcd_width = st->lcd_width;
    const uint32_t lcd_height = st->lcd_height;

    if (!st->mcu_cm300 && !st->mcu_st && !st->mcu_scb55)
    {
        if (!st->lcd_enable && !st->mcu_jv880)
        {
            memset(lcd_buffer, 0, sizeof(lcd_buffer[0]) * lcd_buffer_size);
        }
        else
        {
            if (st->mcu_jv880)
            {
                for (size_t i = 0; i < lcd_height; i++) {
                    for (size_t j = 0; j < lcd_width; j++) {
                        lcd_buffer[i * lcd_width_max + j] = 0xFF03be51;
                    }
                }
            }
            else
            {
                for (size_t i = 0; i < lcd_height; i++) {
                    for (size_t j = 0; j < lcd_width; j++) {
                        lcd_buffer[i * lcd_width_max + j] = lcd_background[i * lcd_background_width + j];
                    }
                }
            }

            if (st->mcu_jv880)
            {
                for (int i = 0; i < 2; i++)
                {
                    for (int j = 0; j < 24; j++)
                    {
                        uint8_t ch = st->LCD_Data[i * 40 + j];
                        LCD_FontRenderStandard(st, lcd_buffer, 4 + i * 50, 4 + j * 34, ch);
                    }
                }
                
                // cursor
                int j = st->LCD_DD_RAM % 0x40;
                int i = st->LCD_DD_RAM / 0x40;
                if (i < 2 && j < 24 && st->LCD_C)
                    LCD_FontRenderStandard(st, lcd_buffer, 4 + i * 50, 4 + j * 34, '_', true);
            }
            else
            {
                for (int i = 0; i < 3; i++)
                {
                    uint8_t ch = st->LCD_Data[0 + i];
                    LCD_FontRenderStandard(st, lcd_buffer, 11, 34 + i * 35, ch);
                }
                for (int i = 0; i < 16; i++)
                {
                    uint8_t ch = st->LCD_Data[3 + i];
                    LCD_FontRenderStandard(st, lcd_buffer, 11, 153 + i * 35, ch);
                }
                for (int i = 0; i < 3; i++)
                {
                    uint8_t ch = st->LCD_Data[40 + i];
                    LCD_FontRenderStandard(st, lcd_buffer, 75, 34 + i * 35, ch);
                }
                for (int i = 0; i < 3; i++)
                {
                    uint8_t ch = st->LCD_Data[43 + i];
                    LCD_FontRenderStandard(st, lcd_buffer, 75, 153 + i * 35, ch);
                }
                for (int i = 0; i < 3; i++)
                {
                    uint8_t ch = st->LCD_Data[49 + i];
                    LCD_FontRenderStandard(st, lcd_buffer, 139, 34 + i * 35, ch);
                }
                for (int i = 0; i < 3; i++)
                {
                    uint8_t ch = st->LCD_Data[46 + i];
                    LCD_FontRenderStandard(st, lcd_buffer, 139, 153 + i * 35, ch);
                }
                for (int i = 0; i < 3; i++)
                {
                    uint8_t ch = st->LCD_Data[52 + i];
                    LCD_FontRenderStandard(st, lcd_buffer, 203, 34 + i * 35, ch);
                }
                for (int i = 0; i < 3; i++)
                {
                    uint8_t ch = st->LCD_Data[55 + i];
                    LCD_FontRenderStandard(st, lcd_buffer, 203, 153 + i * 35, ch);
                }

                LCD_FontRenderLR(st, lcd_buffer, st->LCD_Data[58]);

                for (int i = 0; i < 2; i++)
                {
                    for (int j = 0; j < 4; j++)
                    {
                        uint8_t ch = st->LCD_Data[20 + j + i * 40];
                        LCD_FontRenderLevel(st, lcd_buffer, 71 + i * 88, 293 + j * 130, ch, j == 3 ? 1 : 5);
                    }
                }
            }
        }
    }
}
