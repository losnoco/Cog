/*
 * SSEQ Player - Player structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-04-01
 *
 * Adapted from source code of FeOS Sound System
 * By fincs
 * https://github.com/fincs/FSS
 */

#include "Player.h"
#include "common.h"

Player::Player() : prio(0), nTracks(0), tempo(0), tempoCount(0), tempoRate(0), masterVol(0), sseq(nullptr), sampleRate(0), interpolation(INTERPOLATION_NONE)
{
	memset(this->trackIds, 0, sizeof(this->trackIds));
	for (size_t i = 0; i < 16; ++i)
		this->channels[i].chnId = i;
}

bool Player::Setup(const SSEQ *sseqToPlay)
{
	this->sseq = sseqToPlay;

	int firstTrack = this->TrackAlloc();
	if (firstTrack == -1)
		return false;
	this->tracks[firstTrack].Init(firstTrack, this, nullptr, 0);

	this->nTracks = 1;
	this->trackIds[0] = firstTrack;

	auto pData = &this->sseq->data[0];
	if (*pData == 0xFE)
		for (pData += 3; *pData == 0x93; ) // Prepare extra tracks
		{
			++pData;
			int tNum = read8(&pData);
			auto pos = &this->sseq->data[read24(&pData)];
			int newTrack = this->TrackAlloc();
			if (newTrack == -1)
				continue;
			this->tracks[newTrack].Init(newTrack, this, pos, tNum);
			this->trackIds[this->nTracks++] = newTrack;
		}

	this->tracks[firstTrack].startPos = this->tracks[firstTrack].pos = pData;

	this->secondsPerSample = 1.0 / this->sampleRate;

	this->ClearState();

	return true;
}

void Player::ClearState()
{
	this->tempo = 120;
	this->tempoCount = 0;
	this->tempoRate = 0x100;
	this->masterVol = 0; // this is actually the highest level
	this->secondsIntoPlayback = 0;
	this->secondsUntilNextClock = SecondsPerClockCycle;
}

void Player::FreeTracks()
{
	for (uint8_t i = 0; i < this->nTracks; ++i)
		this->tracks[this->trackIds[i]].Free();
	this->nTracks = 0;
}

void Player::Stop(bool bKillSound)
{
	this->ClearState();
	for (uint8_t i = 0; i < this->nTracks; ++i)
	{
		uint8_t trackId = this->trackIds[i];
		this->tracks[trackId].ClearState();
		for (int j = 0; j < 16; ++j)
		{
			Channel &chn = this->channels[j];
			if (chn.state != CS_NONE && chn.trackId == trackId)
			{
				if (bKillSound)
					chn.Kill();
				else
					chn.Release();
			}
		}
	}
	this->FreeTracks();
}

int Player::ChannelAlloc(int type, int priority)
{
	static const uint8_t pcmChnArray[] = { 4, 5, 6, 7, 2, 0, 3, 1, 8, 9, 10, 11, 14, 12, 15, 13 };
	static const uint8_t psgChnArray[] = { 13, 12, 11, 10, 9, 8 };
	static const uint8_t noiseChnArray[] = { 15, 14 };
	static const uint8_t arraySizes[] = { sizeof(pcmChnArray), sizeof(psgChnArray), sizeof(noiseChnArray) };
	static const uint8_t *const arrayArray[] = { pcmChnArray, psgChnArray, noiseChnArray };

	auto chnArray = arrayArray[type];
	int arraySize = arraySizes[type];

	int curChnNo = -1;
	for (int i = 0; i < arraySize; ++i)
	{
		int thisChnNo = chnArray[i];
		Channel &thisChn = this->channels[thisChnNo];
		Channel &curChn = this->channels[curChnNo];
		if (curChnNo != -1 && thisChn.prio >= curChn.prio)
		{
			if (thisChn.prio != curChn.prio)
				continue;
			if (curChn.vol <= thisChn.vol)
				continue;
		}
		curChnNo = thisChnNo;
	}

	if (curChnNo == -1 || priority < this->channels[curChnNo].prio)
		return -1;
	this->channels[curChnNo].ply = this;
	this->channels[curChnNo].noteLength = -1;
	this->channels[curChnNo].vol = 0;
	this->channels[curChnNo].clearHistory();
	return curChnNo;
}

int Player::TrackAlloc()
{
	for (int i = 0; i < FSS_MAXTRACKS; ++i)
	{
		Track &thisTrk = this->tracks[i];
		if (!thisTrk.state[TS_ALLOCBIT])
		{
			thisTrk.Zero();
			thisTrk.state.set(TS_ALLOCBIT);
			thisTrk.updateFlags.reset();
			return i;
		}
	}
	return -1;
}

void Player::Run()
{
	while (this->tempoCount > 240)
	{
		this->tempoCount -= 240;
		for (uint8_t i = 0; i < this->nTracks; ++i)
			this->tracks[this->trackIds[i]].Run();
	}
	this->tempoCount += (static_cast<int>(this->tempo) * static_cast<int>(this->tempoRate)) >> 8;
}

void Player::UpdateTracks()
{
	for (int i = 0; i < 16; ++i)
		this->channels[i].UpdateTrack();
	for (int i = 0; i < FSS_MAXTRACKS; ++i)
		this->tracks[i].updateFlags.reset();
}

void Player::Timer()
{
	this->UpdateTracks();

	for (int i = 0; i < 16; ++i)
		this->channels[i].Update();

	this->Run();
}

template<typename T1, typename T2> static inline void clamp(T1 &valueToClamp, const T2 &minValue, const T2 &maxValue)
{
	if (valueToClamp < minValue)
		valueToClamp = minValue;
	else if (valueToClamp > maxValue)
		valueToClamp = maxValue;
}

static inline int32_t muldiv7(int32_t val, uint8_t mul)
{
	return mul == 127 ? val : ((val * mul) >> 7);
}

void Player::GenerateSamples(std::vector<uint8_t> &buf, unsigned offset, unsigned samples)
{
	unsigned long mute = this->mutes.to_ulong();

	for (unsigned smpl = 0; smpl < samples; ++smpl)
	{
		this->secondsIntoPlayback += this->secondsPerSample;

		int32_t leftChannel = 0, rightChannel = 0;

		// I need to advance the sound channels here
		for (int i = 0; i < 16; ++i)
		{
			Channel &chn = this->channels[i];

			if (chn.state > CS_NONE)
			{
				int32_t sample = chn.GenerateSample();
				chn.IncrementSample();

				if (mute & BIT(i))
					continue;

				uint8_t datashift = chn.reg.volumeDiv;
				if (datashift == 3)
					datashift = 4;
				sample = muldiv7(sample, chn.reg.volumeMul) >> datashift;

				leftChannel += muldiv7(sample, 127 - chn.reg.panning);
				rightChannel += muldiv7(sample, chn.reg.panning);
			}
		}

		leftChannel = muldiv7(leftChannel, 127 - this->masterVol);
		rightChannel = muldiv7(rightChannel, 127 - this->masterVol);

		clamp(leftChannel, -0x8000, 0x7FFF);
		clamp(rightChannel, -0x8000, 0x7FFF);

		buf[offset++] = leftChannel & 0xFF;
		buf[offset++] = (leftChannel >> 8) & 0xFF;
		buf[offset++] = rightChannel & 0xFF;
		buf[offset++] = (rightChannel >> 8) & 0xFF;

		if (this->secondsIntoPlayback > this->secondsUntilNextClock)
		{
			this->Timer();
			this->secondsUntilNextClock += SecondsPerClockCycle;
		}
	}
}

