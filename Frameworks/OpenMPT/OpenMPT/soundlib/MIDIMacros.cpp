/*
 * MIDIMacros.cpp
 * --------------
 * Purpose: Helper functions / classes for MIDI Macro functionality.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "../soundlib/MIDIEvents.h"
#include "MIDIMacros.h"
#include "../common/mptStringBuffer.h"
#include "../common/misc_util.h"

#ifdef MODPLUG_TRACKER
#include "Sndfile.h"
#include "plugins/PlugInterface.h"
#endif // MODPLUG_TRACKER

OPENMPT_NAMESPACE_BEGIN

ParameteredMacro MIDIMacroConfig::GetParameteredMacroType(uint32 macroIndex) const
{
	const std::string macro = GetSafeMacro(szMidiSFXExt[macroIndex]);

	for(uint32 i = 0; i < kSFxMax; i++)
	{
		ParameteredMacro sfx = static_cast<ParameteredMacro>(i);
		if(sfx != kSFxCustom)
		{
			if(macro.compare(CreateParameteredMacro(sfx)) == 0) return sfx;
		}
	}

	// Special macros with additional "parameter":
	if (macro.compare(CreateParameteredMacro(kSFxCC, MIDIEvents::MIDICC_start)) >= 0 && macro.compare(CreateParameteredMacro(kSFxCC, MIDIEvents::MIDICC_end)) <= 0 && macro.size() == 5)
		return kSFxCC;
	if (macro.compare(CreateParameteredMacro(kSFxPlugParam, 0)) >= 0 && macro.compare(CreateParameteredMacro(kSFxPlugParam, 0x17F)) <= 0 && macro.size() == 7)
		return kSFxPlugParam; 

	return kSFxCustom;	// custom / unknown
}


// Retrieve Zxx (Z80-ZFF) type from current macro configuration
FixedMacro MIDIMacroConfig::GetFixedMacroType() const
{
	// Compare with all possible preset patterns
	for(uint32 i = 0; i < kZxxMax; i++)
	{
		FixedMacro zxx = static_cast<FixedMacro>(i);
		if(zxx != kZxxCustom)
		{
			// Prepare macro pattern to compare
			Macro macros[128];
			CreateFixedMacro(macros, zxx);

			bool found = true;
			for(uint32 j = 0; j < 128; j++)
			{
				if(strncmp(macros[j], szMidiZXXExt[j], MACRO_LENGTH))
				{
					found = false;
					break;
				}
			}
			if(found) return zxx;
		}
	}
	return kZxxCustom; // Custom setup
}


void MIDIMacroConfig::CreateParameteredMacro(Macro &parameteredMacro, ParameteredMacro macroType, int subType) const
{
	switch(macroType)
	{
	case kSFxUnused:     mpt::String::WriteAutoBuf(parameteredMacro) = ""; break;
	case kSFxCutoff:     mpt::String::WriteAutoBuf(parameteredMacro) = "F0F000z"; break;
	case kSFxReso:       mpt::String::WriteAutoBuf(parameteredMacro) = "F0F001z"; break;
	case kSFxFltMode:    mpt::String::WriteAutoBuf(parameteredMacro) = "F0F002z"; break;
	case kSFxDryWet:     mpt::String::WriteAutoBuf(parameteredMacro) = "F0F003z"; break;
	case kSFxCC:         mpt::String::WriteAutoBuf(parameteredMacro) = mpt::format("Bc%1z")(mpt::fmt::HEX0<2>(subType & 0x7F)); break;
	case kSFxPlugParam:  mpt::String::WriteAutoBuf(parameteredMacro) = mpt::format("F0F%1z")(mpt::fmt::HEX0<3>(std::min(subType, 0x17F) + 0x80)); break;
	case kSFxChannelAT:  mpt::String::WriteAutoBuf(parameteredMacro) = "Dcz"; break;
	case kSFxPolyAT:     mpt::String::WriteAutoBuf(parameteredMacro) = "Acnz"; break;
	case kSFxPitch:      mpt::String::WriteAutoBuf(parameteredMacro) = "Ec00z"; break;
	case kSFxProgChange: mpt::String::WriteAutoBuf(parameteredMacro) = "Ccz"; break;
	case kSFxCustom:
	default:
		MPT_ASSERT_NOTREACHED();
		break;
	}
}


// Create Zxx (Z80 - ZFF) from preset
void MIDIMacroConfig::CreateFixedMacro(Macro (&fixedMacros)[128], FixedMacro macroType) const
{
	for(uint32 i = 0; i < 128; i++)
	{
		const char *str = "";
		uint32 param = i;
		switch(macroType)
		{
		case kZxxUnused: str = ""; break;
		case kZxxReso4Bit:
			param = i * 8;
			if(i < 16)
				str = "F0F001%1";
			else
				str = "";
			break;
		case kZxxReso7Bit: str = "F0F001%1"; break;
		case kZxxCutoff:   str = "F0F000%1"; break;
		case kZxxFltMode:  str = "F0F002%1"; break;
		case kZxxResoFltMode:
			param = (i & 0x0F) * 8;
			if(i < 16)
				str = "F0F001%1";
			else if(i < 32)
				str = "F0F002%1";
			else
				str = "";
			break;
		case kZxxChannelAT:  str = "Dc%1"; break;
		case kZxxPolyAT:     str = "Acn%1"; break;
		case kZxxPitch:      str = "Ec00%1"; break;
		case kZxxProgChange: str = "Cc%1"; break;

		case kZxxCustom:
		default:
			MPT_ASSERT_NOTREACHED();
			continue;
		}

		mpt::String::WriteAutoBuf(fixedMacros[i]) = mpt::format(str)(mpt::fmt::HEX0<2>(param));
	}
}


#ifdef MODPLUG_TRACKER

bool MIDIMacroConfig::operator== (const MIDIMacroConfig &other) const
{
	for(auto left = begin(), right = other.begin(); left != end(); left++, right++)
	{
		if(strncmp(*left, *right, MACRO_LENGTH))
			return false;
	}
	return true;
}


// Returns macro description including plugin parameter / MIDI CC information
CString MIDIMacroConfig::GetParameteredMacroName(uint32 macroIndex, IMixPlugin *plugin) const
{
	const ParameteredMacro macroType = GetParameteredMacroType(macroIndex);

	switch(macroType)
	{
	case kSFxPlugParam:
		{
			const int param = MacroToPlugParam(macroIndex);
			CString formattedName;
			formattedName.Format(_T("Param %d"), param);
#ifndef NO_PLUGINS
			if(plugin != nullptr)
			{
				CString paramName = plugin->GetParamName(param);
				if(!paramName.IsEmpty())
				{
					formattedName += _T(" (") + paramName + _T(")");
				}
			} else
#else
			MPT_UNREFERENCED_PARAMETER(plugin);
#endif // NO_PLUGINS
			{
				formattedName += _T(" (N/A)");
			}
			return formattedName;
		}

	case kSFxCC:
		{
			CString formattedCC;
			formattedCC.Format(_T("MIDI CC %d"), MacroToMidiCC(macroIndex));
			return formattedCC;
		}

	default:
		return GetParameteredMacroName(macroType);
	}
}


// Returns generic macro description.
CString MIDIMacroConfig::GetParameteredMacroName(ParameteredMacro macroType) const
{
	switch(macroType)
	{
	case kSFxUnused:     return _T("Unused");
	case kSFxCutoff:     return _T("Set Filter Cutoff");
	case kSFxReso:       return _T("Set Filter Resonance");
	case kSFxFltMode:    return _T("Set Filter Mode");
	case kSFxDryWet:     return _T("Set Plugin Dry/Wet Ratio");
	case kSFxPlugParam:  return _T("Control Plugin Parameter...");
	case kSFxCC:         return _T("MIDI CC...");
	case kSFxChannelAT:  return _T("Channel Aftertouch");
	case kSFxPolyAT:     return _T("Polyphonic Aftertouch");
	case kSFxPitch:      return _T("Pitch Bend");
	case kSFxProgChange: return _T("MIDI Program Change");
	case kSFxCustom:
	default:             return _T("Custom");
	}
}


// Returns generic macro description.
CString MIDIMacroConfig::GetFixedMacroName(FixedMacro macroType) const
{
	switch(macroType)
	{
	case kZxxUnused:      return _T("Unused");
	case kZxxReso4Bit:    return _T("Z80 - Z8F controls Resonant Filter Resonance");
	case kZxxReso7Bit:    return _T("Z80 - ZFF controls Resonant Filter Resonance");
	case kZxxCutoff:      return _T("Z80 - ZFF controls Resonant Filter Cutoff");
	case kZxxFltMode:     return _T("Z80 - ZFF controls Resonant Filter Mode");
	case kZxxResoFltMode: return _T("Z80 - Z9F controls Resonance + Filter Mode");
	case kZxxChannelAT:   return _T("Z80 - ZFF controls Channel Aftertouch");
	case kZxxPolyAT:      return _T("Z80 - ZFF controls Polyphonic Aftertouch");
	case kZxxPitch:       return _T("Z80 - ZFF controls Pitch Bend");
	case kZxxProgChange:  return _T("Z80 - ZFF controls MIDI Program Change");
	case kZxxCustom:
	default:              return _T("Custom");
	}
}


int MIDIMacroConfig::MacroToPlugParam(uint32 macroIndex) const
{
	const std::string macro = GetSafeMacro(szMidiSFXExt[macroIndex]);

	int code = 0;
	const char *param = macro.c_str();
	param += 4;
	if ((param[0] >= '0') && (param[0] <= '9')) code = (param[0] - '0') << 4; else
		if ((param[0] >= 'A') && (param[0] <= 'F')) code = (param[0] - 'A' + 0x0A) << 4;
	if ((param[1] >= '0') && (param[1] <= '9')) code += (param[1] - '0'); else
		if ((param[1] >= 'A') && (param[1] <= 'F')) code += (param[1] - 'A' + 0x0A);

	if (macro.size() >= 4 && macro.at(3) == '0')
		return (code - 128);
	else
		return (code + 128);
}


int MIDIMacroConfig::MacroToMidiCC(uint32 macroIndex) const
{
	const std::string macro = GetSafeMacro(szMidiSFXExt[macroIndex]);

	int code = 0;
	const char *param = macro.c_str();
	param += 2;
	if ((param[0] >= '0') && (param[0] <= '9')) code = (param[0] - '0') << 4; else
		if ((param[0] >= 'A') && (param[0] <= 'F')) code = (param[0] - 'A' + 0x0A) << 4;
	if ((param[1] >= '0') && (param[1] <= '9')) code += (param[1] - '0'); else
		if ((param[1] >= 'A') && (param[1] <= 'F')) code += (param[1] - 'A' + 0x0A);

	return code;
}


int MIDIMacroConfig::FindMacroForParam(PlugParamIndex param) const
{
	for(int macroIndex = 0; macroIndex < NUM_MACROS; macroIndex++)
	{
		if(GetParameteredMacroType(macroIndex) == kSFxPlugParam && MacroToPlugParam(macroIndex) == param)
		{
			return macroIndex;
		}
	}
	return -1;
}

#endif // MODPLUG_TRACKER


// Check if the MIDI Macro configuration used is the default one,
// i.e. the configuration that is assumed when loading a file that has no macros embedded.
bool MIDIMacroConfig::IsMacroDefaultSetupUsed() const
{
	const MIDIMacroConfig defaultConfig;

	// TODO - Global macros (currently not checked because they are not editable)

	// SF0: Z00-Z7F controls cutoff, all other parametered macros are unused
	for(uint32 i = 0; i < NUM_MACROS; i++)
	{
		if(GetParameteredMacroType(i) != defaultConfig.GetParameteredMacroType(i))
		{
			return false;
		}
	}

	// Z80-Z8F controls resonance
	if(GetFixedMacroType() != defaultConfig.GetFixedMacroType())
	{
		return false;
	}

	return true;
}


// Reset MIDI macro config to default values.
void MIDIMacroConfig::Reset()
{
	MemsetZero(szMidiGlb);
	MemsetZero(szMidiSFXExt);
	MemsetZero(szMidiZXXExt);

	strcpy(szMidiGlb[MIDIOUT_START], "FF");
	strcpy(szMidiGlb[MIDIOUT_STOP], "FC");
	strcpy(szMidiGlb[MIDIOUT_NOTEON], "9c n v");
	strcpy(szMidiGlb[MIDIOUT_NOTEOFF], "9c n 0");
	strcpy(szMidiGlb[MIDIOUT_PROGRAM], "Cc p");
	// SF0: Z00-Z7F controls cutoff
	CreateParameteredMacro(0, kSFxCutoff);
	// Z80-Z8F controls resonance
	CreateFixedMacro(kZxxReso4Bit);
}


// Clear all Zxx macros so that they do nothing.
void MIDIMacroConfig::ClearZxxMacros()
{
	MemsetZero(szMidiSFXExt);
	MemsetZero(szMidiZXXExt);
}


// Sanitize all macro config strings.
void MIDIMacroConfig::Sanitize()
{
	for(auto &macro : *this)
	{
		mpt::String::FixNullString(macro);
	}
}


// Helper function for UpgradeMacros()
void MIDIMacroConfig::UpgradeMacroString(Macro &macro) const
{
	for(auto &c : macro)
	{
		if(c >= 'a' && c <= 'f') // Both A-F and a-f were treated as hex constants
		{
			c = c - 'a' + 'A';
		} else if(c == 'K' || c == 'k') // Channel was K or k
		{
			c = 'c';
		} else if(c == 'X' || c == 'x' || c == 'Y' || c == 'y') // Those were pointless
		{
			c = 'z';
		}
	}
}


// Fix old-format (not conforming to IT's MIDI macro definitions) MIDI config strings.
void MIDIMacroConfig::UpgradeMacros()
{
	for(auto &macro : *this)
	{
		UpgradeMacroString(macro);
	}
}


// Normalize by removing blanks and other unwanted characters from macro strings for internal usage.
std::string MIDIMacroConfig::GetSafeMacro(const Macro &macro) const
{
	std::string sanitizedMacro = macro;

	std::string::size_type pos;
	while((pos = sanitizedMacro.find_first_not_of("0123456789ABCDEFabchmnopsuvxyz")) != std::string::npos)
	{
		sanitizedMacro.erase(pos, 1);
	}

	return sanitizedMacro;
}


OPENMPT_NAMESPACE_END
