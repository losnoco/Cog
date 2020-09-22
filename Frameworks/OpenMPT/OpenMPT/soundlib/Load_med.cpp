/*
 * Load_med.cpp
 * ------------
 * Purpose: OctaMED / MED Soundstudio module loader
 * Notes  : Support for synthesized instruments is still missing.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "Loaders.h"

#ifndef NO_VST
#include "../mptrack/Vstplug.h"
#include "plugins/PluginManager.h"
#endif

#include <map>

OPENMPT_NAMESPACE_BEGIN

struct MMD0FileHeader
{
	char     mmd[3];               // "MMD" for the first song in file, "MCN" for the rest
	uint8be  version;              // '0'-'3'
	uint32be modLength;            // Size of file
	uint32be songOffset;           // Position in file for the first song
	uint16be playerSettings1[2];   // Internal variables for the play routine
	uint32be blockArrOffset;       // Position in file for blocks (patterns)
	uint8be  flags;
	uint8be  reserved1[3];
	uint32be sampleArrOffset;      // Position in file for samples (should be identical between songs)
	uint32be reserved2;
	uint32be expDataOffset;        // Absolute offset in file for ExpData (0 if not present)
	uint32be reserved3;
	char     playerSettings2[11];  // Internal variables for the play routine
	uint8be  extraSongs;           // Number of songs - 1
};

MPT_BINARY_STRUCT(MMD0FileHeader, 52)


struct MMD0Sample
{
	uint16be loopStart;
	uint16be loopLength;
	uint8be  midiChannel;
	uint8be  midiPreset;
	uint8be  sampleVolume;
	int8be   sampleTranspose;
};

MPT_BINARY_STRUCT(MMD0Sample, 8)


// Song header for MMD0/MMD1
struct MMD0Song
{
	uint8be sequence[256];
};

MPT_BINARY_STRUCT(MMD0Song, 256)


// Song header for MMD2/MMD3
struct MMD2Song
{
	enum Flags3
	{
		FLAG3_STEREO  = 0x01,  // Mixing in stereo
		FLAG3_FREEPAN = 0x02,  // Mixing flag: free pan
	};

	uint32be playSeqTableOffset;
	uint32be sectionTableOffset;
	uint32be trackVolsOffset;
	uint16be numTracks;
	uint16be numPlaySeqs;
	uint32be trackPanOffset;  // 0: all centered (according to docs, MED Soundstudio uses Amiga hard-panning instead)
	uint32be flags3;
	uint16be volAdjust;       // Volume adjust (%)
	uint16be mixChannels;     // Mixing channels, 0 means 4
	uint8be  mixEchoType;     // 0 = nothing, 1 = normal, 2 = cross
	uint8be  mixEchoDepth;    // 1 - 6, 0 = default
	uint16be mixEchoLength;   // Echo length in milliseconds
	int8be   mixStereoSep;    // Stereo separation
	char     pad0[223];
};

MPT_BINARY_STRUCT(MMD2Song, 256)


// Common song header
struct MMDSong
{
	enum Flags
	{
		FLAG_FILTERON   = 0x01,  // The hardware audio filter is on
		FLAG_JUMPINGON  = 0x02,  // Mouse pointer jumping on
		FLAG_JUMP8TH    = 0x04,  // ump every 8th line (not in OctaMED Pro)
		FLAG_INSTRSATT  = 0x08,  // sng+samples indicator (not useful in MMDs)
		FLAG_VOLHEX     = 0x10,  // volumes are HEX
		FLAG_STSLIDE    = 0x20,  // use ST/NT/PT compatible sliding
		FLAG_8CHANNEL   = 0x40,  // this is OctaMED 5-8 channel song
		FLAG_SLOWHQ     = 0x80,  // HQ V2-4 compatibility mode
	};

	enum Flags2
	{
		FLAG2_BMASK = 0x1F,  // (bits 0-4) BPM beat length (in lines)
		FLAG2_BPM   = 0x20,  // BPM mode on
		FLAG2_MIX   = 0x80,  // Module uses mixing
	};

	uint16be numBlocks;   // Number of blocks in current song
	uint16be songLength;  // MMD0: Number of sequence numbers in the play sequence list, MMD2: Number of sections
	char song[256];
	MMD0Song GetMMD0Song() const
	{
		static_assert(sizeof(MMD0Song) == sizeof(song));
		MMD0Song result;
		std::memcpy(&result, song, sizeof(result));
		return result;
	}
	MMD2Song GetMMD2Song() const
	{
		static_assert(sizeof(MMD2Song) == sizeof(song));
		MMD2Song result;
		std::memcpy(&result, song, sizeof(result));
		return result;
	}
	uint16be defaultTempo;
	int8be   playTranspose;  // The global play transpose value for current song
	uint8be  flags;
	uint8be  flags2;
	uint8be  tempo2;        // Timing pulses per line (ticks)
	uint8be  trackVol[16];  // 1...64 in MMD0/MMD1, reserved in MMD2
	uint8be  masterVol;     // 1...64
	uint8be  numSamples;
};

MPT_BINARY_STRUCT(MMDSong, 284)


struct MMD2PlaySeq
{
	char     name[32];
	uint32be commandTableOffset;
	uint32be reserved;
	uint16be length;  // Number of entries
};

MPT_BINARY_STRUCT(MMD2PlaySeq, 42)


struct MMD0PatternHeader
{
	uint8be numTracks;
	uint8be numRows;
};

MPT_BINARY_STRUCT(MMD0PatternHeader, 2)


struct MMD1PatternHeader
{
	uint16be numTracks;
	uint16be numRows;
	uint32be blockInfoOffset;
};

MPT_BINARY_STRUCT(MMD1PatternHeader, 8)


struct MMDPlaySeqCommand
{
	enum Command
	{
		kStop = 1,
		kJump = 2,
	};

	uint16be offset;   // Offset within current play sequence, 0xFFFF = end of list
	uint8be  command;  // Stop = 1, Jump = 2
	uint8be  extraSize;
};

MPT_BINARY_STRUCT(MMDPlaySeqCommand, 4)


struct MMDBlockInfo
{
	uint32be highlightMaskOffset;
	uint32be nameOffset;
	uint32be nameLength;
	uint32be pageTableOffset;    // File offset of command page table
	uint32be cmdExtTableOffset;  // File offset of command extension table (second parameter)
	uint32be reserved[4];
};

MPT_BINARY_STRUCT(MMDBlockInfo, 36)


struct MMDInstrHeader
{
	enum Types
	{
		VSTI      = -4,
		HIGHLIFE  = -3,
		HYBRID    = -2,
		SYNTHETIC = -1,
		SAMPLE    =  0,  // an ordinary 1-octave sample (or MIDI)
		IFF5OCT   =  1,  // 5 octaves
		IFF3OCT   =  2,  // 3 octaves
		// The following ones are recognized by OctaMED Pro only
		IFF2OCT   = 3,  // 2 octaves
		IFF4OCT   = 4,  // 4 octaves
		IFF6OCT   = 5,  // 6 octaves
		IFF7OCT   = 6,  // 7 octaves
		// OctaMED Pro V5 + later
		EXTSAMPLE = 7,  // two extra-low octaves

		TYPEMASK  = 0x0F,

		S_16          = 0x10,
		STEREO        = 0x20,
		DELTA         = 0x40,
		PACKED        = 0x80,  // MMDPackedSampleHeader follows
		OBSOLETE_MD16 = 0x18,
	};

	uint32be length;
	int16be  type;
};

MPT_BINARY_STRUCT(MMDInstrHeader, 6)


struct MMDPackedSampleHeader
{
	uint16be packType;    // Only 1 = ADPCM is supported
	uint16be subType;     // Packing subtype
	                      // ADPCM subtype
	                      // 1: g723_40
	                      // 2: g721
	                      // 3: g723_24
	uint8be commonFlags;  // flags common to all packtypes (none defined so far)
	uint8be packerFlags;  // flags for the specific packtype
	uint32be leftChLen;   // packed length of left channel in bytes
	uint32be rightChLen;  // packed length of right channel in bytes (ONLY PRESENT IN STEREO SAMPLES)
};

MPT_BINARY_STRUCT(MMDPackedSampleHeader, 14)


struct MMDInstrExt
{
	enum
	{
		SSFLG_LOOP     = 0x01, // Loop On / Off
		SSFLG_EXTPSET  = 0x02, // Ext.Preset
		SSFLG_DISABLED = 0x04, // Disabled
		SSFLG_PINGPONG = 0x08, // Ping-pong looping
	};

	uint8be  hold;   // 0...127
	uint8be  decay;  // 0...127
	uint8be  suppressMidiOff;
	int8be   finetune;
	// Below fields saved by >= V5
	uint8be  defaultPitch;
	uint8be  instrFlags;
	uint16be longMidiPreset;
	// Below fields saved by >= V5.02
	uint8be  outputDevice;
	uint8be  reserved;
	// Below fields saved by >= V7
	uint32be loopStart;
	uint32be loopLength;
};

MPT_BINARY_STRUCT(MMDInstrExt, 18)


struct MMDInstrInfo
{
	char name[40];
};

MPT_BINARY_STRUCT(MMDInstrInfo, 40)


struct MMD0Exp
{
	uint32be nextModOffset;
	uint32be instrExtOffset;
	uint16be instrExtEntries;
	uint16be instrExtEntrySize;
	uint32be annoText;
	uint32be annoLength;
	uint32be instrInfoOffset;
	uint16be instrInfoEntries;
	uint16be instrInfoEntrySize;
	uint32be jumpMask;
	uint32be rgbTable;
	uint8be  channelSplit[4];
	uint32be notationInfoOffset;
	uint32be songNameOffset;
	uint32be songNameLength;
	uint32be midiDumpOffset;
	uint32be mmdInfoOffset;
	uint32be arexxOffset;
	uint32be midiCmd3xOffset;
	uint32be trackInfoOffset;   // Pointer to song->numtracks pointers to tag lists
	uint32be effectInfoOffset;  // Pointers to group pointers
	uint32be tagEnd;
};

MPT_BINARY_STRUCT(MMD0Exp, 80)


struct MMDTag
{
	enum TagType
	{
		// Generic MMD tags
		MMDTAG_END      = 0x00000000,
		MMDTAG_PTR      = 0x80000000,  // Data needs relocation
		MMDTAG_MUSTKNOW = 0x40000000,  // Loader must fail if this isn't recognized
		MMDTAG_MUSTWARN = 0x20000000,  // Loader must warn if this isn't recognized
		MMDTAG_MASK     = 0x1FFFFFFF,

		// ExpData tags
		// # of effect groups, including the global group (will override settings in MMDSong struct), default = 1
		MMDTAG_EXP_NUMFXGROUPS = 1,
		MMDTAG_TRK_FXGROUP     = 3,

		MMDTAG_TRK_NAME    = 1,  // trackinfo tags
		MMDTAG_TRK_NAMELEN = 2,  // namelen includes zero term.

		// effectinfo tags
		MMDTAG_FX_ECHOTYPE   = 1,
		MMDTAG_FX_ECHOLEN    = 2,
		MMDTAG_FX_ECHODEPTH  = 3,
		MMDTAG_FX_STEREOSEP  = 4,
		MMDTAG_FX_GROUPNAME  = 5,  // the Global Effects group shouldn't have name saved!
		MMDTAG_FX_GRPNAMELEN = 6,  // namelen includes zero term.
	};

	uint32be type;
	uint32be data;
};

MPT_BINARY_STRUCT(MMDTag, 8)


static TEMPO MMDTempoToBPM(uint32 tempo, bool is8Ch, bool bpmMode, uint8 rowsPerBeat)
{
	if(bpmMode)
	{
		// You would have thought that we could use modern tempo mode here.
		// Alas, the number of ticks per row still influences the tempo. :(
		return TEMPO((tempo * rowsPerBeat) / 4.0);
	}
	if(is8Ch && tempo > 0)
	{
		LimitMax(tempo, 10u);
		static constexpr uint8 tempos[10] = { 47, 43, 40, 37, 35, 32, 30, 29, 27, 26 };
		tempo = tempos[tempo - 1];
	} else if(tempo > 0 && tempo <= 10)
	{
		// SoundTracker compatible tempo
		return TEMPO((6.0 * 1773447.0 / 14500.0) / tempo);
	}

	return TEMPO(tempo / 0.264);
}


static void ConvertMEDEffect(ModCommand &m, bool is8ch, bool bpmMode, uint8 rowsPerBeat, bool volHex)
{
	switch(m.command)
	{
	case 0x04:  // Vibrato (twice as deep as in ProTracker)
		m.command = CMD_VIBRATO;
		m.param = (std::min<uint8>(m.param >> 3, 0x0F) << 4) | std::min<uint8>((m.param & 0x0F) * 2, 0x0F);
		break;
	case 0x08:  // Hold and decay
		m.command = CMD_NONE;
		break;
	case 0x09:  // Set secondary speed
		if(m.param > 0 && m.param <= 20)
			m.command = CMD_SPEED;
		else
			m.command = CMD_NONE;
		break;
	case 0x0C:  // Set Volume
		m.command = CMD_VOLUME;
		if(!volHex && m.param < 0x99)
			m.param = (m.param >> 4) * 10 + (m.param & 0x0F);
		else if(volHex)
			m.param = ((m.param & 0x7F) + 1) / 2;
		else
			m.command = CMD_NONE;
		break;
	case 0x0D:
		m.command = CMD_VOLUMESLIDE;
		break;
	case 0x0E:  // Synth jump
		m.command = CMD_NONE;
		break;
	case 0x0F:  // Misc
		if(m.param == 0)
		{
			m.command = CMD_PATTERNBREAK;
		} else if(m.param <= 0xF0)
		{
			m.command = CMD_TEMPO;
			m.param = mpt::saturate_round<ModCommand::PARAM>(MMDTempoToBPM(m.param, is8ch, bpmMode, rowsPerBeat).ToDouble());
		} else switch(m.command)
		{
			case 0xF1:  // Play note twice
				m.command = CMD_MODCMDEX;
				m.param = 0x93;
				break;
			case 0xF2:  // Delay note
				m.command = CMD_MODCMDEX;
				m.param = 0xD3;
				break;
			case 0xF3:  // Play note three times
				m.command = CMD_MODCMDEX;
				m.param = 0x92;
				break;
			case 0xF8:  // Turn filter off
			case 0xF9:  // Turn filter on
				m.command = CMD_MODCMDEX;
				m.param = 0xF9 - m.param;
				break;
			case 0xFA:  // MIDI pedal on
			case 0xFB:  // MIDI pedal off
			case 0xFD:  // Set pitch
			case 0xFE:  // End of song
				m.command = CMD_NONE;
				break;
			case 0xFF:  // Turn note off
				m.note = NOTE_NOTECUT;
				m.command = CMD_NONE;
				break;
			default:
				m.command = CMD_NONE;
				break;
		}
		break;
	case 0x11:  // Slide pitch up
		m.command = CMD_MODCMDEX;
		m.param = 0x10 | std::min<uint8>(m.param, 0x0F);
		break;
	case 0x12:  // Slide pitch down
		m.command = CMD_MODCMDEX;
		m.param = 0x20 | std::min<uint8>(m.param, 0x0F);
		break;
	case 0x14:  // Vibrato (ProTracker compatible depth, but faster)
		m.command = CMD_VIBRATO;
		m.param = (std::min<uint8>((m.param >> 4) + 1, 0x0F) << 4) | (m.param & 0x0F);
		break;
	case 0x15:  // Set finetune
		m.command = CMD_MODCMDEX;
		m.param = 0x50 | (m.param & 0x0F);
		break;
	case 0x16:  // Loop
		m.command = CMD_MODCMDEX;
		m.param = 0x60 | std::min<uint8>(m.param, 0x0F);
		break;
	case 0x18:  // Stop note
		m.command = CMD_MODCMDEX;
		m.param = 0xC0 | std::min<uint8>(m.param, 0x0F);
		break;
	case 0x19:  // Sample Offset
		m.command = CMD_OFFSET;
		break;
	case 0x1A:  // Slide volume up once
		m.command = CMD_MODCMDEX;
		m.param = 0xA0 | std::min<uint8>(m.param, 0x0F);
		break;
	case 0x1B:  // Slide volume down once
		m.command = CMD_MODCMDEX;
		m.param = 0xB0 | std::min<uint8>(m.param, 0x0F);
		break;
	case 0x1D:  // Pattern break (in hex)
		m.command = CMD_PATTERNBREAK;
		break;
	case 0x1E:  // Repeat row
		m.command = CMD_MODCMDEX;
		m.param = 0xE0 | std::min<uint8>(m.param, 0x0F);
		break;
	case 0x1F:  // Note delay and retrigger
	{
		if(m.param & 0xF0)
		{
			m.command = CMD_MODCMDEX;
			m.param = 0xD0 | (m.param >> 4);
		} else if(m.param & 0x0F)
		{
			m.command = CMD_MODCMDEX;
			m.param = 0x90 | m.param;
		} else
		{
			m.command = CMD_NONE;
		}
		break;
	}
	case 0x20:  // Reverse sample + skip samples
		if(m.param == 0 && m.vol == 0)
		{
			m.command = CMD_S3MCMDEX;
			m.param = 0x9F;
		} else
		{
			// Skip given number of samples
			m.command = CMD_NONE;
		}
		break;
	case 0x29:  // Relative sample offset
		if(m.vol > 0)
		{
			m.command = CMD_OFFSETPERCENTAGE;
			m.param = mpt::saturate_cast<ModCommand::PARAM>(Util::muldiv_unsigned(m.param, 0x100, m.vol));
		} else
		{
			m.command = CMD_NONE;
		}
		break;
	case 0x2E:  // Set panning
		if(m.param <= 0x10 || m.param >= 0xF0)
		{
			m.command = CMD_PANNING8;
			m.param = mpt::saturate_cast<ModCommand::PARAM>(((m.param ^ 0x80) - 0x70) * 8);
		} else
		{
			m.command = CMD_NONE;
		}
		break;
	default:
		if(m.command < 0x10)
			CSoundFile::ConvertModCommand(m);
		else
			m.command = CMD_NONE;
		break;
	}
}

#ifndef NO_VST
static std::wstring ReadMEDStringUTF16BE(FileReader &file)
{
	auto chunk = file.ReadChunk(file.ReadUint32BE());
	std::wstring s(chunk.GetLength() / 2u, L'\0');
	for(auto &c : s)
	{
		c = chunk.ReadUint16BE();
	}
	return s;
}
#endif


static void MEDReadNextSong(FileReader &file, MMD0FileHeader &fileHeader, MMD0Exp &expData, MMDSong &songHeader)
{
	file.ReadStruct(fileHeader);
	file.Seek(fileHeader.songOffset + 63 * sizeof(MMD0Sample));
	file.ReadStruct(songHeader);
	if(fileHeader.expDataOffset && file.Seek(fileHeader.expDataOffset))
		file.ReadStruct(expData);
	else
		expData = {};
}


static CHANNELINDEX MEDScanNumChannels(FileReader &file, const uint8 version)
{
	MMD0FileHeader fileHeader;
	MMD0Exp expData;
	MMDSong songHeader;

	file.Rewind();
	MEDReadNextSong(file, fileHeader, expData, songHeader);

	auto numSongs = fileHeader.expDataOffset ? fileHeader.extraSongs + 1 : 1;

	CHANNELINDEX numChannels = 4;
	// Scan patterns for max number of channels
	for(SEQUENCEINDEX song = 0; song < numSongs; song++)
	{
		const PATTERNINDEX numPatterns = songHeader.numBlocks;
		if(songHeader.numSamples > 63 || numPatterns > 0x7FFF)
			return 0;

		for(PATTERNINDEX pat = 0; pat < numPatterns; pat++)
		{
			if(!file.Seek(fileHeader.blockArrOffset + pat * 4u)
			   || !file.Seek(file.ReadUint32BE()))
			{
				continue;
			}
			numChannels = std::max(numChannels, static_cast<CHANNELINDEX>(version < 1 ? file.ReadUint8() : file.ReadUint16BE()));
		}

		if(!expData.nextModOffset || !file.Seek(expData.nextModOffset))
			break;
		MEDReadNextSong(file, fileHeader, expData, songHeader);
	}
	return numChannels;
}


static bool ValidateHeader(const MMD0FileHeader &fileHeader)
{
	if(std::memcmp(fileHeader.mmd, "MMD", 3)
	   || fileHeader.version < '0' || fileHeader.version > '3'
	   || fileHeader.songOffset < sizeof(MMD0FileHeader)
	   || fileHeader.songOffset > uint32_max - 63 * sizeof(MMD0Sample) - sizeof(MMDSong)
	   || fileHeader.blockArrOffset < sizeof(MMD0FileHeader)
	   || fileHeader.sampleArrOffset < sizeof(MMD0FileHeader)
	   || fileHeader.expDataOffset > uint32_max - sizeof(MMD0Exp))
	{
		return false;
	}
	return true;
}


static uint64 GetHeaderMinimumAdditionalSize(const MMD0FileHeader &fileHeader)
{
	return std::max<uint64>({ fileHeader.songOffset + 63 * sizeof(MMD0Sample) + sizeof(MMDSong),
		fileHeader.blockArrOffset,
		fileHeader.sampleArrOffset,
		fileHeader.expDataOffset + sizeof(MMD0Exp) }) - sizeof(MMD0FileHeader);
}


CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderMED(MemoryFileReader file, const uint64 *pfilesize)
{
	MMD0FileHeader fileHeader;
	if(!file.ReadStruct(fileHeader))
		return ProbeWantMoreData;
	if(!ValidateHeader(fileHeader))
		return ProbeFailure;
	return ProbeAdditionalSize(file, pfilesize, GetHeaderMinimumAdditionalSize(fileHeader));
}


bool CSoundFile::ReadMED(FileReader &file, ModLoadingFlags loadFlags)
{
	file.Rewind();
	MMD0FileHeader fileHeader;
	if(!file.ReadStruct(fileHeader))
		return false;
	if(!ValidateHeader(fileHeader))
		return false;
	if(!file.CanRead(mpt::saturate_cast<FileReader::off_t>(GetHeaderMinimumAdditionalSize(fileHeader))))
		return false;
	if(loadFlags == onlyVerifyHeader)
		return true;
	
	InitializeGlobals(MOD_TYPE_MED);
	InitializeChannels();
	const uint8 version = fileHeader.version - '0';

	file.Seek(fileHeader.songOffset);
	FileReader sampleHeaderChunk = file.ReadChunk(63 * sizeof(MMD0Sample));

	MMDSong songHeader;
	file.ReadStruct(songHeader);

	if(songHeader.numSamples > 63 || songHeader.numBlocks > 0x7FFF)
		return false;

	MMD0Exp expData{};
	if(fileHeader.expDataOffset && file.Seek(fileHeader.expDataOffset))
	{
		file.ReadStruct(expData);
	}

	m_nChannels = MEDScanNumChannels(file, version);
	if(m_nChannels < 1 || m_nChannels > MAX_BASECHANNELS)
		return false;

	// Start with the instruments, as those are shared between songs

	std::vector<uint32be> instrOffsets;
	file.Seek(fileHeader.sampleArrOffset);
	file.ReadVector(instrOffsets, songHeader.numSamples);
	m_nInstruments = m_nSamples = songHeader.numSamples;

	// In MMD0 / MMD1, octave wrapping is only done in 4-channel modules (hardware mixing!), and not for synth instruments
	// - It's required e.g. for automatic terminated to.mmd0
	// - dissociate.mmd0 (8 channels) and starkelsesirap.mmd0 (synth) on the other hand don't need it
	// In MMD2 / MMD3, the mix flag is used instead.
	const bool hardwareMixSamples = (version < 2 && m_nChannels == 4) || (version >= 2 && !(songHeader.flags2 & MMDSong::FLAG2_MIX));

	bool needInstruments = false;
	bool anySynthInstrs = false;
#ifndef NO_VST
	PLUGINDEX numPlugins = 0;
#endif
	for(SAMPLEINDEX ins = 1, smp = 1; ins <= m_nInstruments; ins++)
	{
		if(!AllocateInstrument(ins, smp))
			return false;
		ModInstrument &instr = *Instruments[ins];

		MMDInstrHeader instrHeader{};
		FileReader sampleChunk;
		if(instrOffsets[ins - 1] != 0 && file.Seek(instrOffsets[ins - 1]))
		{
			file.ReadStruct(instrHeader);
			sampleChunk = file.ReadChunk(instrHeader.length);
		}
		const bool isSynth = instrHeader.type < 0;
		const size_t maskedType = static_cast<size_t>(instrHeader.type & MMDInstrHeader::TYPEMASK);

#ifndef NO_VST
		if(instrHeader.type == MMDInstrHeader::VSTI)
		{
			needInstruments = true;
			sampleChunk.Skip(6); // 00 00 <size of following data>
			const std::wstring type = ReadMEDStringUTF16BE(sampleChunk);
			const std::wstring name = ReadMEDStringUTF16BE(sampleChunk);
			if(type == L"VST")
			{
				auto &mixPlug = m_MixPlugins[numPlugins];
				mixPlug = {};
				mixPlug.Info.dwPluginId1 = Vst::kEffectMagic;
				mixPlug.Info.gain = 10;
				mixPlug.Info.szName = mpt::ToCharset(mpt::CharsetLocaleOrUTF8, name);
				mixPlug.Info.szLibraryName = mpt::ToCharset(mpt::Charset::UTF8, name);
				instr.nMixPlug = numPlugins + 1;
				instr.nMidiChannel = MidiFirstChannel;
				instr.Transpose(-24);
				instr.AssignSample(0);
				// TODO: Figure out patch and routing data

				numPlugins++;
			}
		} else
#endif  // NO_VST
		if(isSynth)
		{
			// TODO: Figure out synth instruments
			anySynthInstrs = true;
			instr.AssignSample(0);
		}

		uint8 numSamples = 1;
		static constexpr uint8 SamplesPerType[] = {1, 5, 3, 2, 4, 6, 7};
		if(!isSynth && maskedType < std::size(SamplesPerType))
			numSamples = SamplesPerType[maskedType];
		if(numSamples > 1)
		{
			static_assert(MAX_SAMPLES > 63 * 9, "Check IFFOCT multisample code");
			m_nSamples += numSamples - 1;
			needInstruments = true;
			static constexpr uint8 OctSampleMap[][8] =
			{
				{1, 1, 0, 0, 0, 0, 0, 0},  // 2
				{2, 2, 1, 1, 0, 0, 0, 0},  // 3
				{3, 3, 2, 2, 1, 0, 0, 0},  // 4
				{4, 3, 2, 1, 1, 0, 0, 0},  // 5
				{5, 4, 3, 2, 1, 0, 0, 0},  // 6
				{6, 5, 4, 3, 2, 1, 0, 0},  // 7
			};

			static constexpr int8 OctTransposeMap[][8] =
			{
				{ 0, 0, -12, -12, -24, -36, -48, -60},  // 2
				{ 0, 0, -12, -12, -24, -36, -48, -60},  // 3
				{ 0, 0, -12, -12, -24, -36, -48, -60},  // 4
				{12, 0, -12, -24, -24, -36, -48, -60},  // 5
				{12, 0, -12, -24, -36, -48, -48, -60},  // 6
				{12, 0, -12, -24, -36, -48, -60, -72},  // 7
			};

			// TODO: Move octaves so that they align better (C-4 = lowest, we don't have access to the highest four octaves)
			for(int octave = 4; octave < 10; octave++)
			{
				for(int note = 0; note < 12; note++)
				{
					instr.Keyboard[12 * octave + note] = smp + OctSampleMap[numSamples - 2][octave - 4];
					instr.NoteMap[12 * octave + note] += OctTransposeMap[numSamples - 2][octave - 4];
				}
			}
		} else if(maskedType == MMDInstrHeader::EXTSAMPLE)
		{
			needInstruments = true;
			instr.Transpose(-24);
		} else if(!isSynth && hardwareMixSamples)
		{
			for(int octave = 7; octave < 10; octave++)
			{
				for(int note = 0; note < 12; note++)
				{
					instr.NoteMap[12 * octave + note] -= static_cast<uint8>((octave - 6) * 12);
				}
			}
		}

		MMD0Sample sampleHeader;
		sampleHeaderChunk.ReadStruct(sampleHeader);

		// midiChannel = 0xFF == midi instrument but with invalid channel, midiChannel = 0x00 == sample-based instrument?
		if(sampleHeader.midiChannel > 0 && sampleHeader.midiChannel <= 16)
		{
			instr.nMidiChannel = sampleHeader.midiChannel - 1 + MidiFirstChannel;
			needInstruments = true;

#ifndef NO_VST
			if(!isSynth)
			{
				auto &mixPlug = m_MixPlugins[numPlugins];
				mixPlug = {};
				mixPlug.Info.dwPluginId1 = PLUGMAGIC('V', 's', 't', 'P');
				mixPlug.Info.dwPluginId2 = PLUGMAGIC('M', 'M', 'I', 'D');
				mixPlug.Info.gain = 10;
				mixPlug.Info.szName = "MIDI Input Output";
				mixPlug.Info.szLibraryName = "MIDI Input Output";

				instr.nMixPlug = numPlugins + 1;
				instr.Transpose(-24);

				numPlugins++;
			}
#endif  // NO_VST
		}
		if(sampleHeader.midiPreset > 0 && sampleHeader.midiPreset <= 128)
		{
			instr.nMidiProgram = sampleHeader.midiPreset;
		}

		for(SAMPLEINDEX i = 0; i < numSamples; i++)
		{
			ModSample &mptSmp = Samples[smp + i];
			mptSmp.Initialize(MOD_TYPE_MED);
			mptSmp.nVolume = 4u * std::min<uint8>(sampleHeader.sampleVolume, 64u);
			mptSmp.RelativeTone = sampleHeader.sampleTranspose;
		}

		if(isSynth || !(loadFlags & loadSampleData))
		{
			smp += numSamples;
			continue;
		}

		SampleIO sampleIO(
			SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::bigEndian,
			SampleIO::signedPCM);

		const bool hasLoop = sampleHeader.loopLength > 1;
		SmpLength loopStart = sampleHeader.loopStart * 2;
		SmpLength loopEnd = loopStart + sampleHeader.loopLength * 2;

		SmpLength length = mpt::saturate_cast<SmpLength>(sampleChunk.GetLength());
		if(instrHeader.type & MMDInstrHeader::S_16)
		{
			sampleIO |= SampleIO::_16bit;
			length /= 2;
		}
		if (instrHeader.type & MMDInstrHeader::STEREO)
		{
			sampleIO |= SampleIO::stereoSplit;
			length /= 2;
		}
		if(instrHeader.type & MMDInstrHeader::DELTA)
		{
			sampleIO |= SampleIO::deltaPCM;
		}

		if(numSamples > 1)
			length = length / ((1u << numSamples) - 1);

		for(SAMPLEINDEX i = 0; i < numSamples; i++)
		{
			ModSample &mptSmp = Samples[smp + i];

			mptSmp.nLength = length;
			sampleIO.ReadSample(mptSmp, sampleChunk);

			if(hasLoop)
			{
				mptSmp.nLoopStart = loopStart;
				mptSmp.nLoopEnd = loopEnd;
				mptSmp.uFlags.set(CHN_LOOP);
			}

			length *= 2;
			loopStart *= 2;
			loopEnd *= 2;
		}

		smp += numSamples;
	}

	if(expData.instrExtOffset != 0 && expData.instrExtEntries != 0 && file.Seek(expData.instrExtOffset))
	{
		const uint16 entries = std::min<uint16>(expData.instrExtEntries, songHeader.numSamples);
		const uint16 size = expData.instrExtEntrySize;
		for(uint16 i = 0; i < entries; i++)
		{
			MMDInstrExt instrExt;
			file.ReadStructPartial(instrExt, size);

			ModInstrument &ins = *Instruments[i + 1];
			if(instrExt.hold)
			{
				ins.VolEnv.assign({
					EnvelopeNode{0u, ENVELOPE_MAX},
					EnvelopeNode{static_cast<EnvelopeNode::tick_t>(instrExt.hold - 1), ENVELOPE_MAX},
					EnvelopeNode{static_cast<EnvelopeNode::tick_t>(instrExt.hold + (instrExt.decay ? 64u / instrExt.decay : 0u)), ENVELOPE_MIN},
				});
				if(instrExt.hold == 1)
					ins.VolEnv.erase(ins.VolEnv.begin());
				ins.nFadeOut = instrExt.decay ? (instrExt.decay * 512) : 32767;
				ins.VolEnv.dwFlags.set(ENV_ENABLED);
				needInstruments = true;
			}
			if(size > offsetof(MMDInstrExt, longMidiPreset))
			{
				ins.wMidiBank = instrExt.longMidiPreset;
			}
#ifndef NO_VST
			if(ins.nMixPlug > 0)
			{
				PLUGINDEX plug = ins.nMixPlug - 1;
				auto &mixPlug = m_MixPlugins[plug];
				if(mixPlug.Info.dwPluginId2 == PLUGMAGIC('M', 'M', 'I', 'D'))
				{
					float dev = (instrExt.outputDevice + 1) / 65536.0f;  // Magic code from MidiInOut.h :(
					mixPlug.pluginData.resize(3 * sizeof(uint32));
					auto memFile = std::make_pair(mpt::as_span(mixPlug.pluginData), mpt::IO::Offset(0));
					mpt::IO::WriteIntLE<uint32>(memFile, 0);          // Plugin data type
					mpt::IO::Write(memFile, IEEE754binary32LE{0});    // Input device
					mpt::IO::Write(memFile, IEEE754binary32LE{dev});  // Output device

					// Check if we already have another plugin referencing this output device
					for(PLUGINDEX p = 0; p < plug; p++)
					{
						const auto &otherPlug = m_MixPlugins[p];
						if(otherPlug.Info.dwPluginId1 == mixPlug.Info.dwPluginId1
							&& otherPlug.Info.dwPluginId2 == mixPlug.Info.dwPluginId2
							&& otherPlug.pluginData == mixPlug.pluginData)
						{
							ins.nMixPlug = p + 1;
							mixPlug = {};
							break;
						}
					}
				}
			}
#endif  // NO_VST

			ModSample &sample = Samples[ins.Keyboard[NOTE_MIDDLEC]];
			sample.nFineTune = MOD2XMFineTune(instrExt.finetune);

			if(size > offsetof(MMDInstrExt, loopLength))
			{
				sample.nLoopStart = instrExt.loopStart;
				sample.nLoopEnd = instrExt.loopStart + instrExt.loopLength;
			}
			if(size > offsetof(MMDInstrExt, instrFlags))
			{
				if(instrExt.instrFlags & MMDInstrExt::SSFLG_LOOP)
					sample.uFlags.set(CHN_LOOP);
				if(instrExt.instrFlags & MMDInstrExt::SSFLG_PINGPONG)
					sample.uFlags.set(CHN_LOOP | CHN_PINGPONGLOOP);
				if(instrExt.instrFlags & MMDInstrExt::SSFLG_DISABLED)
					sample.nGlobalVol = 0;
			}
		}
	}
	if(expData.instrInfoOffset != 0 && expData.instrInfoEntries != 0 && file.Seek(expData.instrInfoOffset))
	{
		const uint16 entries = std::min<uint16>(expData.instrInfoEntries, songHeader.numSamples);
		const uint16 size = expData.instrInfoEntrySize;
		for(uint16 i = 0; i < entries; i++)
		{
			MMDInstrInfo instrInfo;
			file.ReadStructPartial(instrInfo, size);
			Instruments[i + 1]->name = mpt::String::ReadBuf(mpt::String::maybeNullTerminated, instrInfo.name);
			for(auto smp : Instruments[i + 1]->GetSamples())
			{
				m_szNames[smp] = Instruments[i + 1]->name;
			}
		}
	}

	const auto numSongs = std::min<SEQUENCEINDEX>(MAX_SEQUENCES, fileHeader.expDataOffset ? fileHeader.extraSongs + 1 : 1);

	file.Rewind();
	PATTERNINDEX basePattern = 0;
	for(SEQUENCEINDEX song = 0; song < numSongs; song++)
	{
		MEDReadNextSong(file, fileHeader, expData, songHeader);

		if(song != 0)
		{
			if(Order.AddSequence() == SEQUENCEINDEX_INVALID)
				return false;
		}

		ModSequence &order = Order(song);

		std::map<ORDERINDEX, ORDERINDEX> jumpTargets;
		order.clear();
		uint32 preamp = 32;
		if(version < 2)
		{
			if(songHeader.songLength > 256 || m_nChannels > 16)
				return false;
			ReadOrderFromArray(order, songHeader.GetMMD0Song().sequence, songHeader.songLength);
			for(auto &ord : order)
			{
				ord += basePattern;
			}

			SetupMODPanning(true);
			for(CHANNELINDEX chn = 0; chn < m_nChannels; chn++)
			{
				ChnSettings[chn].nVolume = std::min<uint8>(songHeader.trackVol[chn], 64);
			}
		} else
		{
			const MMD2Song header = songHeader.GetMMD2Song();
			if(header.numTracks < 1 || header.numTracks > 64 || m_nChannels > 64)
				return false;

			const bool freePan = (header.flags3 & MMD2Song::FLAG3_FREEPAN);
			if(header.volAdjust)
				preamp = Util::muldivr_unsigned(preamp, std::min<uint16>(header.volAdjust, 800), 100);
			if (freePan)
				preamp /= 2;

			if(file.Seek(header.trackVolsOffset))
			{
				for(CHANNELINDEX chn = 0; chn < m_nChannels; chn++)
				{
					ChnSettings[chn].nVolume = std::min<uint8>(file.ReadUint8(), 64);
				}
			}
			if(header.trackPanOffset && file.Seek(header.trackPanOffset))
			{
				for(CHANNELINDEX chn = 0; chn < m_nChannels; chn++)
				{
					ChnSettings[chn].nPan = (Clamp<int8, int8>(file.ReadInt8(), -16, 16) + 16) * 8;
				}
			} else
			{
				SetupMODPanning(true);
			}

			std::vector<uint16be> sections;
			if(!file.Seek(header.sectionTableOffset)
			   || !file.CanRead(songHeader.songLength * 2)
			   || !file.ReadVector(sections, songHeader.songLength))
				continue;
			for(uint16 section : sections)
			{
				if(section > header.numPlaySeqs)
					continue;

				file.Seek(header.playSeqTableOffset + section * 4);
				if(file.Seek(file.ReadUint32BE()) || !file.CanRead(sizeof(MMD2PlaySeq)))
				{
					MMD2PlaySeq playSeq;
					file.ReadStruct(playSeq);

					if(!order.empty())
						order.push_back(order.GetIgnoreIndex());

					size_t readOrders = playSeq.length;
					if(!file.CanRead(readOrders))
						LimitMax(readOrders, file.BytesLeft());
					LimitMax(readOrders, ORDERINDEX_MAX);

					size_t orderStart = order.size();
					order.reserve(orderStart + readOrders);
					for(size_t ord = 0; ord < readOrders; ord++)
					{
						PATTERNINDEX pat = file.ReadUint16BE();
						if(pat < 0x8000)
						{
							order.push_back(basePattern + pat);
						}
					}
					if(playSeq.name[0])
						order.SetName(mpt::ToUnicode(mpt::Charset::ISO8859_1, playSeq.name));

					// Play commands (jump / stop)
					if(playSeq.commandTableOffset > 0 && file.Seek(playSeq.commandTableOffset))
					{
						MMDPlaySeqCommand command;
						while(file.ReadStruct(command))
						{
							FileReader chunk = file.ReadChunk(command.extraSize);
							ORDERINDEX ord = mpt::saturate_cast<ORDERINDEX>(orderStart + command.offset);
							if(command.offset == 0xFFFF || ord >= order.size())
								break;
							if(command.command == MMDPlaySeqCommand::kStop)
							{
								order[ord] = order.GetInvalidPatIndex();
							} else if(command.command == MMDPlaySeqCommand::kJump)
							{
								jumpTargets[ord] = chunk.ReadUint16BE();
								order[ord] = order.GetIgnoreIndex();
							}
						}
					}
				}
			}
		}

		const bool volHex = (songHeader.flags & MMDSong::FLAG_VOLHEX) != 0;
		const bool is8Ch = (songHeader.flags & MMDSong::FLAG_8CHANNEL) != 0;
		const bool bpmMode = (songHeader.flags2 & MMDSong::FLAG2_BPM) != 0;
		const uint8 rowsPerBeat = 1 + (songHeader.flags2 & MMDSong::FLAG2_BMASK);
		m_nDefaultTempo = MMDTempoToBPM(songHeader.defaultTempo, is8Ch, bpmMode, rowsPerBeat);
		m_nDefaultSpeed = Clamp<uint8, uint8>(songHeader.tempo2, 1, 32);
		if(bpmMode)
		{
			m_nDefaultRowsPerBeat = rowsPerBeat;
			m_nDefaultRowsPerMeasure = m_nDefaultRowsPerBeat * 4u;
		}

		if(songHeader.masterVol)
			m_nDefaultGlobalVolume = std::min<uint8>(songHeader.masterVol, 64) * 4;
		m_nSamplePreAmp = m_nVSTiVolume = preamp;

		// For MED, this affects both volume and pitch slides
		m_SongFlags.set(SONG_FASTVOLSLIDES, !(songHeader.flags & MMDSong::FLAG_STSLIDE));

		if(expData.songNameOffset && file.Seek(expData.songNameOffset))
		{
			file.ReadString<mpt::String::maybeNullTerminated>(m_songName, expData.songNameLength);
			if(numSongs > 1)
				order.SetName(mpt::ToUnicode(mpt::Charset::ISO8859_1, m_songName));
		}
		if(expData.annoLength > 1 && file.Seek(expData.annoText))
		{
			m_songMessage.Read(file, expData.annoLength - 1, SongMessage::leAutodetect);
		}

		if(expData.mmdInfoOffset && file.Seek(expData.mmdInfoOffset))
		{
			file.Skip(6);  // Next info file (unused) + reserved
			if(file.ReadUint16BE() == 1)  // ASCII text
			{
				uint32 length = file.ReadUint32BE();
				if(length && file.CanRead(length))
				{
					const auto oldMsg = std::move(m_songMessage);
					m_songMessage.Read(file, length, SongMessage::leAutodetect);
					if(!oldMsg.empty())
						m_songMessage.SetRaw(oldMsg + std::string(2, SongMessage::InternalLineEnding) + m_songMessage);
				}
			}
		}

		// Track Names
		if(version >= 2 && expData.trackInfoOffset)
		{
			for(CHANNELINDEX chn = 0; chn < m_nChannels; chn++)
			{
				if(file.Seek(expData.trackInfoOffset + chn * 4)
				   && file.Seek(file.ReadUint32BE()))
				{
					uint32 nameOffset = 0, nameLength = 0;
					while(file.CanRead(sizeof(MMDTag)))
					{
						MMDTag tag;
						file.ReadStruct(tag);
						if(tag.type == MMDTag::MMDTAG_END)
							break;
						switch(tag.type & MMDTag::MMDTAG_MASK)
						{
						case MMDTag::MMDTAG_TRK_NAME: nameOffset = tag.data; break;
						case MMDTag::MMDTAG_TRK_NAMELEN: nameLength = tag.data; break;
						}
					}
					if(nameOffset > 0 && nameLength > 0 && file.Seek(nameOffset))
					{
						file.ReadString<mpt::String::maybeNullTerminated>(ChnSettings[chn].szName, nameLength);
					}
				}
			}
		}

		PATTERNINDEX numPatterns = songHeader.numBlocks;
		Patterns.ResizeArray(basePattern + numPatterns);
		for(PATTERNINDEX pat = 0; pat < numPatterns; pat++)
		{
			if(!(loadFlags & loadPatternData)
			   || !file.Seek(fileHeader.blockArrOffset + pat * 4u)
			   || !file.Seek(file.ReadUint32BE()))
			{
				continue;
			}

			CHANNELINDEX numTracks;
			ROWINDEX numRows;
			std::string patName;
			int transpose;
			FileReader cmdExt;

			if(version < 1)
			{
				transpose = NOTE_MIN + 47;
				MMD0PatternHeader patHeader;
				file.ReadStruct(patHeader);
				numTracks = patHeader.numTracks;
				numRows = patHeader.numRows + 1;
			} else
			{
				transpose = NOTE_MIN + (version <= 2 ? 47 : 23) + songHeader.playTranspose;
				MMD1PatternHeader patHeader;
				file.ReadStruct(patHeader);
				numTracks = patHeader.numTracks;
				numRows = patHeader.numRows + 1;
				if(patHeader.blockInfoOffset)
				{
					auto offset = file.GetPosition();
					file.Seek(patHeader.blockInfoOffset);
					MMDBlockInfo blockInfo;
					file.ReadStruct(blockInfo);
					if(file.Seek(blockInfo.nameOffset))
					{
						// We have now chased four pointers to get this far... lovely format.
						file.ReadString<mpt::String::maybeNullTerminated>(patName, blockInfo.nameLength);
					}
					if(blockInfo.cmdExtTableOffset
					   && file.Seek(blockInfo.cmdExtTableOffset)
					   && file.Seek(file.ReadUint32BE()))
					{
						cmdExt = file.ReadChunk(numTracks * numRows);
					}

					file.Seek(offset);
				}
			}

			if(!Patterns.Insert(basePattern + pat, numRows))
				continue;

			CPattern &pattern = Patterns[basePattern + pat];
			pattern.SetName(patName);
			LimitMax(numTracks, m_nChannels);

			for(ROWINDEX row = 0; row < numRows; row++)
			{
				ModCommand *m = pattern.GetpModCommand(row, 0);
				for(CHANNELINDEX chn = 0; chn < numTracks; chn++, m++)
				{
					int note = NOTE_NONE;
					if(version < 1)
					{
						const auto [noteInstr, instrCmd, param] = file.ReadArray<uint8, 3>();

						if(noteInstr & 0x3F)
							note = (noteInstr & 0x3F) + transpose;

						m->instr = (instrCmd >> 4) | ((noteInstr & 0x80) >> 3) | ((noteInstr & 0x40) >> 1);

						m->command = instrCmd & 0x0F;
						m->param = param;
					} else
					{
						const auto [noteVal, instr, command, param1] = file.ReadArray<uint8, 4>();
						m->vol = cmdExt.ReadUint8();

						if(noteVal & 0x7F)
							note = (noteVal & 0x7F) + transpose;
						else if(noteVal == 0x80)
							m->note = NOTE_NOTECUT;

						m->instr = instr & 0x3F;
						m->command = command;
						m->param = param1;
					}
					// Octave wrapping for 4-channel modules (TODO: this should not be set because of synth instruments)
					if(hardwareMixSamples && note >= NOTE_MIDDLEC + 2 * 12)
						needInstruments = true;

					if(note >= NOTE_MIN && note <= NOTE_MAX)
						m->note = static_cast<ModCommand::NOTE>(note);
					ConvertMEDEffect(*m, is8Ch, bpmMode, rowsPerBeat, volHex);
				}
			}
		}

		// Fix jump order commands
		for(const auto & [from, to] : jumpTargets)
		{
			PATTERNINDEX pat;
			if(from > 0 && order.IsValidPat(from - 1))
			{
				pat = order.EnsureUnique(from - 1);
			} else
			{
				if(to == from + 1)  // No action required
					continue;
				pat = Patterns.InsertAny(1);
				if(pat == PATTERNINDEX_INVALID)
					continue;
				order[from] = pat;
			}
			Patterns[pat].WriteEffect(EffectWriter(CMD_POSITIONJUMP, mpt::saturate_cast<ModCommand::PARAM>(to)).Row(Patterns[pat].GetNumRows() - 1).RetryPreviousRow());
			if(pat >= numPatterns)
				numPatterns = pat + 1;
		}

		basePattern += numPatterns;
		
		if(!expData.nextModOffset || !file.Seek(expData.nextModOffset))
			break;
	}
	Order.SetSequence(0);

	if(!needInstruments)
	{
		for(INSTRUMENTINDEX ins = 1; ins <= m_nInstruments; ins++)
		{
			delete Instruments[ins];
			Instruments[ins] = nullptr;
		}
		m_nInstruments = 0;
	}

	if(anySynthInstrs)
		AddToLog(LogWarning, U_("Synthesized MED instruments are not supported."));

	const mpt::uchar *madeWithTracker = MPT_ULITERAL("");
	switch(version)
	{
	case 0: madeWithTracker = m_nChannels > 4 ? MPT_ULITERAL("OctaMED v2.10 (MMD0)") : MPT_ULITERAL("MED v2 (MMD0)"); break;
	case 1: madeWithTracker = MPT_ULITERAL("OctaMED v4 (MMD1)"); break;
	case 2: madeWithTracker = MPT_ULITERAL("OctaMED v5 (MMD2)"); break;
	case 3: madeWithTracker = MPT_ULITERAL("OctaMED Soundstudio (MMD3)"); break;
	}

	m_modFormat.formatName = mpt::format(MPT_USTRING("OctaMED (MMD%1)"))(version);
	m_modFormat.type = MPT_USTRING("med");
	m_modFormat.madeWithTracker = madeWithTracker;
	m_modFormat.charset = mpt::Charset::ISO8859_1;

	return true;
}

OPENMPT_NAMESPACE_END
