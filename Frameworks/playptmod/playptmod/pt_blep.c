/*
** This file is part of the ProTracker 2.3D port/clone
** project by Olav "8bitbubsy" Sorensen.
**
** It contains unstructured and unclean code, but I care
** more about how the program works than how the source
** code looks. Although, I do respect coders that can
** master the art of writing clean and structured code.
** I know I can't.
**
** All of the files are considered 'public domain',
** do whatever you want with it.
**
*/

#include <stdint.h>
#include "pt_blep.h"

#define _LERP(I, F) ((I[0]) + ((I[1]) - (I[0])) * (F))

static const uint32_t blepData[48] =
{
    0x3F7FE1F1, 0x3F7FD548, 0x3F7FD6A3, 0x3F7FD4E3,
    0x3F7FAD85, 0x3F7F2152, 0x3F7DBFAE, 0x3F7ACCDF,
    0x3F752F1E, 0x3F6B7384, 0x3F5BFBCB, 0x3F455CF2,
    0x3F26E524, 0x3F0128C4, 0x3EACC7DC, 0x3E29E86B,
    0x3C1C1D29, 0xBDE4BBE6, 0xBE3AAE04, 0xBE48DEDD,
    0xBE22AD7E, 0xBDB2309A, 0xBB82B620, 0x3D881411,
    0x3DDADBF3, 0x3DE2C81D, 0x3DAAA01F, 0x3D1E769A,
    0xBBC116D7, 0xBD1402E8, 0xBD38A069, 0xBD0C53BB,
    0xBC3FFB8C, 0x3C465FD2, 0x3CEA5764, 0x3D0A51D6,
    0x3CEAE2D5, 0x3C92AC5A, 0x3BE4CBF7, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000
};

void blepAdd(BLEP *b, float offset, float amplitude)
{
    int8_t n;

    uint32_t i;

    const float *src;
    float f;
    float a;
    
    float k[NS];

    n   = NS;
    i   = (uint32_t)(offset * SP);
    src = (const float *)(blepData) + i + OS;
    f   = (offset * SP) - i;
    i   = b->index;
    a   = 0.0f;
    
    while (n--)
    {
        a   += k[n] = _LERP(src, f);
        src += SP;
    }
    
    n   = NS;
    a   = 1.0f / a;

    while (n--)
    {
        b->buffer[i] += (amplitude * k[n]) * a;

        i++;
        i &= RNS;
    }

    b->samplesLeft = NS;
}

float blepRun(BLEP *b)
{
    float output;

    output              = b->buffer[b->index];
    b->buffer[b->index] = 0.0f;

    b->index++;
    b->index &= RNS;

    b->samplesLeft--;

    output += b->lastOutput;
    b->lastOutput = output;

    return (output);
}

