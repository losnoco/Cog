/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2023-2024  Håkon Skjelten
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


#ifndef __SETTINGS_H__
#define __SETTINGS_H__


#include "control_rom.h"
#include "params.h"

#include <stdint.h>

#include <array>
#include <string>


namespace EmuSC {

class Settings
{
public:
  Settings(ControlRom & ctrlRom);
  ~Settings();

  // Sound Canvas modes
  enum class Mode {
    GS,                  // Standard GS mode. Default.
    MT32,                // MT32 mode
    SC55                 // SC-55 mode for SC-88     TODO

    // TODO: What about GM mode?
  };

  // Interpolation modes
  enum class InterpMode {
    Nearest = 0,
    Linear  = 1,
    Cubic   = 2
  };

  // Retrieve settings from Config paramters
  uint8_t  get_param(enum SystemParam sp);
  uint8_t* get_param_ptr(enum SystemParam sp);
  uint32_t get_param_uint32(enum SystemParam sp);
  uint16_t get_param_32nib(enum SystemParam sp);
  uint8_t  get_param(enum PatchParam pp, int8_t part = -1);
  uint8_t* get_param_ptr(enum PatchParam pp, int8_t part = -1);
  uint16_t get_param_uint14(enum PatchParam pp, int8_t part = -1);
  uint16_t get_param_uint16(enum PatchParam pp, int8_t part = -1);
  uint8_t  get_param_nib16(enum PatchParam pp, int8_t part = -1);
  uint8_t  get_patch_param(uint16_t address, int8_t part = -1);
  uint8_t  get_param(enum DrumParam, uint8_t map, uint8_t key);
  int8_t* get_param_ptr(enum DrumParam, uint8_t map);

  // Set settings from Config paramters
  void set_param(enum SystemParam sp, uint8_t value);
  void set_param(enum SystemParam sp, const uint8_t *data, uint8_t size);
  void set_param_uint32(enum SystemParam sp, uint32_t value);
  void set_param_32nib(enum SystemParam sp, uint16_t value);
  void set_system_param(uint16_t address, const uint8_t *data, uint8_t size = 1);

  void set_param(enum PatchParam pp, uint8_t value, int8_t part = -1);
  void set_param(enum PatchParam pp, const uint8_t *data, uint8_t size = 1,
		       int8_t part = -1);
  void set_param_uint14(enum PatchParam pp, uint16_t value, int8_t part = -1);
  void set_param_nib16(enum PatchParam pp, uint8_t value, int8_t part = -1);
  void set_patch_param(uint16_t address, const uint8_t *data, uint8_t size = 1);
  void set_patch_param(uint16_t address, uint8_t value, int8_t part = -1);
  void set_param(enum DrumParam dp, uint8_t map, uint8_t key, uint8_t value);
  void set_param(enum DrumParam dp, uint8_t map, const uint8_t *data, uint8_t length);
  void set_drum_param(uint16_t address, const uint8_t *data, uint8_t size = 1);

  int update_drum_set(uint8_t map, uint8_t bank);

  // Store settings paramters to file (aka battery backup)
  bool load(std::string filePath);
  bool save(std::string filePath);

  // Reset all settings to default GS mode
  void reset();
  void set_gm_mode(void);
  void set_map_mt32(void);

  // Temporary solution
  // Figure out the need for a common way with all controllers
  void update_pitchBend_factor(int8_t part);
  float get_pitchBend_factor(int8_t part) { return _PBController[part]; }

  static int8_t convert_from_roland_part_id(int8_t part);

  void set_sample_rate(int sampleRate) { _sampleRate = sampleRate; }
  inline int sample_rate(void) { return _sampleRate; }

  void set_channels(int channels) { _channels = channels; }
  inline int channels(void) { return _channels; }

  void set_interpolation_mode(enum InterpMode im) { _interpMode = im; }
  inline enum InterpMode interpolation_mode(void) { return _interpMode; }

private:
  std::array<uint8_t, 0x0100> _systemParams;  // Both SysEx and non-SysEx data
  std::array<uint8_t, 0x4000> _patchParams;
  std::array<uint8_t, 0x2000> _drumParams;

  ControlRom &_ctrlRom;

  // Non-native parameters
  int _sampleRate;
  int _channels;
  InterpMode _interpMode;

  void _initialize_system_params(enum Mode = Mode::GS);
  void _initialize_patch_params(enum Mode = Mode::GS);
  void _initialize_drumSet_params();

  // BE / LE conversion
  inline bool _le_native(void) { uint16_t n = 1; return (*(const uint8_t *) & n); }
  uint8_t  _to_native_endian_nib16(const uint8_t *ptr);
  uint16_t _to_native_endian_uint14(const uint8_t *ptr);
  uint16_t _to_native_endian_uint16(const uint8_t *ptr);
  uint32_t _to_native_endian_uint32(const uint8_t *ptr);

  // Macros for certain settings
  void _run_macro_chorus(uint8_t value);
  void _run_macro_reverb(uint8_t value);

  // Update accumulated controller inputs
  void _update_controller_input(enum PatchParam pp, uint8_t value, int8_t part);
  void _update_controller_input_acc(enum PatchParam pp, int8_t part);
  void _accumulate_controller_values(enum PatchParam ctm, enum PatchParam acc,
				     int8_t partm, int min, int max, bool center);

  // Temporary storage for pitchbend
  float _PBController[16];

  static constexpr std::array<uint8_t, 16> _convert_to_roland_part_id_LUT =
    { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 10, 11, 12, 13, 14, 15 };
  static constexpr std::array<uint8_t, 16> _convert_from_roland_part_id_LUT =
    { 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15 };

};

}

#endif  // __SETTINGS_H__
