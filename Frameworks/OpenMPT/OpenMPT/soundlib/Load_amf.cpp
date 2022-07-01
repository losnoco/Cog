/*
 * Load_amf.cpp
 * ------------
 * Purpose: AMF module loader
 * Notes  : There are two types of AMF files, the ASYLUM Music Format (used in Crusader: No Remorse and Crusader: No Regret)
 *          and Advanced Music Format (DSMI / Digital Sound And Music Interface, used in various games such as Pinball World).
 *          Both module types are handled here.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include <algorithm>


OPENMPT_NAMESPACE_BEGIN

// ASYLUM AMF File Header
struct AsylumFileHeader
{
	char  signature[32];
	uint8 defaultSpeed;
	uint8 defaultTempo;
	uint8 numSamples;
	uint8 numPatterns;
	uint8 numOrders;
	uint8 restartPos;
};

MPT_BINARY_STRUCT(AsylumFileHeader, 38)


// ASYLUM AMF Sample Header
struct AsylumSampleHeader
{
	char     name[22];
	uint8le  finetune;
	uint8le  defaultVolume;
	int8le   transpose;
	uint32le length;
	uint32le loopStart;
	uint32le loopLength;

	// Convert an AMF sample header to OpenMPT's internal sample header.
	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize();
		mptSmp.nFineTune = MOD2XMFineTune(finetune);
		mptSmp.nVolume = std::min(defaultVolume.get(), uint8(64)) * 4u;
		mptSmp.RelativeTone = transpose;
		mptSmp.nLength = length;

		if(loopLength > 2 && loopStart + loopLength <= length)
		{
			mptSmp.uFlags.set(CHN_LOOP);
			mptSmp.nLoopStart = loopStart;
			mptSmp.nLoopEnd = loopStart + loopLength;
		}
	}
};

MPT_BINARY_STRUCT(AsylumSampleHeader, 37)


static bool ValidateHeader(const AsylumFileHeader &fileHeader)
{
	if(std::memcmp(fileHeader.signature, "ASYLUM Music Format V1.0\0", 25)
		|| fileHeader.numSamples > 64
		)
	{
		return false;
	}
	return true;
}


static uint64 GetHeaderMinimumAdditionalSize(const AsylumFileHeader &fileHeader)
{
	return 256 + 64 * sizeof(AsylumSampleHeader) + 64 * 4 * 8 * fileHeader.numPatterns;
}


CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderAMF_Asylum(MemoryFileReader file, const uint64 *pfilesize)
{
	AsylumFileHeader fileHeader;
	if(!file.ReadStruct(fileHeader))
	{
		return ProbeWantMoreData;
	}
	if(!ValidateHeader(fileHeader))
	{
		return ProbeFailure;
	}
	return ProbeAdditionalSize(file, pfilesize, GetHeaderMinimumAdditionalSize(fileHeader));
}


bool CSoundFile::ReadAMF_Asylum(FileReader &file, ModLoadingFlags loadFlags)
{
	file.Rewind();

	AsylumFileHeader fileHeader;
	if(!file.ReadStruct(fileHeader))
	{
		return false;
	}
	if(!ValidateHeader(fileHeader))
	{
		return false;
	}
	if(!file.CanRead(mpt::saturate_cast<FileReader::off_t>(GetHeaderMinimumAdditionalSize(fileHeader))))
	{
		return false;
	}
	if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	InitializeGlobals(MOD_TYPE_AMF0);
	InitializeChannels();
	SetupMODPanning(true);
	m_nChannels = 8;
	m_nDefaultSpeed = fileHeader.defaultSpeed;
	m_nDefaultTempo.Set(fileHeader.defaultTempo);
	m_nSamples = fileHeader.numSamples;
	if(fileHeader.restartPos < fileHeader.numOrders)
	{
		Order().SetRestartPos(fileHeader.restartPos);
	}

	m_modFormat.formatName = U_("ASYLUM Music Format");
	m_modFormat.type = U_("amf");
	m_modFormat.charset = mpt::Charset::CP437;

	uint8 orders[256];
	file.ReadArray(orders);
	ReadOrderFromArray(Order(), orders, fileHeader.numOrders);

	// Read Sample Headers
	for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
	{
		AsylumSampleHeader sampleHeader;
		file.ReadStruct(sampleHeader);
		sampleHeader.ConvertToMPT(Samples[smp]);
		m_szNames[smp] = mpt::String::ReadBuf(mpt::String::maybeNullTerminated, sampleHeader.name);
	}

	file.Skip((64 - fileHeader.numSamples) * sizeof(AsylumSampleHeader));

	// Read Patterns
	Patterns.ResizeArray(fileHeader.numPatterns);
	for(PATTERNINDEX pat = 0; pat < fileHeader.numPatterns; pat++)
	{
		if(!(loadFlags & loadPatternData) || !Patterns.Insert(pat, 64))
		{
			file.Skip(64 * 4 * 8);
			continue;
		}

		for(auto &m : Patterns[pat])
		{
			const auto [note, instr, command, param] = file.ReadArray<uint8, 4>();
			if(note && note + 12 + NOTE_MIN <= NOTE_MAX)
			{
				m.note = note + 12 + NOTE_MIN;
			}
			m.instr = instr;
			m.command = command;
			m.param = param;
			ConvertModCommand(m);
#ifdef MODPLUG_TRACKER
			if(m.command == CMD_PANNING8)
			{
				// Convert 7-bit panning to 8-bit
				m.param = mpt::saturate_cast<ModCommand::PARAM>(m.param * 2u);
			}
#endif
		}
	}

	if(loadFlags & loadSampleData)
	{
		// Read Sample Data
		const SampleIO sampleIO(
			SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::signedPCM);

		for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
		{
			sampleIO.ReadSample(Samples[smp], file);
		}
	}

	return true;
}


// DSMI AMF File Header
struct AMFFileHeader
{
	char     amf[3];
	uint8le  version;
	char     title[32];
	uint8le  numSamples;
	uint8le  numOrders;
	uint16le numTracks;
	uint8le  numChannels;
};

MPT_BINARY_STRUCT(AMFFileHeader, 41)


// DSMI AMF Sample Header (v1-v9)
struct AMFSampleHeaderOld
{
	uint8le  type;
	char     name[32];
	char     filename[13];
	uint32le index;
	uint16le length;
	uint16le sampleRate;
	uint8le  volume;
	uint16le loopStart;
	uint16le loopEnd;

	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize();
		mptSmp.filename = mpt::String::ReadBuf(mpt::String::nullTerminated, filename);
		mptSmp.nLength = length;
		mptSmp.nC5Speed = sampleRate;
		mptSmp.nVolume = std::min(volume.get(), uint8(64)) * 4u;
		mptSmp.nLoopStart = loopStart;
		mptSmp.nLoopEnd = loopEnd;
		if(mptSmp.nLoopEnd == uint16_max)
			mptSmp.nLoopStart = mptSmp.nLoopEnd = 0;
		else if(type != 0 && mptSmp.nLoopEnd > mptSmp.nLoopStart + 2 && mptSmp.nLoopEnd <= mptSmp.nLength)
			mptSmp.uFlags.set(CHN_LOOP);
	}
};

MPT_BINARY_STRUCT(AMFSampleHeaderOld, 59)


// DSMI AMF Sample Header (v10+)
struct AMFSampleHeaderNew
{
	uint8le  type;
	char     name[32];
	char     filename[13];
	uint32le index;
	uint32le length;
	uint16le sampleRate;
	uint8le  volume;
	uint32le loopStart;
	uint32le loopEnd;

	void ConvertToMPT(ModSample &mptSmp, bool truncated) const
	{
		mptSmp.Initialize();
		mptSmp.filename = mpt::String::ReadBuf(mpt::String::nullTerminated, filename);
		mptSmp.nLength = length;
		mptSmp.nC5Speed = sampleRate;
		mptSmp.nVolume = std::min(volume.get(), uint8(64)) * 4u;
		mptSmp.nLoopStart = loopStart;
		mptSmp.nLoopEnd = loopEnd;
		if(truncated && mptSmp.nLoopStart > 0)
			mptSmp.nLoopEnd = mptSmp.nLength;
		if(type != 0 && mptSmp.nLoopEnd > mptSmp.nLoopStart + 2 && mptSmp.nLoopEnd <= mptSmp.nLength)
			mptSmp.uFlags.set(CHN_LOOP);
	}

	// Check if sample headers might be truncated
	bool IsValid(uint8 numSamples) const
	{
		return type <= 1 && index <= numSamples && length <= 0x100000 && volume <= 64 && loopStart <= length && loopEnd <= length;
	}
};

MPT_BINARY_STRUCT(AMFSampleHeaderNew, 65)


// Read a single AMF track (channel) into a pattern.
static void AMFReadPattern(CPattern &pattern, CHANNELINDEX chn, FileReader &fileChunk)
{
	fileChunk.Rewind();
	while(fileChunk.CanRead(3))
	{
		const auto [row, command, value] = fileChunk.ReadArray<uint8, 3>();
		if(row >= pattern.GetNumRows())
		{
			break;
		}

		ModCommand &m = *pattern.GetpModCommand(row, chn);
		if(command < 0x7F)
		{
			// Note + Volume
			if(command == 0 && value == 0)
			{
				m.note = NOTE_NOTECUT;
			} else
			{
				m.note = command + NOTE_MIN;
				if(value != 0xFF)
				{
					m.volcmd = VOLCMD_VOLUME;
					m.vol = value;
				}
			}
		} else if(command == 0x7F)
		{
			// Instrument without note retrigger in MOD (no need to do anything here, should be preceded by 0x80 command)
		} else if(command == 0x80)
		{
			// Instrument
			m.instr = value + 1;
		} else
		{
			// Effect
			static constexpr ModCommand::COMMAND effTrans[] =
			{
				CMD_NONE,			CMD_SPEED,			CMD_VOLUMESLIDE,		CMD_VOLUME,
				CMD_PORTAMENTOUP,	CMD_NONE,			CMD_TONEPORTAMENTO,		CMD_TREMOR,
				CMD_ARPEGGIO,		CMD_VIBRATO,		CMD_TONEPORTAVOL,		CMD_VIBRATOVOL,
				CMD_PATTERNBREAK,	CMD_POSITIONJUMP,	CMD_NONE,				CMD_RETRIG,
				CMD_OFFSET,			CMD_VOLUMESLIDE,	CMD_PORTAMENTOUP,		CMD_S3MCMDEX,
				CMD_S3MCMDEX,		CMD_TEMPO,			CMD_PORTAMENTOUP,		CMD_PANNING8,
			};

			uint8 cmd = (command & 0x7F);
			uint8 param = value;

			if(cmd < std::size(effTrans))
				cmd = effTrans[cmd];
			else
				cmd = CMD_NONE;

			// Fix some commands...
			switch(command & 0x7F)
			{
			// 02: Volume Slide
			// 0A: Tone Porta + Vol Slide
			// 0B: Vibrato + Vol Slide
			case 0x02:
			case 0x0A:
			case 0x0B:
				if(param & 0x80)
					param = (-static_cast<int8>(param)) & 0x0F;
				else
					param = (param & 0x0F) << 4;
				break;

			// 03: Volume
			case 0x03:
				param = std::min(param, uint8(64));
				if(m.volcmd == VOLCMD_NONE || m.volcmd == VOLCMD_VOLUME)
				{
					m.volcmd = VOLCMD_VOLUME;
					m.vol = param;
					cmd = CMD_NONE;
				}
				break;

			// 04: Porta Up/Down
			case 0x04:
				if(param & 0x80)
					param = (-static_cast<int8>(param)) & 0x7F;
				else
					cmd = CMD_PORTAMENTODOWN;
				break;

			// 11: Fine Volume Slide
			case 0x11:
				if(param)
				{
					if(param & 0x80)
						param = 0xF0 | ((-static_cast<int8>(param)) & 0x0F);
					else
						param = 0x0F | ((param & 0x0F) << 4);
				} else
				{
					cmd = CMD_NONE;
				}
				break;

			// 12: Fine Portamento
			// 16: Extra Fine Portamento
			case 0x12:
			case 0x16:
				if(param)
				{
					cmd = static_cast<uint8>((param & 0x80) ? CMD_PORTAMENTOUP : CMD_PORTAMENTODOWN);
					if(param & 0x80)
					{
						param = ((-static_cast<int8>(param)) & 0x0F);
					}
					param |= (command == 0x16) ? 0xE0 : 0xF0;
				} else
				{
					cmd = CMD_NONE;
				}
				break;

			// 13: Note Delay
			case 0x13:
				param = 0xD0 | (param & 0x0F);
				break;

			// 14: Note Cut
			case 0x14:
				param = 0xC0 | (param & 0x0F);
				break;

			// 17: Panning
			case 0x17:
				if(param == 100)
				{
					// History lesson intermission: According to Otto Chrons, he remembers that he added support
					// for 8A4 / XA4 "surround" panning in DMP for MOD and S3M files before any other trackers did,
					// So DSMI / DMP are most likely the original source of these 7-bit panning + surround commands!
					param = 0xA4;
				} else
				{
					param = static_cast<uint8>(std::clamp(static_cast<int8>(param) + 64, 0, 128));
					if(m.command != CMD_NONE)
					{
						// Move to volume column if required
						if(m.volcmd == VOLCMD_NONE || m.volcmd == VOLCMD_PANNING)
						{
							m.volcmd = VOLCMD_PANNING;
							m.vol = param / 2;
						}
						cmd = CMD_NONE;
					}
				}
				break;
			}

			if(cmd != CMD_NONE)
			{
				m.command = cmd;
				m.param = param;
			}
		}
	}
}


static bool ValidateHeader(const AMFFileHeader &fileHeader)
{
	if(std::memcmp(fileHeader.amf, "AMF", 3)
	   || (fileHeader.version < 8 && fileHeader.version != 1) || fileHeader.version > 14
	   || ((fileHeader.numChannels < 1 || fileHeader.numChannels > 32) && fileHeader.version >= 9))
	{
		return false;
	}
	return true;
}


CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderAMF_DSMI(MemoryFileReader file, const uint64 *pfilesize)
{
	AMFFileHeader fileHeader;
	if(!file.ReadStruct(fileHeader))
	{
		return ProbeWantMoreData;
	}
	if(!ValidateHeader(fileHeader))
	{
		return ProbeFailure;
	}
	MPT_UNREFERENCED_PARAMETER(pfilesize);
	return ProbeSuccess;
}


bool CSoundFile::ReadAMF_DSMI(FileReader &file, ModLoadingFlags loadFlags)
{
	file.Rewind();

	AMFFileHeader fileHeader;
	if(!file.ReadStruct(fileHeader))
	{
		return false;
	}
	if(!ValidateHeader(fileHeader))
	{
		return false;
	}
	if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	InitializeGlobals(MOD_TYPE_AMF);
	InitializeChannels();

	m_modFormat.formatName = MPT_UFORMAT("DSMI v{}")(fileHeader.version);
	m_modFormat.type = U_("amf");
	m_modFormat.charset = mpt::Charset::CP437;

	m_nChannels = fileHeader.numChannels;
	m_nSamples = fileHeader.numSamples;

	m_songName = mpt::String::ReadBuf(mpt::String::maybeNullTerminated, fileHeader.title);

	if(fileHeader.version < 9)
	{
		// Old format revisions are fixed to 4 channels
		m_nChannels = 4;
		file.SkipBack(1);
		SetupMODPanning(true);
	}

	// Setup Channel Pan Positions
	if(fileHeader.version >= 11)
	{
		const CHANNELINDEX readChannels = fileHeader.version >= 12 ? 32 : 16;
		for(CHANNELINDEX chn = 0; chn < readChannels; chn++)
		{
			int8 pan = file.ReadInt8();
			if(pan == 100)
				ChnSettings[chn].dwFlags = CHN_SURROUND;
			else
				ChnSettings[chn].nPan = static_cast<uint16>(std::clamp((pan + 64) * 2, 0, 256));
		}
	} else if(fileHeader.version >= 9)
	{
		uint8 panPos[16];
		file.ReadArray(panPos);
		for(CHANNELINDEX chn = 0; chn < 16; chn++)
		{
			ChnSettings[chn].nPan = (panPos[chn] & 1) ? 0x40 : 0xC0;
		}
	}

	// Get Tempo/Speed
	if(fileHeader.version >= 13)
	{
		auto [tempo, speed] = file.ReadArray<uint8, 2>();
		if(tempo < 32)
			tempo = 125;
		m_nDefaultTempo.Set(tempo);
		m_nDefaultSpeed = speed;
	} else
	{
		m_nDefaultTempo.Set(125);
		m_nDefaultSpeed = 6;
	}

	// Setup Order List
	Order().resize(fileHeader.numOrders);
	std::vector<uint16> patternLength;
	const FileReader::off_t trackStartPos = file.GetPosition() + (fileHeader.version >= 14 ? 2 : 0);
	if(fileHeader.version >= 14)
	{
		patternLength.resize(fileHeader.numOrders);
	}

	for(ORDERINDEX ord = 0; ord < fileHeader.numOrders; ord++)
	{
		Order()[ord] = ord;
		if(fileHeader.version >= 14)
		{
			patternLength[ord] = file.ReadUint16LE();
		}
		// Track positions will be read as needed.
		file.Skip(m_nChannels * 2);
	}

	// Read Sample Headers
	bool truncatedSampleHeaders = false;
	if(fileHeader.version == 10)
	{
		// M2AMF 1.3 included with DMP 2.32 wrote new (v10+) sample headers, but using the old struct length.
		const auto startPos = file.GetPosition();
		for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
		{
			AMFSampleHeaderNew sample;
			if(file.ReadStruct(sample) && !sample.IsValid(fileHeader.numSamples))
			{
				truncatedSampleHeaders = true;
				break;
			}
		}
		file.Seek(startPos);
	}

	std::vector<uint32> sampleMap(GetNumSamples(), 0);
	for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
	{
		if(fileHeader.version < 10)
		{
			AMFSampleHeaderOld sample;
			file.ReadStruct(sample);
			sample.ConvertToMPT(Samples[smp]);
			m_szNames[smp] = mpt::String::ReadBuf(mpt::String::maybeNullTerminated, sample.name);
			sampleMap[smp - 1] = sample.index;
		} else
		{
			AMFSampleHeaderNew sample;
			file.ReadStructPartial(sample, truncatedSampleHeaders ? sizeof(AMFSampleHeaderOld) : sizeof(AMFSampleHeaderNew));
			sample.ConvertToMPT(Samples[smp], truncatedSampleHeaders);
			m_szNames[smp] = mpt::String::ReadBuf(mpt::String::maybeNullTerminated, sample.name);
			sampleMap[smp - 1] = sample.index;
		}
	}
	
	// Read Track Mapping Table
	std::vector<uint16le> trackMap;
	if(!file.ReadVector(trackMap, fileHeader.numTracks))
	{
		return false;
	}
	uint16 trackCount = 0;
	if(!trackMap.empty())
		trackCount = *std::max_element(trackMap.cbegin(), trackMap.cend());

	// Read pattern tracks
	std::vector<FileReader> trackData(trackCount);
	for(uint16 i = 0; i < trackCount; i++)
	{
		// Track size is a 16-Bit value describing the number of byte triplets in this track, followed by a track type byte.
		uint16 numEvents = file.ReadUint16LE();
		file.Skip(1);
		if(numEvents)
			trackData[i] = file.ReadChunk(numEvents * 3 + (fileHeader.version == 1 ? 3 : 0));
	}

	if(loadFlags & loadSampleData)
	{
		// Read Sample Data
		const SampleIO sampleIO(
			SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::unsignedPCM);

		// Note: in theory a sample can be reused by several instruments and appear in a different order in the file
		// However, M2AMF doesn't take advantage of this and just writes instruments in the order they appear,
		// without de-duplicating identical sample data.
		for(SAMPLEINDEX smp = 1; smp <= GetNumSamples() && file.CanRead(1); smp++)
		{
			auto startPos = file.GetPosition();
			for(SAMPLEINDEX target = 0; target < GetNumSamples(); target++)
			{
				if(sampleMap[target] != smp)
					continue;
				file.Seek(startPos);
				sampleIO.ReadSample(Samples[target + 1], file);
			}
		}
	}

	if(!(loadFlags & loadPatternData))
	{
		return true;
	}

	// Create the patterns from the list of tracks
	Patterns.ResizeArray(fileHeader.numOrders);
	for(PATTERNINDEX pat = 0; pat < fileHeader.numOrders; pat++)
	{
		uint16 patLength = pat < patternLength.size() ? patternLength[pat] : 64;
		if(!Patterns.Insert(pat, patLength))
		{
			continue;
		}

		// Get table with per-channel track assignments
		file.Seek(trackStartPos + pat * (GetNumChannels() * 2 + (fileHeader.version >= 14 ? 2 : 0)));
		std::vector<uint16le> tracks;
		if(!file.ReadVector(tracks, GetNumChannels()))
		{
			continue;
		}

		for(CHANNELINDEX chn = 0; chn < GetNumChannels(); chn++)
		{
			if(tracks[chn] > 0 && tracks[chn] <= fileHeader.numTracks)
			{
				uint16 realTrack = trackMap[tracks[chn] - 1];
				if(realTrack > 0 && realTrack <= trackCount)
				{
					realTrack--;
					AMFReadPattern(Patterns[pat], chn, trackData[realTrack]);
				}
			}
		}
	}

	return true;
}


OPENMPT_NAMESPACE_END
