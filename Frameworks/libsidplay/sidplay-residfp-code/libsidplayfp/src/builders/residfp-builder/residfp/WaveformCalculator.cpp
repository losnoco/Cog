/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
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

#include "WaveformCalculator.h"

namespace reSIDfp
{

WaveformCalculator* WaveformCalculator::getInstance()
{
    static WaveformCalculator instance;
    return &instance;
}

/*
 * the "bits wrong" figures below are not directly comparable. 0 bits are very easy to predict,
 * and waveforms that are mostly zero have low scores. More comparable scores would be found by
 * dividing with the count of 1-bits, or something.
 */
const CombinedWaveformConfig config[2][4] =
{
    { /* kevtris chip G (6581r2/r3) */
        {0.880592f, 0.f,      0.f,       0.327589f,  0.611491f}, // error 1741
        {0.892438f, 2.00995f, 1.00392f,  0.0298894f, 0.f      }, // error 11418
        {0.874544f, 1.91885f, 1.12702f,  0.0312843f, 0.f      }, // error 21223
        {0.930481f, 1.42322f, 0.f,       0.0481732f, 0.752611f}, // error 78
    },
    { /* kevtris chip V (8580) */
        {0.979807f, 0.f,      0.990736f, 9.23845f,   0.82445f},  // error 5371
        {0.909646f, 2.03944f, 0.958471f, 0.175597f,  0.f     },  // error 18507
        {0.918338f, 2.00243f, 0.949102f, 0.180793f,  0.f     },  // error 16763
        {0.984532f, 1.53602f, 0.961933f, 3.46871f,   0.803955f}, // error 3199
    },
};

/**
 * Generate bitstate based on emulation of combined waves.
 *
 * @param config
 * @param waveform the waveform to emulate, 1 .. 7
 * @param accumulator the high bits of the accumulator value
 */
short calculateCombinedWaveform(CombinedWaveformConfig config, int waveform, int accumulator)
{
    float o[12];

    /* S with strong top bit for 6581 */
    for (int i = 0; i < 12; i++)
    {
        o[i] = (accumulator & (1 << i)) != 0 ? 1.f : 0.f;
    }

    /* convert to T */
    if ((waveform & 3) == 1)
    {
        const bool top = (accumulator & 0x800) != 0;

        for (int i = 11; i > 0; i--)
        {
            o[i] = top ? 1.0f - o[i - 1] : o[i - 1];
        }

        o[0] = 0.f;
    }

    /* convert to ST */
    if ((waveform & 3) == 3)
    {
        /* bottom bit is grounded via T waveform selector */
        o[0] *= config.stmix;

        for (int i = 1; i < 12; i++)
        {
            o[i] = o[i - 1] * (1.f - config.stmix) + o[i] * config.stmix;
        }
    }

    o[11] *= config.topbit;

    /* ST, P* waveform? */
    if (waveform == 3 || waveform > 4)
    {
        float distancetable[12 * 2 + 1];

        for (int i = 0; i <= 12; i++)
        {
            distancetable[12 + i] = distancetable[12 - i] = 1.f / (1.f + i * i * config.distance);
        }

        float tmp[12];

        for (int i = 0; i < 12; i++)
        {
            float avg = 0.f;
            float n = 0.f;

            for (int j = 0; j < 12; j++)
            {
                const float weight = distancetable[i - j + 12];
                avg += o[j] * weight;
                n += weight;
            }

            /* pulse control bit */
            if (waveform > 4)
            {
                const float weight = distancetable[i - 12 + 12];
                avg += config.pulsestrength * weight;
                n += weight;
            }

            tmp[i] = (o[i] + avg / n) * 0.5f;
        }

        for (int i = 0; i < 12; i++)
        {
            o[i] = tmp[i];
        }
    }

    short value = 0;

    for (int i = 0; i < 12; i++)
    {
        if (o[i] > config.bias)
        {
            value |= 1 << i;
        }
    }

    return value;
}

matrix_t* WaveformCalculator::buildTable(ChipModel model)
{
    const CombinedWaveformConfig* cfgArray = config[model == MOS6581 ? 0 : 1];

    cw_cache_t::iterator lb = CACHE.lower_bound(cfgArray);

    if (lb != CACHE.end() && !(CACHE.key_comp()(cfgArray, lb->first)))
    {
        return &(lb->second);
    }

    matrix_t wftable(8, 4096);

    for (unsigned int idx = 0; idx < 1 << 12; idx++)
    {
        wftable[0][idx] = 0xfff;
        wftable[1][idx] = static_cast<short>((idx & 0x800) == 0 ? idx << 1 : (idx ^ 0xfff) << 1);
        wftable[2][idx] = static_cast<short>(idx);
        wftable[3][idx] = calculateCombinedWaveform(cfgArray[0], 3, idx);
        wftable[4][idx] = 0xfff;
        wftable[5][idx] = calculateCombinedWaveform(cfgArray[1], 5, idx);
        wftable[6][idx] = calculateCombinedWaveform(cfgArray[2], 6, idx);
        wftable[7][idx] = calculateCombinedWaveform(cfgArray[3], 7, idx);
    }

    return &(CACHE.insert(lb, cw_cache_t::value_type(cfgArray, wftable))->second);
}

} // namespace reSIDfp
