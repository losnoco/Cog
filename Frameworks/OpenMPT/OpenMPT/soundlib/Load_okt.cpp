/*
 * Load_okt.cpp
 * ------------
 * Purpose: OKT (Oktalyzer) module loader
 * Notes  : (currently none)
 * Authors: Storlek (Original author - http://schismtracker.org/ - code ported with permission)
 *			Johannes Schultz (OpenMPT Port, tweaks)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

OPENMPT_NAMESPACE_BEGIN

struct OktIffChunk
{
	// IFF chunk names
	enum ChunkIdentifiers
	{
		idCMOD	= MagicBE("CMOD"),
		idSAMP	= MagicBE("SAMP"),
		idSPEE	= MagicBE("SPEE"),
		idSLEN	= MagicBE("SLEN"),
		idPLEN	= MagicBE("PLEN"),
		idPATT	= MagicBE("PATT"),
		idPBOD	= MagicBE("PBOD"),
		idSBOD	= MagicBE("SBOD"),
	};

	uint32be signature;	// IFF chunk name
	uint32be chunksize;	// chunk size without header
};

MPT_BINARY_STRUCT(OktIffChunk, 8)

struct OktSample
{
	char     name[20];
	uint32be length;		// length in bytes
	uint16be loopStart;		// *2 for real value
	uint16be loopLength;	// ditto
	uint16be volume;		// default volume
	uint16be type;			// 7-/8-bit sample
};

MPT_BINARY_STRUCT(OktSample, 32)


// Parse the sample header block
static void ReadOKTSamples(FileReader &chunk, std::vector<bool> &sample7bit, CSoundFile &sndFile)
{
	sndFile.m_nSamples = std::min(static_cast<SAMPLEINDEX>(chunk.BytesLeft() / sizeof(OktSample)), static_cast<SAMPLEINDEX>(MAX_SAMPLES - 1));
	sample7bit.resize(sndFile.GetNumSamples());

	for(SAMPLEINDEX smp = 1; smp <= sndFile.GetNumSamples(); smp++)
	{
		ModSample &mptSmp = sndFile.GetSample(smp);
		OktSample oktSmp;
		chunk.ReadStruct(oktSmp);

		mptSmp.Initialize();
		sndFile.m_szNames[smp] = mpt::String::ReadBuf(mpt::String::maybeNullTerminated, oktSmp.name);

		mptSmp.nC5Speed = 8287;
		mptSmp.nGlobalVol = 64;
		mptSmp.nVolume = std::min(oktSmp.volume.get(), uint16(64)) * 4u;
		mptSmp.nLength = oktSmp.length & ~1;	// round down
		// Parse loops
		const SmpLength loopStart = oktSmp.loopStart * 2;
		const SmpLength loopLength = oktSmp.loopLength * 2;
		if(loopLength > 2 && loopStart + loopLength <= mptSmp.nLength)
		{
			mptSmp.uFlags.set(CHN_SUSTAINLOOP);
			mptSmp.nSustainStart = loopStart;
			mptSmp.nSustainEnd = loopStart + loopLength;
		}
		sample7bit[smp - 1] = (oktSmp.type == 0 || oktSmp.type == 2);
	}
}


// Parse a pattern block
static void ReadOKTPattern(FileReader &chunk, PATTERNINDEX pat, CSoundFile &sndFile)
{
	if(!chunk.CanRead(2))
	{
		// Invent empty pattern
		sndFile.Patterns.Insert(pat, 64);
		return;
	}

	ROWINDEX rows = Clamp(static_cast<ROWINDEX>(chunk.ReadUint16BE()), ROWINDEX(1), MAX_PATTERN_ROWS);

	if(!sndFile.Patterns.Insert(pat, rows))
	{
		return;
	}

	const CHANNELINDEX chns = sndFile.GetNumChannels();

	for(ROWINDEX row = 0; row < rows; row++)
	{
		ModCommand *rowCmd = sndFile.Patterns[pat].GetRow(row);
		for(CHANNELINDEX chn = 0; chn < chns; chn++)
		{
			ModCommand &m = rowCmd[chn];
			const auto [note, instr, effect, param] = chunk.ReadArray<uint8, 4>();
			m.param = param;

			if(note > 0 && note <= 36)
			{
				m.note = note + (NOTE_MIDDLEC - 13);
				m.instr = instr + 1;
			} else
			{
				m.instr = 0;
			}

			switch(effect)
			{
			case 0:	// Nothing
				m.param = 0;
				break;

			case 1: // 1 Portamento Down (Period)
				m.command = CMD_PORTAMENTOUP;
				m.param &= 0x0F;
				break;
			case 2: // 2 Portamento Up (Period)
				m.command = CMD_PORTAMENTODOWN;
				m.param &= 0x0F;
				break;

#if 0
			/* these aren't like Jxx: "down" means to *subtract* the offset from the note.
			For now I'm going to leave these unimplemented. */
			case 10: // A Arpeggio 1 (down, orig, up)
			case 11: // B Arpeggio 2 (orig, up, orig, down)
				if (m.param)
					m.command = CMD_ARPEGGIO;
				break;
#endif
			// This one is close enough to "standard" arpeggio -- I think!
			case 12: // C Arpeggio 3 (up, up, orig)
				if (m.param)
					m.command = CMD_ARPEGGIO;
				break;

			case 13: // D Slide Down (Notes)
				if (m.param)
				{
					m.command = CMD_NOTESLIDEDOWN;
					m.param = 0x10 | std::min(uint8(0x0F), m.param);
				}
				break;
			case 30: // U Slide Up (Notes)
				if (m.param)
				{
					m.command = CMD_NOTESLIDEUP;
					m.param = 0x10 | std::min(uint8(0x0F), m.param);
				}
				break;
			// We don't have fine note slide, but this is supposed to happen once
			// per row. Sliding every 5 (non-note) ticks kind of works (at least at
			// speed 6), but implementing fine slides would of course be better.
			case 21: // L Slide Down Once (Notes)
				if (m.param)
				{
					m.command = CMD_NOTESLIDEDOWN;
					m.param = 0x50 | std::min(uint8(0x0F), m.param);
				}
				break;
			case 17: // H Slide Up Once (Notes)
				if (m.param)
				{
					m.command = CMD_NOTESLIDEUP;
					m.param = 0x50 | std::min(uint8(0x0F), m.param);
				}
				break;

			case 15: // F Set Filter <>00:ON
				m.command = CMD_MODCMDEX;
				m.param = !!m.param;
				break;

			case 25: // P Pos Jump
				m.command = CMD_POSITIONJUMP;
				break;

			case 27: // R Release sample (apparently not listed in the help!)
				m.Clear();
				m.note = NOTE_KEYOFF;
				break;

			case 28: // S Speed
				m.command = CMD_SPEED; // or tempo?
				break;

			case 31: // V Volume
				m.command = CMD_VOLUMESLIDE;
				switch (m.param >> 4)
				{
				case 4:
					if (m.param != 0x40)
					{
						m.param &= 0x0F; // D0x
						break;
					}
					// 0x40 is set volume -- fall through
					[[fallthrough]];
				case 0: case 1: case 2: case 3:
					m.volcmd = VOLCMD_VOLUME;
					m.vol = m.param;
					m.command = CMD_NONE;
					m.param = 0;
					break;
				case 5:
					m.param = (m.param & 0x0F) << 4; // Dx0
					break;
				case 6:
					m.param = 0xF0 | std::min(static_cast<uint8>(m.param & 0x0F), uint8(0x0E)); // DFx
					break;
				case 7:
					m.param = (std::min(static_cast<uint8>(m.param & uint8(0x0F)), uint8(0x0E)) << 4) | 0x0F; // DxF
					break;
				default:
					// Junk.
					m.command = CMD_NONE;
					m.param = 0;
					break;
				}
				break;

#if 0
			case 24: // O Old Volume (???)
				m.command = CMD_VOLUMESLIDE;
				m.param = 0;
				break;
#endif

			default:
				m.command = CMD_NONE;
				m.param = 0;
				break;
			}
		}
	}
}


CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderOKT(MemoryFileReader file, const uint64 *pfilesize)
{
	if(!file.CanRead(8))
	{
		return ProbeWantMoreData;
	}
	if(!file.ReadMagic("OKTASONG"))
	{
		return ProbeFailure;
	}
	OktIffChunk iffHead;
	if(!file.ReadStruct(iffHead))
	{
		return ProbeWantMoreData;
	}
	if(iffHead.chunksize == 0)
	{
		return ProbeFailure;
	}
	if((iffHead.signature & 0x80808080u) != 0) // ASCII?
	{
		return ProbeFailure;
	}
	MPT_UNREFERENCED_PARAMETER(pfilesize);
	return ProbeSuccess;
}


bool CSoundFile::ReadOKT(FileReader &file, ModLoadingFlags loadFlags)
{
	file.Rewind();
	if(!file.ReadMagic("OKTASONG"))
	{
		return false;
	}

	// prepare some arrays to store offsets etc.
	std::vector<FileReader> patternChunks;
	std::vector<FileReader> sampleChunks;
	std::vector<bool> sample7bit;	// 7-/8-bit sample
	ORDERINDEX numOrders = 0;

	InitializeGlobals(MOD_TYPE_OKT);

	m_modFormat.formatName = U_("Oktalyzer");
	m_modFormat.type = U_("okt");
	m_modFormat.charset = mpt::Charset::ISO8859_1;

	// Go through IFF chunks...
	while(file.CanRead(sizeof(OktIffChunk)))
	{
		OktIffChunk iffHead;
		if(!file.ReadStruct(iffHead))
		{
			break;
		}

		FileReader chunk = file.ReadChunk(iffHead.chunksize);
		if(!chunk.IsValid())
		{
			break;
		}

		switch(iffHead.signature)
		{
		case OktIffChunk::idCMOD:
			// Read that weird channel setup table
			if(m_nChannels > 0 || chunk.GetLength() < 8)
			{
				break;
			}

			for(CHANNELINDEX chn = 0; chn < 4; chn++)
			{
				const uint8 ch1 = chunk.ReadUint8(), ch2 = chunk.ReadUint8();
				if(ch1 || ch2)
				{
					ChnSettings[m_nChannels].Reset();
					ChnSettings[m_nChannels++].nPan = (((chn & 3) == 1) || ((chn & 3) == 2)) ? 0xC0 : 0x40;
				}
				ChnSettings[m_nChannels].Reset();
				ChnSettings[m_nChannels++].nPan = (((chn & 3) == 1) || ((chn & 3) == 2)) ? 0xC0 : 0x40;
			}

			if(loadFlags == onlyVerifyHeader)
			{
				return true;
			}
			break;

		case OktIffChunk::idSAMP:
			// Convert sample headers
			if(m_nSamples > 0)
			{
				break;
			}
			ReadOKTSamples(chunk, sample7bit, *this);
			break;

		case OktIffChunk::idSPEE:
			// Read default speed
			if(chunk.GetLength() >= 2)
			{
				m_nDefaultSpeed = Clamp(chunk.ReadUint16BE(), uint16(1), uint16(255));
			}
			break;

		case OktIffChunk::idSLEN:
			// Number of patterns, we don't need this.
			break;

		case OktIffChunk::idPLEN:
			// Read number of valid orders
			if(chunk.GetLength() >= 2)
			{
				numOrders = chunk.ReadUint16BE();
			}
			break;

		case OktIffChunk::idPATT:
			// Read the orderlist
			ReadOrderFromFile<uint8>(Order(), chunk, chunk.GetLength(), 0xFF, 0xFE);
			break;

		case OktIffChunk::idPBOD:
			// Don't read patterns for now, as the number of channels might be unknown at this point.
			if(patternChunks.size() < 256)
			{
				patternChunks.push_back(chunk);
			}
			break;

		case OktIffChunk::idSBOD:
			// Sample data - same as with patterns, as we need to know the sample format / length
			if(sampleChunks.size() < MAX_SAMPLES - 1 && chunk.GetLength() > 0)
			{
				sampleChunks.push_back(chunk);
			}
			break;
		}
	}

	// If there wasn't even a CMOD chunk, we can't really load this.
	if(m_nChannels == 0)
		return false;

	m_nDefaultTempo.Set(125);
	m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;
	m_nSamplePreAmp = m_nVSTiVolume = 48;
	m_nMinPeriod = 0x71 * 4;
	m_nMaxPeriod = 0x358 * 4;

	// Fix orderlist
	Order().resize(numOrders);

	// Read patterns
	if(loadFlags & loadPatternData)
	{
		Patterns.ResizeArray(static_cast<PATTERNINDEX>(patternChunks.size()));
		for(PATTERNINDEX pat = 0; pat < patternChunks.size(); pat++)
		{
			ReadOKTPattern(patternChunks[pat], pat, *this);
		}
	}

	// Read samples
	size_t fileSmp = 0;
	for(SAMPLEINDEX smp = 1; smp < m_nSamples; smp++)
	{
		if(fileSmp >= sampleChunks.size() || !(loadFlags & loadSampleData))
			break;

		ModSample &mptSample = Samples[smp];
		if(mptSample.nLength == 0)
			continue;

		// Weird stuff?
		LimitMax(mptSample.nLength, mpt::saturate_cast<SmpLength>(sampleChunks[fileSmp].GetLength()));

		SampleIO(
			SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::bigEndian,
			sample7bit[smp - 1] ? SampleIO::PCM7to8 : SampleIO::signedPCM)
			.ReadSample(mptSample, sampleChunks[fileSmp]);

		fileSmp++;
	}

	return true;
}


OPENMPT_NAMESPACE_END
