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


#ifndef __SYNTH_H__
#define __SYNTH_H__


#include "control_rom.h"
#include "params.h"
#include "pcm_rom.h"

#include <array>
#include <functional>
#include <mutex>
#include <string>
#include <vector>


/* Public API for libEmuSC
 *
 * This Synth class is responsible for receiving MIDI events from client,
 * process the event based on information in Control and PCM ROMS, and
 * finally responding to audio buffer requests.
 *
 * Synth class constructor depends on a valid Control Rom and PCM ROM.
 *
 * MIDI events is sent to the emulator via the midi_input() method using the
 * three bytes from raw MIDI events.
 * 
 * Audio samples are extracted by calling the get_next_sample() method. This
 * is typically done from a callback function triggered by the OS audio
 * driver when the audio buffer is running low.
 *
 * All settings are configured through the Settings class.
 */


namespace EmuSC {

class Part;
class Settings;

class Synth
{
public:

  enum class SoundMap {
    GS,                       // Default GS settings
    GS_GM,                    // GS settings in GM mode (available on SC55mk2+)
    MT32                      // MT32 arrangement
  };

  Synth(ControlRom &cRom, PcmRom &pRom, SoundMap map = SoundMap::GS);
  ~Synth();

  // Add start() and stop()? Won't start if sampleRate is not set?

  void midi_input(uint8_t status, uint8_t data1, uint8_t data2);
  void midi_input_sysex(const uint8_t *data, uint16_t length);

  int get_next_sample(int16_t *sample);
  int get_next_sample(float *sample);
  std::array<float, 16> get_parts_last_peak_sample(void);

  // Setting audio properties (default is 44100, 2)
  void set_audio_format(uint32_t sampleRate, uint8_t channels);
  void set_interpolation_mode(int mode);

  void reset(SoundMap sm, bool resetParts = false);

  void panic(void);

  // Mute all parts. Similar to push MUTE-button on real hardware
  void mute(void);

  // Unmute all parts. Similar to push MUTE-button on real hardware
  void unmute(void);

  // Mute 1-n parts
  void mute_parts(std::vector<uint8_t> parts);

  // Unute all parts
  void unmute_parts(std::vector<uint8_t> parts);

  // Returns libEmuSC version as a string
  static std::string version(void);

  // REMOVE!
  bool get_part_mute(uint8_t partId);
  uint8_t get_part_instrument(uint8_t partId, uint8_t &bank);
  void set_part_mute(uint8_t partId, bool mute);
  void set_part_instrument(uint8_t partId, uint8_t index, uint8_t bank);

  void add_part_midi_mod_callback(std::function<void(const int)> callback);
  void clear_part_midi_mod_callback(void);

  void set_part_envelope_callback(int partId,
                                  std::function<void(const float, const float,
                                                     const float, const float,
                                                     const float, const float)> callback);
  void clear_part_envelope_callback(int partId);
  void set_part_lfo_callback(int partId,
                             std::function<void(const float, const float,
                                                const float)> callback);
  void clear_part_lfo_callback(int partId);

  // EmuSC clients methods for getting synth paramters
  uint8_t  get_param(enum SystemParam sp);
  uint8_t* get_param_ptr(enum SystemParam sp);
  uint16_t get_param_32nib(enum SystemParam sp);
  uint8_t  get_param(enum PatchParam pp, int8_t part = -1);
  uint8_t* get_param_ptr(enum PatchParam pp, int8_t part = -1);
  uint16_t get_param_uint14(enum PatchParam pp, int8_t part = -1);
  uint8_t  get_param_nib16(enum PatchParam pp, int8_t part = -1);
  uint8_t  get_patch_param(uint16_t address, int8_t part = -1);
  uint8_t  get_param(enum DrumParam, uint8_t map, uint8_t key);
  int8_t* get_param_ptr(enum DrumParam, uint8_t map);

  // EmuSC clients methods for setting synth paramters
  void set_param(enum SystemParam sp, uint8_t value);
  void set_param(enum SystemParam sp, uint32_t value);
  void set_param(enum SystemParam sp, const uint8_t *data, uint8_t size = 1);
  void set_param_32nib(enum SystemParam sp, uint16_t value);
  void set_param(enum PatchParam pp, uint8_t value, int8_t part = -1);
  void set_param(enum PatchParam sp, const uint8_t *data, uint8_t size = 1,
		 int8_t part = -1);
  void set_param_uint14(enum PatchParam pp, uint16_t value, int8_t part = -1);
  void set_param_nib16(enum PatchParam pp, uint8_t value, int8_t part = -1);
  void set_patch_param(uint16_t address, uint8_t value, int8_t part = 1);
  void set_param(enum DrumParam dp, uint8_t map, uint8_t key, uint8_t value);
  void set_param(enum DrumParam dp, uint8_t map, const uint8_t *data, uint8_t length);

  /* End of public API. Below are internal data structures only */

private:
  Settings *_settings;
  
  uint32_t _sampleRate;
  uint8_t _channels;

  std::mutex midiMutex;

  struct std::vector<Part> _parts;
  std::vector<std::function<void(const int)>> _partMidiModCallbacks;

  ControlRom &_ctrlRom;
  PcmRom &_pcmRom;

  // MIDI message types
  static const uint8_t midi_NoteOff         = 0x80;
  static const uint8_t midi_NoteOn          = 0x90;
  static const uint8_t midi_PolyKeyPressure = 0xa0;
  static const uint8_t midi_CtrlChange      = 0xb0;
  static const uint8_t midi_PrgChange       = 0xc0;
  static const uint8_t midi_ChPressure      = 0xd0;
  static const uint8_t midi_PitchBend       = 0xe0;

  void _init_parts(void);
// int _export_sample_24(std::vector<int32_t> &sampleSet, std::string filename);
  void _add_note(uint8_t midiChannel, uint8_t key, uint8_t velocity);

  void _midi_input_sysex_DT1(uint8_t model, const uint8_t *data, uint16_t length);

  Synth();
};

}

#endif  // __SYNTH_H__
