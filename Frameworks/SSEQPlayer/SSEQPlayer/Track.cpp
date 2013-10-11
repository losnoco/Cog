/*
 * SSEQ Player - Track structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-04-01
 *
 * Adapted from source code of FeOS Sound System
 * By fincs
 * https://github.com/fincs/FSS
 */

#include "Track.h"
#include "Player.h"
#include "common.h"

Track::Track()
{
	this->Zero();
}

void Track::Init(uint8_t handle, Player *player, const uint8_t *dataPos, int n)
{
	this->trackId = handle;
	this->num = n;
	this->ply = player;
	this->startPos = dataPos;
	this->ClearState();
}

void Track::Zero()
{
	this->trackId = -1;

	this->state.reset();
	this->num = this->prio = 0;
	this->ply = nullptr;

	this->startPos = this->pos = nullptr;
	memset(this->stack, 0, sizeof(this->stack));
	this->stackPos = 0;
	memset(this->loopCount, 0, sizeof(this->loopCount));

	this->wait = 0;
	this->patch = 0;
	this->portaKey = this->portaTime = 0;
	this->sweepPitch = 0;
	this->vol = this->expr = 0;
	this->pan = 0;
	this->pitchBendRange = 0;
	this->pitchBend = this->transpose = 0;

	this->a = this->d = this->s = this->r = 0;

	this->modType = this->modSpeed = this->modDepth = this->modRange = 0;
	this->modDelay = 0;

	this->updateFlags.reset();
}

void Track::ClearState()
{
	this->state.reset();
	this->state.set(TS_ALLOCBIT);
	this->state.set(TS_NOTEWAIT);
	this->prio = this->ply->prio + 64;

	this->pos = this->startPos;
	this->stackPos = 0;

	this->wait = 0;
	this->patch = 0;
	this->portaKey = 60;
	this->portaTime = 0;
	this->sweepPitch = 0;
	this->vol = 64;
	this->expr = 127;
	this->pan = 0;
	this->pitchBendRange = 2;
	this->pitchBend = this->transpose = 0;

	this->a = this->d = this->s = this->r = 0xFF;

	this->modType = 0;
	this->modRange = 1;
	this->modSpeed = 16;
	this->modDelay = 10;
	this->modDepth = 0;
}

void Track::Free()
{
	this->state.reset();
	this->updateFlags.reset();
}

int Track::NoteOn(int key, int vel, int len)
{
	auto sbnk = this->ply->sseq->bank;

	if (this->patch >= sbnk->instruments.size())
		return -1;

	bool bIsPCM = true;
	Channel *chn = nullptr;
	int nCh = -1;

	auto &instrument = sbnk->instruments[this->patch];
	const SBNKInstrumentRange *noteDef = nullptr;
	int fRecord = instrument.record;

	if (fRecord == 16)
	{
		if (!(instrument.ranges[0].lowNote <= key && key <= instrument.ranges[instrument.ranges.size() - 1].highNote))
			return -1;
		int rn = key - instrument.ranges[0].lowNote;
		noteDef = &instrument.ranges[rn];
		fRecord = noteDef->record;
	}
	else if (fRecord == 17)
	{
		size_t reg, ranges;
		for (reg = 0, ranges = instrument.ranges.size(); reg < ranges; ++reg)
			if (key <= instrument.ranges[reg].highNote)
				break;
		if (reg == ranges)
			return -1;

		noteDef = &instrument.ranges[reg];
		fRecord = noteDef->record;
	}

	if (!fRecord)
		return -1;
	else if (fRecord == 1)
	{
		if (!noteDef)
			noteDef = &instrument.ranges[0];
	}
	else if (fRecord < 4)
	{
		// PSG
		// fRecord = 2 -> PSG tone, pNoteDef->wavid -> PSG duty
		// fRecord = 3 -> PSG noise
		bIsPCM = false;
		if (!noteDef)
			noteDef = &instrument.ranges[0];
		if (fRecord == 3)
		{
			nCh = this->ply->ChannelAlloc(TYPE_NOISE, this->prio);
			if (nCh < 0)
				return -1;
			chn = &this->ply->channels[nCh];
			chn->tempReg.CR = SOUND_FORMAT_PSG | SCHANNEL_ENABLE;
		}
		else
		{
			nCh = this->ply->ChannelAlloc(TYPE_PSG, this->prio);
			if (nCh < 0)
				return -1;
			chn = &this->ply->channels[nCh];
			chn->tempReg.CR = SOUND_FORMAT_PSG | SCHANNEL_ENABLE | SOUND_DUTY(noteDef->swav & 0x7);
		}
		// TODO: figure out what pNoteDef->tnote means for PSG channels
		chn->tempReg.TIMER = -SOUND_FREQ(440 * 8); // key #69 (A4)
		chn->reg.samplePosition = -1;
		chn->reg.psgX = 0x7FFF;
	}

	if (bIsPCM)
	{
		nCh = this->ply->ChannelAlloc(TYPE_PCM, this->prio);
		if (nCh < 0)
			return -1;
		chn = &this->ply->channels[nCh];

		auto swav = &sbnk->waveArc[noteDef->swar]->swavs.find(noteDef->swav)->second;
		chn->tempReg.CR = SOUND_FORMAT(swav->waveType & 3) | SOUND_LOOP(!!swav->loop) | SCHANNEL_ENABLE;
		chn->tempReg.SOURCE = swav;
		chn->tempReg.TIMER = swav->time;
		chn->tempReg.REPEAT_POINT = swav->loopOffset;
		chn->tempReg.LENGTH = swav->nonLoopLength;
		chn->reg.samplePosition = -3;
	}

	chn->state = CS_START;
	chn->trackId = this->trackId;
	chn->flags.reset();
	chn->prio = this->prio;
	chn->key = key;
	chn->orgKey = bIsPCM ? noteDef->noteNumber : 69;
	chn->velocity = Cnv_Sust(vel);
	chn->pan = static_cast<int>(noteDef->pan) - 64;
	chn->modDelayCnt = 0;
	chn->modCounter = 0;
	chn->noteLength = len;
	chn->reg.sampleIncrease = 0;

	chn->attackLvl = Cnv_Attack(this->a == 0xFF ? noteDef->attackRate : this->a);
	chn->decayRate = Cnv_Fall(this->d == 0xFF ? noteDef->decayRate : this->d);
	chn->sustainLvl = this->s == 0xFF ? noteDef->sustainLevel : this->s;
	chn->releaseRate = Cnv_Fall(this->r == 0xFF ? noteDef->releaseRate : this->r);

	chn->UpdateVol(*this);
	chn->UpdatePan(*this);
	chn->UpdateTune(*this);
	chn->UpdateMod(*this);
	chn->UpdatePorta(*this);

	this->portaKey = key;

	return nCh;
}

int Track::NoteOnTie(int key, int vel)
{
	// Find an existing note
	int i;
	Channel *chn = nullptr;
	for (i = 0; i < 16; ++i)
	{
		chn = &this->ply->channels[i];
		if (chn->state > CS_NONE && chn->trackId == this->trackId && chn->state != CS_RELEASE)
			break;
	}

	if (i == 16)
		// Can't find note -> create an endless one
		return this->NoteOn(key, vel, -1);

	chn->flags.reset();
	chn->prio = this->prio;
	chn->key = key;
	chn->velocity = Cnv_Sust(vel);
	chn->modDelayCnt = 0;
	chn->modCounter = 0;

	chn->UpdateVol(*this);
	//chn->UpdatePan(*this);
	chn->UpdateTune(*this);
	chn->UpdateMod(*this);
	chn->UpdatePorta(*this);

	this->portaKey = key;
	chn->flags.set(CF_UPDTMR);

	return i;
}

void Track::ReleaseAllNotes()
{
	for (int i = 0; i < 16; ++i)
	{
		Channel &chn = this->ply->channels[i];
		if (chn.state > CS_NONE && chn.trackId == this->trackId && chn.state != CS_RELEASE)
			chn.Release();
	}
}

enum SseqCommand
{
	SSEQ_CMD_REST = 0x80,
	SSEQ_CMD_PATCH = 0x81,
	SSEQ_CMD_PAN = 0xC0,
	SSEQ_CMD_VOL = 0xC1,
	SSEQ_CMD_MASTERVOL = 0xC2,
	SSEQ_CMD_PRIO = 0xC6,
	SSEQ_CMD_NOTEWAIT = 0xC7,
	SSEQ_CMD_TIE = 0xC8,
	SSEQ_CMD_EXPR = 0xD5,
	SSEQ_CMD_TEMPO = 0xE1,
	SSEQ_CMD_END = 0xFF,

	SSEQ_CMD_GOTO = 0x94,
	SSEQ_CMD_CALL = 0x95,
	SSEQ_CMD_RET = 0xFD,
	SSEQ_CMD_LOOPSTART = 0xD4,
	SSEQ_CMD_LOOPEND = 0xFC,

	SSEQ_CMD_TRANSPOSE = 0xC3,
	SSEQ_CMD_PITCHBEND = 0xC4,
	SSEQ_CMD_PITCHBENDRANGE = 0xC5,

	SSEQ_CMD_ATTACK = 0xD0,
	SSEQ_CMD_DECAY = 0xD1,
	SSEQ_CMD_SUSTAIN = 0xD2,
	SSEQ_CMD_RELEASE = 0xD3,

	SSEQ_CMD_PORTAKEY = 0xC9,
	SSEQ_CMD_PORTAFLAG = 0xCE,
	SSEQ_CMD_PORTATIME = 0xCF,
	SSEQ_CMD_SWEEPPITCH = 0xE3,

	SSEQ_CMD_MODDEPTH = 0xCA,
	SSEQ_CMD_MODSPEED = 0xCB,
	SSEQ_CMD_MODTYPE = 0xCC,
	SSEQ_CMD_MODRANGE = 0xCD,
	SSEQ_CMD_MODDELAY = 0xE0,

	SSEQ_CMD_RANDOM = 0xA0,
	SSEQ_CMD_PRINTVAR = 0xD6,
	SSEQ_CMD_IF = 0xA2,
	SSEQ_CMD_UNSUP1 = 0xA1,
	SSEQ_CMD_UNSUP2_LO = 0xB0,
	SSEQ_CMD_UNSUP2_HI = 0xBD
};

void Track::Run()
{
	// Indicate "heartbeat" for this track
	this->updateFlags.set(TUF_LEN);

	// Exit if the track has already ended
	if (this->state[TS_END])
		return;

	if (this->wait)
	{
		--this->wait;
		if (this->wait)
			return;
	}

	auto pData = &this->pos;

	while (!this->wait)
	{
		int cmd = read8(pData);
		if (cmd < 0x80)
		{
			// Note on
			int key = cmd + this->transpose;
			int vel = read8(pData);
			int len = readvl(pData);
			if (this->state[TS_NOTEWAIT])
				this->wait = len;
			if (this->state[TS_TIEBIT])
				this->NoteOnTie(key, vel);
			else
				this->NoteOn(key, vel, len);
		}
		else
			switch (cmd)
			{
				//-----------------------------------------------------------------
				// Main commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_REST:
					this->wait = readvl(pData);
					break;

				case SSEQ_CMD_PATCH:
					this->patch = readvl(pData);
					break;

				case SSEQ_CMD_GOTO:
					*pData = &this->ply->sseq->data[read24(pData)];
					break;

				case SSEQ_CMD_CALL:
				{
					const uint8_t *dest = &this->ply->sseq->data[read24(pData)];
					this->stack[this->stackPos++] = *pData;
					*pData = dest;
					break;
				}

				case SSEQ_CMD_RET:
					*pData = this->stack[--this->stackPos];
					break;

				case SSEQ_CMD_PAN:
					this->pan = read8(pData) - 64;
					this->updateFlags.set(TUF_PAN);
					break;

				case SSEQ_CMD_VOL:
					this->vol = read8(pData);
					this->updateFlags.set(TUF_VOL);
					break;

				case SSEQ_CMD_MASTERVOL:
					this->ply->masterVol = Cnv_Sust(read8(pData));
					for (uint8_t i = 0; i < this->ply->nTracks; ++i)
						this->ply->tracks[this->ply->trackIds[i]].updateFlags.set(TUF_VOL);
					break;

				case SSEQ_CMD_PRIO:
					this->prio = this->ply->prio + read8(pData);
					// Update here?
					break;

				case SSEQ_CMD_NOTEWAIT:
					this->state.set(TS_NOTEWAIT, !!read8(pData));
					break;

				case SSEQ_CMD_TIE:
					this->state.set(TS_TIEBIT, !!read8(pData));
					this->ReleaseAllNotes();
					break;

				case SSEQ_CMD_EXPR:
					this->expr = read8(pData);
					this->updateFlags.set(TUF_VOL);
					break;

				case SSEQ_CMD_TEMPO:
					this->ply->tempo = read16(pData);
					break;

				case SSEQ_CMD_END:
					this->state.set(TS_END);
					return;

				case SSEQ_CMD_LOOPSTART:
					this->loopCount[this->stackPos] = read8(pData);
					this->stack[this->stackPos++] = *pData;
					break;

				case SSEQ_CMD_LOOPEND:
					if (this->stackPos)
					{
						const uint8_t *rPos = this->stack[this->stackPos - 1];
						uint8_t &nR = this->loopCount[this->stackPos - 1];
						uint8_t prevR = nR;
						if (prevR && !--nR)
							--this->stackPos;
						*pData = rPos;
					}
					break;

				//-----------------------------------------------------------------
				// Tuning commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_TRANSPOSE:
					this->transpose = read8(pData);
					break;

				case SSEQ_CMD_PITCHBEND:
					this->pitchBend = read8(pData);
					this->updateFlags.set(TUF_TIMER);
					break;

				case SSEQ_CMD_PITCHBENDRANGE:
					this->pitchBendRange = read8(pData);
					this->updateFlags.set(TUF_TIMER);
					break;

				//-----------------------------------------------------------------
				// Envelope-related commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_ATTACK:
					this->a = read8(pData);
					break;

				case SSEQ_CMD_DECAY:
					this->d = read8(pData);
					break;

				case SSEQ_CMD_SUSTAIN:
					this->s = read8(pData);
					break;

				case SSEQ_CMD_RELEASE:
					this->r = read8(pData);
					break;

				//-----------------------------------------------------------------
				// Portamento-related commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_PORTAKEY:
					this->portaKey = read8(pData) + this->transpose;
					this->state.set(TS_PORTABIT);
					// Update here?
					break;

				case SSEQ_CMD_PORTAFLAG:
					this->state.set(TS_PORTABIT, !!read8(pData));
					// Update here?
					break;

				case SSEQ_CMD_PORTATIME:
					this->portaTime = read8(pData);
					// Update here?
					break;

				case SSEQ_CMD_SWEEPPITCH:
					this->sweepPitch = read16(pData);
					// Update here?
					break;

				//-----------------------------------------------------------------
				// Modulation-related commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_MODDEPTH:
					this->modDepth = read8(pData);
					this->updateFlags.set(TUF_MOD);
					break;

				case SSEQ_CMD_MODSPEED:
					this->modSpeed = read8(pData);
					this->updateFlags.set(TUF_MOD);
					break;

				case SSEQ_CMD_MODTYPE:
					this->modType = read8(pData);
					this->updateFlags.set(TUF_MOD);
					break;

				case SSEQ_CMD_MODRANGE:
					this->modRange = read8(pData);
					this->updateFlags.set(TUF_MOD);
					break;

				case SSEQ_CMD_MODDELAY:
					this->modDelay = read16(pData);
					this->updateFlags.set(TUF_MOD);
					break;

				//-----------------------------------------------------------------
				// Variable-related commands
				//-----------------------------------------------------------------

				case SSEQ_CMD_RANDOM: // TODO
					*pData += 5;
					break;

				case SSEQ_CMD_PRINTVAR: // TODO
					*pData += 1;
					break;

				case SSEQ_CMD_UNSUP1: // TODO
				{
					int t = read8(pData);
					if (t >= SSEQ_CMD_UNSUP2_LO && t <= SSEQ_CMD_UNSUP2_HI)
						*pData += 1;
					*pData += 1;
					break;
				}

				case SSEQ_CMD_IF: // TODO
					break;

				default:
					if (cmd >= SSEQ_CMD_UNSUP2_LO && cmd <= SSEQ_CMD_UNSUP2_HI) // TODO
						*pData += 3;
			}
	}
}
