/*
 * modcommand.h
 * ------------
 * Purpose: ModCommand declarations and helpers. One ModCommand corresponds to one pattern cell.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "Snd_defs.h"
#include "../common/misc_util.h"

OPENMPT_NAMESPACE_BEGIN

class CSoundFile;

// Note definitions
enum : uint8 // ModCommand::NOTE
{
	NOTE_NONE        = 0,    // Empty note cell
	NOTE_MIN         = 1,    // Minimum note value
	NOTE_MAX         = 120,  // Maximum note value
	NOTE_MIDDLEC     = (5 * 12 + NOTE_MIN),
	NOTE_KEYOFF      = 0xFF, // === (Note Off, releases envelope / fades samples, stops plugin note)
	NOTE_NOTECUT     = 0xFE, // ^^^ (Cuts sample / stops all plugin notes)
	NOTE_FADE        = 0xFD, // ~~~ (Fades samples, stops plugin note)
	NOTE_PC          = 0xFC, // Param Control 'note'. Changes param value on first tick.
	NOTE_PCS         = 0xFB, // Param Control (Smooth) 'note'. Interpolates param value during the whole row.
	NOTE_MIN_SPECIAL = NOTE_PCS,
	NOTE_MAX_SPECIAL = NOTE_KEYOFF,
};


// Volume Column commands
enum VolumeCommand : uint8
{
	VOLCMD_NONE           = 0,
	VOLCMD_VOLUME         = 1,
	VOLCMD_PANNING        = 2,
	VOLCMD_VOLSLIDEUP     = 3,
	VOLCMD_VOLSLIDEDOWN   = 4,
	VOLCMD_FINEVOLUP      = 5,
	VOLCMD_FINEVOLDOWN    = 6,
	VOLCMD_VIBRATOSPEED   = 7,
	VOLCMD_VIBRATODEPTH   = 8,
	VOLCMD_PANSLIDELEFT   = 9,
	VOLCMD_PANSLIDERIGHT  = 10,
	VOLCMD_TONEPORTAMENTO = 11,
	VOLCMD_PORTAUP        = 12,
	VOLCMD_PORTADOWN      = 13,
	VOLCMD_DELAYCUT       = 14, //currently unused
	VOLCMD_OFFSET         = 15,
	MAX_VOLCMDS
};


// Effect column commands
enum EffectCommand : uint8
{
	CMD_NONE                = 0,
	CMD_ARPEGGIO            = 1,
	CMD_PORTAMENTOUP        = 2,
	CMD_PORTAMENTODOWN      = 3,
	CMD_TONEPORTAMENTO      = 4,
	CMD_VIBRATO             = 5,
	CMD_TONEPORTAVOL        = 6,
	CMD_VIBRATOVOL          = 7,
	CMD_TREMOLO             = 8,
	CMD_PANNING8            = 9,
	CMD_OFFSET              = 10,
	CMD_VOLUMESLIDE         = 11,
	CMD_POSITIONJUMP        = 12,
	CMD_VOLUME              = 13,
	CMD_PATTERNBREAK        = 14,
	CMD_RETRIG              = 15,
	CMD_SPEED               = 16,
	CMD_TEMPO               = 17,
	CMD_TREMOR              = 18,
	CMD_MODCMDEX            = 19,
	CMD_S3MCMDEX            = 20,
	CMD_CHANNELVOLUME       = 21,
	CMD_CHANNELVOLSLIDE     = 22,
	CMD_GLOBALVOLUME        = 23,
	CMD_GLOBALVOLSLIDE      = 24,
	CMD_KEYOFF              = 25,
	CMD_FINEVIBRATO         = 26,
	CMD_PANBRELLO           = 27,
	CMD_XFINEPORTAUPDOWN    = 28,
	CMD_PANNINGSLIDE        = 29,
	CMD_SETENVPOSITION      = 30,
	CMD_MIDI                = 31,
	CMD_SMOOTHMIDI          = 32,
	CMD_DELAYCUT            = 33,
	CMD_XPARAM              = 34,
	CMD_NOTESLIDEUP         = 35, // IMF Gxy / PTM Jxy (Slide y notes up every x ticks)
	CMD_NOTESLIDEDOWN       = 36, // IMF Hxy / PTM Kxy (Slide y notes down every x ticks)
	CMD_NOTESLIDEUPRETRIG   = 37, // PTM Lxy (Slide y notes up every x ticks + retrigger note)
	CMD_NOTESLIDEDOWNRETRIG = 38, // PTM Mxy (Slide y notes down every x ticks + retrigger note)
	CMD_REVERSEOFFSET       = 39, // PTM Nxx Revert sample + offset
	CMD_DBMECHO             = 40, // DBM enable/disable echo
	CMD_OFFSETPERCENTAGE    = 41, // PLM Percentage Offset
	CMD_DUMMY               = 42,
	MAX_EFFECTS
};


enum EffectType : uint8
{
	EFFECT_TYPE_NORMAL  = 0,
	EFFECT_TYPE_GLOBAL  = 1,
	EFFECT_TYPE_VOLUME  = 2,
	EFFECT_TYPE_PANNING = 3,
	EFFECT_TYPE_PITCH   = 4,
	MAX_EFFECT_TYPE     = 5
};


class ModCommand
{
public:
	typedef uint8 NOTE;
	typedef uint8 INSTR;
	typedef uint8 VOL;
	typedef uint8 VOLCMD;
	typedef uint8 COMMAND;
	typedef uint8 PARAM;

	// Defines the maximum value for column data when interpreted as 2-byte value
	// (for example volcmd and vol). The valid value range is [0, maxColumnValue].
	static constexpr int maxColumnValue = 999;

	// Returns empty modcommand.
	static ModCommand Empty() { return ModCommand(); }

	bool operator==(const ModCommand& mc) const
	{
		return (note == mc.note)
			&& (instr == mc.instr)
			&& (volcmd == mc.volcmd)
			&& (command == mc.command)
			&& ((volcmd == VOLCMD_NONE && !IsPcNote()) || vol == mc.vol)
			&& ((command == CMD_NONE && !IsPcNote()) || param == mc.param);
	}
	bool operator!=(const ModCommand& mc) const { return !(*this == mc); }

	void Set(NOTE n, INSTR ins, uint16 volcol, uint16 effectcol) { note = n; instr = ins; SetValueVolCol(volcol); SetValueEffectCol(effectcol); }

	uint16 GetValueVolCol() const { return GetValueVolCol(volcmd, vol); }
	static uint16 GetValueVolCol(uint8 volcmd, uint8 vol) { return (volcmd << 8) + vol; }
	void SetValueVolCol(const uint16 val) { volcmd = static_cast<VOLCMD>(val >> 8); vol = static_cast<uint8>(val & 0xFF); }

	uint16 GetValueEffectCol() const { return GetValueEffectCol(command, param); }
	static uint16 GetValueEffectCol(uint8 command, uint8 param) { return (command << 8) + param; }
	void SetValueEffectCol(const uint16 val) { command = static_cast<COMMAND>(val >> 8); param = static_cast<uint8>(val & 0xFF); }

	// Clears modcommand.
	void Clear() { memset(this, 0, sizeof(ModCommand)); }

	// Returns true if modcommand is empty, false otherwise.
	bool IsEmpty() const
	{
		return (note == NOTE_NONE && instr == 0 && volcmd == VOLCMD_NONE && command == CMD_NONE);
	}

	// Returns true if instrument column represents plugin index.
	bool IsInstrPlug() const { return IsPcNote(); }

	// Returns true if and only if note is NOTE_PC or NOTE_PCS.
	bool IsPcNote() const { return IsPcNote(note); }
	static bool IsPcNote(NOTE note) { return note == NOTE_PC || note == NOTE_PCS; }

	// Returns true if and only if note is a valid musical note.
	bool IsNote() const { return IsInRange(note, NOTE_MIN, NOTE_MAX); }
	static bool IsNote(NOTE note) { return IsInRange(note, NOTE_MIN, NOTE_MAX); }
	// Returns true if and only if note is a valid special note.
	bool IsSpecialNote() const { return IsInRange(note, NOTE_MIN_SPECIAL, NOTE_MAX_SPECIAL); }
	static bool IsSpecialNote(NOTE note) { return IsInRange(note, NOTE_MIN_SPECIAL, NOTE_MAX_SPECIAL); }
	// Returns true if and only if note is a valid musical note or the note entry is empty.
	bool IsNoteOrEmpty() const { return note == NOTE_NONE || IsNote(); }
	static bool IsNoteOrEmpty(NOTE note) { return note == NOTE_NONE || IsNote(note); }
	// Returns true if any of the commands in this cell trigger a tone portamento.
	bool IsPortamento() const { return command == CMD_TONEPORTAMENTO || command == CMD_TONEPORTAVOL || volcmd == VOLCMD_TONEPORTAMENTO; }
	// Returns true if the cell contains an effect command that may affect the global state of the module.
	bool IsGlobalCommand() const { return IsGlobalCommand(command, param); }
	static bool IsGlobalCommand(COMMAND command, PARAM param);

	// Returns true if the note is inside the Amiga frequency range
	bool IsAmigaNote() const { return IsAmigaNote(note); }
	static bool IsAmigaNote(NOTE note) { return !IsNote(note) || (note >= NOTE_MIDDLEC - 12 && note < NOTE_MIDDLEC + 24); }

	static EffectType GetEffectType(COMMAND cmd);
	EffectType GetEffectType() const { return GetEffectType(command); }
	static EffectType GetVolumeEffectType(VOLCMD volcmd);
	EffectType GetVolumeEffectType() const { return GetVolumeEffectType(volcmd); }

	// Convert a complete ModCommand item from one format to another
	void Convert(MODTYPE fromType, MODTYPE toType, const CSoundFile &sndFile);
	// Convert MOD/XM Exx to S3M/IT Sxx
	void ExtendedMODtoS3MEffect();
	// Convert S3M/IT Sxx to MOD/XM Exx
	void ExtendedS3MtoMODEffect();

	// "Importance" of every FX command. Table is used for importing from formats with multiple effect columns
	// and is approximately the same as in SchismTracker.
	static size_t GetEffectWeight(COMMAND cmd);
	// Try to convert a an effect into a volume column effect. Returns true on success.
	static bool ConvertVolEffect(uint8 &effect, uint8 &param, bool force);
	// Takes two "normal" effect commands and converts them to volume column + effect column commands. Returns the dropped command + param (CMD_NONE if nothing had to be dropped).
	static std::pair<EffectCommand, PARAM> TwoRegularCommandsToMPT(uint8 &effect1, uint8 &param1, uint8 &effect2, uint8 &param2);
	// Try to combine two commands into one. Returns true on success and the combined command is placed in eff1 / param1.
	static bool CombineEffects(uint8 &eff1, uint8 &param1, uint8 &eff2, uint8 &param2);

public:
	uint8 note = NOTE_NONE;
	uint8 instr = 0;
	uint8 volcmd = VOLCMD_NONE;
	uint8 command = CMD_NONE;
	uint8 vol = 0;
	uint8 param = 0;
};

OPENMPT_NAMESPACE_END
