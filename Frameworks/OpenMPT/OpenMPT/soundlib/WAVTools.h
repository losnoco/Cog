/*
 * WAVTools.h
 * ----------
 * Purpose: Definition of WAV file structures and helper functions
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/uuid/uuid.hpp"

#include "../common/FileReader.h"
#include "Loaders.h"

#ifndef MODPLUG_NO_FILESAVE
#include "mpt/io/io.hpp"
#include "mpt/io/io_virtual_wrapper.hpp"
#endif

OPENMPT_NAMESPACE_BEGIN

struct FileTags;

// RIFF header
struct RIFFHeader
{
	// 32-Bit chunk identifiers
	enum RIFFMagic
	{
		idRIFF	= MagicLE("RIFF"),	// magic for WAV files
		idLIST	= MagicLE("LIST"),	// magic for samples in DLS banks
		idWAVE	= MagicLE("WAVE"),	// type for WAV files
		idwave	= MagicLE("wave"),	// type for samples in DLS banks
	};

	uint32le magic;		// RIFF (in WAV files) or LIST (in DLS banks)
	uint32le length;	// Size of the file, not including magic and length
	uint32le type;		// WAVE (in WAV files) or wave (in DLS banks)
};

MPT_BINARY_STRUCT(RIFFHeader, 12)


// General RIFF Chunk header
struct RIFFChunk
{
	// 32-Bit chunk identifiers
	enum ChunkIdentifiers
	{
		idfmt_	= MagicLE("fmt "),	// Sample format information
		iddata	= MagicLE("data"),	// Sample data
		idpcm_	= MagicLE("pcm "),	// IMA ADPCM samples
		idfact	= MagicLE("fact"),	// Compressed samples
		idsmpl	= MagicLE("smpl"),	// Sampler and loop information
		idinst	= MagicLE("inst"),	// Instrument information
		idLIST	= MagicLE("LIST"),	// List of chunks
		idxtra	= MagicLE("xtra"),	// OpenMPT extra infomration
		idcue_	= MagicLE("cue "),	// Cue points
		idwsmp	= MagicLE("wsmp"),	// DLS bank samples
		idCSET	= MagicLE("CSET"),	// Character Set
		id____	= 0x00000000,	// Found when loading buggy MPT samples

		// Identifiers in "LIST" chunk
		idINAM	= MagicLE("INAM"), // title
		idISFT	= MagicLE("ISFT"), // software
		idICOP	= MagicLE("ICOP"), // copyright
		idIART	= MagicLE("IART"), // artist
		idIPRD	= MagicLE("IPRD"), // product (album)
		idICMT	= MagicLE("ICMT"), // comment
		idIENG	= MagicLE("IENG"), // engineer
		idISBJ	= MagicLE("ISBJ"), // subject
		idIGNR	= MagicLE("IGNR"), // genre
		idICRD	= MagicLE("ICRD"), // date created

		idYEAR  = MagicLE("YEAR"), // year
		idTRCK  = MagicLE("TRCK"), // track number
		idTURL  = MagicLE("TURL"), // url
	};

	uint32le id;		// See ChunkIdentifiers
	uint32le length;	// Chunk size without header

	size_t GetLength() const
	{
		return length;
	}

	ChunkIdentifiers GetID() const
	{
		return static_cast<ChunkIdentifiers>(id.get());
	}
};

MPT_BINARY_STRUCT(RIFFChunk, 8)


// Format Chunk
struct WAVFormatChunk
{
	// Sample formats
	enum SampleFormats
	{
		fmtPCM			= 1,
		fmtFloat		= 3,
		fmtALaw			= 6,
		fmtULaw			= 7,
		fmtIMA_ADPCM	= 17,
		fmtMP3			= 85,
		fmtExtensible	= 0xFFFE,
	};

	uint16le format;			// Sample format, see SampleFormats
	uint16le numChannels;		// Number of audio channels
	uint32le sampleRate;		// Sample rate in Hz
	uint32le byteRate;			// Bytes per second (should be freqHz * blockAlign)
	uint16le blockAlign;		// Size of a sample, in bytes (do not trust this value, it's incorrect in some files)
	uint16le bitsPerSample;		// Bits per sample
};

MPT_BINARY_STRUCT(WAVFormatChunk, 16)


// Extension of the WAVFormatChunk structure, used if format == formatExtensible
struct WAVFormatChunkExtension
{
	uint16le    size;
	uint16le    validBitsPerSample;
	uint32le    channelMask;
	mpt::GUIDms subFormat;
};

MPT_BINARY_STRUCT(WAVFormatChunkExtension, 24)


// Sample information chunk
struct WAVSampleInfoChunk
{
	uint32le manufacturer;
	uint32le product;
	uint32le samplePeriod;	// 1000000000 / sampleRate
	uint32le baseNote;		// MIDI base note of sample
	uint32le pitchFraction;
	uint32le SMPTEFormat;
	uint32le SMPTEOffset;
	uint32le numLoops;		// number of loops
	uint32le samplerData;

	// Set up information
	void ConvertToWAV(uint32 freq, uint8 rootNote)
	{
		manufacturer = 0;
		product = 0;
		samplePeriod = 1000000000 / freq;
		if(rootNote != 0)
			baseNote = rootNote - NOTE_MIN;
		else
			baseNote = NOTE_MIDDLEC - NOTE_MIN;
		pitchFraction = 0;
		SMPTEFormat = 0;
		SMPTEOffset = 0;
		numLoops = 0;
		samplerData = 0;
	}
};

MPT_BINARY_STRUCT(WAVSampleInfoChunk, 36)


// Sample loop information chunk (found after WAVSampleInfoChunk in "smpl" chunk)
struct WAVSampleLoop
{
	// Sample Loop Types
	enum LoopType
	{
		loopForward		= 0,
		loopBidi		= 1,
		loopBackward	= 2,
	};

	uint32le identifier;
	uint32le loopType;		// See LoopType
	uint32le loopStart;		// Loop start in samples
	uint32le loopEnd;		// Loop end in samples
	uint32le fraction;
	uint32le playCount;		// Loop Count, 0 = infinite

	// Apply WAV loop information to a mod sample.
	void ApplyToSample(SmpLength &start, SmpLength &end, SmpLength sampleLength, SampleFlags &flags, ChannelFlags enableFlag, ChannelFlags bidiFlag, bool mptLoopFix) const;

	// Convert internal loop information into a WAV loop.
	void ConvertToWAV(SmpLength start, SmpLength end, bool bidi);
};

MPT_BINARY_STRUCT(WAVSampleLoop, 24)


// Instrument information chunk
struct WAVInstrumentChunk
{
	uint8 unshiftedNote;	// Root key of sample, 0...127
	int8  finetune;			// Finetune of root key in cents
	int8  gain;				// in dB
	uint8 lowNote;			// Note range, 0...127
	uint8 highNote;
	uint8 lowVelocity;		// Velocity range, 0...127
	uint8 highVelocity;
};

MPT_BINARY_STRUCT(WAVInstrumentChunk, 7)


// MPT-specific "xtra" chunk
struct WAVExtraChunk
{
	enum Flags
	{
		setPanning	= 0x20,
	};

	uint32le flags;
	uint16le defaultPan;
	uint16le defaultVolume;
	uint16le globalVolume;
	uint16le reserved;
	uint8le  vibratoType;
	uint8le  vibratoSweep;
	uint8le  vibratoDepth;
	uint8le  vibratoRate;

	// Set up sample information
	void ConvertToWAV(const ModSample &sample, MODTYPE modType)
	{
		if(sample.uFlags[CHN_PANNING])
		{
			flags = WAVExtraChunk::setPanning;
		} else
		{
			flags = 0;
		}

		defaultPan = sample.nPan;
		defaultVolume = sample.nVolume;
		globalVolume = sample.nGlobalVol;
		vibratoType = sample.nVibType;
		vibratoSweep = sample.nVibSweep;
		vibratoDepth = sample.nVibDepth;
		vibratoRate = sample.nVibRate;

		if((modType & MOD_TYPE_XM) && (vibratoDepth | vibratoRate))
		{
			// XM vibrato is upside down
			vibratoSweep = 255 - vibratoSweep;
		}
	}
};

MPT_BINARY_STRUCT(WAVExtraChunk, 16)


// Sample cue point structure for the "cue " chunk
struct WAVCuePoint
{
	uint32le id;			// Unique identification value
	uint32le position;		// Play order position
	uint32le riffChunkID;	// RIFF ID of corresponding data chunk
	uint32le chunkStart;	// Byte Offset of Data Chunk
	uint32le blockStart;	// Byte Offset to sample of First Channel
	uint32le offset;		// Byte Offset to sample byte of First Channel

	// Set up sample information
	void ConvertToWAV(uint32 id_, SmpLength offset_)
	{
		id = id_;
		position = offset_;
		riffChunkID = static_cast<uint32>(RIFFChunk::iddata);
		chunkStart = 0;	// we use no Wave List Chunk (wavl) as we have only one data block, so this should be 0.
		blockStart = 0;	// ditto
		offset = offset_;
	}
};

MPT_BINARY_STRUCT(WAVCuePoint, 24)


class WAVReader
{
protected:
	FileReader file;
	FileReader sampleData, smplChunk, instChunk, xtraChunk, wsmpChunk, cueChunk;
	FileReader::ChunkList<RIFFChunk> infoChunk;

	FileReader::off_t sampleLength;
	WAVFormatChunk formatInfo;
	uint16 subFormat;
	uint16 codePage;
	bool isDLS;
	bool mayBeCoolEdit16_8;

	uint16 GetFileCodePage(FileReader::ChunkList<RIFFChunk> &chunks);

public:
	WAVReader(FileReader &inputFile);

	bool IsValid() const { return sampleData.IsValid(); }

	void FindMetadataChunks(FileReader::ChunkList<RIFFChunk> &chunks);

	// Self-explanatory getters.
	WAVFormatChunk::SampleFormats GetSampleFormat() const { return IsExtensibleFormat() ? static_cast<WAVFormatChunk::SampleFormats>(subFormat) : static_cast<WAVFormatChunk::SampleFormats>(formatInfo.format.get()); }
	uint16 GetNumChannels() const { return formatInfo.numChannels; }
	uint16 GetBitsPerSample() const { return formatInfo.bitsPerSample; }
	uint32 GetSampleRate() const { return formatInfo.sampleRate; }
	uint16 GetBlockAlign() const { return formatInfo.blockAlign; }
	FileReader GetSampleData() const { return sampleData; }
	FileReader GetWsmpChunk() const { return wsmpChunk; }
	bool IsExtensibleFormat() const { return formatInfo.format == WAVFormatChunk::fmtExtensible; }
	bool MayBeCoolEdit16_8() const { return mayBeCoolEdit16_8; }

	// Get size of a single sample point, in bytes.
	uint16 GetSampleSize() const { return static_cast<uint16>(((static_cast<uint32>(GetNumChannels()) * static_cast<uint32>(GetBitsPerSample())) + 7) / 8); }

	// Get sample length (in samples)
	SmpLength GetSampleLength() const { return mpt::saturate_cast<SmpLength>(sampleLength); }

	// Apply sample settings from file (loop points, MPT extra settings, ...) to a sample.
	void ApplySampleSettings(ModSample &sample, mpt::Charset sampleCharset, mpt::charbuf<MAX_SAMPLENAME> &sampleName);
};

#ifndef MODPLUG_NO_FILESAVE

class WAVWriter
{
protected:
	// Output stream
	mpt::IO::OFileBase &s;

	// Currently written chunk
	mpt::IO::Offset chunkHeaderPos = 0;
	RIFFChunk chunkHeader;
	bool finalized = false;

public:
	// Output to stream
	WAVWriter(mpt::IO::OFileBase &stream);
	~WAVWriter();

	// Finalize the file by closing the last open chunk and updating the file header. Returns total size of file.
	mpt::IO::Offset Finalize();
	// Begin writing a new chunk to the file.
	void StartChunk(RIFFChunk::ChunkIdentifiers id);

	// Write the WAV format to the file.
	void WriteFormat(uint32 sampleRate, uint16 bitDepth, uint16 numChannels, WAVFormatChunk::SampleFormats encoding);
	// Write text tags to the file.
	void WriteMetatags(const FileTags &tags);

protected:
	// End current chunk by updating the chunk header and writing a padding byte if necessary.
	void FinalizeChunk();

	// Write a single tag into a open idLIST chunk
	void WriteTag(RIFFChunk::ChunkIdentifiers id, const mpt::ustring &utext);
};

class WAVSampleWriter
	: public WAVWriter
{

public:
	WAVSampleWriter(mpt::IO::OFileBase &stream);
	~WAVSampleWriter();

public:
	// Write a sample loop information chunk to the file.
	void WriteLoopInformation(const ModSample &sample);
	// Write a sample's cue points to the file.
	void WriteCueInformation(const ModSample &sample);
	// Write MPT's sample information chunk to the file.
	void WriteExtraInformation(const ModSample &sample, MODTYPE modType, const char *sampleName = nullptr);
};

#endif // MODPLUG_NO_FILESAVE

OPENMPT_NAMESPACE_END
