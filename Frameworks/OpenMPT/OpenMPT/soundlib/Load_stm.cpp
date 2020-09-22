/*
 * Load_stm.cpp
 * ------------
 * Purpose: STM (ScreamTracker 2) module loader
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

OPENMPT_NAMESPACE_BEGIN

// STM sample header struct
struct STMSampleHeader
{
	char     filename[12];  // Can't have long comments - just filename comments :)
	uint8le  zero;
	uint8le  disk;       // A blast from the past
	uint16le offset;     // 20-bit offset in file (lower 4 bits are zero)
	uint16le length;     // Sample length
	uint16le loopStart;  // Loop start point
	uint16le loopEnd;    // Loop end point
	uint8le  volume;     // Volume
	uint8le  reserved2;
	uint16le sampleRate;
	uint8le  reserved3[6];

	// Convert an STM sample header to OpenMPT's internal sample header.
	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize();
		mptSmp.filename = mpt::String::ReadBuf(mpt::String::maybeNullTerminated, filename);

		mptSmp.nC5Speed = sampleRate;
		mptSmp.nVolume = std::min(volume.get(), uint8(64)) * 4;
		mptSmp.nLength = length;
		mptSmp.nLoopStart = loopStart;
		mptSmp.nLoopEnd = loopEnd;

		if(mptSmp.nLength < 2) mptSmp.nLength = 0;

		if(mptSmp.nLoopStart < mptSmp.nLength
			&& mptSmp.nLoopEnd > mptSmp.nLoopStart
			&& mptSmp.nLoopEnd != 0xFFFF)
		{
			mptSmp.uFlags = CHN_LOOP;
			mptSmp.nLoopEnd = std::min(mptSmp.nLoopEnd, mptSmp.nLength);
		}
	}
};

MPT_BINARY_STRUCT(STMSampleHeader, 32)


// STM file header
struct STMFileHeader
{
	char  songname[20];
	char  trackername[8];  // !Scream! for ST 2.xx
	uint8 dosEof;          // 0x1A
	uint8 filetype;        // 1=song, 2=module (only 2 is supported, of course) :)
	uint8 verMajor;
	uint8 verMinor;
	uint8 initTempo;
	uint8 numPatterns;
	uint8 globalVolume;
	uint8 reserved[13];
};

MPT_BINARY_STRUCT(STMFileHeader, 48)


static bool ValidateHeader(const STMFileHeader &fileHeader)
{
	if(fileHeader.filetype != 2
		|| (fileHeader.dosEof != 0x1A && fileHeader.dosEof != 2)	// ST2 ignores this, ST3 doesn't. Broken versions of putup10.stm / putup11.stm have dosEof = 2.
		|| fileHeader.verMajor != 2
		|| (fileHeader.verMinor != 0 && fileHeader.verMinor != 10 && fileHeader.verMinor != 20 && fileHeader.verMinor != 21)
		|| fileHeader.numPatterns > 64
		|| (fileHeader.globalVolume > 64 && fileHeader.globalVolume != 0x58))	// 0x58 may be a placeholder value in earlier ST2 versions.
	{
		return false;
	}
	// Tracker string can be anything really (ST2 and ST3 won't check it),
	// but we do not want to generate too many false positives here, as
	// STM already has very few magic bytes anyway.
	// Magic bytes that have been found in the wild are !Scream!, BMOD2STM, WUZAMOD! and SWavePro.
	for(uint8 c : fileHeader.trackername)
	{
		if(c < 0x20 || c >= 0x7F)
			return false;
	}

	return true;
}


static uint64 GetHeaderMinimumAdditionalSize(const STMFileHeader &fileHeader)
{
	return 31 * sizeof(STMSampleHeader) + (fileHeader.verMinor == 0 ? 64 : 128) + fileHeader.numPatterns * 64 * 4;
}


CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderSTM(MemoryFileReader file, const uint64 *pfilesize)
{
	STMFileHeader fileHeader;
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


bool CSoundFile::ReadSTM(FileReader &file, ModLoadingFlags loadFlags)
{
	file.Rewind();

	STMFileHeader fileHeader;
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

	InitializeGlobals(MOD_TYPE_STM);

	m_songName = mpt::String::ReadBuf(mpt::String::maybeNullTerminated, fileHeader.songname);

	m_modFormat.formatName = U_("Scream Tracker 2");
	m_modFormat.type = U_("stm");
	m_modFormat.madeWithTracker = mpt::format(U_("Scream Tracker %1.%2"))(fileHeader.verMajor, mpt::ufmt::dec0<2>(fileHeader.verMinor));
	m_modFormat.charset = mpt::Charset::CP437;

	m_nSamples = 31;
	m_nChannels = 4;
	m_nMinPeriod = 64;
	m_nMaxPeriod = 0x7FFF;
	
	uint8 initTempo = fileHeader.initTempo;
	if(fileHeader.verMinor < 21)
		initTempo = ((initTempo / 10u) << 4u) + initTempo % 10u;
	if(initTempo == 0)
		initTempo = 0x60;

	m_nDefaultTempo = ConvertST2Tempo(initTempo);
	m_nDefaultSpeed = initTempo >> 4;
	if(fileHeader.verMinor > 10)
		m_nDefaultGlobalVolume = std::min(fileHeader.globalVolume, uint8(64)) * 4u;

	// Setting up channels
	for(CHANNELINDEX chn = 0; chn < 4; chn++)
	{
		ChnSettings[chn].Reset();
		ChnSettings[chn].nPan = (chn & 1) ? 0x40 : 0xC0;
	}

	// Read samples
	uint16 sampleOffsets[31];
	for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
	{
		STMSampleHeader sampleHeader;
		file.ReadStruct(sampleHeader);
		if(sampleHeader.zero != 0 && sampleHeader.zero != 46)	// putup10.stm has zero = 46
			return false;
		sampleHeader.ConvertToMPT(Samples[smp]);
		m_szNames[smp] = mpt::String::ReadBuf(mpt::String::maybeNullTerminated, sampleHeader.filename);
		sampleOffsets[smp - 1] = sampleHeader.offset;
	}

	// Read order list
	ReadOrderFromFile<uint8>(Order(), file, fileHeader.verMinor == 0 ? 64 : 128);
	for(auto &pat : Order())
	{
		if(pat == 99 || pat == 255)	// 99 is regular, sometimes a single 255 entry can be found too
			pat = Order.GetInvalidPatIndex();
		else if(pat > 63)
			return false;
	}

	if(loadFlags & loadPatternData)
		Patterns.ResizeArray(fileHeader.numPatterns);
	for(PATTERNINDEX pat = 0; pat < fileHeader.numPatterns; pat++)
	{
		if(!(loadFlags & loadPatternData) || !Patterns.Insert(pat, 64))
		{
			for(int i = 0; i < 64 * 4; i++)
			{
				uint8 note = file.ReadUint8();
				if(note < 0xFB || note > 0xFD)
					file.Skip(3);
			}
			continue;
		}

		auto m = Patterns[pat].begin();
		ORDERINDEX breakPos = ORDERINDEX_INVALID;
		ROWINDEX breakRow = 63;  // Candidate row for inserting pattern break

		for(ROWINDEX row = 0; row < 64; row++)
		{
			uint8 newTempo = 0;
			for(CHANNELINDEX chn = 0; chn < 4; chn++, m++)
			{
				uint8 note = file.ReadUint8(), insVol, volCmd, cmdInf;
				switch(note)
				{
				case 0xFB:
					note = insVol = volCmd = cmdInf = 0x00;
					break;
				case 0xFC:
					continue;
				case 0xFD:
					m->note = NOTE_NOTECUT;
					continue;
				default:
					{
					const auto patData = file.ReadArray<uint8, 3>();
					insVol = patData[0];
					volCmd = patData[1];
					cmdInf = patData[2];
					}
					break;
				}

				if(note == 0xFE)
					m->note = NOTE_NOTECUT;
				else if(note < 0x60)
					m->note = (note >> 4) * 12 + (note & 0x0F) + 36 + NOTE_MIN;

				m->instr = insVol >> 3;
				if(m->instr > 31)
				{
					m->instr = 0;
				}
			
				uint8 vol = (insVol & 0x07) | ((volCmd & 0xF0) >> 1);
				if(vol <= 64)
				{
					m->volcmd = VOLCMD_VOLUME;
					m->vol = vol;
				}

				static constexpr EffectCommand stmEffects[] =
				{
					CMD_NONE,        CMD_SPEED,          CMD_POSITIONJUMP, CMD_PATTERNBREAK,   // .ABC
					CMD_VOLUMESLIDE, CMD_PORTAMENTODOWN, CMD_PORTAMENTOUP, CMD_TONEPORTAMENTO, // DEFG
					CMD_VIBRATO,     CMD_TREMOR,         CMD_ARPEGGIO,     CMD_NONE,           // HIJK
					CMD_NONE,        CMD_NONE,           CMD_NONE,         CMD_NONE,           // LMNO
					// KLMNO can be entered in the editor but don't do anything
				};

				m->command = stmEffects[volCmd & 0x0F];
				m->param = cmdInf;

				switch(m->command)
				{
				case CMD_VOLUMESLIDE:
					// Lower nibble always has precedence, and there are no fine slides.
					if(m->param & 0x0F)
						m->param &= 0x0F;
					else
						m->param &= 0xF0;
					break;

				case CMD_PATTERNBREAK:
					m->param = (m->param & 0xF0) * 10 + (m->param & 0x0F);
					if(breakPos != ORDERINDEX_INVALID && m->param == 0)
					{
						// Merge Bxx + C00 into just Bxx
						m->command = CMD_POSITIONJUMP;
						m->param = static_cast<ModCommand::PARAM>(breakPos);
						breakPos = ORDERINDEX_INVALID;
					}
					LimitMax(breakRow, row);
					break;

				case CMD_POSITIONJUMP:
					// This effect is also very weird.
					// Bxx doesn't appear to cause an immediate break -- it merely
					// sets the next order for when the pattern ends (either by
					// playing it all the way through, or via Cxx effect)
					breakPos = m->param;
					breakRow = 63;
					m->command = CMD_NONE;
					break;

				case CMD_TREMOR:
					// this actually does something with zero values, and has no
					// effect memory. which makes SENSE for old-effects tremor,
					// but ST3 went and screwed it all up by adding an effect
					// memory and IT followed that, and those are much more popular
					// than STM so we kind of have to live with this effect being
					// broken... oh well. not a big loss.
					break;

				case CMD_SPEED:
					if(fileHeader.verMinor < 21)
					{
						m->param = ((m->param / 10u) << 4u) + m->param % 10u;
					}

					if(!m->param)
					{
						m->command = CMD_NONE;
						break;
					}

#ifdef MODPLUG_TRACKER
					// ST2 has a very weird tempo mode where the length of a tick depends both
					// on the ticks per row and a scaling factor. Try to write the tempo into a separate command.
					newTempo = m->param;
					m->param >>= 4;
#endif // MODPLUG_TRACKER
					break;

				default:
					// Anything not listed above is a no-op if there's no value, as ST2 doesn't have effect memory.
					if(!m->param)
					{
						m->command = CMD_NONE;
					}
					break;
				}
			}
			if(newTempo != 0)
			{
				Patterns[pat].WriteEffect(EffectWriter(CMD_TEMPO, mpt::saturate_round<ModCommand::PARAM>(ConvertST2Tempo(newTempo).ToDouble())).Row(row).RetryPreviousRow());
			}
		}

		if(breakPos != ORDERINDEX_INVALID)
		{
			Patterns[pat].WriteEffect(EffectWriter(CMD_POSITIONJUMP, static_cast<ModCommand::PARAM>(breakPos)).Row(breakRow).RetryPreviousRow());
		}
	}

	// Reading Samples
	if(loadFlags & loadSampleData)
	{
		const SampleIO sampleIO(
			SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::signedPCM);

		for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
		{
			ModSample &sample = Samples[smp];
			// ST2 just plays random noise for samples with a default volume of 0
			if(sample.nLength && sample.nVolume > 0)
			{
				FileReader::off_t sampleOffset = sampleOffsets[smp - 1] << 4;
				// acidlamb.stm has some bogus samples with sample offsets past EOF
				if(sampleOffset > sizeof(STMFileHeader) && file.Seek(sampleOffset))
				{
					sampleIO.ReadSample(sample, file);
				}
			}
		}
	}

	return true;
}


OPENMPT_NAMESPACE_END
