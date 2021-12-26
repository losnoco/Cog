/*
 * MIDIMacros.h
 * ------------
 * Purpose: Helper functions / classes for MIDI Macro functionality.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "openmpt/base/Endian.hpp"

OPENMPT_NAMESPACE_BEGIN

enum
{
	NUM_MACROS = 16,	// number of parametered macros
	MACRO_LENGTH = 32,	// max number of chars per macro
};

OPENMPT_NAMESPACE_END

#ifdef MODPLUG_TRACKER
#include "plugins/PluginStructs.h"
#endif // MODPLUG_TRACKER

OPENMPT_NAMESPACE_BEGIN

// Parametered macro presets
enum ParameteredMacro
{
	kSFxUnused = 0,
	kSFxCutoff,     // Z00 - Z7F controls resonant filter cutoff
	kSFxReso,       // Z00 - Z7F controls resonant filter resonance
	kSFxFltMode,    // Z00 - Z7F controls resonant filter mode (lowpass / highpass)
	kSFxDryWet,     // Z00 - Z7F controls plugin Dry / Wet ratio
	kSFxPlugParam,  // Z00 - Z7F controls a plugin parameter
	kSFxCC,         // Z00 - Z7F controls MIDI CC
	kSFxChannelAT,  // Z00 - Z7F controls Channel Aftertouch
	kSFxPolyAT,     // Z00 - Z7F controls Poly Aftertouch
	kSFxPitch,      // Z00 - Z7F controls Pitch Bend
	kSFxProgChange, // Z00 - Z7F controls MIDI Program Change
	kSFxCustom,

	kSFxMax
};


// Fixed macro presets
enum FixedMacro
{
	kZxxUnused = 0,
	kZxxReso4Bit,    // Z80 - Z8F controls resonant filter resonance
	kZxxReso7Bit,    // Z80 - ZFF controls resonant filter resonance
	kZxxCutoff,      // Z80 - ZFF controls resonant filter cutoff
	kZxxFltMode,     // Z80 - ZFF controls resonant filter mode (lowpass / highpass)
	kZxxResoFltMode, // Z80 - Z9F controls resonance + filter mode
	kZxxChannelAT,   // Z80 - ZFF controls Channel Aftertouch
	kZxxPolyAT,      // Z80 - ZFF controls Poly Aftertouch
	kZxxPitch,       // Z80 - ZFF controls Pitch Bend
	kZxxProgChange,  // Z80 - ZFF controls MIDI Program Change
	kZxxCustom,

	kZxxMax
};


// Global macro types
enum
{
	MIDIOUT_START = 0,
	MIDIOUT_STOP,
	MIDIOUT_TICK,
	MIDIOUT_NOTEON,
	MIDIOUT_NOTEOFF,
	MIDIOUT_VOLUME,
	MIDIOUT_PAN,
	MIDIOUT_BANKSEL,
	MIDIOUT_PROGRAM,
};


struct MIDIMacroConfigData
{
	typedef char Macro[MACRO_LENGTH];
	// encoding is ASCII
	Macro szMidiGlb[9];      // Global MIDI macros
	Macro szMidiSFXExt[16];  // Parametric MIDI macros
	Macro szMidiZXXExt[128]; // Fixed MIDI macros

	Macro *begin() { return std::begin(szMidiGlb); }
	const Macro *begin() const { return std::begin(szMidiGlb); }
	Macro *end() { return std::end(szMidiZXXExt); }
	const Macro *end() const { return std::end(szMidiZXXExt); }
};

MPT_BINARY_STRUCT(MIDIMacroConfigData, 4896) // this is directly written to files, so the size must be correct!

class MIDIMacroConfig : public MIDIMacroConfigData
{

public:

	MIDIMacroConfig() { Reset(); }

	// Get macro type from a macro string
	ParameteredMacro GetParameteredMacroType(uint32 macroIndex) const;
	FixedMacro GetFixedMacroType() const;

	// Create a new macro
protected:
	void CreateParameteredMacro(Macro &parameteredMacro, ParameteredMacro macroType, int subType) const;
public:
	void CreateParameteredMacro(uint32 macroIndex, ParameteredMacro macroType, int subType = 0)
	{
		CreateParameteredMacro(szMidiSFXExt[macroIndex], macroType, subType);
	}
	std::string CreateParameteredMacro(ParameteredMacro macroType, int subType = 0) const;

protected:
	void CreateFixedMacro(Macro (&fixedMacros)[128], FixedMacro macroType) const;
public:
	void CreateFixedMacro(FixedMacro macroType)
	{
		CreateFixedMacro(szMidiZXXExt, macroType);
	}

#ifdef MODPLUG_TRACKER

	bool operator== (const MIDIMacroConfig &other) const;
	bool operator!= (const MIDIMacroConfig &other) const { return !(*this == other); }

	// Translate macro type or macro string to macro name
	CString GetParameteredMacroName(uint32 macroIndex, IMixPlugin *plugin = nullptr) const;
	CString GetParameteredMacroName(ParameteredMacro macroType) const;
	CString GetFixedMacroName(FixedMacro macroType) const;

	// Extract information from a parametered macro string.
	int MacroToPlugParam(uint32 macroIndex) const;
	int MacroToMidiCC(uint32 macroIndex) const;

	// Check if any macro can automate a given plugin parameter
	int FindMacroForParam(PlugParamIndex param) const;

#endif // MODPLUG_TRACKER

	// Check if a given set of macros is the default IT macro set.
	bool IsMacroDefaultSetupUsed() const;

	// Reset MIDI macro config to default values.
	void Reset();

	// Clear all Zxx macros so that they do nothing.
	void ClearZxxMacros();

	// Sanitize all macro config strings.
	void Sanitize();

	// Fix old-format (not conforming to IT's MIDI macro definitions) MIDI config strings.
	void UpgradeMacros();

protected:

	// Helper function for FixMacroFormat()
	void UpgradeMacroString(Macro &macro) const;

	// Remove blanks and other unwanted characters from macro strings for internal usage.
	std::string GetSafeMacro(const Macro &macro) const;

};

static_assert(sizeof(MIDIMacroConfig) == sizeof(MIDIMacroConfigData)); // this is directly written to files, so the size must be correct!


OPENMPT_NAMESPACE_END
