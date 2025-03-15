/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2024  Håkon Skjelten
 *
 *  libEmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  libEmuSC is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libEmuSC. If not, see <http://www.gnu.org/licenses/>.
 */

// PCM ROM decoding is based on the SC55_Soundfont generator written by
// Kitrinx and NewRisingSun [ https://github.com/Kitrinx/SC55_Soundfont ]


#ifndef __PCM_ROM_H__
#define __PCM_ROM_H__


#include "control_rom.h"

#include <stdint.h>

#include <string>
#include <vector>


namespace EmuSC {

class PcmRom
{
private:
  std::string _version;
  std::string _date;

  struct Samples {
//  std::vector<int32_t> samplesI;    // All samples stored in 24 bit 32kHz mono
    std::vector<float>   samplesF;    // 32 bit float, 32kHz, mono
  };
  std::vector<struct Samples> _sampleSets;

  uint32_t _unscramble_address(uint32_t address);
  int8_t   _unscramble_data(int8_t byte);

  uint32_t _find_samples_rom_address(uint32_t address,
                                     enum ControlRom::SynthGen synthGen);
  int _read_samples(std::vector<char> &rom,
                    struct ControlRom::Sample &ctrlSample,
                    enum ControlRom::SynthGen synthGen);

  PcmRom();

public:
  PcmRom(std::vector<std::string> romPath, ControlRom &ctrlRom);
  ~PcmRom();

  inline struct Samples& samples(uint16_t ss) { return _sampleSets[ss]; }

  std::string version(void) { return _version; }
  std::string date(void) { return _date; }
};

}

#endif  // __PCM_ROM_H__
