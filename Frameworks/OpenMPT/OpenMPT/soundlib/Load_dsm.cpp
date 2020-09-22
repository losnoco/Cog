/*
 * Load_dsm.cpp
 * ------------
 * Purpose: Digisound Interface Kit (DSIK) Internal Format (DSM v2 / RIFF) module loader
 * Notes  : 1. There is also another fundamentally different DSIK DSM v1 module format, not handled here.
 *          MilkyTracker can load it, but the only files of this format seen in the wild are also
 *          available in their original format, so I did not bother implementing it so far.
 *
 *          2. Using both PLAY.EXE v1.02 and v2.00, commands not supported in MOD do not seem to do
 *          anything at all.
 *          In particular commands 0x11-0x13 handled below are ignored, and no files have been spotted
 *          in the wild using any commands > 0x0F at all.
 *          S3M-style retrigger does not seem to exist - it is translated to volume slides by CONV.EXE,
 *          and J00 in S3M files is not converted either. S3M pattern loops (SBx) are not converted
 *          properly by CONV.EXE and completely ignored by PLAY.EXE.
 *          Command 8 (set panning) uses 00-80 for regular panning and A4 for surround, probably
 *          making DSIK one of the first applications to use this particular encoding scheme still
 *          used in "extended" S3Ms today.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

OPENMPT_NAMESPACE_BEGIN

struct DSMChunk
{
	char     magic[4];
	uint32le size;
};

MPT_BINARY_STRUCT(DSMChunk, 8)


struct DSMSongHeader
{
	char     songName[28];
	uint16le fileVersion;
	uint16le flags;
	uint16le orderPos;
	uint16le restartPos;
	uint16le numOrders;
	uint16le numSamples;
	uint16le numPatterns;
	uint16le numChannels;
	uint8le  globalVol;
	uint8le  mastervol;
	uint8le  speed;
	uint8le  bpm;
	uint8le  panPos[16];
	uint8le  orders[128];
};

MPT_BINARY_STRUCT(DSMSongHeader, 192)


struct DSMSampleHeader
{
	char     filename[13];
	uint16le flags;
	uint8le  volume;
	uint32le length;
	uint32le loopStart;
	uint32le loopEnd;
	uint32le dataPtr;	// Interal sample pointer during playback in DSIK
	uint32le sampleRate;
	char     sampleName[28];

	// Convert a DSM sample header to OpenMPT's internal sample header.
	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize();
		mptSmp.filename = mpt::String::ReadBuf(mpt::String::nullTerminated, filename);

		mptSmp.nC5Speed = sampleRate;
		mptSmp.uFlags.set(CHN_LOOP, (flags & 1) != 0);
		mptSmp.nLength = length;
		mptSmp.nLoopStart = loopStart;
		mptSmp.nLoopEnd = loopEnd;
		mptSmp.nVolume = std::min(volume.get(), uint8(64)) * 4;
	}

	// Retrieve the internal sample format flags for this sample.
	SampleIO GetSampleFormat() const
	{
		SampleIO sampleIO(
			SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::unsignedPCM);
		if(flags & 0x40)
			sampleIO |= SampleIO::deltaPCM;	// fairlight.dsm by Comrade J
		else if(flags & 0x02)
			sampleIO |= SampleIO::signedPCM;
		if(flags & 0x04)
			sampleIO |= SampleIO::_16bit;
		return sampleIO;
	}
};

MPT_BINARY_STRUCT(DSMSampleHeader, 64)


struct DSMHeader
{
	char fileMagic0[4];
	char fileMagic1[4];
	char fileMagic2[4];
};

MPT_BINARY_STRUCT(DSMHeader, 12)


static bool ValidateHeader(const DSMHeader &fileHeader)
{
	if(!std::memcmp(fileHeader.fileMagic0, "RIFF", 4)
		&& !std::memcmp(fileHeader.fileMagic2, "DSMF", 4))
	{
		// "Normal" DSM files with RIFF header
		// <RIFF> <file size> <DSMF>
		return true;
	} else if(!std::memcmp(fileHeader.fileMagic0, "DSMF", 4))
	{
		// DSM files with alternative header
		// <DSMF> <4 bytes, usually 4x NUL or RIFF> <file size> <4 bytes, usually DSMF but not always>
		return true;
	} else
	{
		return false;
	}
}


CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderDSM(MemoryFileReader file, const uint64 *pfilesize)
{
	DSMHeader fileHeader;
	if(!file.ReadStruct(fileHeader))
	{
		return ProbeWantMoreData;
	}
	if(!ValidateHeader(fileHeader))
	{
		return ProbeFailure;
	}
	if(std::memcmp(fileHeader.fileMagic0, "DSMF", 4) == 0)
	{
		if(!file.Skip(4))
		{
			return ProbeWantMoreData;
		}
	}
	DSMChunk chunkHeader;
	if(!file.ReadStruct(chunkHeader))
	{
		return ProbeWantMoreData;
	}
	if(std::memcmp(chunkHeader.magic, "SONG", 4))
	{
		return ProbeFailure;
	}
	MPT_UNREFERENCED_PARAMETER(pfilesize);
	return ProbeSuccess;
}


bool CSoundFile::ReadDSM(FileReader &file, ModLoadingFlags loadFlags)
{
	file.Rewind();

	DSMHeader fileHeader;
	if(!file.ReadStruct(fileHeader))
	{
		return false;
	}
	if(!ValidateHeader(fileHeader))
	{
		return false;
	}
	if(std::memcmp(fileHeader.fileMagic0, "DSMF", 4) == 0)
	{
		file.Skip(4);
	}
	DSMChunk chunkHeader;
	if(!file.ReadStruct(chunkHeader))
	{
		return false;
	}
	// Technically, the song chunk could be anywhere in the file, but we're going to simplify
	// things by not using a chunk header here and just expect it to be right at the beginning.
	if(std::memcmp(chunkHeader.magic, "SONG", 4))
	{
		return false;
	}
	if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	DSMSongHeader songHeader;
	file.ReadStructPartial(songHeader, chunkHeader.size);
	if(songHeader.numOrders > 128 || songHeader.numChannels > 16 || songHeader.numPatterns > 256 || songHeader.restartPos > 128)
	{
		return false;
	}

	InitializeGlobals(MOD_TYPE_DSM);

	m_modFormat.formatName = U_("DSIK Format");
	m_modFormat.type = U_("dsm");
	m_modFormat.charset = mpt::Charset::CP437;

	m_songName = mpt::String::ReadBuf(mpt::String::maybeNullTerminated, songHeader.songName);
	m_nChannels = std::max(songHeader.numChannels.get(), uint16(1));
	m_nDefaultSpeed = songHeader.speed;
	m_nDefaultTempo.Set(songHeader.bpm);
	m_nDefaultGlobalVolume = std::min(songHeader.globalVol.get(), uint8(64)) * 4u;
	if(!m_nDefaultGlobalVolume) m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;
	if(songHeader.mastervol == 0x80)
	{
		m_nSamplePreAmp = std::min(256u / m_nChannels, 128u);
	} else
	{
		m_nSamplePreAmp = songHeader.mastervol & 0x7F;
	}

	// Read channel panning
	for(CHANNELINDEX chn = 0; chn < 16; chn++)
	{
		ChnSettings[chn].Reset();
		if(songHeader.panPos[chn] <= 0x80)
		{
			ChnSettings[chn].nPan = songHeader.panPos[chn] * 2;
		}
	}

	ReadOrderFromArray(Order(), songHeader.orders, songHeader.numOrders, 0xFF, 0xFE);
	if(songHeader.restartPos < songHeader.numOrders)
		Order().SetRestartPos(songHeader.restartPos);

	// Read pattern and sample chunks
	PATTERNINDEX patNum = 0;
	while(file.ReadStruct(chunkHeader))
	{
		FileReader chunk = file.ReadChunk(chunkHeader.size);

		if(!memcmp(chunkHeader.magic, "PATT", 4) && (loadFlags & loadPatternData))
		{
			// Read pattern
			if(!Patterns.Insert(patNum, 64))
			{
				continue;
			}
			chunk.Skip(2);

			ModCommand dummy = ModCommand::Empty();
			ROWINDEX row = 0;
			while(chunk.CanRead(1) && row < 64)
			{
				uint8 flag = chunk.ReadUint8();
				if(!flag)
				{
					row++;
					continue;
				}

				CHANNELINDEX chn = (flag & 0x0F);
				ModCommand &m = (chn < GetNumChannels() ? *Patterns[patNum].GetpModCommand(row, chn) : dummy);

				if(flag & 0x80)
				{
					uint8 note = chunk.ReadUint8();
					if(note)
					{
						if(note <= 12 * 9) note += 11 + NOTE_MIN;
						m.note = note;
					}
				}
				if(flag & 0x40)
				{
					m.instr = chunk.ReadUint8();
				}
				if (flag & 0x20)
				{
					m.volcmd = VOLCMD_VOLUME;
					m.vol = std::min(chunk.ReadUint8(), uint8(64));
				}
				if(flag & 0x10)
				{
					auto [command, param] = chunk.ReadArray<uint8, 2>();
					switch(command)
					{
						// Portamentos
					case 0x11:
					case 0x12:
						command &= 0x0F;
						break;
						// 3D Sound (?)
					case 0x13:
						command = 'X' - 55;
						param = 0x91;
						break;
					default:
						// Volume + Offset (?)
						if(command > 0x10)
							command = ((command & 0xF0) == 0x20) ? 0x09 : 0xFF;
					}
					m.command = command;
					m.param = param;
					ConvertModCommand(m);
				}
			}
			patNum++;
		} else if(!memcmp(chunkHeader.magic, "INST", 4) && GetNumSamples() < SAMPLEINDEX(MAX_SAMPLES - 1))
		{
			// Read sample
			m_nSamples++;
			ModSample &sample = Samples[m_nSamples];

			DSMSampleHeader sampleHeader;
			chunk.ReadStruct(sampleHeader);
			sampleHeader.ConvertToMPT(sample);

			m_szNames[m_nSamples] = mpt::String::ReadBuf(mpt::String::maybeNullTerminated, sampleHeader.sampleName);

			if(loadFlags & loadSampleData)
			{
				sampleHeader.GetSampleFormat().ReadSample(sample, chunk);
			}
		}
	}

	return true;
}


OPENMPT_NAMESPACE_END
