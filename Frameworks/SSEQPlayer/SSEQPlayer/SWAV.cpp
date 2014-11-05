/*
 * SSEQ Player - SDAT SWAV (Waveform/Sample) structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-04-12
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#include "SWAV.h"

static int ima_index_table[] =
{
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8
};

static int ima_step_table[] =
{
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
	19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
	130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
	876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
	2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
	5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

SWAV::SWAV() : waveType(0), loop(0), sampleRate(0), time(0), loopOffset(0), nonLoopLength(0), data(), dataptr(nullptr)
{
}

static inline void DecodeADPCMNibble(int32_t nibble, int32_t &stepIndex, int32_t &predictedValue)
{
	int32_t step = ima_step_table[stepIndex];

	stepIndex += ima_index_table[nibble];

	if (stepIndex < 0)
		stepIndex = 0;
	else if (stepIndex > 88)
		stepIndex = 88;

	int32_t diff = step >> 3;

	if (nibble & 4)
		diff += step;
	if (nibble & 2)
		diff += step >> 1;
	if (nibble & 1)
		diff += step >> 2;
	if (nibble & 8)
		predictedValue -= diff;
	else
		predictedValue += diff;

	if (predictedValue < -0x8000)
		predictedValue = -0x8000;
	else if (predictedValue > 0x7FFF)
		predictedValue = 0x7FFF;
}

void SWAV::DecodeADPCM(const uint8_t *origData, uint32_t len)
{
	int32_t predictedValue = origData[0] | (origData[1] << 8);
	int32_t stepIndex = origData[2] | (origData[3] << 8);
	auto finalData = &this->data[0];

	for (uint32_t i = 0; i < len; ++i)
	{
		int32_t nibble = origData[i + 4] & 0x0F;
		DecodeADPCMNibble(nibble, stepIndex, predictedValue);
		finalData[2 * i] = predictedValue;

		nibble = (origData[i + 4] >> 4) & 0x0F;
		DecodeADPCMNibble(nibble, stepIndex, predictedValue);
		finalData[2 * i + 1] = predictedValue;
	}
}

void SWAV::Read(PseudoFile &file)
{
	this->waveType = file.ReadLE<uint8_t>();
	this->loop = file.ReadLE<uint8_t>();
	this->sampleRate = file.ReadLE<uint16_t>();
	this->time = file.ReadLE<uint16_t>();
	this->loopOffset = file.ReadLE<uint16_t>();
	this->nonLoopLength = file.ReadLE<uint32_t>();
	uint32_t size = (this->loopOffset + this->nonLoopLength) * 4;
	auto origData = std::vector<uint8_t>(size);
	file.ReadLE(origData);

	// Convert data accordingly
	if (!this->waveType)
	{
		// PCM 8-bit -> PCM signed 16-bit
		this->data.resize(origData.size(), 0);
		for (size_t i = 0, len = origData.size(); i < len; ++i)
			this->data[i] = origData[i] << 8;
		this->loopOffset *= 4;
		this->nonLoopLength *= 4;
	}
	else if (this->waveType == 1)
	{
		// PCM signed 16-bit, no conversion
		this->data.resize(origData.size() / 2, 0);
		for (size_t i = 0, len = origData.size() / 2; i < len; ++i)
			this->data[i] = ReadLE<int16_t>(&origData[2 * i]);
		this->loopOffset *= 2;
		this->nonLoopLength *= 2;
	}
	else if (this->waveType == 2)
	{
		// IMA ADPCM -> PCM signed 16-bit
		this->data.resize((origData.size() - 4) * 2, 0);
		this->DecodeADPCM(&origData[0], origData.size() - 4);
		--this->loopOffset;
		this->loopOffset *= 8;
		this->nonLoopLength *= 8;
	}
	this->dataptr = &this->data[0];
}
