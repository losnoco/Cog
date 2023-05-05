/*
 * Load_ult.cpp
 * ------------
 * Purpose: ULT (UltraTracker) module loader
 * Notes  : (currently none)
 * Authors: Storlek (Original author - http://schismtracker.org/ - code ported with permission)
 *			Johannes Schultz (OpenMPT Port, tweaks)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

OPENMPT_NAMESPACE_BEGIN

struct UltFileHeader
{
	char  signature[14];		// "MAS_UTrack_V00"
	uint8 version;				// '1'...'4'
	char  songName[32];			// Song Name, not guaranteed to be null-terminated
	uint8 messageLength;		// Number of Lines
};

MPT_BINARY_STRUCT(UltFileHeader, 48)


struct UltSample
{
	enum UltSampleFlags
	{
		ULT_16BIT = 4,
		ULT_LOOP  = 8,
		ULT_PINGPONGLOOP = 16,
	};

	char     name[32];
	char     filename[12];
	uint32le loopStart;
	uint32le loopEnd;
	uint32le sizeStart;
	uint32le sizeEnd;
	uint8le  volume;	// 0-255, apparently prior to 1.4 this was logarithmic?
	uint8le  flags;		// above
	uint16le speed;		// only exists for 1.4+
	int16le  finetune;

	// Convert an ULT sample header to OpenMPT's internal sample header.
	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize();
		mptSmp.Set16BitCuePoints();

		mptSmp.filename = mpt::String::ReadBuf(mpt::String::maybeNullTerminated, filename);

		if(sizeEnd <= sizeStart)
		{
			return;
		}

		mptSmp.nLength = sizeEnd - sizeStart;
		mptSmp.nSustainStart = loopStart;
		mptSmp.nSustainEnd = std::min(static_cast<SmpLength>(loopEnd), mptSmp.nLength);
		mptSmp.nVolume = volume;

		mptSmp.nC5Speed = speed;
		if(finetune)
		{
			mptSmp.Transpose(finetune / (12.0 * 32768.0));
		}

		if(flags & ULT_LOOP)
			mptSmp.uFlags.set(CHN_SUSTAINLOOP);
		if(flags & ULT_PINGPONGLOOP)
			mptSmp.uFlags.set(CHN_PINGPONGSUSTAIN);
		if(flags & ULT_16BIT)
		{
			mptSmp.uFlags.set(CHN_16BIT);
			mptSmp.nSustainStart /= 2;
			mptSmp.nSustainEnd /= 2;
		}
		
	}
};

MPT_BINARY_STRUCT(UltSample, 66)


/* Unhandled effects:
5x1 - do not loop sample (x is unused)
E0x - set vibrato strength (2 is normal)

The logarithmic volume scale used in older format versions here, or pretty
much anywhere for that matter. I don't even think Ultra Tracker tries to
convert them. */


static std::pair<EffectCommand, uint8> TranslateULTCommands(const uint8 e, uint8 param, uint8 version)
{

	static constexpr EffectCommand ultEffTrans[] =
	{
		CMD_ARPEGGIO,
		CMD_PORTAMENTOUP,
		CMD_PORTAMENTODOWN,
		CMD_TONEPORTAMENTO,
		CMD_VIBRATO,
		CMD_NONE,
		CMD_NONE,
		CMD_TREMOLO,
		CMD_NONE,
		CMD_OFFSET,
		CMD_VOLUMESLIDE,
		CMD_PANNING8,
		CMD_VOLUME8,
		CMD_PATTERNBREAK,
		CMD_NONE,  // extended effects, processed separately
		CMD_SPEED,
	};

	EffectCommand effect = ultEffTrans[e & 0x0F];

	switch(e & 0x0F)
	{
	case 0x00:
		if(!param || version < '3')
			effect = CMD_NONE;
		break;
	case 0x05:
		// play backwards
		if((param & 0x0F) == 0x02 || (param & 0xF0) == 0x20)
		{
			effect = CMD_S3MCMDEX;
			param = 0x9F;
		}
		if(((param & 0x0F) == 0x0C || (param & 0xF0) == 0xC0) && version >= '3')
		{
			effect = CMD_KEYOFF;
			param = 0;
		}
		break;
	case 0x07:
		if(version < '4')
			effect = CMD_NONE;
		break;
	case 0x0A:
		if(param & 0xF0)
			param &= 0xF0;
		break;
	case 0x0B:
		param = (param & 0x0F) * 0x11;
		break;
	case 0x0D: // pattern break
		param = 10 * (param >> 4) + (param & 0x0F);
		break;
	case 0x0E: // special
		switch(param >> 4)
		{
		case 0x01:
			effect = CMD_PORTAMENTOUP;
			param = 0xF0 | (param & 0x0F);
			break;
		case 0x02:
			effect = CMD_PORTAMENTODOWN;
			param = 0xF0 | (param & 0x0F);
			break;
		case 0x08:
			if(version >= '4')
			{
				effect = CMD_S3MCMDEX;
				param = 0x60 | (param & 0x0F);
			}
			break;
		case 0x09:
			effect = CMD_RETRIG;
			param &= 0x0F;
			break;
		case 0x0A:
			effect = CMD_VOLUMESLIDE;
			param = ((param & 0x0F) << 4) | 0x0F;
			break;
		case 0x0B:
			effect = CMD_VOLUMESLIDE;
			param = 0xF0 | (param & 0x0F);
			break;
		case 0x0C: case 0x0D:
			effect = CMD_S3MCMDEX;
			break;
		}
		break;
	case 0x0F:
		if(param > 0x2F)
			effect = CMD_TEMPO;
		break;
	}
	return {effect, param};
}


static int ReadULTEvent(ModCommand &m, FileReader &file, uint8 version)
{
	uint8 repeat = 1;
	uint8 b = file.ReadUint8();
	if(b == 0xFC)	// repeat event
	{
		repeat = file.ReadUint8();
		b = file.ReadUint8();
	}

	m.note = (b > 0 && b < 61) ? (b + 35 + NOTE_MIN) : NOTE_NONE;

	const auto [instr, cmd, para1, para2] = file.ReadArray<uint8, 4>();
	
	m.instr = instr;
	auto [cmd1, param1] = TranslateULTCommands(cmd & 0x0F, para1, version);
	auto [cmd2, param2]= TranslateULTCommands(cmd >> 4, para2, version);

	// sample offset -- this is even more special than digitrakker's
	if(cmd1 == CMD_OFFSET && cmd2 == CMD_OFFSET)
	{
		uint32 offset = ((param2 << 8) | param1) >> 6;
		m.SetEffectCommand(CMD_OFFSET, static_cast<ModCommand::PARAM>(offset));
		if(offset > 0xFF)
			m.SetVolumeCommand(VOLCMD_OFFSET, static_cast<ModCommand::VOL>(offset >> 8));
		return repeat;
	} else if(cmd1 == CMD_OFFSET)
	{
		uint32 offset = param1 * 4;
		param1 = mpt::saturate_cast<uint8>(offset);
		if(offset > 0xFF && ModCommand::GetEffectWeight(cmd2) < ModCommand::GetEffectWeight(CMD_OFFSET))
		{
			m.SetEffectCommand(CMD_OFFSET, static_cast<ModCommand::PARAM>(offset));
			m.SetVolumeCommand(VOLCMD_OFFSET, static_cast<ModCommand::VOL>(offset >> 8));
			return repeat;
		}
	} else if(cmd2 == CMD_OFFSET)
	{
		uint32 offset = param2 * 4;
		param2 = mpt::saturate_cast<uint8>(offset);
		if(offset > 0xFF && ModCommand::GetEffectWeight(cmd1) < ModCommand::GetEffectWeight(CMD_OFFSET))
		{
			m.SetEffectCommand(CMD_OFFSET, static_cast<ModCommand::PARAM>(offset));
			m.SetVolumeCommand(VOLCMD_OFFSET, static_cast<ModCommand::VOL>(offset >> 8));
			return repeat;
		}
	} else if(cmd1 == cmd2)
	{
		// don't try to figure out how ultratracker does this, it's quite random
		cmd2 = CMD_NONE;
	}
	if(cmd2 == CMD_VOLUME || (cmd2 == CMD_NONE && cmd1 != CMD_VOLUME))
	{
		// swap commands
		std::swap(cmd1, cmd2);
		std::swap(param1, param2);
	}

	// Combine slide commands, if possible
	ModCommand::CombineEffects(cmd2, param2, cmd1, param1);
	m.FillInTwoCommands(cmd1, param1, cmd2, param2);

	return repeat;
}


// Functor for postfixing ULT patterns (this is easier than just remembering everything WHILE we're reading the pattern events)
struct PostFixUltCommands
{
	PostFixUltCommands(CHANNELINDEX channels) : numChannels{channels}
	{
		isPortaActive.resize(channels, false);
	}

	void operator()(ModCommand &m)
	{
		// Attempt to fix portamentos.
		// UltraTracker will slide until the destination note is reached or 300 is encountered.

		// Stop porta?
		if(m.command == CMD_TONEPORTAMENTO && m.param == 0)
		{
			isPortaActive[curChannel] = false;
			m.command = CMD_NONE;
		}
		if(m.volcmd == VOLCMD_TONEPORTAMENTO && m.vol == 0)
		{
			isPortaActive[curChannel] = false;
			m.volcmd = VOLCMD_NONE;
		}

		// Apply porta?
		if(m.note == NOTE_NONE && isPortaActive[curChannel])
		{
			if(m.command == CMD_NONE && m.volcmd != VOLCMD_TONEPORTAMENTO)
			{
				m.command = CMD_TONEPORTAMENTO;
				m.param = 0;
			} else if(m.volcmd == VOLCMD_NONE && m.command != CMD_TONEPORTAMENTO)
			{
				m.volcmd = VOLCMD_TONEPORTAMENTO;
				m.vol = 0;
			}
		} else	// new note -> stop porta (or initialize again)
		{
			isPortaActive[curChannel] = (m.command == CMD_TONEPORTAMENTO || m.volcmd == VOLCMD_TONEPORTAMENTO);
		}

		// attempt to fix F00 (reset to tempo 125, speed 6)
		if(writeT125 && m.command == CMD_NONE)
		{
			m.command = CMD_TEMPO;
			m.param = 125;
		}
		if(m.command == CMD_SPEED && m.param == 0)
		{
			m.param = 6;
			writeT125 = true;
		}
		if(m.command == CMD_TEMPO)	// don't try to fix this anymore if the tempo has already changed.
		{
			writeT125 = false;
		}
		curChannel++;
		if(curChannel >= numChannels)
			curChannel = 0;
	}

	std::vector<bool> isPortaActive;
	const CHANNELINDEX numChannels;
	CHANNELINDEX curChannel = 0;
	bool writeT125 = false;
};


static bool ValidateHeader(const UltFileHeader &fileHeader)
{
	if(fileHeader.version < '1' || fileHeader.version > '4'
	   || std::memcmp(fileHeader.signature, "MAS_UTrack_V00", sizeof(fileHeader.signature)))
	{
		return false;
	}
	return true;
}

static uint64 GetHeaderMinimumAdditionalSize(const UltFileHeader &fileHeader)
{
	return fileHeader.messageLength * 32u + 3u + 256u;
}

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderULT(MemoryFileReader file, const uint64 *pfilesize)
{
	UltFileHeader fileHeader;
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


bool CSoundFile::ReadULT(FileReader &file, ModLoadingFlags loadFlags)
{
	file.Rewind();

	UltFileHeader fileHeader;
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
	if(!file.CanRead(mpt::saturate_cast<FileReader::off_t>(GetHeaderMinimumAdditionalSize(fileHeader))))
	{
		return false;
	}

	InitializeGlobals(MOD_TYPE_ULT);
	m_songName = mpt::String::ReadBuf(mpt::String::maybeNullTerminated, fileHeader.songName);

	const mpt::uchar *versions[] = {UL_("<1.4"), UL_("1.4"), UL_("1.5"), UL_("1.6")};
	m_modFormat.formatName = U_("UltraTracker");
	m_modFormat.type = U_("ult");
	m_modFormat.madeWithTracker = U_("UltraTracker ") + versions[fileHeader.version - '1'];
	m_modFormat.charset = mpt::Charset::CP437;

	m_SongFlags = SONG_ITCOMPATGXX | SONG_ITOLDEFFECTS;  // this will be converted to IT format by MPT.

	// Read "messageLength" lines, each containing 32 characters.
	m_songMessage.ReadFixedLineLength(file, fileHeader.messageLength * 32, 32, 0);

	if(SAMPLEINDEX numSamples = file.ReadUint8(); numSamples < MAX_SAMPLES)
		m_nSamples = numSamples;
	else
		return false;

	for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
	{
		UltSample sampleHeader;

		// Annoying: v4 added a field before the end of the struct
		if(fileHeader.version >= '4')
		{
			file.ReadStruct(sampleHeader);
		} else
		{
			file.ReadStructPartial(sampleHeader, 64);
			sampleHeader.finetune = sampleHeader.speed;
			sampleHeader.speed = 8363;
		}

		sampleHeader.ConvertToMPT(Samples[smp]);
		m_szNames[smp] = mpt::String::ReadBuf(mpt::String::maybeNullTerminated, sampleHeader.name);
	}

	ReadOrderFromFile<uint8>(Order(), file, 256, 0xFF, 0xFE);

	if(CHANNELINDEX numChannels = file.ReadUint8() + 1u; numChannels <= MAX_BASECHANNELS)
		m_nChannels = numChannels;
	else
		return false;

	PATTERNINDEX numPats = file.ReadUint8() + 1;

	for(CHANNELINDEX chn = 0; chn < GetNumChannels(); chn++)
	{
		ChnSettings[chn].Reset();
		if(fileHeader.version >= '3')
			ChnSettings[chn].nPan = ((file.ReadUint8() & 0x0F) << 4) + 8;
		else
			ChnSettings[chn].nPan = (chn & 1) ? 192 : 64;
	}

	Patterns.ResizeArray(numPats);
	for(PATTERNINDEX pat = 0; pat < numPats; pat++)
	{
		if(!Patterns.Insert(pat, 64))
			return false;
	}

	for(CHANNELINDEX chn = 0; chn < m_nChannels; chn++)
	{
		ModCommand evnote;
		for(PATTERNINDEX pat = 0; pat < numPats && file.CanRead(5); pat++)
		{
			ModCommand *note = Patterns[pat].GetpModCommand(0, chn);
			ROWINDEX row = 0;
			while(row < 64)
			{
				int repeat = ReadULTEvent(evnote, file, fileHeader.version);
				if(repeat + row > 64)
					repeat = 64 - row;
				if(repeat == 0)
					break;
				while(repeat--)
				{
					*note = evnote;
					note += GetNumChannels();
					row++;
				}
			}
		}
	}

	// Post-fix some effects.
	Patterns.ForEachModCommand(PostFixUltCommands(GetNumChannels()));

	if(loadFlags & loadSampleData)
	{
		for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
		{
			SampleIO(
				Samples[smp].uFlags[CHN_16BIT] ? SampleIO::_16bit : SampleIO::_8bit,
				SampleIO::mono,
				SampleIO::littleEndian,
				SampleIO::signedPCM)
				.ReadSample(Samples[smp], file);
		}
	}
	return true;
}


OPENMPT_NAMESPACE_END
