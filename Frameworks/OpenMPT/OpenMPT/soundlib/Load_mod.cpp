/*
 * Load_mod.cpp
 * ------------
 * Purpose: MOD / NST (ProTracker / NoiseTracker), M15 / STK (Ultimate Soundtracker / Soundtracker) and ST26 (SoundTracker 2.6 / Ice Tracker) module loader / saver
 * Notes  : "2000 LOC for processing MOD files?!" you say? Well, this file also contains loaders for some formats that are almost identical to MOD, and extensive
 *          heuristics for more or less broken MOD files and files saved with tons of different trackers, to allow for the most optimal playback.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include "Tables.h"
#ifndef MODPLUG_NO_FILESAVE
#include "mpt/io/base.hpp"
#include "mpt/io/io.hpp"
#include "mpt/io/io_stdstream.hpp"
#include "../common/mptFileIO.h"
#endif
#ifdef MPT_EXTERNAL_SAMPLES
// For loading external data in Startrekker files
#include "mpt/fs/fs.hpp"
#include "mpt/io_file/inputfile.hpp"
#include "mpt/io_file_read/inputfile_filecursor.hpp"
#include "../common/mptPathString.h"
#endif  // MPT_EXTERNAL_SAMPLES

OPENMPT_NAMESPACE_BEGIN

void CSoundFile::ConvertModCommand(ModCommand &m, const uint8 command, const uint8 param)
{
	m.param = param;
	switch(command)
	{
	case 0x00: m.command = m.param ? CMD_ARPEGGIO : CMD_NONE; break;
	case 0x01: m.command = CMD_PORTAMENTOUP; break;
	case 0x02: m.command = CMD_PORTAMENTODOWN; break;
	case 0x03: m.command = CMD_TONEPORTAMENTO; break;
	case 0x04: m.command = CMD_VIBRATO; break;
	case 0x05: m.command = CMD_TONEPORTAVOL; break;
	case 0x06: m.command = CMD_VIBRATOVOL; break;
	case 0x07: m.command = CMD_TREMOLO; break;
	case 0x08: m.command = CMD_PANNING8; break;
	case 0x09: m.command = CMD_OFFSET; break;
	case 0x0A: m.command = CMD_VOLUMESLIDE; break;
	case 0x0B: m.command = CMD_POSITIONJUMP; break;
	case 0x0C: m.command = CMD_VOLUME; break;
	case 0x0D: m.command = CMD_PATTERNBREAK; m.param = ((m.param >> 4) * 10) + (m.param & 0x0F); break;
	case 0x0E: m.command = CMD_MODCMDEX; break;
	case 0x0F:
		// For a very long time, this code imported 0x20 as CMD_SPEED for MOD files, but this seems to contradict
		// pretty much the majority of other MOD player out there.
		// 0x20 is Speed: Impulse Tracker, Scream Tracker, old ModPlug
		// 0x20 is Tempo: ProTracker, XMPlay, Imago Orpheus, Cubic Player, ChibiTracker, BeRoTracker, DigiTrakker, DigiTrekker, Disorder Tracker 2, DMP, Extreme's Tracker, ...
		if(m.param < 0x20)
			m.command = CMD_SPEED;
		else
			m.command = CMD_TEMPO;
		break;

	// Extension for XM extended effects
	case 'G' - 55: m.command = CMD_GLOBALVOLUME; break;  //16
	case 'H' - 55: m.command = CMD_GLOBALVOLSLIDE; break;
	case 'K' - 55: m.command = CMD_KEYOFF; break;
	case 'L' - 55: m.command = CMD_SETENVPOSITION; break;
	case 'P' - 55: m.command = CMD_PANNINGSLIDE; break;
	case 'R' - 55: m.command = CMD_RETRIG; break;
	case 'T' - 55: m.command = CMD_TREMOR; break;
	case 'W' - 55: m.command = CMD_DUMMY; break;
	case 'X' - 55: m.command = CMD_XFINEPORTAUPDOWN;	break;
	case 'Y' - 55: m.command = CMD_PANBRELLO; break;   // 34
	case 'Z' - 55: m.command = CMD_MIDI; break;        // 35
	case '\\' - 56: m.command = CMD_SMOOTHMIDI; break;  // 36 - note: this is actually displayed as "-" in FT2, but seems to be doing nothing.
	case 37:        m.command = CMD_SMOOTHMIDI; break;  // BeRoTracker uses this for smooth MIDI macros for some reason; in old OpenMPT versions this was reserved for the unimplemented "velocity" command
	case '#' + 3:   m.command = CMD_XPARAM; break;      // 38
	default:        m.command = CMD_NONE;
	}
}

#ifndef MODPLUG_NO_FILESAVE

void CSoundFile::ModSaveCommand(const ModCommand &source, uint8 &command, uint8 &param, const bool toXM, const bool compatibilityExport) const
{
	command = 0;
	param = source.param;
	switch(source.command)
	{
	case CMD_NONE:		command = param = 0; break;
	case CMD_ARPEGGIO:	command = 0; break;
	case CMD_PORTAMENTOUP:
		if (GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_STM|MOD_TYPE_MPT))
		{
			if ((param & 0xF0) == 0xE0) { command = 0x0E; param = ((param & 0x0F) >> 2) | 0x10; break; }
			else if ((param & 0xF0) == 0xF0) { command = 0x0E; param &= 0x0F; param |= 0x10; break; }
		}
		command = 0x01;
		break;
	case CMD_PORTAMENTODOWN:
		if(GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_STM|MOD_TYPE_MPT))
		{
			if ((param & 0xF0) == 0xE0) { command = 0x0E; param= ((param & 0x0F) >> 2) | 0x20; break; }
			else if ((param & 0xF0) == 0xF0) { command = 0x0E; param &= 0x0F; param |= 0x20; break; }
		}
		command = 0x02;
		break;
	case CMD_TONEPORTAMENTO:	command = 0x03; break;
	case CMD_VIBRATO:			command = 0x04; break;
	case CMD_TONEPORTAVOL:		command = 0x05; break;
	case CMD_VIBRATOVOL:		command = 0x06; break;
	case CMD_TREMOLO:			command = 0x07; break;
	case CMD_PANNING8:
		command = 0x08;
		if(GetType() & MOD_TYPE_S3M)
		{
			if(param <= 0x80)
			{
				param = mpt::saturate_cast<uint8>(param * 2);
			}
			else if(param == 0xA4)	// surround
			{
				if(compatibilityExport || !toXM)
				{
					command = param = 0;
				}
				else
				{
					command = 'X' - 55;
					param = 91;
				}
			}
		}
		break;
	case CMD_OFFSET:			command = 0x09; break;
	case CMD_VOLUMESLIDE:		command = 0x0A; break;
	case CMD_POSITIONJUMP:		command = 0x0B; break;
	case CMD_VOLUME:			command = 0x0C; break;
	case CMD_PATTERNBREAK:		command = 0x0D; param = ((param / 10) << 4) | (param % 10); break;
	case CMD_MODCMDEX:			command = 0x0E; break;
	case CMD_SPEED:				command = 0x0F; param = std::min(param, uint8(0x1F)); break;
	case CMD_TEMPO:				command = 0x0F; param = std::max(param, uint8(0x20)); break;
	case CMD_GLOBALVOLUME:		command = 'G' - 55; break;
	case CMD_GLOBALVOLSLIDE:	command = 'H' - 55; break;
	case CMD_KEYOFF:			command = 'K' - 55; break;
	case CMD_SETENVPOSITION:	command = 'L' - 55; break;
	case CMD_PANNINGSLIDE:		command = 'P' - 55; break;
	case CMD_RETRIG:			command = 'R' - 55; break;
	case CMD_TREMOR:			command = 'T' - 55; break;
	case CMD_DUMMY:				command = 'W' - 55; break;
	case CMD_XFINEPORTAUPDOWN:	command = 'X' - 55;
		if(compatibilityExport && param >= 0x30)	// X1x and X2x are legit, everything above are MPT extensions, which don't belong here.
			param = 0;	// Don't set command to 0 to indicate that there *was* some X command here...
		break;
	case CMD_PANBRELLO:
		if(compatibilityExport)
			command = param = 0;
		else
			command = 'Y' - 55;
		break;
	case CMD_MIDI:
		if(compatibilityExport)
			command = param = 0;
		else
			command = 'Z' - 55;
		break;
	case CMD_SMOOTHMIDI: //rewbs.smoothVST: 36
		if(compatibilityExport)
			command = param = 0;
		else
			command = '\\' - 56;
		break;
	case CMD_XPARAM: //rewbs.XMfixes - XParam is 38
		if(compatibilityExport)
			command = param = 0;
		else
			command = '#' + 3;
		break;
	case CMD_S3MCMDEX:
		{
			ModCommand mConv;
			mConv.command = CMD_S3MCMDEX;
			mConv.param = param;
			mConv.ExtendedS3MtoMODEffect();
			ModSaveCommand(mConv, command, param, toXM, compatibilityExport);
		}
		return;
	case CMD_VOLUME8:
		command = 0x0C;
		param = static_cast<uint8>((param + 3u) / 4u);
		break;
	default:
		command = param = 0;
	}

	// Don't even think about saving XM effects in MODs...
	if(command > 0x0F && !toXM)
	{
		command = param = 0;
	}
}

#endif  // MODPLUG_NO_FILESAVE


// File Header
struct MODFileHeader
{
	uint8be numOrders;
	uint8be restartPos;  // Tempo (early SoundTracker) or restart position (only PC trackers?)
	uint8be orderList[128];
};

MPT_BINARY_STRUCT(MODFileHeader, 130)


// Sample Header
struct MODSampleHeader
{
	char     name[22];
	uint16be length;
	uint8be  finetune;
	uint8be  volume;
	uint16be loopStart;
	uint16be loopLength;

	// Convert an MOD sample header to OpenMPT's internal sample header.
	void ConvertToMPT(ModSample &mptSmp, bool is4Chn) const
	{
		mptSmp.Initialize(MOD_TYPE_MOD);
		mptSmp.nLength = length * 2;
		mptSmp.nFineTune = MOD2XMFineTune(finetune & 0x0F);
		mptSmp.nVolume = 4u * std::min(volume.get(), uint8(64));

		SmpLength lStart = loopStart * 2;
		SmpLength lLength = loopLength * 2;
		// See if loop start is incorrect as words, but correct as bytes (like in Soundtracker modules)
		if(lLength > 2 && (lStart + lLength > mptSmp.nLength)
		   && (lStart / 2 + lLength <= mptSmp.nLength))
		{
			lStart /= 2;
		}

		if(mptSmp.nLength == 2)
		{
			mptSmp.nLength = 0;
		}

		if(mptSmp.nLength)
		{
			mptSmp.nLoopStart = lStart;
			mptSmp.nLoopEnd = lStart + lLength;

			if(mptSmp.nLoopStart >= mptSmp.nLength)
			{
				mptSmp.nLoopStart = mptSmp.nLength - 1;
			}
			if(mptSmp.nLoopStart > mptSmp.nLoopEnd || mptSmp.nLoopEnd < 4 || mptSmp.nLoopEnd - mptSmp.nLoopStart < 4)
			{
				mptSmp.nLoopStart = 0;
				mptSmp.nLoopEnd = 0;
			}

			// Fix for most likely broken sample loops. This fixes super_sufm_-_new_life.mod (M.K.) which has a long sample which is looped from 0 to 4.
			// This module also has notes outside of the Amiga frequency range, so we cannot say that it should be played using ProTracker one-shot loops.
			// On the other hand, "Crew Generation" by Necros (6CHN) has a sample with a similar loop, which is supposed to be played.
			// To be able to correctly play both modules, we will draw a somewhat arbitrary line here and trust the loop points in MODs with more than
			// 4 channels, even if they are tiny and at the very beginning of the sample.
			if(mptSmp.nLoopEnd <= 8 && mptSmp.nLoopStart == 0 && mptSmp.nLength > mptSmp.nLoopEnd && is4Chn)
			{
				mptSmp.nLoopEnd = 0;
			}
			if(mptSmp.nLoopEnd > mptSmp.nLoopStart)
			{
				mptSmp.uFlags.set(CHN_LOOP);
			}
		}
	}

	// Convert OpenMPT's internal sample header to a MOD sample header.
	SmpLength ConvertToMOD(const ModSample &mptSmp)
	{
		SmpLength writeLength = mptSmp.HasSampleData() ? mptSmp.nLength : 0;
		// If the sample size is odd, we have to add a padding byte, as all sample sizes in MODs are even.
		if((writeLength % 2u) != 0)
		{
			writeLength++;
		}
		LimitMax(writeLength, SmpLength(0x1FFFE));

		length = static_cast<uint16>(writeLength / 2u);

		if(mptSmp.RelativeTone < 0)
		{
			finetune = 0x08;
		} else if(mptSmp.RelativeTone > 0)
		{
			finetune = 0x07;
		} else
		{
			finetune = XM2MODFineTune(mptSmp.nFineTune);
		}
		volume = static_cast<uint8>(mptSmp.nVolume / 4u);

		loopStart = 0;
		loopLength = 1;
		if(mptSmp.uFlags[CHN_LOOP] && (mptSmp.nLoopStart + 2u) < writeLength)
		{
			const SmpLength loopEnd = Clamp(mptSmp.nLoopEnd, (mptSmp.nLoopStart & ~1) + 2u, writeLength) & ~1;
			loopStart = static_cast<uint16>(mptSmp.nLoopStart / 2u);
			loopLength = static_cast<uint16>((loopEnd - (mptSmp.nLoopStart & ~1)) / 2u);
		}

		return writeLength;
	}

	// Compute a "rating" of this sample header by counting invalid header data to ultimately reject garbage files.
	uint32 GetInvalidByteScore() const
	{
		return ((volume > 64) ? 1 : 0)
		       + ((finetune > 15) ? 1 : 0)
		       + ((loopStart > length * 2) ? 1 : 0);
	}

	bool HasDiskName() const
	{
		return (!memcmp(name, "st-", 3) || !memcmp(name, "ST-", 3)) && name[5] == ':';
	}

	// Suggested threshold for rejecting invalid files based on cumulated score returned by GetInvalidByteScore
	static constexpr uint32 INVALID_BYTE_THRESHOLD = 40;

	// This threshold is used for files where the file magic only gives a
	// fragile result which alone would lead to too many false positives.
	// In particular, the files from Inconexia demo by Iguana
	// (https://www.pouet.net/prod.php?which=830) which have 3 \0 bytes in
	// the file magic tend to cause misdetection of random files.
	static constexpr uint32 INVALID_BYTE_FRAGILE_THRESHOLD = 1;

	// Retrieve the internal sample format flags for this sample.
	static SampleIO GetSampleFormat()
	{
		return SampleIO(
			SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::bigEndian,
			SampleIO::signedPCM);
	}
};

MPT_BINARY_STRUCT(MODSampleHeader, 30)

// Pattern data of a 4-channel MOD file
using MODPatternData = std::array<std::array<std::array<uint8, 4>, 4>, 64>;

// Synthesized StarTrekker instruments
struct AMInstrument
{
	char     am[2];        // "AM"
	char     zero[4];
	uint16be startLevel;   // Start level
	uint16be attack1Level; // Attack 1 level
	uint16be attack1Speed; // Attack 1 speed
	uint16be attack2Level; // Attack 2 level
	uint16be attack2Speed; // Attack 2 speed
	uint16be sustainLevel; // Sustain level
	uint16be decaySpeed;   // Decay speed
	uint16be sustainTime;  // Sustain time
	uint16be nt;           // ?
	uint16be releaseSpeed; // Release speed
	uint16be waveform;     // Waveform
	int16be  pitchFall;    // Pitch fall
	uint16be vibAmp;       // Vibrato amplitude
	uint16be vibSpeed;     // Vibrato speed
	uint16be octave;       // Base frequency

	void ConvertToMPT(ModSample &sample, ModInstrument &ins, mpt::fast_prng &rng) const
	{
		sample.nLength = waveform == 3 ? 1024 : 32;
		sample.nLoopStart = 0;
		sample.nLoopEnd = sample.nLength;
		sample.uFlags.set(CHN_LOOP);
		sample.nVolume = 256;  // prelude.mod has volume 0 in sample header
		sample.nVibDepth = mpt::saturate_cast<uint8>(vibAmp * 2);
		sample.nVibRate = static_cast<uint8>(vibSpeed);
		sample.nVibType = VIB_SINE;
		sample.RelativeTone = static_cast<int8>(-12 * octave);
		if(sample.AllocateSample())
		{
			int8 *p = sample.sample8();
			for(SmpLength i = 0; i < sample.nLength; i++)
			{
				switch(waveform)
				{
				default:
				case 0: p[i] = ModSinusTable[i * 2];            break; // Sine
				case 1: p[i] = static_cast<int8>(-128 + i * 8); break; // Saw
				case 2: p[i] = i < 16 ? -128 : 127;             break; // Square
				case 3: p[i] = mpt::random<int8>(rng);          break; // Noise
				}
			}
		}

		InstrumentEnvelope &volEnv = ins.VolEnv;
		volEnv.dwFlags.set(ENV_ENABLED);
		volEnv.reserve(6);
		volEnv.push_back(0, static_cast<EnvelopeNode::value_t>(startLevel / 4));

		const struct
		{
			uint16 level, speed;
		} points[] = {{startLevel, 0}, {attack1Level, attack1Speed}, {attack2Level, attack2Speed}, {sustainLevel, decaySpeed}, {sustainLevel, sustainTime}, {0, releaseSpeed}};

		for(uint8 i = 1; i < std::size(points); i++)
		{
			int duration = std::min(points[i].speed, uint16(256));
			// Sustain time is already in ticks, no need to compute the segment duration.
			if(i != 4)
			{
				if(duration == 0)
				{
					volEnv.dwFlags.set(ENV_LOOP);
					volEnv.nLoopStart = volEnv.nLoopEnd = static_cast<uint8>(volEnv.size() - 1);
					break;
				}

				// Startrekker increments / decrements the envelope level by the stage speed
				// until it reaches the next stage level.
				int a, b;
				if(points[i].level > points[i - 1].level)
				{
					a = points[i].level - points[i - 1].level;
					b = 256 - points[i - 1].level;
				} else
				{
					a = points[i - 1].level - points[i].level;
					b = points[i - 1].level;
				}
				// Release time is again special.
				if(i == 5)
					b = 256;
				else if(b == 0)
					b = 1;
				duration = std::max((256 * a) / (duration * b), 1);
			}
			if(duration > 0)
			{
				volEnv.push_back(volEnv.back().tick + static_cast<EnvelopeNode::tick_t>(duration), static_cast<EnvelopeNode::value_t>(points[i].level / 4));
			}
		}

		if(pitchFall)
		{
			InstrumentEnvelope &pitchEnv = ins.PitchEnv;
			pitchEnv.dwFlags.set(ENV_ENABLED);
			pitchEnv.reserve(2);
			pitchEnv.push_back(0, ENVELOPE_MID);
			// cppcheck false-positive
			// cppcheck-suppress zerodiv
			pitchEnv.push_back(static_cast<EnvelopeNode::tick_t>(1024 / abs(pitchFall)), pitchFall > 0 ? ENVELOPE_MIN : ENVELOPE_MAX);
		}
	}
};

MPT_BINARY_STRUCT(AMInstrument, 36)

struct PT36IffChunk
{
	// IFF chunk names
	enum ChunkIdentifiers
	{
		idVERS = MagicBE("VERS"),
		idINFO = MagicBE("INFO"),
		idCMNT = MagicBE("CMNT"),
		idPTDT = MagicBE("PTDT"),
	};

	uint32be signature;  // IFF chunk name
	uint32be chunksize;  // chunk size without header
};

MPT_BINARY_STRUCT(PT36IffChunk, 8)

struct PT36InfoChunk
{
	char     name[32];
	uint16be numSamples;
	uint16be numOrders;
	uint16be numPatterns;
	uint16be volume;
	uint16be tempo;
	uint16be flags;
	uint16be dateDay;
	uint16be dateMonth;
	uint16be dateYear;
	uint16be dateHour;
	uint16be dateMinute;
	uint16be dateSecond;
	uint16be playtimeHour;
	uint16be playtimeMinute;
	uint16be playtimeSecond;
	uint16be playtimeMsecond;
};

MPT_BINARY_STRUCT(PT36InfoChunk, 64)


// Check if header magic equals a given string.
static bool IsMagic(const char *magic1, const char (&magic2)[5]) noexcept
{
	return std::memcmp(magic1, magic2, 4) == 0;
}


// For .DTM files from Apocalypse Abyss, where the first 2108 bytes are swapped
template<typename T, typename TFileReader>
static T ReadAndSwap(TFileReader &file, const bool swapBytes)
{
	T value;
	if(file.Read(value) && swapBytes)
	{
		static_assert(sizeof(value) % 2u == 0);
		auto byteView = mpt::as_raw_memory(value);
		for(size_t i = 0; i < sizeof(T); i += 2)
		{
			std::swap(byteView[i], byteView[i + 1]);
		}
	}
	return value;
}


static uint32 ReadSample(const MODSampleHeader &sampleHeader, ModSample &sample, mpt::charbuf<MAX_SAMPLENAME> &sampleName, bool is4Chn)
{
	sampleHeader.ConvertToMPT(sample, is4Chn);
	sampleName = mpt::String::ReadBuf(mpt::String::spacePadded, sampleHeader.name);
	// Get rid of weird characters in sample names.
	for(auto &c : sampleName.buf)
	{
		if(c > 0 && c < ' ')
		{
			c = ' ';
		}
	}
	// Check for invalid values
	return sampleHeader.GetInvalidByteScore();
}


// Count malformed bytes in MOD pattern data
static uint32 CountMalformedMODPatternData(const MODPatternData &patternData, const bool extendedFormat)
{
	const uint8 mask = extendedFormat ? 0xE0 : 0xF0;
	uint32 malformedBytes = 0;
	for(const auto &row : patternData)
	{
		for(const auto &data : row)
		{
			if(data[0] & mask)
				malformedBytes++;
			if(!extendedFormat)
			{
				const uint16 period = (((static_cast<uint16>(data[0]) & 0x0F) << 8) | data[1]);
				if(period && period != 0xFFF)
				{
					// Allow periods to deviate by +/-1 as found in some files
					const auto CompareFunc = [](uint16 l, uint16 r) { return l > (r + 1); };
					const auto PeriodTable = mpt::as_span(ProTrackerPeriodTable).subspan(24, 36);
					if(!std::binary_search(PeriodTable.begin(), PeriodTable.end(), period, CompareFunc))
						malformedBytes += 2;
				}
			}
		}
	}
	return malformedBytes;
}


// Check if number of malformed bytes in MOD pattern data exceeds some threshold
template <typename TFileReader>
static bool ValidateMODPatternData(TFileReader &file, const uint32 threshold, const bool extendedFormat)
{
	MODPatternData patternData;
	if(!file.Read(patternData))
		return false;
	return CountMalformedMODPatternData(patternData, extendedFormat) <= threshold;
}


// Parse the order list to determine how many patterns are used in the file.
static PATTERNINDEX GetNumPatterns(FileReader &file, ModSequence &Order, ORDERINDEX numOrders, SmpLength totalSampleLen, CHANNELINDEX &numChannels, SmpLength wowSampleLen, bool validateHiddenPatterns)
{
	PATTERNINDEX numPatterns = 0;         // Total number of patterns in file (determined by going through the whole order list) with pattern number < 128
	PATTERNINDEX officialPatterns = 0;    // Number of patterns only found in the "official" part of the order list (i.e. order positions < claimed order length)
	PATTERNINDEX numPatternsIllegal = 0;  // Total number of patterns in file, also counting in "invalid" pattern indexes >= 128

	for(ORDERINDEX ord = 0; ord < 128; ord++)
	{
		PATTERNINDEX pat = Order[ord];
		if(pat < 128 && numPatterns <= pat)
		{
			numPatterns = pat + 1;
			if(ord < numOrders)
			{
				officialPatterns = numPatterns;
			}
		}
		if(pat >= numPatternsIllegal)
		{
			numPatternsIllegal = pat + 1;
		}
	}

	// Remove the garbage patterns past the official order end now that we don't need them anymore.
	Order.resize(numOrders);

	const size_t patternStartOffset = file.GetPosition();
	const size_t sizeWithoutPatterns = totalSampleLen + patternStartOffset;
	const size_t sizeWithOfficialPatterns = sizeWithoutPatterns + officialPatterns * numChannels * 256;

	if(wowSampleLen && (wowSampleLen + patternStartOffset) + numPatterns * 8 * 256 == (file.GetLength() & ~1))
	{
		// Check if this is a Mod's Grave WOW file... WOW files use the M.K. magic but are actually 8CHN files.
		// We do a simple pattern validation as well for regular MOD files that have non-module data attached at the end
		// (e.g. ponylips.mod, MD5 c039af363b1d99a492dafc5b5f9dd949, SHA1 1bee1941c47bc6f913735ce0cf1880b248b8fc93)
		file.Seek(patternStartOffset + numPatterns * 4 * 256);
		if(ValidateMODPatternData(file, 16, true))
			numChannels = 8;
		file.Seek(patternStartOffset);
	} else if(numPatterns != officialPatterns && (validateHiddenPatterns || sizeWithOfficialPatterns == file.GetLength()))
	{
		// 15-sample SoundTracker specifics:
		// Fix SoundTracker modules where "hidden" patterns should be ignored.
		// razor-1911.mod (MD5 b75f0f471b0ae400185585ca05bf7fe8, SHA1 4de31af234229faec00f1e85e1e8f78f405d454b)
		// and captain_fizz.mod (MD5 55bd89fe5a8e345df65438dbfc2df94e, SHA1 9e0e8b7dc67939885435ea8d3ff4be7704207a43)
		// seem to have the "correct" file size when only taking the "official" patterns into account,
		// but they only play correctly when also loading the inofficial patterns.
		// On the other hand, the SoundTracker module
		// wolf1.mod (MD5 a4983d7a432d324ce8261b019257f4ed, SHA1 aa6b399d02546bcb6baf9ec56a8081730dea3f44),
		// wolf3.mod (MD5 af60840815aa9eef43820a7a04417fa6, SHA1 24d6c2e38894f78f6c5c6a4b693a016af8fa037b)
		// and jean_baudlot_-_bad_dudes_vs_dragonninja-dragonf.mod (MD5 fa48e0f805b36bdc1833f6b82d22d936, SHA1 39f2f8319f4847fe928b9d88eee19d79310b9f91)
		// only play correctly if we ignore the hidden patterns.
		// Hence, we have a peek at the first hidden pattern and check if it contains a lot of illegal data.
		// If that is the case, we assume it's part of the sample data and only consider the "official" patterns.

		// 31-sample NoiseTracker / ProTracker specifics:
		// Interestingly, (broken) variants of the ProTracker modules
		// "killing butterfly" (MD5 bd676358b1dbb40d40f25435e845cf6b, SHA1 9df4ae21214ff753802756b616a0cafaeced8021),
		// "quartex" by Reflex (MD5 35526bef0fb21cb96394838d94c14bab, SHA1 116756c68c7b6598dcfbad75a043477fcc54c96c),
		// seem to have the "correct" file size when only taking the "official" patterns into account, but they only play
		// correctly when also loading the inofficial patterns.
		// On the other hand, "Shofixti Ditty.mod" from Star Control 2 (MD5 62b7b0819123400e4d5a7813eef7fc7d, SHA1 8330cd595c61f51c37a3b6f2a8559cf3fcaaa6e8)
		// doesn't sound correct when taking the second "inofficial" pattern into account.
		file.Seek(patternStartOffset + officialPatterns * numChannels * 256);
		if(!ValidateMODPatternData(file, 64, true))
			numPatterns = officialPatterns;
		file.Seek(patternStartOffset);
	}

	if(numPatternsIllegal > numPatterns && sizeWithoutPatterns + numPatternsIllegal * numChannels * 256 == file.GetLength())
	{
		// Even those illegal pattern indexes (> 128) appear to be valid... What a weird file!
		// e.g. NIETNU.MOD, where the end of the order list is filled with FF rather than 00, and the file actually contains 256 patterns.
		numPatterns = numPatternsIllegal;
	} else if(numPatternsIllegal >= 0xFF)
	{
		// Patterns FE and FF are used with S3M semantics (e.g. some MODs written with old OpenMPT versions)
		Order.Replace(0xFE, Order.GetIgnoreIndex());
		Order.Replace(0xFF, Order.GetInvalidPatIndex());
	}

	return numPatterns;
}


std::pair<uint8, uint8> CSoundFile::ReadMODPatternEntry(FileReader &file, ModCommand &m)
{
	return ReadMODPatternEntry(file.ReadArray<uint8, 4>(), m);
}


std::pair<uint8, uint8> CSoundFile::ReadMODPatternEntry(const std::array<uint8, 4> data, ModCommand &m)
{
	// Read Period
	uint16 period = (((static_cast<uint16>(data[0]) & 0x0F) << 8) | data[1]);
	size_t note = NOTE_NONE;
	if(period > 0 && period != 0xFFF)
	{
		note = std::size(ProTrackerPeriodTable) + 23 + NOTE_MIN;
		for(size_t i = 0; i < std::size(ProTrackerPeriodTable); i++)
		{
			if(period >= ProTrackerPeriodTable[i])
			{
				if(period != ProTrackerPeriodTable[i] && i != 0)
				{
					uint16 p1 = ProTrackerPeriodTable[i - 1];
					uint16 p2 = ProTrackerPeriodTable[i];
					if(p1 - period < (period - p2))
					{
						note = i + 23 + NOTE_MIN;
						break;
					}
				}
				note = i + 24 + NOTE_MIN;
				break;
			}
		}
	}
	m.note = static_cast<ModCommand::NOTE>(note);
	// Read Instrument
	m.instr = (data[2] >> 4) | (data[0] & 0x10);
	// Read Effect
	m.command = CMD_NONE;
	uint8 command = data[2] & 0x0F, param = data[3];
	return {command, param};
}


struct MODMagicResult
{
	const mpt::uchar *madeWithTracker = nullptr;
	uint32 invalidByteThreshold = MODSampleHeader::INVALID_BYTE_THRESHOLD;
	uint16 patternDataOffset    = 1084;
	CHANNELINDEX numChannels    = 0;
	bool isNoiseTracker         = false;
	bool isStartrekker          = false;
	bool isGenericMultiChannel  = false;
	bool setMODVBlankTiming     = false;
	bool swapBytes              = false;
};


static bool CheckMODMagic(const char magic[4], MODMagicResult &result)
{
	if(IsMagic(magic, "M.K.")      // ProTracker and compatible
	   || IsMagic(magic, "M!K!")   // ProTracker (>64 patterns)
	   || IsMagic(magic, "PATT")   // ProTracker 3.6
	   || IsMagic(magic, "NSMS")   // kingdomofpleasure.mod by bee hunter
	   || IsMagic(magic, "LARD"))  // judgement_day_gvine.mod by 4-mat
	{
		result.madeWithTracker = UL_("Generic ProTracker or compatible");
		result.numChannels = 4;
	} else if(IsMagic(magic, "M&K!")     // "His Master's Noise" musicdisk
	          || IsMagic(magic, "FEST")  // "His Master's Noise" musicdisk
	          || IsMagic(magic, "N.T."))
	{
		result.madeWithTracker = UL_("NoiseTracker");
		result.isNoiseTracker = true;
		result.setMODVBlankTiming = true;
		result.numChannels = 4;
	} else if(IsMagic(magic, "OKTA")
	          || IsMagic(magic, "OCTA"))
	{
		// Oktalyzer
		result.madeWithTracker = UL_("Oktalyzer");
		result.numChannels = 8;
	} else if(IsMagic(magic, "CD81")
	          || IsMagic(magic, "CD61"))
	{
		// Octalyser on Atari STe/Falcon
		result.madeWithTracker = UL_("Octalyser (Atari)");
		result.numChannels = magic[2] - '0';
	} else if(IsMagic(magic, "M\0\0\0") || IsMagic(magic, "8\0\0\0"))
	{
		// Inconexia demo by Iguana, delta samples (https://www.pouet.net/prod.php?which=830)
		result.madeWithTracker = UL_("Inconexia demo (delta samples)");
		result.invalidByteThreshold = MODSampleHeader::INVALID_BYTE_FRAGILE_THRESHOLD;
		result.numChannels = (magic[0] == '8') ? 8 : 4;
	} else if(!memcmp(magic, "FA0", 3) && magic[3] >= '4' && magic[3] <= '8')
	{
		// Digital Tracker on Atari Falcon
		result.madeWithTracker = UL_("Digital Tracker");
		result.numChannels = magic[3] - '0';
		// Digital Tracker MODs contain four bytes (00 40 00 00) right after the magic bytes which don't seem to do anything special.
		result.patternDataOffset = 1088;
	} else if((!memcmp(magic, "FLT", 3) || !memcmp(magic, "EXO", 3)) && magic[3] >= '4' && magic[3] <= '9')
	{
		// FLTx / EXOx - Startrekker by Exolon / Fairlight
		result.madeWithTracker = UL_("Startrekker");
		result.isStartrekker = true;
		result.setMODVBlankTiming = true;
		result.numChannels = magic[3] - '0';
	} else if(magic[0] >= '1' && magic[0] <= '9' && !memcmp(magic + 1, "CHN", 3))
	{
		// xCHN - Many trackers
		result.madeWithTracker = UL_("Generic MOD-compatible Tracker");
		result.isGenericMultiChannel = true;
		result.numChannels = magic[0] - '0';
	} else if(magic[0] >= '1' && magic[0] <= '9' && magic[1] >= '0' && magic[1] <= '9'
	          && (!memcmp(magic + 2, "CH", 2) || !memcmp(magic + 2, "CN", 2)))
	{
		// xxCN / xxCH - Many trackers
		result.madeWithTracker = UL_("Generic MOD-compatible Tracker");
		result.isGenericMultiChannel = true;
		result.numChannels = (magic[0] - '0') * 10 + magic[1] - '0';
	} else if(!memcmp(magic, "TDZ", 3) && magic[3] >= '1' && magic[3] <= '9')
	{
		// TDZx - TakeTracker (only TDZ1-TDZ3 should exist, but historically this code only supported 4-9 channels, so we keep those for the unlikely case that they were actually used for something)
		result.madeWithTracker = UL_("TakeTracker");
		result.numChannels = magic[3] - '0';
	} else if(IsMagic(magic, ".M.K"))
	{
		// Hacked .DMF files from the game "Apocalypse Abyss"
		result.numChannels = 4;
		result.swapBytes = true;
	} else if(IsMagic(magic, "WARD"))
	{
		// MUSIC*.DTA files from the DOS game Aleshar - The World Of Ice
		result.madeWithTracker = UL_("Generic MOD-compatible Tracker");
		result.isGenericMultiChannel = true;
		result.numChannels = 8;
	} else
	{
		return false;
	}
	return true;
}


CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderMOD(MemoryFileReader file, const uint64 *pfilesize)
{
	if(!file.LengthIsAtLeast(1080 + 4))
	{
		return ProbeWantMoreData;
	}
	file.Seek(1080);
	char magic[4];
	file.ReadArray(magic);
	MODMagicResult modMagicResult;
	if(!CheckMODMagic(magic, modMagicResult))
	{
		return ProbeFailure;
	}

	file.Seek(20);
	uint32 invalidBytes = 0;
	for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
	{
		MODSampleHeader sampleHeader = ReadAndSwap<MODSampleHeader>(file, modMagicResult.swapBytes);
		invalidBytes += sampleHeader.GetInvalidByteScore();
	}
	if(invalidBytes > modMagicResult.invalidByteThreshold)
	{
		return ProbeFailure;
	}

	MPT_UNREFERENCED_PARAMETER(pfilesize);
	return ProbeSuccess;
}


bool CSoundFile::ReadMOD(FileReader &file, ModLoadingFlags loadFlags)
{
	char magic[4];
	if(!file.Seek(1080) || !file.ReadArray(magic))
	{
		return false;
	}

	MODMagicResult modMagicResult;
	if(!CheckMODMagic(magic, modMagicResult)
	   || modMagicResult.numChannels < 1
	   || modMagicResult.numChannels > MAX_BASECHANNELS)
	{
		return false;
	}

	if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	InitializeGlobals(MOD_TYPE_MOD);
	m_nChannels = modMagicResult.numChannels;

	bool isNoiseTracker = modMagicResult.isNoiseTracker;
	bool isStartrekker = modMagicResult.isStartrekker;
	bool isGenericMultiChannel = modMagicResult.isGenericMultiChannel;
	bool isInconexia = IsMagic(magic, "M\0\0\0") || IsMagic(magic, "8\0\0\0");
	// A loop length of zero will freeze ProTracker, so assume that modules having such a value were not meant to be played on Amiga. Fixes LHS_MI.MOD
	bool hasRepLen0 = false;
	// Empty sample slots typically should have a default volume of 0 in ProTracker
	bool hasEmptySampleWithVolume = false;
	if(modMagicResult.setMODVBlankTiming)
	{
		m_playBehaviour.set(kMODVBlankTiming);
	}

	// Startrekker 8 channel mod (needs special treatment, see below)
	const bool isFLT8 = isStartrekker && m_nChannels == 8;
	const bool isMdKd = IsMagic(magic, "M.K.");
	// Adjust finetune values for modules saved with "His Master's Noisetracker"
	const bool isHMNT = IsMagic(magic, "M&K!") || IsMagic(magic, "FEST");
	bool maybeWOW = isMdKd;

	// Reading song title
	file.Seek(0);
	const auto songTitle = ReadAndSwap<std::array<char, 20>>(file, modMagicResult.swapBytes);
	m_songName = mpt::String::ReadBuf(mpt::String::spacePadded, songTitle);

	// Load Sample Headers
	SmpLength totalSampleLen = 0, wowSampleLen = 0;
	m_nSamples = 31;
	uint32 invalidBytes = 0;
	bool hasLongSamples = false;
	for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
	{
		MODSampleHeader sampleHeader = ReadAndSwap<MODSampleHeader>(file, modMagicResult.swapBytes);
		invalidBytes += ReadSample(sampleHeader, Samples[smp], m_szNames[smp], m_nChannels == 4);
		totalSampleLen += Samples[smp].nLength;

		if(isHMNT)
			Samples[smp].nFineTune = -static_cast<int8>(sampleHeader.finetune << 3);
		else if(Samples[smp].nLength > 65535)
			hasLongSamples = true;
		
		if(sampleHeader.length && !sampleHeader.loopLength)
			hasRepLen0 = true;
		else if(!sampleHeader.length && sampleHeader.volume == 64)
			hasEmptySampleWithVolume = true;

		if(maybeWOW)
		{
			// Some WOW files rely on sample length 1 being counted as well
			wowSampleLen += sampleHeader.length * 2;
			// WOW files are converted 669 files, which don't support finetune or default volume
			if(sampleHeader.finetune)
				maybeWOW = false;
			else if(sampleHeader.length > 0 && sampleHeader.volume != 64)
				maybeWOW = false;
		}
	}
	// If there is too much binary garbage in the sample headers, reject the file.
	if(invalidBytes > modMagicResult.invalidByteThreshold)
	{
		return false;
	}

	// Read order information
	const MODFileHeader fileHeader = ReadAndSwap<MODFileHeader>(file, modMagicResult.swapBytes);

	file.Seek(modMagicResult.patternDataOffset);

	if(fileHeader.restartPos > 0)
		maybeWOW = false;
	if(!maybeWOW)
		wowSampleLen = 0;

	ReadOrderFromArray(Order(), fileHeader.orderList);

	ORDERINDEX realOrders = fileHeader.numOrders;
	if(realOrders > 128)
	{
		// beatwave.mod by Sidewinder claims to have 129 orders. (MD5: 8a029ac498d453beb929db9a73c3c6b4, SHA1: f7b76fb9f477b07a2e78eb10d8624f0df262cde7 - the version from ModArchive, not ModLand)
		realOrders = 128;
	} else if(realOrders == 0)
	{
		// Is this necessary?
		realOrders = 128;
		while(realOrders > 1 && Order()[realOrders - 1] == 0)
		{
			realOrders--;
		}
	}

	// Get number of patterns (including some order list sanity checks)
	PATTERNINDEX numPatterns = GetNumPatterns(file, Order(), realOrders, totalSampleLen, m_nChannels, wowSampleLen, false);
	if(maybeWOW && GetNumChannels() == 8)
	{
		// M.K. with 8 channels = Mod's Grave
		modMagicResult.madeWithTracker = UL_("Mod's Grave");
		isGenericMultiChannel = true;
	}

	if(isFLT8)
	{
		// FLT8 has only even order items, so divide by two.
		for(auto &pat : Order())
		{
			pat /= 2u;
		}
	}

	// Restart position sanity checks
	realOrders--;
	Order().SetRestartPos(fileHeader.restartPos);

	// (Ultimate) Soundtracker didn't have a restart position, but instead stored a default tempo in this value.
	// The default value for this is 0x78 (120 BPM). This is probably the reason why some M.K. modules
	// have this weird restart position. I think I've read somewhere that NoiseTracker actually writes 0x78 there.
	// M.K. files that have restart pos == 0x78: action's batman by DJ Uno, VALLEY.MOD, WormsTDC.MOD, ZWARTZ.MOD
	// Files that have an order list longer than 0x78 with restart pos = 0x78: my_shoe_is_barking.mod, papermix.mod
	// - in both cases it does not appear like the restart position should be used.
	MPT_ASSERT(fileHeader.restartPos != 0x78 || fileHeader.restartPos + 1u >= realOrders);
	if(fileHeader.restartPos > realOrders || (fileHeader.restartPos == 0x78 && m_nChannels == 4))
	{
		Order().SetRestartPos(0);
	}

	m_nDefaultSpeed = 6;
	m_nDefaultTempo.Set(125);
	m_nMinPeriod = 14 * 4;
	m_nMaxPeriod = 3424 * 4;
	// Prevent clipping based on number of channels... If all channels are playing at full volume, "256 / #channels"
	// is the maximum possible sample pre-amp without getting distortion (Compatible mix levels given).
	// The more channels we have, the less likely it is that all of them are used at the same time, though, so cap at 32...
	m_nSamplePreAmp = Clamp(256 / m_nChannels, 32, 128);
	m_SongFlags.reset();  // SONG_ISAMIGA will be set conditionally

	// Setup channel pan positions and volume
	SetupMODPanning();

	// Before loading patterns, apply some heuristics:
	// - Scan patterns to check if file could be a NoiseTracker file in disguise.
	//   In this case, the parameter of Dxx commands needs to be ignored (see 1.11song2.mod, 2-3song6.mod).
	// - Use the same code to find notes that would be out-of-range on Amiga.
	// - Detect 7-bit panning and whether 8xx / E8x commands should be interpreted as panning at all.
	bool onlyAmigaNotes = true;
	bool fix7BitPanning = false;
	uint8 maxPanning = 0;  // For detecting 8xx-as-sync
	const uint8 ENABLE_MOD_PANNING_THRESHOLD = 0x30;
	if(!isNoiseTracker)
	{
		const uint32 patternLength = m_nChannels * 64;
		bool leftPanning = false, extendedPanning = false;  // For detecting 800-880 panning
		isNoiseTracker = isMdKd && !hasEmptySampleWithVolume && !hasLongSamples;
		for(PATTERNINDEX pat = 0; pat < numPatterns; pat++)
		{
			uint16 patternBreaks = 0;
			for(uint32 i = 0; i < patternLength; i++)
			{
				ModCommand m;
				const auto data = ReadAndSwap<std::array<uint8, 4>>(file, modMagicResult.swapBytes && pat == 0);
				const auto [command, param] = ReadMODPatternEntry(data, m);
				if(!m.IsAmigaNote())
				{
					isNoiseTracker = onlyAmigaNotes = false;
				}
				if((command > 0x06 && command < 0x0A)
					|| (command == 0x0E && param > 0x01)
					|| (command == 0x0F && param > 0x1F)
					|| (command == 0x0D && ++patternBreaks > 1))
				{
					isNoiseTracker = false;
				}
				if(command == 0x08)
				{
					// Note: commands 880...88F are not considered for determining the panning style, as some modules use 7-bit panning but slightly overshoot:
					// LOOKATME.MOD (MD5: dedcec1a2a135aeb1a311841cea2c60c, SHA1: 42bf92bf824ef9fb904704b8ee7e3a30df60038d) has an 88A command as its rightmost panning.
					maxPanning = std::max(maxPanning, param);
					if(param < 0x80)
						leftPanning = true;
					else if(param > 0x8F && param != 0xA4)
						extendedPanning = true;
				} else if(command == 0x0E && (param & 0xF0) == 0x80)
				{
					maxPanning = std::max(maxPanning, static_cast<uint8>((param & 0x0F) << 4));
				}
			}
		}
		fix7BitPanning = leftPanning && !extendedPanning && maxPanning >= ENABLE_MOD_PANNING_THRESHOLD;
	}
	file.Seek(modMagicResult.patternDataOffset);

	const CHANNELINDEX readChannels = (isFLT8 ? 4 : m_nChannels);  // 4 channels per pattern in FLT8 format.
	if(isFLT8)
		numPatterns++;                                              // as one logical pattern consists of two real patterns in FLT8 format, the highest pattern number has to be increased by one.
	bool hasTempoCommands = false, definitelyCIA = hasLongSamples;  // for detecting VBlank MODs
	// Heuristic for rejecting E0x commands that are most likely not intended to actually toggle the Amiga LED filter, like in naen_leijasi_ptk.mod by ilmarque
	bool filterState = false;
	int filterTransitions = 0;

	// Reading patterns
	Patterns.ResizeArray(numPatterns);
	for(PATTERNINDEX pat = 0; pat < numPatterns; pat++)
	{
		ModCommand *rowBase = nullptr;

		if(isFLT8)
		{
			// FLT8: Only create "even" patterns and either write to channel 1 to 4 (even patterns) or 5 to 8 (odd patterns).
			PATTERNINDEX actualPattern = pat / 2u;
			if((pat % 2u) == 0 && !Patterns.Insert(actualPattern, 64))
			{
				break;
			}
			rowBase = Patterns[actualPattern].GetpModCommand(0, (pat % 2u) == 0 ? 0 : 4);
		} else
		{
			if(!Patterns.Insert(pat, 64))
			{
				break;
			}
			rowBase = Patterns[pat].GetpModCommand(0, 0);
		}

		if(rowBase == nullptr || !(loadFlags & loadPatternData))
		{
			break;
		}

		// For detecting PT1x mode
		std::vector<ModCommand::INSTR> lastInstrument(GetNumChannels(), 0);
		std::vector<uint8> instrWithoutNoteCount(GetNumChannels(), 0);

		for(ROWINDEX row = 0; row < 64; row++, rowBase += m_nChannels)
		{
			// If we have more than one Fxx command on this row and one can be interpreted as speed
			// and the other as tempo, we can be rather sure that it is not a VBlank mod.
			bool hasSpeedOnRow = false, hasTempoOnRow = false;

			for(CHANNELINDEX chn = 0; chn < readChannels; chn++)
			{
				ModCommand &m = rowBase[chn];
				const auto data = ReadAndSwap<std::array<uint8, 4>>(file, modMagicResult.swapBytes && pat == 0);
				auto [command, param] = ReadMODPatternEntry(data, m);

				if(command || param)
				{
					if(isStartrekker && command == 0x0E)
					{
						// No support for Startrekker assembly macros
						command = param = 0;
					} else if(isStartrekker && command == 0x0F && param > 0x1F)
					{
						// Startrekker caps speed at 31 ticks per row
						param = 0x1F;
					}
					ConvertModCommand(m, command, param);
				}

				// Perform some checks for our heuristics...
				if(m.command == CMD_TEMPO)
				{
					hasTempoOnRow = true;
					if(m.param < 100)
						hasTempoCommands = true;
				} else if(m.command == CMD_SPEED)
				{
					hasSpeedOnRow = true;
				} else if(m.command == CMD_PATTERNBREAK && isNoiseTracker)
				{
					m.param = 0;
				} else if(m.command == CMD_PANNING8 && fix7BitPanning)
				{
					// Fix MODs with 7-bit + surround panning
					if(m.param == 0xA4)
					{
						m.command = CMD_S3MCMDEX;
						m.param = 0x91;
					} else
					{
						m.param = mpt::saturate_cast<ModCommand::PARAM>(m.param * 2);
					}
				} else if(m.command == CMD_MODCMDEX && m.param < 0x10)
				{
					// Count LED filter transitions
					bool newState = !(m.param & 0x01);
					if(newState != filterState)
					{
						filterState = newState;
						filterTransitions++;
					}
				}
				if(m.note == NOTE_NONE && m.instr > 0 && !isFLT8)
				{
					if(lastInstrument[chn] > 0 && lastInstrument[chn] != m.instr)
					{
						// Arbitrary threshold for enabling sample swapping: 4 consecutive "sample swaps" in one pattern.
						if(++instrWithoutNoteCount[chn] >= 4)
						{
							m_playBehaviour.set(kMODSampleSwap);
						}
					}
				} else if(m.note != NOTE_NONE)
				{
					instrWithoutNoteCount[chn] = 0;
				}
				if(m.instr != 0)
				{
					lastInstrument[chn] = m.instr;
				}
			}
			if(hasSpeedOnRow && hasTempoOnRow)
				definitelyCIA = true;
		}
	}

	if(onlyAmigaNotes && !hasRepLen0 && (IsMagic(magic, "M.K.") || IsMagic(magic, "M!K!") || IsMagic(magic, "PATT")))
	{
		// M.K. files that don't exceed the Amiga note limit (fixes mod.mothergoose)
		m_SongFlags.set(SONG_AMIGALIMITS);
		// Need this for professionaltracker.mod by h0ffman (SHA1: 9a7c52cbad73ed2a198ee3fa18d3704ea9f546ff)
		m_SongFlags.set(SONG_PT_MODE);
		m_playBehaviour.set(kMODSampleSwap);
		m_playBehaviour.set(kMODOutOfRangeNoteDelay);
		m_playBehaviour.set(kMODTempoOnSecondTick);
		// Arbitrary threshold for deciding that 8xx effects are only used as sync markers
		if(maxPanning < ENABLE_MOD_PANNING_THRESHOLD)
		{
			m_playBehaviour.set(kMODIgnorePanning);
			if(fileHeader.restartPos != 0x7F)
			{
				// Don't enable these hacks for ScreamTracker modules (restart position = 0x7F), to fix e.g. sample 10 in BASIC001.MOD (SHA1: 11298a5620e677beaa50bd4ed00c3710b75c81af)
				// Note: restart position = 0x7F can also be found in ProTracker modules, e.g. professionaltracker.mod by h0ffman
				m_playBehaviour.set(kMODOneShotLoops);
			}
		}
	} else if(!onlyAmigaNotes && fileHeader.restartPos == 0x7F && isMdKd && fileHeader.restartPos + 1u >= realOrders)
	{
		modMagicResult.madeWithTracker = UL_("Scream Tracker");
	}

	if(onlyAmigaNotes && !isGenericMultiChannel && filterTransitions < 7)
	{
		m_SongFlags.set(SONG_ISAMIGA);
	}
	if(isGenericMultiChannel || isMdKd)
	{
		m_playBehaviour.set(kFT2MODTremoloRampWaveform);
	}
	if(isInconexia)
	{
		m_playBehaviour.set(kMODIgnorePanning);
	}

	// Reading samples
	bool anyADPCM = false;
	if(loadFlags & loadSampleData)
	{
		file.Seek(modMagicResult.patternDataOffset + (readChannels * 64 * 4) * numPatterns);
		for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
		{
			ModSample &sample = Samples[smp];
			if(sample.nLength)
			{
				SampleIO::Encoding encoding = SampleIO::signedPCM;
				if(isInconexia)
				{
					encoding = SampleIO::deltaPCM;
				} else if(file.ReadMagic("ADPCM"))
				{
					encoding = SampleIO::ADPCM;
					anyADPCM = true;
				}

				SampleIO sampleIO(
					SampleIO::_8bit,
					SampleIO::mono,
					SampleIO::littleEndian,
					encoding);

				// Fix sample 6 in MOD.shorttune2, which has a replen longer than the sample itself.
				// ProTracker reads beyond the end of the sample when playing. Normally samples are
				// adjacent in PT's memory, so we simply read into the next sample in the file.
				// On the other hand, the loop points in Purple Motions's SOUL-O-M.MOD are completely broken and shouldn't be treated like this.
				// As it was most likely written in Scream Tracker, it has empty sample slots with a default volume of 64, which we use for
				// rejecting this quirk for that file.
				FileReader::off_t nextSample = file.GetPosition() + sampleIO.CalculateEncodedSize(sample.nLength);
				if(isMdKd && onlyAmigaNotes && !hasEmptySampleWithVolume)
					sample.nLength = std::max(sample.nLength, sample.nLoopEnd);

				sampleIO.ReadSample(sample, file);
				file.Seek(nextSample);
			}
		}
		if(isMdKd && file.ReadArray<char, 9>() == std::array<char, 9>{0x00, 0x11, 0x55, 0x33, 0x22, 0x11, 0x04, 0x01, 0x01})
			modMagicResult.madeWithTracker = UL_("Tetramed");
	}

#if defined(MPT_EXTERNAL_SAMPLES) || defined(MPT_BUILD_FUZZER)
	// Detect Startrekker files with external synth instruments.
	// Note: Synthesized AM samples may overwrite existing samples (e.g. sample 1 in fa.worse face.mod),
	// hence they are loaded here after all regular samples have been loaded.
	if((loadFlags & loadSampleData) && isStartrekker)
	{
#ifdef MPT_EXTERNAL_SAMPLES
		std::optional<mpt::IO::InputFile> amFile;
		FileReader amData;
		if(file.GetOptionalFileName())
		{
			mpt::PathString filename = file.GetOptionalFileName().value();
			// Find instrument definition file
			const mpt::PathString exts[] = {P_(".nt"), P_(".NT"), P_(".as"), P_(".AS")};
			for(const auto &ext : exts)
			{
				mpt::PathString infoName = filename + ext;
				char stMagic[16];
				if(mpt::native_fs{}.is_file(infoName))
				{
					amFile.emplace(infoName, SettingCacheCompleteFileBeforeLoading());
					if(amFile->IsValid() && (amData = GetFileReader(*amFile)).IsValid() && amData.ReadArray(stMagic))
					{
						if(!memcmp(stMagic, "ST1.2 ModuleINFO", 16))
							modMagicResult.madeWithTracker = UL_("Startrekker 1.2");
						else if(!memcmp(stMagic, "ST1.3 ModuleINFO", 16))
							modMagicResult.madeWithTracker = UL_("Startrekker 1.3");
						else if(!memcmp(stMagic, "AudioSculpture10", 16))
							modMagicResult.madeWithTracker = UL_("AudioSculpture 1.0");
						else
							continue;

						if(amData.Seek(144))
						{
							// Looks like a valid instrument definition file!
							m_nInstruments = 31;
							break;
						}
					}
				}
			}
		}
#elif defined(MPT_BUILD_FUZZER)
		// For fuzzing this part of the code, just take random data from patterns
		FileReader amData = file.GetChunkAt(1084, 31 * 120);
		m_nInstruments = 31;
#endif

		for(SAMPLEINDEX smp = 1; smp <= m_nInstruments; smp++)
		{
			// For Startrekker AM synthesis, we need instrument envelopes.
			ModInstrument *ins = AllocateInstrument(smp, smp);
			if(ins == nullptr)
			{
				break;
			}
			ins->name = m_szNames[smp];

			AMInstrument am;
			// Allow partial reads for fa.worse face.mod
			if(amData.ReadStructPartial(am) && !memcmp(am.am, "AM", 2) && am.waveform < 4)
			{
				am.ConvertToMPT(Samples[smp], *ins, AccessPRNG());
			}

			// This extra padding is probably present to have identical block sizes for AM and FM instruments.
			amData.Skip(120 - sizeof(AMInstrument));
		}
	}
#endif  // MPT_EXTERNAL_SAMPLES || MPT_BUILD_FUZZER

	// Fix VBlank MODs. Arbitrary threshold: 8 minutes (enough for "frame of mind" by Dascon...).
	// Basically, this just converts all tempo commands into speed commands
	// for MODs which are supposed to have VBlank timing (instead of CIA timing).
	// There is no perfect way to do this, since both MOD types look the same,
	// but the most reliable way is to simply check for extremely long songs
	// (as this would indicate that e.g. a F30 command was really meant to set
	// the ticks per row to 48, and not the tempo to 48 BPM).
	// In the pattern loader above, a second condition is used: Only tempo commands
	// below 100 BPM are taken into account. Furthermore, only M.K. (ProTracker)
	// modules are checked.
	if((isMdKd || IsMagic(magic, "M!K!")) && hasTempoCommands && !definitelyCIA)
	{
		const double songTime = GetLength(eNoAdjust).front().duration;
		if(songTime >= 480.0)
		{
			m_playBehaviour.set(kMODVBlankTiming);
			if(GetLength(eNoAdjust, GetLengthTarget(songTime)).front().targetReached)
			{
				// This just makes things worse, song is at least as long as in CIA mode
				// Obviously we should keep using CIA timing then...
				m_playBehaviour.reset(kMODVBlankTiming);
			} else
			{
				modMagicResult.madeWithTracker = UL_("ProTracker (VBlank)");
			}
		}
	}

	std::transform(std::begin(magic), std::end(magic), std::begin(magic), [](unsigned char c) -> unsigned char { return (c < ' ') ? ' ' : c; });
	m_modFormat.formatName = MPT_UFORMAT("ProTracker MOD ({})")(mpt::ToUnicode(mpt::Charset::ASCII, std::string(std::begin(magic), std::end(magic))));
	m_modFormat.type = U_("mod");
	if(modMagicResult.madeWithTracker)
		m_modFormat.madeWithTracker = modMagicResult.madeWithTracker;
	m_modFormat.charset = mpt::Charset::Amiga_no_C1;

	if(anyADPCM)
		m_modFormat.madeWithTracker += U_(" (ADPCM packed)");

	return true;
}


// Check if a name string is valid (i.e. doesn't contain binary garbage data)
static uint32 CountInvalidChars(const mpt::span<const char> name) noexcept
{
	uint32 invalidChars = 0;
	for(int8 c : name)  // char can be signed or unsigned
	{
		// Check for any Extended ASCII and control characters
		if(c != 0 && c < ' ')
			invalidChars++;
	}
	return invalidChars;
}


enum class NameClassification
{
	Empty,
	ValidASCII,
	Invalid,
};

// Check if a name is a valid null-terminated ASCII string with no garbage after the null terminator, or if it's empty
static NameClassification ClassifyName(const mpt::span<const char> name) noexcept
{
	bool foundNull = false, foundNormal = false;
	for(auto c : name)
	{
		if(c > 0 && c < ' ')
			return NameClassification::Invalid;
		if(c == 0)
			foundNull = true;
		else if(foundNull)
			return NameClassification::Invalid;
		else
			foundNormal = true;
	}
	if(!foundNull)
		return NameClassification::Invalid;
	return foundNormal ? NameClassification::ValidASCII : NameClassification::Empty;
}


// We'll have to do some heuristic checks to find out whether this is an old Ultimate Soundtracker module
// or if it was made with the newer Soundtracker versions.
// Thanks for Fraggie for this information! (https://www.un4seen.com/forum/?topic=14471.msg100829#msg100829)
enum STVersions
{
	UST1_00,              // Ultimate Soundtracker 1.0-1.21 (K. Obarski)
	UST1_80,              // Ultimate Soundtracker 1.8-2.0 (K. Obarski)
	ST2_00_Exterminator,  // SoundTracker 2.0 (The Exterminator), D.O.C. Sountracker II (Unknown/D.O.C.)
	ST_III,               // Defjam Soundtracker III (Il Scuro/Defjam), Alpha Flight SoundTracker IV (Alpha Flight), D.O.C. SoundTracker IV (Unknown/D.O.C.), D.O.C. SoundTracker VI (Unknown/D.O.C.)
	ST_IX,                // D.O.C. SoundTracker IX (Unknown/D.O.C.)
	MST1_00,              // Master Soundtracker 1.0 (Tip/The New Masters)
	ST2_00,               // SoundTracker 2.0, 2.1, 2.2 (Unknown/D.O.C.)
};



struct M15FileHeaders
{
	char            songname[20];
	MODSampleHeader sampleHeaders[15];
	MODFileHeader   fileHeader;
};

MPT_BINARY_STRUCT(M15FileHeaders, 20 + 15 * 30 + 130)


static bool ValidateHeader(const M15FileHeaders &fileHeaders)
{
	// In theory, sample and song names should only ever contain printable ASCII chars and null.
	// However, there are quite a few SoundTracker modules in the wild with random
	// characters. To still be able to distguish them from other formats, we just reject
	// files with *too* many bogus characters. Arbitrary threshold: 48 bogus characters in total
	// or more than 5 invalid characters just in the title alone
	uint32 invalidCharsInTitle = CountInvalidChars(fileHeaders.songname);
	uint32 invalidChars = invalidCharsInTitle;

	SmpLength totalSampleLen = 0;
	uint8 allVolumes = 0;
	uint8 validNameCount = 0;
	bool invalidNames = false;

	for(SAMPLEINDEX smp = 0; smp < 15; smp++)
	{
		const MODSampleHeader &sampleHeader = fileHeaders.sampleHeaders[smp];

		invalidChars += CountInvalidChars(sampleHeader.name);

		// schmokk.mod has a non-zero value here but it should not be treated as finetune
		if(sampleHeader.finetune != 0)
			invalidChars += 16;
		if(const auto nameType = ClassifyName(sampleHeader.name); nameType == NameClassification::ValidASCII)
			validNameCount++;
		else if(nameType == NameClassification::Invalid)
			invalidNames = true;

		// Sanity checks - invalid character count adjusted for ata.mod (MD5 937b79b54026fa73a1a4d3597c26eace, SHA1 3322ca62258adb9e0ae8e9afe6e0c29d39add874)
		// Sample length adjusted for romantic.stk which has a (valid) sample of length 72222
		if(invalidChars > 48
		   || sampleHeader.volume > 64
		   || sampleHeader.length > 37000)
		{
			return false;
		}

		totalSampleLen += sampleHeader.length;
		allVolumes |= sampleHeader.volume;
	}

	// scramble_2.mod has a lot of garbage in the song title, but it has lots of properly-formatted sample names, so we consider those to be more important than the garbage bytes.
	if(invalidCharsInTitle > 5 && (validNameCount < 4 || invalidNames))
	{
		return false;
	}

	// Reject any files with no (or only silent) samples at all, as this might just be a random binary file (e.g. ID3 tags with tons of padding)
	if(totalSampleLen == 0 || allVolumes == 0)
	{
		return false;
	}

	// Sanity check: No more than 128 positions. ST's GUI limits tempo to [1, 220].
	// There are some mods with a tempo of 0 (explora3-death.mod) though, so ignore the lower limit.
	if(fileHeaders.fileHeader.numOrders > 128 || fileHeaders.fileHeader.restartPos > 220)
	{
		return false;
	}

	uint8 maxPattern = *std::max_element(std::begin(fileHeaders.fileHeader.orderList), std::end(fileHeaders.fileHeader.orderList));
	// Sanity check: 64 patterns max.
	if(maxPattern > 63)
	{
		return false;
	}

	// No playable song, and lots of null values => most likely a sparse binary file but not a module
	if(fileHeaders.fileHeader.restartPos == 0 && fileHeaders.fileHeader.numOrders == 0 && maxPattern == 0)
	{
		return false;
	}

	return true;
}


template <typename TFileReader>
static bool ValidateFirstM15Pattern(TFileReader &file)
{
	// threshold is chosen as: [threshold for all patterns combined] / [max patterns] * [margin, do not reject too much]
	return ValidateMODPatternData(file, 512 / 64 * 2, false);
}


CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderM15(MemoryFileReader file, const uint64 *pfilesize)
{
	M15FileHeaders fileHeaders;
	if(!file.ReadStruct(fileHeaders))
	{
		return ProbeWantMoreData;
	}
	if(!ValidateHeader(fileHeaders))
	{
		return ProbeFailure;
	}
	if(!file.CanRead(sizeof(MODPatternData)))
	{
		return ProbeWantMoreData;
	}
	if(!ValidateFirstM15Pattern(file))
	{
		return ProbeFailure;
	}
	MPT_UNREFERENCED_PARAMETER(pfilesize);
	return ProbeSuccess;
}


bool CSoundFile::ReadM15(FileReader &file, ModLoadingFlags loadFlags)
{
	file.Rewind();

	M15FileHeaders fileHeaders;
	if(!file.ReadStruct(fileHeaders))
	{
		return false;
	}
	if(!ValidateHeader(fileHeaders))
	{
		return false;
	}
	if(!ValidateFirstM15Pattern(file))
	{
		return false;
	}

	char songname[20];
	std::memcpy(songname, fileHeaders.songname, 20);

	InitializeGlobals(MOD_TYPE_MOD);
	m_playBehaviour.reset(kMODOneShotLoops);
	m_playBehaviour.set(kMODIgnorePanning);
	m_playBehaviour.set(kMODSampleSwap);  // untested
	m_nChannels = 4;

	STVersions minVersion = UST1_00;

	bool hasDiskNames = true;
	SmpLength totalSampleLen = 0;
	m_nSamples = 15;

	file.Seek(20);
	for(SAMPLEINDEX smp = 1; smp <= 15; smp++)
	{
		ModSample &mptSmp = Samples[smp];
		MODSampleHeader sampleHeader;
		file.ReadStruct(sampleHeader);
		ReadSample(sampleHeader, Samples[smp], m_szNames[smp], true);
		mptSmp.nFineTune = 0;

		totalSampleLen += mptSmp.nLength;

		if(m_szNames[smp][0] && sampleHeader.HasDiskName())
		{
			// Ultimate Soundtracker 1.8 and D.O.C. SoundTracker IX always have sample names containing disk names.
			hasDiskNames = false;
		}

		// Loop start is always in bytes, not words, so don't trust the auto-fix magic in the sample header conversion (fixes loop of "st-01:asia" in mod.drag 10)
		if(sampleHeader.loopLength > 1)
		{
			mptSmp.nLoopStart = sampleHeader.loopStart;
			mptSmp.nLoopEnd = sampleHeader.loopStart + sampleHeader.loopLength * 2;
			mptSmp.SanitizeLoops();
		}

		// UST only handles samples up to 9999 bytes. Master Soundtracker 1.0 and SoundTracker 2.0 introduce 32KB samples.
		if(sampleHeader.length > 4999 || sampleHeader.loopStart > 9999)
			minVersion = std::max(minVersion, MST1_00);
	}

	MODFileHeader fileHeader;
	file.ReadStruct(fileHeader);

	ReadOrderFromArray(Order(), fileHeader.orderList);
	PATTERNINDEX numPatterns = GetNumPatterns(file, Order(), fileHeader.numOrders, totalSampleLen, m_nChannels, 0, true);

	// Most likely just a file with lots of NULs at the start
	if(fileHeader.restartPos == 0 && fileHeader.numOrders == 0 && numPatterns <= 1)
	{
		return false;
	}

	// Let's see if the file is too small (including some overhead for broken files like sll7.mod or ghostbus.mod)
	std::size_t requiredRemainingDataSize = numPatterns * 64u * 4u * 4u + totalSampleLen;
	if(!file.CanRead(requiredRemainingDataSize - std::min<std::size_t>(requiredRemainingDataSize, 65536u)))
		return false;

	if(loadFlags == onlyVerifyHeader)
		return true;

	// Now we can be pretty sure that this is a valid Soundtracker file. Set up default song settings.
	// explora3-death.mod has a tempo of 0
	if(!fileHeader.restartPos)
		fileHeader.restartPos = 0x78;
	// jjk55 by Jesper Kyd has a weird tempo set, but it needs to be ignored.
	if(!memcmp(songname, "jjk55", 6))
		fileHeader.restartPos = 0x78;
	// Sample 7 in echoing.mod won't "loop" correctly if we don't convert the VBlank tempo.
	m_nDefaultTempo.Set(125);
	if(fileHeader.restartPos != 0x78)
	{
		// Convert to CIA timing
		m_nDefaultTempo = TEMPO((709379.0 * 125.0 / 50.0) / ((240 - fileHeader.restartPos) * 122.0));
		if(minVersion > UST1_80)
		{
			// D.O.C. SoundTracker IX re-introduced the variable tempo after some other versions dropped it.
			minVersion = std::max(minVersion, hasDiskNames ? ST_IX : MST1_00);
		} else
		{
			// Ultimate Soundtracker 1.8 adds variable tempo
			minVersion = std::max(minVersion, hasDiskNames ? UST1_80 : ST2_00_Exterminator);
		}
	}
	m_nMinPeriod = 113 * 4;
	m_nMaxPeriod = 856 * 4;
	m_nSamplePreAmp = 64;
	m_SongFlags.set(SONG_PT_MODE);
	m_songName = mpt::String::ReadBuf(mpt::String::spacePadded, songname);

	// Setup channel pan positions and volume
	SetupMODPanning();

	FileReader::off_t patOffset = file.GetPosition();

	// Scan patterns to identify Ultimate Soundtracker modules.
	uint32 illegalBytes = 0, totalNumDxx = 0;
	for(PATTERNINDEX pat = 0; pat < numPatterns; pat++)
	{
		const bool patternInUse = mpt::contains(Order(), pat);
		uint8 numDxx = 0;
		uint8 emptyCmds = 0;
		MODPatternData patternData;
		file.ReadArray(patternData);
		if(patternInUse)
		{
			illegalBytes += CountMalformedMODPatternData(patternData, false);
			// Reject files that contain a lot of illegal pattern data.
			// STK.the final remix (MD5 5ff13cdbd77211d1103be7051a7d89c9, SHA1 e94dba82a5da00a4758ba0c207eb17e3a89c3aa3)
			// has one illegal byte, so we only reject after an arbitrary threshold has been passed.
			// This also allows to play some rather damaged files like
			// crockets.mod (MD5 995ed9f44cab995a0eeb19deb52e2a8b, SHA1 6c79983c3b7d55c9bc110b625eaa07ce9d75f369)
			// but naturally we cannot recover the broken data.

			// We only check patterns that are actually being used in the order list, because some bad rips of the
			// "operation wolf" soundtrack have 15 patterns for several songs, but the last few patterns are just garbage.
			// Apart from those hidden patterns, the files play fine.
			// Example: operation wolf - wolf1.mod (MD5 739acdbdacd247fbefcac7bc2d8abe6b, SHA1 e6b4813daacbf95f41ce9ec3b22520a2ae07eed8)
			if(illegalBytes > std::max(512u, numPatterns * 128u))
				return false;
		}
		for(ROWINDEX row = 0; row < 64; row++)
		{
			for(CHANNELINDEX chn = 0; chn < 4; chn++)
			{
				const auto &data = patternData[row][chn];
				const uint8 eff = data[2] & 0x0F, param = data[3];
				// Check for empty space between the last Dxx command and the beginning of another pattern
				if(emptyCmds != 0 && !memcmp(data.data(), "\0\0\0\0", 4))
				{
					emptyCmds++;
					if(emptyCmds > 32)
					{
						// Since there is a lot of empty space after the last Dxx command,
						// we assume it's supposed to be a pattern break effect.
						minVersion = ST2_00;
					}
				} else
				{
					emptyCmds = 0;
				}

				switch(eff)
				{
				case 1:
				case 2:
					if(param > 0x1F && minVersion == UST1_80)
					{
						// If a 1xx / 2xx effect has a parameter greater than 0x20, it is assumed to be UST.
						minVersion = hasDiskNames ? UST1_80 : UST1_00;
					} else if(eff == 1 && param > 0 && param < 0x03)
					{
						// This doesn't look like an arpeggio.
						minVersion = std::max(minVersion, ST2_00_Exterminator);
					} else if(eff == 1 && (param == 0x37 || param == 0x47) && minVersion <= ST2_00_Exterminator)
					{
						// This suspiciously looks like an arpeggio.
						// Catch sleepwalk.mod by Karsten Obarski, which has a default tempo of 125 rather than 120 in the header, so gets mis-identified as a later tracker version.
						minVersion = hasDiskNames ? UST1_80 : UST1_00;
					}
					break;
				case 0x0B:
					minVersion = ST2_00;
					break;
				case 0x0C:
				case 0x0D:
				case 0x0E:
					minVersion = std::max(minVersion, ST2_00_Exterminator);
					if(eff == 0x0D)
					{
						emptyCmds = 1;
						if(param == 0 && row == 0)
						{
							// Fix a possible tracking mistake in Blood Money title - who wants to do a pattern break on the first row anyway?
							break;
						}
						numDxx++;
					}
					break;
				case 0x0F:
					minVersion = std::max(minVersion, ST_III);
					break;
				}
			}
		}

		if(numDxx > 0 && numDxx < 3)
		{
			// Not many Dxx commands in one pattern means they were probably pattern breaks
			minVersion = ST2_00;
		}
		totalNumDxx += numDxx;
	}

	// If there is a huge number of Dxx commands, this is extremely unlikely to be a  SoundTracker 2.0 module
	if(totalNumDxx > numPatterns + 32u && minVersion == ST2_00)
		minVersion = MST1_00;

	file.Seek(patOffset);

	// Reading patterns
	if(loadFlags & loadPatternData)
		Patterns.ResizeArray(numPatterns);
	for(PATTERNINDEX pat = 0; pat < numPatterns; pat++)
	{
		MODPatternData patternData;
		file.ReadArray(patternData);

		if(!(loadFlags & loadPatternData) || !Patterns.Insert(pat, 64))
		{
			continue;
		}

		uint8 autoSlide[4] = {0, 0, 0, 0};
		for(ROWINDEX row = 0; row < 64; row++)
		{
			auto rowBase = Patterns[pat].GetRow(row);
			for(CHANNELINDEX chn = 0; chn < 4; chn++)
			{
				ModCommand &m = rowBase[chn];
				auto [command, param] = ReadMODPatternEntry(patternData[row][chn], m);

				if(!param || command == 0x0E)
				{
					autoSlide[chn] = 0;
				}
				if(command || param)
				{
					if(autoSlide[chn] != 0)
					{
						if(autoSlide[chn] & 0xF0)
						{
							m.volcmd = VOLCMD_VOLSLIDEUP;
							m.vol = autoSlide[chn] >> 4;
						} else
						{
							m.volcmd = VOLCMD_VOLSLIDEDOWN;
							m.vol = autoSlide[chn] & 0x0F;
						}
					}
					if(command == 0x0D)
					{
						if(minVersion != ST2_00)
						{
							// Dxy is volume slide in some Soundtracker versions, D00 is a pattern break in the latest versions.
							command = 0x0A;
						} else
						{
							param = 0;
						}
					} else if(command == 0x0C)
					{
						// Volume is sent as-is to the chip, which ignores the highest bit.
						param &= 0x7F;
					} else if(command == 0x0E && (param > 0x01 || minVersion < ST_IX))
					{
						// Import auto-slides as normal slides and fake them using volume column slides.
						command = 0x0A;
						autoSlide[chn] = param;
					} else if(command == 0x0F)
					{
						// Only the low nibble is evaluated in Soundtracker.
						param &= 0x0F;
					}

					if(minVersion <= UST1_80)
					{
						// UST effects
						m.param = param;
						switch(command)
						{
						case 0:
							// jackdance.mod by Karsten Obarski has 0xy arpeggios...
							if(param < 0x03)
							{
								m.command = CMD_NONE;
							} else
							{
								m.command = CMD_ARPEGGIO;
							}
							break;
						case 1:
							m.command = CMD_ARPEGGIO;
							break;
						case 2:
							if(m.param & 0x0F)
							{
								m.command = CMD_PORTAMENTOUP;
								m.param &= 0x0F;
							} else if(m.param >> 4)
							{
								m.command = CMD_PORTAMENTODOWN;
								m.param >>= 4;
							}
							break;
						default:
							m.command = CMD_NONE;
							break;
						}
					} else
					{
						ConvertModCommand(m, command, param);
					}
				} else
				{
					autoSlide[chn] = 0;
				}
			}
		}
	}

	[[maybe_unused]] /* silence clang-tidy deadcode.DeadStores */ const mpt::uchar *madeWithTracker = UL_("");
	switch(minVersion)
	{
	case UST1_00:
		madeWithTracker = UL_("Ultimate Soundtracker 1.0-1.21");
		break;
	case UST1_80:
		madeWithTracker = UL_("Ultimate Soundtracker 1.8-2.0");
		break;
	case ST2_00_Exterminator:
		madeWithTracker = UL_("SoundTracker 2.0 / D.O.C. SoundTracker II");
		break;
	case ST_III:
		madeWithTracker = UL_("Defjam Soundtracker III / Alpha Flight SoundTracker IV / D.O.C. SoundTracker IV / VI");
		break;
	case ST_IX:
		madeWithTracker = UL_("D.O.C. SoundTracker IX");
		break;
	case MST1_00:
		madeWithTracker = UL_("Master Soundtracker 1.0");
		break;
	case ST2_00:
		madeWithTracker = UL_("SoundTracker 2.0 / 2.1 / 2.2");
		break;
	}

	m_modFormat.formatName = U_("Soundtracker");
	m_modFormat.type = U_("stk");
	m_modFormat.madeWithTracker = madeWithTracker;
	m_modFormat.charset = mpt::Charset::Amiga_no_C1;

	// Reading samples
	if(loadFlags & loadSampleData)
	{
		for(SAMPLEINDEX smp = 1; smp <= 15; smp++)
		{
			// Looped samples in (Ultimate) Soundtracker seem to ignore all sample data before the actual loop start.
			// This avoids the clicks in the first sample of pretend.mod by Karsten Obarski.
			file.Skip(Samples[smp].nLoopStart);
			Samples[smp].nLength -= Samples[smp].nLoopStart;
			Samples[smp].nLoopEnd -= Samples[smp].nLoopStart;
			Samples[smp].nLoopStart = 0;
			MODSampleHeader::GetSampleFormat().ReadSample(Samples[smp], file);
		}
	}

	return true;
}


CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderICE(MemoryFileReader file, const uint64 *pfilesize)
{
	if(!file.CanRead(1464 + 4))
	{
		return ProbeWantMoreData;
	}
	file.Seek(1464);
	char magic[4];
	file.ReadArray(magic);
	if(!IsMagic(magic, "MTN\0") && !IsMagic(magic, "IT10"))
	{
		return ProbeFailure;
	}
	file.Seek(20);
	uint32 invalidBytes = 0;
	for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
	{
		MODSampleHeader sampleHeader;
		if(!file.ReadStruct(sampleHeader))
		{
			return ProbeWantMoreData;
		}
		invalidBytes += sampleHeader.GetInvalidByteScore();
	}
	if(invalidBytes > MODSampleHeader::INVALID_BYTE_THRESHOLD)
	{
		return ProbeFailure;
	}
	const auto [numOrders, numTracks] = file.ReadArray<uint8, 2>();
	if(numOrders > 128)
	{
		return ProbeFailure;
	}
	uint8 tracks[128 * 4];
	file.ReadArray(tracks);
	for(auto track : tracks)
	{
		if(track > numTracks)
		{
			return ProbeFailure;
		}
	}
	MPT_UNREFERENCED_PARAMETER(pfilesize);
	return ProbeSuccess;
}


// SoundTracker 2.6 / Ice Tracker variation of the MOD format
// The only real difference to other SoundTracker formats is the way patterns are stored:
// Every pattern consists of four independent, re-usable tracks.
bool CSoundFile::ReadICE(FileReader &file, ModLoadingFlags loadFlags)
{
	char magic[4];
	if(!file.Seek(1464) || !file.ReadArray(magic))
	{
		return false;
	}

	InitializeGlobals(MOD_TYPE_MOD);
	m_playBehaviour.reset(kMODOneShotLoops);
	m_playBehaviour.set(kMODIgnorePanning);
	m_playBehaviour.set(kMODSampleSwap);  // untested

	if(IsMagic(magic, "MTN\0"))
	{
		m_modFormat.formatName = U_("MnemoTroN SoundTracker");
		m_modFormat.type = U_("st26");
		m_modFormat.madeWithTracker = U_("SoundTracker 2.6");
		m_modFormat.charset = mpt::Charset::Amiga_no_C1;
	} else if(IsMagic(magic, "IT10"))
	{
		m_modFormat.formatName = U_("Ice Tracker");
		m_modFormat.type = U_("ice");
		m_modFormat.madeWithTracker = U_("Ice Tracker 1.0 / 1.1");
		m_modFormat.charset = mpt::Charset::Amiga_no_C1;
	} else
	{
		return false;
	}

	// Reading song title
	file.Seek(0);
	file.ReadString<mpt::String::spacePadded>(m_songName, 20);

	// Load Samples
	m_nSamples = 31;
	uint32 invalidBytes = 0;
	for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
	{
		MODSampleHeader sampleHeader;
		file.ReadStruct(sampleHeader);
		invalidBytes += ReadSample(sampleHeader, Samples[smp], m_szNames[smp], true);
	}
	if(invalidBytes > MODSampleHeader::INVALID_BYTE_THRESHOLD)
	{
		return false;
	}

	const auto [numOrders, numTracks] = file.ReadArray<uint8, 2>();
	if(numOrders > 128)
	{
		return false;
	}

	uint8 tracks[128 * 4];
	file.ReadArray(tracks);
	for(auto track : tracks)
	{
		if(track > numTracks)
		{
			return false;
		}
	}

	if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	// Now we can be pretty sure that this is a valid MOD file. Set up default song settings.
	m_nChannels = 4;
	m_nInstruments = 0;
	m_nDefaultSpeed = 6;
	m_nDefaultTempo.Set(125);
	m_nMinPeriod = 14 * 4;
	m_nMaxPeriod = 3424 * 4;
	m_nSamplePreAmp = 64;
	m_SongFlags.set(SONG_PT_MODE | SONG_IMPORTED);

	// Setup channel pan positions and volume
	SetupMODPanning();

	// Reading patterns
	Order().resize(numOrders);
	uint8 speed[2] = {0, 0}, speedPos = 0;
	Patterns.ResizeArray(numOrders);
	for(PATTERNINDEX pat = 0; pat < numOrders; pat++)
	{
		Order()[pat] = pat;
		if(!Patterns.Insert(pat, 64))
			continue;

		for(CHANNELINDEX chn = 0; chn < 4; chn++)
		{
			file.Seek(1468 + tracks[pat * 4 + chn] * 64u * 4u);
			ModCommand *m = Patterns[pat].GetpModCommand(0, chn);

			for(ROWINDEX row = 0; row < 64; row++, m += 4)
			{
				const auto [command, param] = ReadMODPatternEntry(file, *m);

				if((command || param)
				   && !(command == 0x0E && param >= 0x10)     // Exx only sets filter
				   && !(command >= 0x05 && command <= 0x09))  // These don't exist in ST2.6
				{
					ConvertModCommand(*m, command, param);
				} else
				{
					m->command = CMD_NONE;
				}
			}
		}

		// Handle speed command with both nibbles set - this enables auto-swing (alternates between the two nibbles)
		auto m = Patterns[pat].begin();
		for(ROWINDEX row = 0; row < 64; row++)
		{
			for(CHANNELINDEX chn = 0; chn < 4; chn++, m++)
			{
				if(m->command == CMD_SPEED || m->command == CMD_TEMPO)
				{
					m->command = CMD_SPEED;
					speedPos = 0;
					if(m->param & 0xF0)
					{
						if((m->param >> 4) != (m->param & 0x0F) && (m->param & 0x0F) != 0)
						{
							// Both nibbles set
							speed[0] = m->param >> 4;
							speed[1] = m->param & 0x0F;
							speedPos = 1;
						}
						m->param >>= 4;
					}
				}
			}
			if(speedPos)
			{
				Patterns[pat].WriteEffect(EffectWriter(CMD_SPEED, speed[speedPos - 1]).Row(row));
				speedPos++;
				if(speedPos == 3)
					speedPos = 1;
			}
		}
	}

	// Reading samples
	if(loadFlags & loadSampleData)
	{
		file.Seek(1468 + numTracks * 64u * 4u);
		for(SAMPLEINDEX smp = 1; smp <= 31; smp++) if(Samples[smp].nLength)
		{
			SampleIO(
				SampleIO::_8bit,
				SampleIO::mono,
				SampleIO::littleEndian,
				SampleIO::signedPCM)
				.ReadSample(Samples[smp], file);
		}
	}

	return true;
}



struct PT36Header
{
	char     magicFORM[4]; // "FORM"
	uint32be size;
	char     magicMODL[4]; // "MODL"
};

MPT_BINARY_STRUCT(PT36Header, 12)


static bool ValidateHeader(const PT36Header &fileHeader)
{
	if(std::memcmp(fileHeader.magicFORM, "FORM", 4))
	{
		return false;
	}
	if(std::memcmp(fileHeader.magicMODL, "MODL", 4))
	{
		return false;
	}
	return true;
}


CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderPT36(MemoryFileReader file, const uint64 *pfilesize)
{
	PT36Header fileHeader;
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


// ProTracker 3.6 version of the MOD format
// Basically just a normal ProTracker mod with different magic, wrapped in an IFF file.
// The "PTDT" chunk is passed to the normal MOD loader.
bool CSoundFile::ReadPT36(FileReader &file, ModLoadingFlags loadFlags)
{
	file.Rewind();

	PT36Header fileHeader;
	if(!file.ReadStruct(fileHeader))
	{
		return false;
	}
	if(!ValidateHeader(fileHeader))
	{
		return false;
	}

	bool ok = false, infoOk = false;
	FileReader commentChunk;
	mpt::ustring version;
	PT36InfoChunk info;
	MemsetZero(info);

	// Go through IFF chunks...
	PT36IffChunk iffHead;
	if(!file.ReadStruct(iffHead))
	{
		return false;
	}
	// First chunk includes "MODL" magic in size
	iffHead.chunksize -= 4;

	do
	{
		// All chunk sizes include chunk header
		iffHead.chunksize -= 8;
		if(loadFlags == onlyVerifyHeader && iffHead.signature == PT36IffChunk::idPTDT)
		{
			return true;
		}

		FileReader chunk = file.ReadChunk(iffHead.chunksize);
		if(!chunk.IsValid())
		{
			break;
		}

		switch(iffHead.signature)
		{
		case PT36IffChunk::idVERS:
			chunk.Skip(4);
			if(chunk.ReadMagic("PT") && iffHead.chunksize > 6)
			{
				chunk.ReadString<mpt::String::maybeNullTerminated>(version, mpt::Charset::Amiga_no_C1, iffHead.chunksize - 6);
			}
			break;

		case PT36IffChunk::idINFO:
			infoOk = chunk.ReadStruct(info);
			break;

		case PT36IffChunk::idCMNT:
			commentChunk = chunk;
			break;

		case PT36IffChunk::idPTDT:
			ok = ReadMOD(chunk, loadFlags);
			break;
		}
	} while(file.ReadStruct(iffHead));

	if(version.empty())
	{
		version = U_("3.6");
	}

	// both an info chunk and a module are required
	if(ok && infoOk)
	{
		bool vblank = (info.flags & 0x100) == 0;
		m_playBehaviour.set(kMODVBlankTiming, vblank);
		if(info.volume != 0)
			m_nSamplePreAmp = std::min(uint16(64), static_cast<uint16>(info.volume));
		if(info.tempo != 0 && !vblank)
			m_nDefaultTempo.Set(info.tempo);

		if(info.name[0])
			m_songName = mpt::String::ReadBuf(mpt::String::maybeNullTerminated, info.name);

		if(mpt::is_in_range(info.dateMonth, 1, 12) && mpt::is_in_range(info.dateDay, 1, 31) && mpt::is_in_range(info.dateHour, 0, 23)
		   && mpt::is_in_range(info.dateMinute, 0, 59) && mpt::is_in_range(info.dateSecond, 0, 59))
		{
#ifdef MODPLUG_TRACKER
			m_modFormat.timezone = mpt::Date::LogicalTimezone::Local;
#else
			m_modFormat.timezone = mpt::Date::LogicalTimezone::Unspecified;
#endif
			FileHistory mptHistory;
			mptHistory.loadDate.year = info.dateYear + 1900;
			mptHistory.loadDate.month = info.dateMonth;
			mptHistory.loadDate.day = info.dateDay;
			mptHistory.loadDate.hours = info.dateHour;
			mptHistory.loadDate.minutes = info.dateMinute;
			mptHistory.loadDate.seconds = info.dateSecond;
			m_FileHistory.push_back(mptHistory);
		}
	}
	if(ok)
	{
		if(commentChunk.IsValid())
		{
			std::string author;
			commentChunk.ReadString<mpt::String::maybeNullTerminated>(author, 32);
			if(author != "UNNAMED AUTHOR")
				m_songArtist = mpt::ToUnicode(mpt::Charset::Amiga_no_C1, author);
			if(!commentChunk.NoBytesLeft())
			{
				m_songMessage.ReadFixedLineLength(commentChunk, commentChunk.BytesLeft(), 40, 0);
			}
		}

		m_modFormat.madeWithTracker = U_("ProTracker ") + version;
	}
	m_SongFlags.set(SONG_PT_MODE);
	m_playBehaviour.set(kMODIgnorePanning);
	m_playBehaviour.set(kMODOneShotLoops);
	m_playBehaviour.reset(kMODSampleSwap);

	return ok;
}


#ifndef MODPLUG_NO_FILESAVE

bool CSoundFile::SaveMod(std::ostream &f) const
{
	if(m_nChannels == 0)
	{
		return false;
	}

	// Write song title
	{
		char name[20];
		mpt::String::WriteBuf(mpt::String::maybeNullTerminated, name) = m_songName;
		mpt::IO::Write(f, name);
	}

	std::vector<SmpLength> sampleLength(32, 0);
	std::vector<SAMPLEINDEX> sampleSource(32, 0);

	if(GetNumInstruments())
	{
		INSTRUMENTINDEX lastIns = std::min(INSTRUMENTINDEX(31), GetNumInstruments());
		for(INSTRUMENTINDEX ins = 1; ins <= lastIns; ins++) if (Instruments[ins])
		{
			// Find some valid sample associated with this instrument.
			for(auto smp : Instruments[ins]->Keyboard)
			{
				if(smp > 0 && smp <= GetNumSamples())
				{
					sampleSource[ins] = smp;
					break;
				}
			}
		}
	} else
	{
		for(SAMPLEINDEX i = 1; i <= 31; i++)
		{
			sampleSource[i] = i;
		}
	}

	// Write sample headers
	for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
	{
		MODSampleHeader sampleHeader;
		mpt::String::WriteBuf(mpt::String::maybeNullTerminated, sampleHeader.name) = m_szNames[sampleSource[smp]];
		sampleLength[smp] = sampleHeader.ConvertToMOD(sampleSource[smp] <= GetNumSamples() ? GetSample(sampleSource[smp]) : ModSample(MOD_TYPE_MOD));
		mpt::IO::Write(f, sampleHeader);
	}

	// Write order list
	MODFileHeader fileHeader;
	MemsetZero(fileHeader);

	PATTERNINDEX writePatterns = 0;
	uint8 writtenOrders = 0;
	for(ORDERINDEX ord = 0; ord < Order().GetLength() && writtenOrders < 128; ord++)
	{
		// Ignore +++ and --- patterns in order list, as well as high patterns (MOD officially only supports up to 128 patterns)
		if(ord == Order().GetRestartPos())
		{
			fileHeader.restartPos = writtenOrders;
		}
		if(Order()[ord] < 128)
		{
			fileHeader.orderList[writtenOrders++] = static_cast<uint8>(Order()[ord]);
			if(writePatterns <= Order()[ord])
			{
				writePatterns = Order()[ord] + 1;
			}
		}
	}
	fileHeader.numOrders = writtenOrders;
	mpt::IO::Write(f, fileHeader);

	// Write magic bytes
	char modMagic[4];
	CHANNELINDEX writeChannels = std::min(CHANNELINDEX(99), GetNumChannels());
	if(writeChannels == 4)
	{
		// ProTracker may not load files with more than 64 patterns correctly if we do not specify the M!K! magic.
		if(writePatterns <= 64)
			memcpy(modMagic, "M.K.", 4);
		else
			memcpy(modMagic, "M!K!", 4);
	} else if(writeChannels < 10)
	{
		memcpy(modMagic, "0CHN", 4);
		modMagic[0] += static_cast<char>(writeChannels);
	} else
	{
		memcpy(modMagic, "00CH", 4);
		modMagic[0] += static_cast<char>(writeChannels / 10u);
		modMagic[1] += static_cast<char>(writeChannels % 10u);
	}
	mpt::IO::Write(f, modMagic);

	// Write patterns
	bool invalidInstruments = false;
	std::vector<uint8> events;
	for(PATTERNINDEX pat = 0; pat < writePatterns; pat++)
	{
		if(!Patterns.IsValidPat(pat))
		{
			// Invent empty pattern
			events.assign(writeChannels * 64 * 4, 0);
			mpt::IO::Write(f, events);
			continue;
		}

		for(ROWINDEX row = 0; row < 64; row++)
		{
			if(row >= Patterns[pat].GetNumRows())
			{
				// Invent empty row
				events.assign(writeChannels * 4, 0);
				mpt::IO::Write(f, events);
				continue;
			}
			const auto rowBase = Patterns[pat].GetRow(row);
			bool writePatternBreak = (Patterns[pat].GetNumRows() < 64 && row + 1 == Patterns[pat].GetNumRows() && !Patterns[pat].RowHasJump(row));

			events.resize(writeChannels * 4);
			size_t eventByte = 0;
			for(CHANNELINDEX chn = 0; chn < writeChannels; chn++, eventByte += 4)
			{
				const ModCommand &m = rowBase[chn];
				uint8 command = 0, param = 0;
				ModSaveCommand(m, command, param, false, true);

				if(m.volcmd == VOLCMD_VOLUME && !command && !param)
				{
					// Maybe we can save some volume commands...
					command = 0x0C;
					param = std::min(m.vol, uint8(64));
				}
				if(writePatternBreak && !command && !param)
				{
					command = 0x0D;
					writePatternBreak = false;
				}

				uint16 period = 0;
				// Convert note to period
				if(m.note >= 24 + NOTE_MIN && m.note < std::size(ProTrackerPeriodTable) + 24 + NOTE_MIN)
				{
					period = ProTrackerPeriodTable[m.note - 24 - NOTE_MIN];
				}

				const uint8 instr = (m.instr > 31) ? 0 : m.instr;
				if(m.instr > 31)
					invalidInstruments = true;

				events[eventByte + 0] = ((period >> 8) & 0x0F) | (instr & 0x10);
				events[eventByte + 1] = period & 0xFF;
				events[eventByte + 2] = ((instr & 0x0F) << 4) | (command & 0x0F);
				events[eventByte + 3] = param;
			}
			mpt::IO::WriteRaw(f, mpt::as_span(events));
		}
	}

	if(invalidInstruments)
	{
		AddToLog(LogWarning, U_("Warning: This track references sample slots higher than 31. Such samples cannot be saved in the MOD format, and thus the notes will not sound correct. Use the Cleanup tool to rearrange and remove unused samples."));
	}

	//Check for unsaved patterns
	for(PATTERNINDEX pat = writePatterns; pat < Patterns.Size(); pat++)
	{
		if(Patterns.IsValidPat(pat))
		{
			AddToLog(LogWarning, U_("Warning: This track contains at least one pattern after the highest pattern number referred to in the sequence. Such patterns are not saved in the MOD format."));
			break;
		}
	}

	// Writing samples
	for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
	{
		if(sampleLength[smp] == 0)
		{
			continue;
		}
		const ModSample &sample = Samples[sampleSource[smp]];

		const mpt::IO::Offset sampleStart = mpt::IO::TellWrite(f);
		const size_t writtenBytes = MODSampleHeader::GetSampleFormat().WriteSample(f, sample, sampleLength[smp]);

		const int8 silence = 0;

		// Write padding byte if the sample size is odd.
		if((writtenBytes % 2u) != 0)
		{
			mpt::IO::Write(f, silence);
		}

		if(!sample.uFlags[CHN_LOOP] && writtenBytes >= 2)
		{
			// First two bytes of oneshot samples have to be 0 due to PT's one-shot loop
			const mpt::IO::Offset sampleEnd = mpt::IO::TellWrite(f);
			mpt::IO::SeekAbsolute(f, sampleStart);
			mpt::IO::Write(f, silence);
			mpt::IO::Write(f, silence);
			mpt::IO::SeekAbsolute(f, sampleEnd);
		}
	}

	return true;
}

#endif  // MODPLUG_NO_FILESAVE


OPENMPT_NAMESPACE_END
