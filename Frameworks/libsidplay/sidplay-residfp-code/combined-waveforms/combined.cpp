/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2013 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include <cstdlib>
#include <cassert>
#include <ctime>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>

#include "parameters.h"

static double randomNextDouble()
{
    return static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
}

static float GetRandomValue()
{
    const float t = 1.f - (float) randomNextDouble() * 0.9f;
    return (randomNextDouble() > 0.5) ? 1.f / t : t;
}

static void Optimize(const std::vector<int> &reference, int wave, char chip)
{
    Parameters bestparams;
    if (chip == 'D')
    {
        switch (wave)
        {
        case 3:
            // current score 2740
            bestparams.bias = 0.987103f;
            bestparams.pulsestrength = 0.f;
            bestparams.topbit = 0.f;
            bestparams.distance = 37.294f;
            bestparams.stmix = 0.783281f;
            break;
        case 5:
            // current score 18404
            bestparams.bias = 0.888048f;
            bestparams.pulsestrength = 2.26606f;
            bestparams.topbit = 0.99697f;
            bestparams.distance = 0.0422943f;
            bestparams.stmix = 0.f;
            break;
        case 6:
            // current score 18839
            bestparams.bias = 0.886823f;
            bestparams.pulsestrength = 2.35161f;
            bestparams.topbit = 1.77758f;
            bestparams.distance = 0.0236612f;
            bestparams.stmix = 0.f;
            break;
        case 7:
            // current score 428
            bestparams.bias = 0.928903f;
            bestparams.pulsestrength = 1.4555f;
            bestparams.topbit = 0.f;
            bestparams.distance = 0.0791919f;
            bestparams.stmix = 0.905484f;
            break;
        }
    }
    if (chip == 'E')
    {
        switch (wave)
        {
        case 3:
            // current score 721
            bestparams.bias = 0.957552f;
            bestparams.pulsestrength = 0.f;
            bestparams.topbit = 0.f;
            bestparams.distance = 1.80014f;
            bestparams.stmix = 0.698112f;
            break;
        case 5:
            // current score 5552
            bestparams.bias = 0.916105f;
            bestparams.pulsestrength = 1.93374f;
            bestparams.topbit = 1.00139f;
            bestparams.distance = 0.0211841f;
            bestparams.stmix = 0.f;
            break;
        case 6:
            // current score 12
            bestparams.bias = 0.935096f;
            bestparams.pulsestrength = 2.64471f;
            bestparams.topbit = 0.f;
            bestparams.distance = 0.00538012f;
            bestparams.stmix = 0.f;
            break;
        case 7:
            // current score 120
            bestparams.bias = 0.900499f;
            bestparams.pulsestrength = 0.f;
            bestparams.topbit = 0.f;
            bestparams.distance = 0.f;
            bestparams.stmix = 1.f;
            break;
        }
    }
    if (chip == 'G')
    {
        switch (wave)
        {
        case 3:
            // current score 1741
            bestparams.bias = 0.880592f;
            bestparams.pulsestrength = 0.f;
            bestparams.topbit = 0.f;
            bestparams.distance = 0.327589f;
            bestparams.stmix = 0.611491f;
            break;
        case 5:
            // current score 11418
            bestparams.bias = 0.892438f;
            bestparams.pulsestrength = 2.00995f;
            bestparams.topbit = 1.00392f;
            bestparams.distance = 0.0298894f;
            bestparams.stmix = 0.f;
            break;
        case 6:
            // current score 21096
            bestparams.bias = 0.863292f;
            bestparams.pulsestrength = 1.69239f;
            bestparams.topbit = 1.12637f;
            bestparams.distance = 0.0335683f;
            bestparams.stmix = 0.f;
            break;
        case 7:
            // current score 78
            bestparams.bias = 0.930481f;
            bestparams.pulsestrength = 1.42322f;
            bestparams.topbit = 0.f;
            bestparams.distance = 0.0481732f;
            bestparams.stmix = 0.752611f;
            break;
        }
    }
    if (chip == 'V')
    {
        switch (wave)
        {
        case 3:
            // current score 5339
            bestparams.bias = 0.979807f;
            bestparams.pulsestrength = 0.f;
            bestparams.topbit = 0.990736f;
            bestparams.distance = 9.22678f;
            bestparams.stmix = 0.824563f;
            break;
        case 5:
            // current score 18507
            bestparams.bias = 0.909646f;
            bestparams.pulsestrength = 2.03944f;
            bestparams.topbit = 0.958471f;
            bestparams.distance = 0.175597f;
            bestparams.stmix = 0.f;
            break;
        case 6:
            // current score 16763
            bestparams.bias = 0.918338f;
            bestparams.pulsestrength = 2.41154f;
            bestparams.topbit = 0.927047f;
            bestparams.distance = 0.171891f;
            bestparams.stmix = 0.f;
            break;
        case 7:
            // current score 3199
            bestparams.bias = 0.984532f;
            bestparams.pulsestrength = 1.53602f;
            bestparams.topbit = 0.961933f;
            bestparams.distance = 3.46871f;
            bestparams.stmix = 0.803955f;
            break;
        }
    }
    if (chip == 'W')
    {
        switch (wave)
        {
        case 3:
            // current score 6648
            bestparams.bias = 0.986102f;
            bestparams.pulsestrength = 0.f; // 458120
            bestparams.topbit = 0.995344f;
            bestparams.distance = 8.64964f;
            bestparams.stmix = 0.957502f;
            break;
        case 5:
            // current score 24634
            bestparams.bias = 0.906838f;
            bestparams.pulsestrength = 2.23245f;
            bestparams.topbit = 0.958234f;
            bestparams.distance = 0.205228f;
            bestparams.stmix = 0.0475253f;
            break;
        case 6:
            // current score 22693
            bestparams.bias = 0.889691f;
            bestparams.pulsestrength = 1.64253f;
            bestparams.topbit = 0.977121f;
            bestparams.distance = 0.293443f;
            bestparams.stmix = 0.f; // 4.65769e-013
            break;
        case 7:
            // current score 5182
            bestparams.bias = 0.956644f;
            bestparams.pulsestrength = 1.50249f;
            bestparams.topbit = 0.943326f;
            bestparams.distance = 0.520891f;
            bestparams.stmix = 0.956507f;
            break;
        }
    }

    int bestscore = bestparams.Score(wave, reference, true, 4096 * 255);
    std::cout << "# initial score " << bestscore << std::endl << std::endl;

    Parameters p;
    for (;;)
    {
        bool changed = false;
        while (! changed)
        {
            for (int i = Parameters::BIAS; i <= Parameters::STMIX; i++)
            {
                const float oldValue = bestparams.GetValue(i);
                float newValue = oldValue;
                if (randomNextDouble() > 0.5)
                {
                    newValue *= GetRandomValue();

                    if ((i == Parameters::STMIX) && (newValue > 1.f))
                    {
                        newValue = 1.f;
                    }
                }

                p.SetValue(i, newValue);
                changed = changed || oldValue != newValue;
            }
        }
        const int score = p.Score(wave, reference, false, bestscore);
        /* accept if improvement */
        if (score <= bestscore)
        {
            bestparams = p;
            p.reset();
            bestscore = score;
            std::cout << "# current score " << score << std::endl << bestparams.toString() << std::endl << std::endl;
        }
    }
}

static std::vector<std::string> split(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    std::istringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

static std::vector<int> ReadChip(int wave, char chip)
{
    std::cout << "Reading chip: " << chip << std::endl;
    std::vector<int> result;

    std::ostringstream fileName;
    fileName << "sidwaves/WAVE" << wave << ".CSV";
    std::ifstream ifs(fileName.str().c_str(), std::ifstream::in);
    std::string line;
    while (getline(ifs, line).good())
    {
        std::vector<std::string> values = split(line, ',');
        result.push_back(atoi(values[chip - 'A'].c_str()));
    }
    return result;
}

int main(int argc, const char* argv[])
{
    if (argc != 3)
    {
        std::cout << "Usage " << argv[0] << " <waveform> <chip>" << std::endl;
        exit(-1);
    }

    const int wave = atoi(argv[1]);
    assert(wave == 3 || wave == 5 || wave == 6 || wave == 7); 

    const char chip = argv[2][0];
    assert(chip >= 'A' && chip <= 'Z');

    std::vector<int> reference = ReadChip(wave, chip);

#ifndef NDEBUG
    for (std::vector<int>::iterator it = reference.begin(); it != reference.end(); ++it)
        std::cout << (*it) << std::endl;
#endif

    srand(time(0));

    Optimize(reference, wave, chip);
}
