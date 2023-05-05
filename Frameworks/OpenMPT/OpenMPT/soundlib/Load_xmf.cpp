/*
 * Load_xmf.cpp
 * ------------
 * Purpose: Module loader for music files from the DOS game "Imperium Galactica"
 * Notes  : This format has nothing to do with the XMF format by the MIDI foundation.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include "mpt/endian/int24.hpp"

OPENMPT_NAMESPACE_BEGIN

struct XMFSampleHeader
{
	using uint24le = mpt::uint24le;

	enum SampleFlags : uint8
	{
		smp16Bit      = 0x04,
		smpEnableLoop = 0x08,
		smpBidiLoop   = 0x10,
	};
	
	uint24le loopStart;
	uint24le loopEnd;
	uint24le dataStart;
	uint24le dataEnd;
	uint8    defaultVolume;
	uint8    flags;
	uint16le sampleRate;

	bool IsValid() const noexcept
	{
		if(flags & ~(smp16Bit | smpEnableLoop | smpBidiLoop))
			return false;
		if((flags & (smpEnableLoop | smpBidiLoop)) == smpBidiLoop)
			return false;
		if(dataStart.get() > dataEnd.get())
			return false;
		const uint32 length = dataEnd.get() - dataStart.get();
		if(length > 0 && sampleRate < 100)
			return false;
		if((flags & smp16Bit) && (length % 2u))
			return false;
		if((flags & smpEnableLoop) && !loopEnd.get())
			return false;
		if(loopEnd.get() != 0 && (loopEnd.get() > length || loopStart.get() >= loopEnd.get()))
			return false;
		return true;
	}

	bool HasSampleData() const noexcept
	{
		return dataEnd.get() > dataStart.get();
	}

	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize(MOD_TYPE_MOD);
		mptSmp.nLength = dataEnd.get() - dataStart.get();
		mptSmp.nLoopStart = loopStart.get();
		mptSmp.nLoopEnd = loopEnd.get();
		mptSmp.uFlags.set(CHN_LOOP, flags & smpEnableLoop);
		mptSmp.uFlags.set(CHN_PINGPONGLOOP, flags & smpBidiLoop);
		if(flags & smp16Bit)
		{
			mptSmp.uFlags.set(CHN_16BIT);
			mptSmp.nLength /= 2;
		}
		mptSmp.nVolume = defaultVolume;
		mptSmp.nC5Speed = sampleRate;
		mptSmp.FrequencyToTranspose();
	}
};

MPT_BINARY_STRUCT(XMFSampleHeader, 16)


static bool TranslateXMFEffect(ModCommand &m, uint8 command, uint8 param)
{
	if(command == 0x0B && param < 0xFF)
	{
		param++;
	} else if(command == 0x10)
	{
		command = 0x0E;
		param = 0x80 | (param & 0x0F);
	} else if(command > 0x10)
	{
		return false;
	}
	CSoundFile::ConvertModCommand(m, command, param);
	if(m.command == CMD_VOLUME)
		m.command = CMD_VOLUME8;
	return true;
}


CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderXMF(MemoryFileReader file, const uint64 *pfilesize)
{
	if(!file.CanRead(1))
		return ProbeWantMoreData;
	if(file.ReadUint8() != 0x03)
		return ProbeFailure;
	
	constexpr size_t probeHeaders = std::min(size_t(256), (ProbeRecommendedSize - 1) / sizeof(XMFSampleHeader));
	for(size_t sample = 0; sample < probeHeaders; sample++)
	{
		XMFSampleHeader sampleHeader;
		if(!file.ReadStruct(sampleHeader))
			return ProbeWantMoreData;
		if(!sampleHeader.IsValid())
			return ProbeFailure;
	}

	MPT_UNREFERENCED_PARAMETER(pfilesize);
	return ProbeSuccess;
}


bool CSoundFile::ReadXMF(FileReader &file, ModLoadingFlags loadFlags)
{
	file.Rewind();
	if(file.ReadUint8() != 0x03)
		return false;
	if(!file.CanRead(256 * sizeof(XMFSampleHeader) + 256 + 3))
		return false;
	static_assert(MAX_SAMPLES > 256);
	SAMPLEINDEX numSamples = 0;
	for(SAMPLEINDEX smp = 1; smp <= 256; smp++)
	{
		XMFSampleHeader sampleHeader;
		file.ReadStruct(sampleHeader);
		if(!sampleHeader.IsValid())
			return false;
		if(sampleHeader.HasSampleData())
			numSamples = smp;
	}
	if(!numSamples)
		return false;
	if(loadFlags == onlyVerifyHeader)
		return true;

	InitializeGlobals(MOD_TYPE_MOD);
	m_SongFlags.set(SONG_IMPORTED);
	m_nSamples = numSamples;
	m_nSamplePreAmp = 192;

	file.Seek(1);
	for(SAMPLEINDEX smp = 1; smp <= numSamples; smp++)
	{
		XMFSampleHeader sampleHeader;
		file.ReadStruct(sampleHeader);
		sampleHeader.ConvertToMPT(Samples[smp]);
		m_szNames[smp] = "";
	}

	file.Seek(1 + 256 * sizeof(XMFSampleHeader));
	ReadOrderFromFile<uint8>(Order(), file, 256, 0xFF);

	const uint8 lastChannel = file.ReadUint8();
	if(lastChannel > 31)
		return false;
	m_nChannels = lastChannel + 1u;
	const PATTERNINDEX numPatterns  = file.ReadUint8() + 1u;

	if(!file.CanRead(m_nChannels + numPatterns * m_nChannels * 64 * 6))
		return false;

	for(CHANNELINDEX chn = 0; chn < m_nChannels; chn++)
	{
		ChnSettings[chn].Reset();
		ChnSettings[chn].nPan = file.ReadUint8() * 0x11;
	}

	Patterns.ResizeArray(numPatterns);
	for(PATTERNINDEX pat = 0; pat < numPatterns; pat++)
	{
		if(!(loadFlags & loadPatternData) || !Patterns.Insert(pat, 64))
		{
			file.Skip(m_nChannels * 64 * 6);
			continue;
		}
		ModCommand dummy;
		for(ROWINDEX row = 0; row < 64; row++)
		{
			for(ModCommand &m : Patterns[pat].GetRow(row))
			{
				const auto data = file.ReadArray<uint8, 6>();
				if(data[0] > 0 && data[0] <= 77)
					m.note = NOTE_MIN + 35 + data[0];
				m.instr = data[1];
				if(!TranslateXMFEffect(m, data[2], data[5]) || !TranslateXMFEffect(dummy, data[3], data[4]))
					return false;
				m.FillInTwoCommands(m.command, m.param, dummy.command, dummy.param);
			}
		}
	}

	if(loadFlags & loadSampleData)
	{
		for(SAMPLEINDEX smp = 1; smp <= numSamples; smp++)
		{
			SampleIO(Samples[smp].uFlags[CHN_16BIT] ? SampleIO::_16bit : SampleIO::_8bit,
				SampleIO::mono,
				SampleIO::littleEndian,
				SampleIO::signedPCM).ReadSample(Samples[smp], file);
		}
	}

	m_modFormat.formatName = UL_("Imperium Galactica XMF");
	m_modFormat.type = UL_("xmf");
	m_modFormat.madeWithTracker.clear();
	m_modFormat.charset = mpt::Charset::CP437;  // No strings in this format...

	return true;
}


OPENMPT_NAMESPACE_END
