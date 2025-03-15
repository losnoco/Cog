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

// Control ROM decoding is based on the SC55_Soundfont generator written by
// Kitrinx and NewRisingSun [ https://github.com/Kitrinx/SC55_Soundfont ]


#ifndef __CONTROL_ROM_H__
#define __CONTROL_ROM_H__


#include <stdint.h>

#include <array>
#include <fstream>
#include <string>
#include <vector>


namespace EmuSC {

class ControlRom
{
public:
  ControlRom(std::string romPath, std::string cpuRomPath);
  ~ControlRom();

  // Internal data structures extracted from the control ROM file

  struct Sample {         // 16 bytes
    uint8_t  volume;      // Volume attenuation (0x7f - 0)
    uint32_t address;     // Offset on vsc, bank + scrambled address on SC55.
                          // Bits above 20 are wave bank.
    uint16_t attackEnd;   // boundry between attack and decay? Unconfirmed.
    uint16_t sampleLen;   // Sample Size
    uint16_t loopLen;     // Loop point, used as sample_len - loop_len - 1
    uint8_t  loopMode;    // 2 if not a looping sound, 1 forward then back,
                          // 0 forward only.
    uint8_t  rootKey;     // Base pitch of the sample
    uint16_t pitch;       // Fine pitch adjustment, 2048 to 0. Pos. incr. pitch.
    uint16_t fineVolume;  // Always 0x400 on VSC, appears to be 1000ths of a
                          // decibel. Positive is higher volume.
  };

  struct Partial {        // 48 bytes in total
    std::string name;
    uint8_t breaks[16];   // Note breakpoints corresponding to sample addresses
    uint16_t samples[16]; // Set of addresses to the sample table. 0 is default
  };                      // and above corresponds to breakpoints

  struct InstPartial {      // 92 bytes in total
    uint8_t LFO2Waveform;
    uint8_t LFO2Rate;       // LFO frequency
    uint8_t LFO2Delay;      // LFO delay before LFO Fade starts
    uint8_t LFO2Fade;       // LFO fade in, linear increase

    uint16_t partialIndex;  // Partial table index, 0xFFFF for unused
    int8_t panpot;          // [-64, 64]. Default 0x40 (0-127)
    int8_t coarsePitch;     // Shifts pitch in semitones. Default 0x40
    int8_t finePitch;       // Shifts pitch in cents. Default 0x40
    int8_t randPitch;

    int8_t pitchKeyFlw;

    uint8_t TVPLFO1Depth;
    uint8_t TVPLFO2Depth;
    uint8_t pitchEnvDepth;
    uint8_t pitchEnvL0;     // Pitch Envelope L0
    uint8_t pitchEnvL1;     // Pitch Envelope L1
    uint8_t pitchEnvL2;     // Pitch Envelope L2
    uint8_t pitchEnvL3;     // Pitch Envelope L3 (L4 = 0)
    uint8_t pitchEnvL5;     // Pitch Envelope L5
    uint8_t pitchEnvT1;     // Pitch Envelope T1 (Attack1)
    uint8_t pitchEnvT2;     // Pitch Envelope T2 (Attack2)
    uint8_t pitchEnvT3;     // Pitch Envelope T3 (Decay1)
    uint8_t pitchEnvT4;     // Pitch Envelope T4 (Decay2)
    uint8_t pitchEnvT5;     // Pitch Envelope T5 (Release)

    uint8_t pitchETKeyF14;  // Pitch Envelope Time Key Follow (T1 - T4)
    uint8_t pitchETKeyF5;   // Pitch Envelope Time Key Follow (T5)

    uint8_t TVFCOFVelCur;   // TVF Cutoff Velocity Curve
    int8_t TVFBaseFlt;
    int8_t TVFResonance;
    int8_t TVFType;         // TVF Type [ low pass | high pass | disabled ]

    uint8_t TVFCFKeyFlw;    // TVF Cutoff Frequency Key Follow
    uint8_t TVFCFKeyFlwC;   // TVF Cutoff Frequency Key Follow Curves

    uint8_t TVFLFO1Depth;
    uint8_t TVFLFO2Depth;
    uint8_t TVFEnvDepth;
    uint8_t TVFEnvL1;       // TVF Envelope L1 (L0 = 0)
    uint8_t TVFEnvL2;       // TVF Envelope L2
    uint8_t TVFEnvL3;       // TVF Envelope L3
    uint8_t TVFEnvL4;       // TVF Envelope L4
    uint8_t TVFEnvL5;       // TVF Envelope L5
    uint8_t TVFEnvT1;       // TVF Envelope T1
    uint8_t TVFEnvT2;       // TVF Envelope T2
    uint8_t TVFEnvT3;       // TVF Envelope T3
    uint8_t TVFEnvT4;       // TVF Envelope T4
    uint8_t TVFEnvT5;       // TVF Envelope T5

    uint8_t TVFETKeyF14;    // TVF Envelope Time Key Follow (T1 - T4)
    uint8_t TVFETKeyF5;     // TVF Envelope Time Key Follow (T5)

    uint8_t TVALvlVelCur;
    int8_t volume;          // Volume attenuation (0x7f - 0)
    uint8_t TVABiasPoint;   // TVA Bias Point, 0=V shape, 1=key>85, 2=flat curve
    uint8_t TVABiasLevel;
    uint8_t TVALFO1Depth;
    uint8_t TVALFO2Depth;
    uint8_t TVAEnvL1;       // TVA Envelope L1 (L0 = 0)
    uint8_t TVAEnvL2;       // TVA Envelope L2
    uint8_t TVAEnvL3;       // TVA Envelope L3
    uint8_t TVAEnvL4;       // TVA Envelope L4 (L5 = 0)
    uint8_t TVAEnvT1;       // TVA Envelope T1 (Attack1)
    uint8_t TVAEnvT2;       // TVA Envelope T2 (Attack2)
    uint8_t TVAEnvT3;       // TVA Envelope T3 (Decay1)
    uint8_t TVAEnvT4;       // TVA Envelope T4 (Decay2)
    uint8_t TVAEnvT5;       // TVA Envelope T5 (Release)

    uint8_t TVAETKeyP14;    // TVA Envelope Time Key Presets (T1 - T4)
    uint8_t TVAETKeyP5;     // TVA Envelope Time Key Presets (T5)
    uint8_t TVAETKeyF14;    // TVA Envelope Time Key Follow (T1 - T4)
    uint8_t TVAETKeyF5;     // TVA Envelope Time Key Follow (T5)
    uint8_t TVAETVSens14;   // TVA Envelope Time Velocity Sensitivity (T1 - T4)
    uint8_t TVAETVSens5;    // TVA Envelope Time Velocity Sensitivity (T5)
  };

  struct Instrument {       // 204 bytes in total
    std::string name;

    uint8_t volume;         // Volume attenuation (0x7f - 0)
    uint8_t LFO1Waveform;
    uint8_t LFO1Rate;       // LFO frequency
    uint8_t LFO1Delay;
    uint8_t LFO1Fade;
    uint8_t partialsUsed;   // Bit 0 & 1 => which of the two partials are in use
    uint8_t pitchCurve;

    struct InstPartial partials[2];
  };

  struct DrumSet {          // 1164 bytes
    uint16_t preset[128];
    uint8_t volume[128];
    uint8_t key[128];
    uint8_t assignGroup[128];// AKA exclusive class
    uint8_t panpot[128];
    uint8_t reverb[128];
    uint8_t chorus[128];
    uint8_t flags[128];     // 0x10 -> accept note on,  0x01 -> accept note off
    std::string name;       // 12 chars
  };

  struct LookupTables {
    // PROGROM
    std::array<uint8_t, 128> VelocityCurve0;
    std::array<uint8_t, 128> VelocityCurve1;
    std::array<uint8_t, 128> VelocityCurve2;
    std::array<uint8_t, 128> VelocityCurve3;
    std::array<uint8_t, 128> VelocityCurve4;
    std::array<uint8_t, 128> VelocityCurve5;
    std::array<uint8_t, 128> VelocityCurve6;
    std::array<uint8_t, 128> VelocityCurve7;
    std::array<uint8_t, 128> VelocityCurve8;
    std::array<uint8_t, 128> VelocityCurve9;
    std::array<uint8_t, 128> mul2;
    std::array<uint8_t, 128> mul2From85;
    std::array<uint8_t, 128> TVABiasPoint1;
    std::array<uint8_t, 128> TVAEnvTKFP1T14Index;
    std::array<uint8_t, 128> TVAEnvTKFP1T5Index;
//    std::array<int,     128> mul256;
//    std::array<int,     128> mul256From60;
//    std::array<int,     128> mul256From96;
//    std::array<int,     128> mul256Upto96;
    std::array<int,     128> PitchScale1;
    std::array<int,     128> PitchScale2;
    std::array<int,     128> PitchScale3;

    // CPUROM
    std::array<uint8_t,  21> TimeKeyFollowDiv;
    std::array<int,     256> TimeKeyFollow;
    std::array<int,     128> envelopeTime;
    std::array<int,     128> LFORate;
    std::array<int,     128> LFODelayTime;
    std::array<int,     128> LFOTVFDepth;
    std::array<int,     128> LFOTVPDepth;
    std::array<uint8_t, 128> LFOSine;
    std::array<int,     128> TVFEnvDepth;
    std::array<int,     128> TVFCutoffFreq;
    std::array<uint8_t, 256> TVFResonanceFreq;
    std::array<uint8_t, 128> TVFResonance;
    std::array<int,     128> PitchEnvDepth;
    std::array<uint8_t,  64> TVFEnvScale;
    std::array<int,     256> TVAEnvExpChange;
    std::array<uint8_t, 129> TVABiasLevel;
    std::array<uint8_t, 128> TVAPanpot;
    std::array<uint8_t, 128> TVALevelIndex;
    std::array<uint8_t, 256> TVALevel;
  };
  struct LookupTables lookupTables;

  enum class SynthGen {
    SC55    = 0,
    SC55mk2 = 1,
    SC88    = 2,
    SC88Pro = 3
  };

  int dump_demo_songs(std::string path);
  bool intro_anim_available(void);
  std::vector<uint8_t> get_intro_anim(int animIndex = 0);

  std::string model(void) { return _model; }
  std::string version(void) { return _version; }
  std::string date(void) { return _date; }
  enum SynthGen generation(void) { return _synthGeneration; }

  const std::array<uint8_t, 128>& get_drum_sets_LUT(void) { return _drumSetsLUT; }
  const uint8_t max_polyphony(void);

  std::vector<std::vector<std::string>> get_instruments_list(void);
  std::vector<std::vector<std::string>> get_partials_list(void);
  std::vector<std::vector<std::string>> get_samples_list(void);
  std::vector<std::vector<std::string>> get_variations_list(void);
  std::vector<std::string> get_drum_sets_list(void);

  inline struct Instrument& instrument(int i) { return _instruments[i]; }
  inline struct Partial& partial(int p) { return _partials[p]; }
  inline struct Sample& sample(int s) { return _samples[s]; }
  inline struct DrumSet& drumSet(int ds) { return _drumSets[ds]; }
  inline const std::array<uint16_t, 128>& variation(int v) const { return _variations[v]; }

  inline int numSampleSets(void) { return _samples.size(); }
  inline int numInstruments(void) { return _instruments.size(); }

  inline std::vector<DrumSet> &get_drumsets_ref(void) { return _drumSets; }

private:
  std::string _romPath;

  std::string _model;
  std::string _version;
  std::string _date;

  enum SynthModel {
    sm_SC55,              // Original Sound Canvas
    sm_SC55mkII,          // Upgraded model
    sm_SCC1,              // ISA card version
    sm_SC88,
    sm_SC88Pro,
  };
  enum SynthModel _synthModel;

  enum SynthGen _synthGeneration;

  static constexpr uint8_t _maxPolyphonySC55     = 24;
  static constexpr uint8_t _maxPolyphonySC55mkII = 28;
  static constexpr uint8_t _maxPolyphonySC88     = 64;

  static const std::vector<uint32_t> _banksSC55;

  // Only a placeholder, SC-88 layout is currently unkown
  static const std::vector<uint32_t> _banksSC88;

  struct _ProgMemoryMapLUT {
    int VelocityCurve0;
    int VelocityCurve1;
    int VelocityCurve2;
    int VelocityCurve3;
    int VelocityCurve4;
    int VelocityCurve5;
    int VelocityCurve6;
    int VelocityCurve7;
    int VelocityCurve8;
    int VelocityCurve9;
    int mul2;
    int mul2From85;
    int TVABiasPoint1;
    int TVAEnvTKFP1T14Index;
    int TVAEnvTKFP1T5Index;
//    int mul256;
//    int mul256From60;
//    int mul256From96;
//    int mul256Upto96;
    int PitchScale1;
    int PitchScale2;
    int PitchScale3;
  };

  const _ProgMemoryMapLUT SC55_1_21_Prog_LUT {
    0x3d1e8, 0x3d268, 0x3d2e8, 0x3d368, 0x3d3e8, 0x3d468, 0x3d4e8, 0x3d568,
    0x3d5e8, 0x3d668, 0x3dd82, 0x3de02, 0x3de02, 0x3df82, 0x3e102, 0x3e982,
    0x3ea82, 0x3eb82 };
  const _ProgMemoryMapLUT SC55mkII_1_01_Prog_LUT {
    0x3d1e8, 0x3d268, 0x3d2e8, 0x3d368, 0x3d3e8, 0x3d468, 0x3d4e8, 0x3d568,
    0x3d5e8, 0x3d668, 0x3de8c, 0x3df0c, 0x3df0c, 0x3e10c, 0x3e30c, 0x3ee0c,
    0x3ef0c, 0x3f00c };

  struct _CPUMemoryMapLUT {
    int TimeKeyFollowDiv;
    int TimeKeyFollow;
    int EnvelopeTime;
    int LFORate;
    int LFODelayTime;
    int LFOTVFDepth;
    int LFOTVPDepth;
    int LFOSine;
    int TVFEnvDepth;
    int TVFCutoffFreq;
    int TVFResonanceFreq;
    int TVFResonance;
    int PitchEnvDepth;
    int TVFEnvScale;
    int TVAEnvExpChange;
    int TVABiasLevel;
    int TVAPanpot;
    int TVALevelIndex;
    int TVALevel;
  };

  const _CPUMemoryMapLUT SC55_1_21_CPU_LUT {
    0x679a, 0x67c6, 0x6f12, 0x7012, 0x7112, 0x7212, 0x7312, 0x7412,
    0x7512, 0x7612, 0x7715, 0x7816, 0x78f2, 0x79f2, 0x6d10, 0x69c6,
    0x6c8f, 0x6b0f, 0x6b8f };
  const _CPUMemoryMapLUT SC55mkII_1_01_CPU_LUT {
    0x650e, 0x653a, 0x6c86, 0x6486, 0x6e86, 0x6f86, 0x7086, 0x7186,
    0x7286, 0x7386, 0x7489, 0x758a, 0x7765, 0x7766, 0x6a84, 0x673a,
    0x6a03, 0x6883, 0x6903 };

  int _read_lookup_tables_progrom(std::ifstream &romFile);
  int _read_lookup_tables_cpurom(std::ifstream &romFile);
  int _read_lut_16bit(std::ifstream &ifs, int pos, std::array<int, 128> &lut);
  int _read_lut_16bit(std::ifstream &ifs, int pos, std::array<int, 129> &lut);
  int _read_lut_16bit(std::ifstream &ifs, int pos, std::array<int, 256> &lut);

  int _identify_model(std::ifstream &romFile);
  const std::vector<uint32_t> &_banks(void);

  // To be replaced with std::endian::native from C++20
  inline bool _le_native(void) { uint16_t n = 1; return (*(uint8_t *) & n); } 

  uint16_t _native_endian_uint16(uint8_t *ptr);
  uint32_t _native_endian_3bytes_uint32(uint8_t *ptr);
  uint32_t _native_endian_4bytes_uint32(uint8_t *ptr);

  int _read_instruments(std::ifstream &romFile);
  int _read_partials(std::ifstream &romFile);
  int _read_variations(std::ifstream &romFile);
  int _read_samples(std::ifstream &romFile);
  int _read_drum_sets(std::ifstream &romFile);

  std::array<uint8_t, 128> _drumSetsLUT;

  std::vector<Instrument> _instruments;
  std::vector<Partial> _partials;
  std::vector<Sample> _samples;
  std::vector<DrumSet> _drumSets;
  // TODO: define constants for variation table dimensions
  std::array<std::array<uint16_t, 128>, 128> _variations;

  ControlRom();

};

}

#endif  // __CONTROL_ROM_H__
