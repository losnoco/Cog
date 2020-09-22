/*
 * Snd_fx.cpp
 * -----------
 * Purpose: Processing of pattern commands, song length calculation...
 * Notes  : This needs some heavy refactoring.
 *          I thought of actually adding an effect interface class. Every pattern effect
 *          could then be moved into its own class that inherits from the effect interface.
 *          If effect handling differs severly between module formats, every format would have
 *          its own class for that effect. Then, a call chain of effect classes could be set up
 *          for each format, since effects cannot be processed in the same order in all formats.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Sndfile.h"
#include "mod_specifications.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/Moddoc.h"
#endif // MODPLUG_TRACKER
#include "tuning.h"
#include "Tables.h"
#include "modsmp_ctrl.h"	// For updating the loop wraparound data with the invert loop effect
#include "plugins/PlugInterface.h"
#include "OPL.h"
#include "MIDIEvents.h"

OPENMPT_NAMESPACE_BEGIN

// Formats which have 7-bit (0...128) instead of 6-bit (0...64) global volume commands, or which are imported to this range (mostly formats which are converted to IT internally)
#ifdef MODPLUG_TRACKER
#define GLOBALVOL_7BIT_FORMATS_EXT (MOD_TYPE_MT2)
#else
#define GLOBALVOL_7BIT_FORMATS_EXT (MOD_TYPE_NONE)
#endif // MODPLUG_TRACKER
#define GLOBALVOL_7BIT_FORMATS (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_IMF | MOD_TYPE_J2B | MOD_TYPE_MID | MOD_TYPE_AMS | MOD_TYPE_DBM | MOD_TYPE_PTM | MOD_TYPE_MDL | MOD_TYPE_DTM | GLOBALVOL_7BIT_FORMATS_EXT)


// Compensate frequency slide LUTs depending on whether we are handling periods or frequency - "up" and "down" in function name are seen from frequency perspective.
static uint32 GetLinearSlideDownTable    (const CSoundFile *sndFile, uint32 i) { MPT_ASSERT(i < CountOf(LinearSlideDownTable));     return sndFile->m_playBehaviour[kHertzInLinearMode] ? LinearSlideDownTable[i]     : LinearSlideUpTable[i]; }
static uint32 GetLinearSlideUpTable      (const CSoundFile *sndFile, uint32 i) { MPT_ASSERT(i < CountOf(LinearSlideDownTable));     return sndFile->m_playBehaviour[kHertzInLinearMode] ? LinearSlideUpTable[i]       : LinearSlideDownTable[i]; }
static uint32 GetFineLinearSlideDownTable(const CSoundFile *sndFile, uint32 i) { MPT_ASSERT(i < CountOf(FineLinearSlideDownTable)); return sndFile->m_playBehaviour[kHertzInLinearMode] ? FineLinearSlideDownTable[i] : FineLinearSlideUpTable[i]; }
static uint32 GetFineLinearSlideUpTable  (const CSoundFile *sndFile, uint32 i) { MPT_ASSERT(i < CountOf(FineLinearSlideDownTable)); return sndFile->m_playBehaviour[kHertzInLinearMode] ? FineLinearSlideUpTable[i]   : FineLinearSlideDownTable[i]; }


////////////////////////////////////////////////////////////
// Length


// Memory class for GetLength() code
class GetLengthMemory
{
protected:
	const CSoundFile &sndFile;

public:
	std::unique_ptr<CSoundFile::PlayState> state;
	struct ChnSettings
	{
		double patLoop = 0.0;
		CSoundFile::samplecount_t patLoopSmp = 0;
		ROWINDEX patLoopStart = 0;
		uint32 ticksToRender = 0;	// When using sample sync, we still need to render this many ticks
		bool incChanged = false;	// When using sample sync, note frequency has changed
		uint8 vol = 0xFF;
	};

#ifndef NO_PLUGINS
	typedef std::map<std::pair<ModCommand::INSTR, uint16>, uint16> PlugParamMap;
	PlugParamMap plugParams;
#endif
	std::vector<ChnSettings> chnSettings;
	double elapsedTime;
	static constexpr uint32 IGNORE_CHANNEL = uint32_max;

	GetLengthMemory(const CSoundFile &sf)
		: sndFile(sf)
		, state(std::make_unique<CSoundFile::PlayState>(sf.m_PlayState))
	{
		Reset();
	}

	void Reset()
	{
#ifndef NO_PLUGINS
		plugParams.clear();
#endif
		elapsedTime = 0.0;
		state->m_lTotalSampleCount = 0;
		state->m_nMusicSpeed = sndFile.m_nDefaultSpeed;
		state->m_nMusicTempo = sndFile.m_nDefaultTempo;
		state->m_nGlobalVolume = sndFile.m_nDefaultGlobalVolume;
		chnSettings.assign(sndFile.GetNumChannels(), ChnSettings());
		for(CHANNELINDEX chn = 0; chn < sndFile.GetNumChannels(); chn++)
		{
			state->Chn[chn].Reset(ModChannel::resetTotal, sndFile, chn);
			state->Chn[chn].nOldGlobalVolSlide = 0;
			state->Chn[chn].nOldChnVolSlide = 0;
			state->Chn[chn].nNote = state->Chn[chn].nNewNote = state->Chn[chn].nLastNote = NOTE_NONE;
		}
	}

	// Increment playback position of sample and envelopes on a channel
	void RenderChannel(CHANNELINDEX channel, uint32 tickDuration, uint32 portaStart = uint32_max)
	{
		ModChannel &chn = state->Chn[channel];
		uint32 numTicks = chnSettings[channel].ticksToRender;
		if(numTicks == IGNORE_CHANNEL || numTicks == 0 || (!chn.IsSamplePlaying() && !chnSettings[channel].incChanged) || chn.pModSample == nullptr)
		{
			return;
		}

		const SmpLength sampleEnd = chn.dwFlags[CHN_LOOP] ? chn.nLoopEnd : chn.nLength;
		const SmpLength loopLength = chn.nLoopEnd - chn.nLoopStart;
		const bool itEnvMode = sndFile.m_playBehaviour[kITEnvelopePositionHandling];
		const bool updatePitchEnv = (chn.PitchEnv.flags & (ENV_ENABLED | ENV_FILTER)) == ENV_ENABLED;
		bool stopNote = false;

		SamplePosition inc = chn.increment * tickDuration;
		if(chn.dwFlags[CHN_PINGPONGFLAG]) inc.Negate();

		for(uint32 i = 0; i < numTicks; i++)
		{
			bool updateInc = (chn.PitchEnv.flags & (ENV_ENABLED | ENV_FILTER)) == ENV_ENABLED;
			if(i >= portaStart)
			{
				chn.isFirstTick = false;
				const ModCommand &m = *sndFile.Patterns[state->m_nPattern].GetpModCommand(state->m_nRow, channel);
				auto command = m.command;
				if(m.volcmd == VOLCMD_TONEPORTAMENTO)
				{
					const auto [porta, clearEffectCommand] = sndFile.GetVolCmdTonePorta(m, 0);
					sndFile.TonePortamento(chn, porta);
					if(clearEffectCommand)
						command = CMD_NONE;
				}
				if(command == CMD_TONEPORTAMENTO)
					sndFile.TonePortamento(chn, m.param);
				else if(command == CMD_TONEPORTAVOL)
					sndFile.TonePortamento(chn, 0);
				updateInc = true;
			}

			int period = chn.nPeriod;
			if(itEnvMode) sndFile.IncrementEnvelopePositions(chn);
			if(updatePitchEnv)
			{
				sndFile.ProcessPitchFilterEnvelope(chn, period);
				updateInc = true;
			}
			if(!itEnvMode) sndFile.IncrementEnvelopePositions(chn);
			int vol = 0;
			sndFile.ProcessInstrumentFade(chn, vol);

			if(updateInc || chnSettings[channel].incChanged)
			{
				if(chn.m_CalculateFreq || chn.m_ReCalculateFreqOnFirstTick)
				{
					chn.RecalcTuningFreq(1, 0, sndFile);
					if(!chn.m_CalculateFreq)
						chn.m_ReCalculateFreqOnFirstTick = false;
					else
						chn.m_CalculateFreq = false;
				}
				chn.increment = sndFile.GetChannelIncrement(chn, period, 0);
				chnSettings[channel].incChanged = false;
				inc = chn.increment * tickDuration;
				if(chn.dwFlags[CHN_PINGPONGFLAG]) inc.Negate();
			}

			chn.position += inc;

			if(chn.position.GetUInt() >= sampleEnd)
			{
				if(chn.dwFlags[CHN_LOOP])
				{
					// We exceeded the sample loop, go back to loop start.
					if(chn.dwFlags[CHN_PINGPONGLOOP])
					{
						if(chn.position < SamplePosition(chn.nLoopStart, 0))
						{
							chn.position = SamplePosition(chn.nLoopStart + chn.nLoopStart, 0) - chn.position;
							chn.dwFlags.flip(CHN_PINGPONGFLAG);
							inc.Negate();
						}
						SmpLength posInt = chn.position.GetUInt() - chn.nLoopStart;
						SmpLength pingpongLength = loopLength * 2;
						if(sndFile.m_playBehaviour[kITPingPongMode]) pingpongLength--;
						posInt %= pingpongLength;
						bool forward = (posInt < loopLength);
						if(forward)
							chn.position.SetInt(chn.nLoopStart + posInt);
						else
							chn.position.SetInt(chn.nLoopEnd - (posInt - loopLength));
						if(forward == chn.dwFlags[CHN_PINGPONGFLAG])
						{
							chn.dwFlags.flip(CHN_PINGPONGFLAG);
							inc.Negate();
						}
					} else
					{
						SmpLength posInt = chn.position.GetUInt();
						if(posInt >= chn.nLoopEnd + loopLength)
						{
							const SmpLength overshoot = posInt - chn.nLoopEnd;
							posInt -= (overshoot / loopLength) * loopLength;
						}
						while(posInt >= chn.nLoopEnd)
						{
							posInt -= loopLength;
						}
						chn.position.SetInt(posInt);
					}
				} else
				{
					// Past sample end.
					stopNote = true;
					break;
				}
			}
		}

		if(stopNote)
		{
			chn.Stop();
			chn.nPortamentoDest = 0;
		}
		chnSettings[channel].ticksToRender = 0;
	}
};


// Get mod length in various cases. Parameters:
// [in]  adjustMode: See enmGetLengthResetMode for possible adjust modes.
// [in]  target: Time or position target which should be reached, or no target to get length of the first sub song. Use GetLengthTarget::StartPos to also specify a position from where the seeking should begin.
// [out] See definition of type GetLengthType for the returned values.
std::vector<GetLengthType> CSoundFile::GetLength(enmGetLengthResetMode adjustMode, GetLengthTarget target)
{
	std::vector<GetLengthType> results;
	GetLengthType retval;
	retval.startOrder = target.startOrder;
	retval.startRow = target.startRow;

	// Are we trying to reach a certain pattern position?
	const bool hasSearchTarget = target.mode != GetLengthTarget::NoTarget;
	const bool adjustSamplePos = (adjustMode & eAdjustSamplePositions) == eAdjustSamplePositions;

	SEQUENCEINDEX sequence = target.sequence;
	if(sequence >= Order.GetNumSequences()) sequence = Order.GetCurrentSequenceIndex();
	const ModSequence &orderList = Order(sequence);

	GetLengthMemory memory(*this);
	CSoundFile::PlayState &playState = *memory.state;
	// Temporary visited rows vector (so that GetLength() won't interfere with the player code if the module is playing at the same time)
	RowVisitor visitedRows(*this, sequence);

	playState.m_nNextRow = playState.m_nRow = target.startRow;
	playState.m_nNextOrder = playState.m_nCurrentOrder = target.startOrder;

	// Fast LUTs for commands that are too weird / complicated / whatever to emulate in sample position adjust mode.
	std::bitset<MAX_EFFECTS> forbiddenCommands;
	std::bitset<MAX_VOLCMDS> forbiddenVolCommands;

	if(adjustSamplePos)
	{
		forbiddenCommands.set(CMD_ARPEGGIO);             forbiddenCommands.set(CMD_PORTAMENTOUP);
		forbiddenCommands.set(CMD_PORTAMENTODOWN);       forbiddenCommands.set(CMD_XFINEPORTAUPDOWN);
		forbiddenCommands.set(CMD_NOTESLIDEUP);          forbiddenCommands.set(CMD_NOTESLIDEUPRETRIG);
		forbiddenCommands.set(CMD_NOTESLIDEDOWN);        forbiddenCommands.set(CMD_NOTESLIDEDOWNRETRIG);
		forbiddenVolCommands.set(VOLCMD_PORTAUP);        forbiddenVolCommands.set(VOLCMD_PORTADOWN);

		// Optimize away channels for which it's pointless to adjust sample positions
		for(CHANNELINDEX i = 0; i < GetNumChannels(); i++)
		{
			if(ChnSettings[i].dwFlags[CHN_MUTE]) memory.chnSettings[i].ticksToRender = GetLengthMemory::IGNORE_CHANNEL;
		}
		if(target.mode == GetLengthTarget::SeekPosition && target.pos.order < orderList.size())
		{
			// If we know where to seek, we can directly rule out any channels on which a new note would be triggered right at the start.
			const PATTERNINDEX seekPat = orderList[target.pos.order];
			if(Patterns.IsValidPat(seekPat) && Patterns[seekPat].IsValidRow(target.pos.row))
			{
				const ModCommand *m = Patterns[seekPat].GetpModCommand(target.pos.row, 0);
				for(CHANNELINDEX i = 0; i < GetNumChannels(); i++, m++)
				{
					if(m->note == NOTE_NOTECUT || m->note == NOTE_KEYOFF || (m->note == NOTE_FADE && GetNumInstruments())
						|| (m->IsNote() && !m->IsPortamento()))
					{
						memory.chnSettings[i].ticksToRender = GetLengthMemory::IGNORE_CHANNEL;
					}
				}
			}
		}
	}

	// If samples are being synced, force them to resync if tick duration changes
	uint32 oldTickDuration = 0;

	for (;;)
	{
		// Time target reached.
		if(target.mode == GetLengthTarget::SeekSeconds && memory.elapsedTime >= target.time)
		{
			retval.targetReached = true;
			break;
		}

		uint32 rowDelay = 0, tickDelay = 0;
		playState.m_nRow = playState.m_nNextRow;
		playState.m_nCurrentOrder = playState.m_nNextOrder;

		if(orderList.IsValidPat(playState.m_nCurrentOrder) && playState.m_nRow >= Patterns[orderList[playState.m_nCurrentOrder]].GetNumRows())
		{
			playState.m_nRow = 0;
			if(m_playBehaviour[kFT2LoopE60Restart])
			{
				playState.m_nRow = playState.m_nNextPatStartRow;
				playState.m_nNextPatStartRow = 0;
			}
			playState.m_nCurrentOrder = ++playState.m_nNextOrder;
		}

		// Check if pattern is valid
		playState.m_nPattern = playState.m_nCurrentOrder < orderList.size() ? orderList[playState.m_nCurrentOrder] : orderList.GetInvalidPatIndex();
		bool positionJumpOnThisRow = false, positionJumpRightOfPatternLoop = false;
		bool patternBreakOnThisRow = false;
		bool patternLoopEndedOnThisRow = false, patternLoopStartedOnThisRow = false;

		if(!Patterns.IsValidPat(playState.m_nPattern) && playState.m_nPattern != orderList.GetInvalidPatIndex() && target.mode == GetLengthTarget::SeekPosition && playState.m_nCurrentOrder == target.pos.order)
		{
			// Early test: Target is inside +++ or non-existing pattern
			retval.targetReached = true;
			break;
		}

		while(playState.m_nPattern >= Patterns.Size())
		{
			// End of song?
			if((playState.m_nPattern == orderList.GetInvalidPatIndex()) || (playState.m_nCurrentOrder >= orderList.size()))
			{
				if(playState.m_nCurrentOrder == orderList.GetRestartPos())
					break;
				else
					playState.m_nCurrentOrder = orderList.GetRestartPos();
			} else
			{
				playState.m_nCurrentOrder++;
			}
			playState.m_nPattern = (playState.m_nCurrentOrder < orderList.size()) ? orderList[playState.m_nCurrentOrder] : orderList.GetInvalidPatIndex();
			playState.m_nNextOrder = playState.m_nCurrentOrder;
			if((!Patterns.IsValidPat(playState.m_nPattern)) && visitedRows.IsVisited(playState.m_nCurrentOrder, 0, true))
			{
				if(!hasSearchTarget || !visitedRows.GetFirstUnvisitedRow(playState.m_nNextOrder, playState.m_nRow, true))
				{
					// We aren't searching for a specific row, or we couldn't find any more unvisited rows.
					break;
				} else
				{
					// We haven't found the target row yet, but we found some other unplayed row... continue searching from here.
					retval.duration = memory.elapsedTime;
					results.push_back(retval);
					retval.startRow = playState.m_nRow;
					retval.startOrder = playState.m_nNextOrder;
					memory.Reset();

					playState.m_nCurrentOrder = playState.m_nNextOrder;
					playState.m_nPattern = orderList[playState.m_nCurrentOrder];
					playState.m_nNextRow = playState.m_nRow;
					break;
				}
			}
		}
		if(playState.m_nNextOrder == ORDERINDEX_INVALID)
		{
			// GetFirstUnvisitedRow failed, so there is nothing more to play
			break;
		}

		// Skip non-existing patterns
		if(!Patterns.IsValidPat(playState.m_nPattern))
		{
			// If there isn't even a tune, we should probably stop here.
			if(playState.m_nCurrentOrder == orderList.GetRestartPos())
			{
				if(!hasSearchTarget || !visitedRows.GetFirstUnvisitedRow(playState.m_nNextOrder, playState.m_nRow, true))
				{
					// We aren't searching for a specific row, or we couldn't find any more unvisited rows.
					break;
				} else
				{
					// We haven't found the target row yet, but we found some other unplayed row... continue searching from here.
					retval.duration = memory.elapsedTime;
					results.push_back(retval);
					retval.startRow = playState.m_nRow;
					retval.startOrder = playState.m_nNextOrder;
					memory.Reset();
					playState.m_nNextRow = playState.m_nRow;
					continue;
				}
			}
			playState.m_nNextOrder = playState.m_nCurrentOrder + 1;
			continue;
		}
		// Should never happen
		if(playState.m_nRow >= Patterns[playState.m_nPattern].GetNumRows())
			playState.m_nRow = 0;

		// Check whether target was reached.
		if(target.mode == GetLengthTarget::SeekPosition && playState.m_nCurrentOrder == target.pos.order && playState.m_nRow == target.pos.row)
		{
			retval.targetReached = true;
			break;
		}

		if(visitedRows.IsVisited(playState.m_nCurrentOrder, playState.m_nRow, true))
		{
			if(!hasSearchTarget || !visitedRows.GetFirstUnvisitedRow(playState.m_nNextOrder, playState.m_nRow, true))
			{
				// We aren't searching for a specific row, or we couldn't find any more unvisited rows.
				break;
			} else
			{
				// We haven't found the target row yet, but we found some other unplayed row... continue searching from here.
				retval.duration = memory.elapsedTime;
				results.push_back(retval);
				retval.startRow = playState.m_nRow;
				retval.startOrder = playState.m_nNextOrder;
				memory.Reset();
				playState.m_nNextRow = playState.m_nRow;
				continue;
			}
		}

		retval.endOrder = playState.m_nCurrentOrder;
		retval.endRow = playState.m_nRow;

		// Update next position
		playState.m_nNextRow = playState.m_nRow + 1;

		// Jumped to invalid pattern row?
		if(playState.m_nRow >= Patterns[playState.m_nPattern].GetNumRows())
		{
			playState.m_nRow = 0;
		}
		// New pattern?
		if(!playState.m_nRow)
		{
			for(CHANNELINDEX chn = 0; chn < GetNumChannels(); chn++)
			{
				memory.chnSettings[chn].patLoop = memory.elapsedTime;
				memory.chnSettings[chn].patLoopSmp = playState.m_lTotalSampleCount;
			}
		}

		// For various effects, we need to know first how many ticks there are in this row.
		const ModCommand *p = Patterns[playState.m_nPattern].GetpModCommand(playState.m_nRow, 0);
		for(CHANNELINDEX nChn = 0; nChn < GetNumChannels(); nChn++, p++)
		{
			ModChannel &chn = playState.Chn[nChn];
			if(m_playBehaviour[kST3NoMutedChannels] && ChnSettings[nChn].dwFlags[CHN_MUTE])	// not even effects are processed on muted S3M channels
				continue;
			if(p->IsPcNote())
			{
#ifndef NO_PLUGINS
				if((adjustMode & eAdjust) && p->instr > 0 && p->instr <= MAX_MIXPLUGINS)
				{
					memory.plugParams[std::make_pair(p->instr, p->GetValueVolCol())] = p->GetValueEffectCol();
				}
#endif // NO_PLUGINS
				chn.rowCommand.Clear();
				continue;
			}
			chn.rowCommand = *p;
			switch(p->command)
			{
			case CMD_SPEED:
				SetSpeed(playState, p->param);
				break;

			case CMD_TEMPO:
				if(m_playBehaviour[kMODVBlankTiming])
				{
					// ProTracker MODs with VBlank timing: All Fxx parameters set the tick count.
					if(p->param != 0) SetSpeed(playState, p->param);
				}
				break;

			case CMD_S3MCMDEX:
				if((p->param & 0xF0) == 0x60)
				{
					// Fine Pattern Delay
					tickDelay += (p->param & 0x0F);
				} else if((p->param & 0xF0) == 0xE0 && !rowDelay)
				{
					// Pattern Delay
					if(!(GetType() & MOD_TYPE_S3M) || (p->param & 0x0F) != 0)
					{
						// While Impulse Tracker *does* count S60 as a valid row delay (and thus ignores any other row delay commands on the right),
						// Scream Tracker 3 simply ignores such commands.
						rowDelay = 1 + (p->param & 0x0F);
					}
				}
				break;

			case CMD_MODCMDEX:
				if((p->param & 0xF0) == 0xE0)
				{
					// Pattern Delay
					rowDelay = 1 + (p->param & 0x0F);
				}
				break;
			}
		}
		if(rowDelay == 0) rowDelay = 1;
		const uint32 numTicks = (playState.m_nMusicSpeed + tickDelay) * rowDelay;
		const uint32 nonRowTicks = numTicks - rowDelay;

		for(CHANNELINDEX nChn = 0; nChn < GetNumChannels(); nChn++)
		{
			ModChannel &chn = playState.Chn[nChn];
			if(chn.rowCommand.IsEmpty())
				continue;
			if(m_playBehaviour[kST3NoMutedChannels] && ChnSettings[nChn].dwFlags[CHN_MUTE])	// not even effects are processed on muted S3M channels
				continue;
			ModCommand::COMMAND command = chn.rowCommand.command;
			ModCommand::PARAM param = chn.rowCommand.param;
			ModCommand::NOTE note = chn.rowCommand.note;

			if (chn.rowCommand.instr)
			{
				chn.nNewIns = chn.rowCommand.instr;
				chn.nLastNote = NOTE_NONE;
				memory.chnSettings[nChn].vol = 0xFF;
			}
			if (chn.rowCommand.IsNote()) chn.nLastNote = note;

			// Update channel panning
			if(chn.rowCommand.IsNote() || chn.rowCommand.instr)
			{
				SAMPLEINDEX smp = 0;
				if(GetNumInstruments())
				{
					ModInstrument *pIns;
					if(chn.nNewIns <= GetNumInstruments() && (pIns = Instruments[chn.nNewIns]) != nullptr)
					{
						if(pIns->dwFlags[INS_SETPANNING])
							chn.SetInstrumentPan(pIns->nPan, *this);
						if(ModCommand::IsNote(note))
							smp = pIns->Keyboard[note - NOTE_MIN];
					}
				} else
				{
					smp = chn.nNewIns;
				}
				if(smp > 0 && smp <= GetNumSamples())
				{
					if(Samples[smp].uFlags[CHN_PANNING])
						chn.SetInstrumentPan(Samples[smp].nPan, *this);
					if(Samples[smp].uFlags[CHN_ADLIB])
					{
						memory.state->Chn[nChn].Stop();
						memory.chnSettings[nChn].ticksToRender = 0;
					}
				}
			}

			switch(chn.rowCommand.volcmd)
			{
			case VOLCMD_VOLUME:
				memory.chnSettings[nChn].vol = chn.rowCommand.vol;
				break;
			case VOLCMD_VOLSLIDEUP:
			case VOLCMD_VOLSLIDEDOWN:
				if(chn.rowCommand.vol != 0)
					chn.nOldVolParam = chn.rowCommand.vol;
				break;
			case VOLCMD_TONEPORTAMENTO:
				if(chn.rowCommand.vol)
				{
					const auto [porta, clearEffectCommand] = GetVolCmdTonePorta(chn.rowCommand, 0);
					chn.nPortamentoSlide = porta * 4;
					if(clearEffectCommand)
						command = CMD_NONE;
				}
				break;
			}

			switch(command)
			{
			// Position Jump
			case CMD_POSITIONJUMP:
				positionJumpOnThisRow = positionJumpRightOfPatternLoop = true;
				playState.m_nNextOrder = static_cast<ORDERINDEX>(CalculateXParam(playState.m_nPattern, playState.m_nRow, nChn));
				playState.m_nNextPatStartRow = 0;  // FT2 E60 bug
				// see https://forum.openmpt.org/index.php?topic=2769.0 - FastTracker resets Dxx if Bxx is called _after_ Dxx
				// Test case: PatternJump.mod
				if(!patternBreakOnThisRow || (GetType() & (MOD_TYPE_MOD | MOD_TYPE_XM)))
					playState.m_nNextRow = 0;

				if (adjustMode & eAdjust)
				{
					chn.nPatternLoopCount = 0;
					chn.nPatternLoop = 0;
				}
				break;
			// Pattern Break
			case CMD_PATTERNBREAK:
				{
					ROWINDEX row = PatternBreak(playState, nChn, param);
					if(row != ROWINDEX_INVALID)
					{
						patternBreakOnThisRow = true;
						playState.m_nNextRow = row;

						if(!positionJumpOnThisRow)
						{
							playState.m_nNextOrder = playState.m_nCurrentOrder + 1;
						}
						if(adjustMode & eAdjust)
						{
							chn.nPatternLoopCount = 0;
							chn.nPatternLoop = 0;
						}
					}
				}
				break;
			// Set Tempo
			case CMD_TEMPO:
				if(!m_playBehaviour[kMODVBlankTiming])
				{
					TEMPO tempo(CalculateXParam(playState.m_nPattern, playState.m_nRow, nChn), 0);
					if ((adjustMode & eAdjust) && (GetType() & (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT)))
					{
						if (tempo.GetInt()) chn.nOldTempo = static_cast<uint8>(tempo.GetInt()); else tempo.Set(chn.nOldTempo);
					}

					if (tempo.GetInt() >= 0x20) playState.m_nMusicTempo = tempo;
					else
					{
						// Tempo Slide
						TEMPO tempoDiff((tempo.GetInt() & 0x0F) * nonRowTicks, 0);
						if ((tempo.GetInt() & 0xF0) == 0x10)
						{
							playState.m_nMusicTempo += tempoDiff;
						} else
						{
							if(tempoDiff < playState.m_nMusicTempo)
								playState.m_nMusicTempo -= tempoDiff;
							else
								playState.m_nMusicTempo.Set(0);
						}
					}

					TEMPO tempoMin = GetModSpecifications().GetTempoMin(), tempoMax = GetModSpecifications().GetTempoMax();
					if(m_playBehaviour[kTempoClamp])	// clamp tempo correctly in compatible mode
					{
						tempoMax.Set(255);
					}
					Limit(playState.m_nMusicTempo, tempoMin, tempoMax);
				}
				break;

			case CMD_S3MCMDEX:
				switch(param & 0xF0)
				{
				case 0x90:
					if(param <= 0x91)
					{
						chn.dwFlags.set(CHN_SURROUND, param == 0x91);
					}
					break;

				case 0xA0:
					// High sample offset
					chn.nOldHiOffset = param & 0x0F;
					break;
				
				case 0xB0:
					// Pattern Loop
					{
						positionJumpRightOfPatternLoop = false;
						CHANNELINDEX firstChn = nChn, lastChn = nChn;
						if(GetType() == MOD_TYPE_S3M)
						{
							// ST3 has only one global loop memory.
							firstChn = 0;
							lastChn = GetNumChannels() - 1;
						}
						if(param & 0x0F)
						{
							if(m_playBehaviour[kITPatternLoopTargetReset] || (GetType() == MOD_TYPE_S3M))
							{
								for(CHANNELINDEX c = firstChn; c <= lastChn; c++)
								{
									playState.Chn[c].nPatternLoop = playState.m_nRow + 1;
								}
							}
							patternLoopEndedOnThisRow = true;
						}
						else
						{
							for(CHANNELINDEX c = firstChn; c <= lastChn; c++)
							{
								memory.chnSettings[c].patLoop = memory.elapsedTime;
								memory.chnSettings[c].patLoopSmp = playState.m_lTotalSampleCount;
								memory.chnSettings[c].patLoopStart = playState.m_nRow;
								playState.Chn[c].nPatternLoop = playState.m_nRow;
							}
							patternLoopStartedOnThisRow = true;
						}
					}
					break;

				case 0xF0:
					// Active macro
					chn.nActiveMacro = param & 0x0F;
					break;
				}
				break;

			case CMD_MODCMDEX:
				switch(param & 0xF0)
				{
				case 0x60:
					// Pattern Loop
					if (param & 0x0F)
					{
						playState.m_nNextPatStartRow = memory.chnSettings[nChn].patLoopStart; // FT2 E60 bug
						patternLoopEndedOnThisRow = true;
					} else
					{
						patternLoopStartedOnThisRow = true;
						memory.chnSettings[nChn].patLoop = memory.elapsedTime;
						memory.chnSettings[nChn].patLoopSmp = playState.m_lTotalSampleCount;
						memory.chnSettings[nChn].patLoopStart = playState.m_nRow;
						playState.Chn[nChn].nPatternLoop = playState.m_nRow;
					}
					break;

				case 0xF0:
					// Active macro
					chn.nActiveMacro = param & 0x0F;
					break;
				}
				break;

			case CMD_XFINEPORTAUPDOWN:
				// ignore high offset in compatible mode
				if(((param & 0xF0) == 0xA0) && !m_playBehaviour[kFT2RestrictXCommand]) chn.nOldHiOffset = param & 0x0F;
				break;
			}

			// The following calculations are not interesting if we just want to get the song length.
			if (!(adjustMode & eAdjust)) continue;
			switch(command)
			{
			// Portamento Up/Down
			case CMD_PORTAMENTOUP:
				if(param)
				{
					// FT2 compatibility: Separate effect memory for all portamento commands
					// Test case: Porta-LinkMem.xm
					if(!m_playBehaviour[kFT2PortaUpDownMemory])
						chn.nOldPortaDown = param;
					chn.nOldPortaUp = param;
				}
				break;
			case CMD_PORTAMENTODOWN:
				if(param)
				{
					// FT2 compatibility: Separate effect memory for all portamento commands
					// Test case: Porta-LinkMem.xm
					if(!m_playBehaviour[kFT2PortaUpDownMemory])
						chn.nOldPortaUp = param;
					chn.nOldPortaDown = param;
				}
				break;
			// Tone-Portamento
			case CMD_TONEPORTAMENTO:
				if (param) chn.nPortamentoSlide = param << 2;
				break;
			// Offset
			case CMD_OFFSET:
				if (param) chn.oldOffset = param << 8;
				break;
			// Volume Slide
			case CMD_VOLUMESLIDE:
			case CMD_TONEPORTAVOL:
				if (param) chn.nOldVolumeSlide = param;
				break;
			// Set Volume
			case CMD_VOLUME:
				memory.chnSettings[nChn].vol = param;
				break;
			// Global Volume
			case CMD_GLOBALVOLUME:
				if(!(GetType() & GLOBALVOL_7BIT_FORMATS) && param < 128) param *= 2;
				// IT compatibility 16. ST3 and IT ignore out-of-range values
				if(param <= 128)
				{
					playState.m_nGlobalVolume = param * 2;
				} else if(!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_S3M)))
				{
					playState.m_nGlobalVolume = 256;
				}
				break;
			// Global Volume Slide
			case CMD_GLOBALVOLSLIDE:
				if(m_playBehaviour[kPerChannelGlobalVolSlide])
				{
					// IT compatibility 16. Global volume slide params are stored per channel (FT2/IT)
					if (param) chn.nOldGlobalVolSlide = param; else param = chn.nOldGlobalVolSlide;
				} else
				{
					if (param) playState.Chn[0].nOldGlobalVolSlide = param; else param = playState.Chn[0].nOldGlobalVolSlide;
				}
				if (((param & 0x0F) == 0x0F) && (param & 0xF0))
				{
					param >>= 4;
					if (!(GetType() & GLOBALVOL_7BIT_FORMATS)) param <<= 1;
					playState.m_nGlobalVolume += param << 1;
				} else if (((param & 0xF0) == 0xF0) && (param & 0x0F))
				{
					param = (param & 0x0F) << 1;
					if (!(GetType() & GLOBALVOL_7BIT_FORMATS)) param <<= 1;
					playState.m_nGlobalVolume -= param;
				} else if (param & 0xF0)
				{
					param >>= 4;
					param <<= 1;
					if (!(GetType() & GLOBALVOL_7BIT_FORMATS)) param <<= 1;
					playState.m_nGlobalVolume += param * nonRowTicks;
				} else
				{
					param = (param & 0x0F) << 1;
					if (!(GetType() & GLOBALVOL_7BIT_FORMATS)) param <<= 1;
					playState.m_nGlobalVolume -= param * nonRowTicks;
				}
				Limit(playState.m_nGlobalVolume, 0, 256);
				break;
			case CMD_CHANNELVOLUME:
				if (param <= 64) chn.nGlobalVol = param;
				break;
			case CMD_CHANNELVOLSLIDE:
				{
					if (param) chn.nOldChnVolSlide = param; else param = chn.nOldChnVolSlide;
					int32 volume = chn.nGlobalVol;
					if((param & 0x0F) == 0x0F && (param & 0xF0))
						volume += (param >> 4);		// Fine Up
					else if((param & 0xF0) == 0xF0 && (param & 0x0F))
						volume -= (param & 0x0F);	// Fine Down
					else if(param & 0x0F)			// Down
						volume -= (param & 0x0F) * nonRowTicks;
					else							// Up
						volume += ((param & 0xF0) >> 4) * nonRowTicks;
					Limit(volume, 0, 64);
					chn.nGlobalVol = volume;
				}
				break;
			case CMD_PANNING8:
				Panning(chn, param, Pan8bit);
				break;
			case CMD_MODCMDEX:
				if(param < 0x10)
				{
					// LED filter
					for(CHANNELINDEX channel = 0; channel < GetNumChannels(); channel++)
					{
						playState.Chn[channel].dwFlags.set(CHN_AMIGAFILTER, !(param & 1));
					}
				}
				[[fallthrough]];
			case CMD_S3MCMDEX:
				if((param & 0xF0) == 0x80)
				{
					Panning(chn, (param & 0x0F), Pan4bit);
				}
				break;

			case CMD_VIBRATOVOL:
				if (param) chn.nOldVolumeSlide = param;
				param = 0;
				[[fallthrough]];
			case CMD_VIBRATO:
				Vibrato(chn, param);
				break;
			case CMD_FINEVIBRATO:
				FineVibrato(chn, param);
				break;
			case CMD_TREMOLO:
				Tremolo(chn, param);
				break;
			case CMD_PANBRELLO:
				Panbrello(chn, param);
				break;
			}

			switch(chn.rowCommand.volcmd)
			{
			case VOLCMD_PANNING:
				Panning(chn, chn.rowCommand.vol, Pan6bit);
				break;

			case VOLCMD_VIBRATOSPEED:
				// FT2 does not automatically enable vibrato with the "set vibrato speed" command
				if(m_playBehaviour[kFT2VolColVibrato])
					chn.nVibratoSpeed = chn.rowCommand.vol & 0x0F;
				else
					Vibrato(chn, chn.rowCommand.vol << 4);
				break;
			case VOLCMD_VIBRATODEPTH:
				Vibrato(chn, chn.rowCommand.vol);
				break;
			}

			// Process vibrato / tremolo / panbrello
			switch(chn.rowCommand.command)
			{
			case CMD_VIBRATO:
			case CMD_FINEVIBRATO:
			case CMD_VIBRATOVOL:
				if(adjustMode & eAdjust)
				{
					uint32 vibTicks = ((GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) && !m_SongFlags[SONG_ITOLDEFFECTS]) ? numTicks : nonRowTicks;
					uint32 inc = chn.nVibratoSpeed * vibTicks;
					if(m_playBehaviour[kITVibratoTremoloPanbrello])
						inc *= 4;
					chn.nVibratoPos += static_cast<uint8>(inc);
				}
				break;

			case CMD_TREMOLO:
				if(adjustMode & eAdjust)
				{
					uint32 tremTicks = ((GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) && !m_SongFlags[SONG_ITOLDEFFECTS]) ? numTicks : nonRowTicks;
					uint32 inc = chn.nTremoloSpeed * tremTicks;
					if(m_playBehaviour[kITVibratoTremoloPanbrello])
						inc *= 4;
					chn.nTremoloPos += static_cast<uint8>(inc);
				}
				break;

			case CMD_PANBRELLO:
				if(adjustMode & eAdjust)
				{
					// Panbrello effect is permanent in compatible mode, so actually apply panbrello for the last tick of this row
					chn.nPanbrelloPos += static_cast<uint8>(chn.nPanbrelloSpeed * (numTicks - 1));
					ProcessPanbrello(chn);
				}
				break;
			}
		}

		// Interpret F00 effect in XM files as "stop song"
		if(GetType() == MOD_TYPE_XM && playState.m_nMusicSpeed == uint16_max)
		{
			break;
		}

		playState.m_nCurrentRowsPerBeat = m_nDefaultRowsPerBeat;
		if(Patterns[playState.m_nPattern].GetOverrideSignature())
		{
			playState.m_nCurrentRowsPerBeat = Patterns[playState.m_nPattern].GetRowsPerBeat();
		}

		const uint32 tickDuration = GetTickDuration(playState);
		const uint32 rowDuration = tickDuration * numTicks;
		memory.elapsedTime += static_cast<double>(rowDuration) / static_cast<double>(m_MixerSettings.gdwMixingFreq);
		playState.m_lTotalSampleCount += rowDuration;

		if(adjustSamplePos)
		{
			// Super experimental and dirty sample seeking
			for(CHANNELINDEX nChn = 0; nChn < GetNumChannels(); nChn++)
			{
				if(memory.chnSettings[nChn].ticksToRender == GetLengthMemory::IGNORE_CHANNEL)
					continue;

				ModChannel &chn = playState.Chn[nChn];
				const ModCommand &m = chn.rowCommand;
				uint32 paramHi = m.param >> 4, paramLo = m.param & 0x0F;
				uint32 startTick = 0;
				bool porta = m.command == CMD_TONEPORTAMENTO || m.command == CMD_TONEPORTAVOL || m.volcmd == VOLCMD_TONEPORTAMENTO;
				bool stopNote = patternLoopStartedOnThisRow;	// It's too much trouble to keep those pattern loops in sync...

				if(m.instr) chn.prevNoteOffset = 0;
				if(m.IsNote())
				{
					if(porta && memory.chnSettings[nChn].incChanged)
					{
						// If there's a portamento, the current channel increment mustn't be 0 in NoteChange()
						chn.increment = GetChannelIncrement(chn, chn.nPeriod, 0);
					}
					int32 setPan = chn.nPan;
					chn.nNewNote = chn.nLastNote;
					if(chn.nNewIns != 0) InstrumentChange(chn, chn.nNewIns, porta);
					NoteChange(chn, m.note, porta);
					memory.chnSettings[nChn].incChanged = true;

					if((m.command == CMD_MODCMDEX || m.command == CMD_S3MCMDEX) && (m.param & 0xF0) == 0xD0 && paramLo < numTicks)
					{
						startTick = paramLo;
					} else if(m.command == CMD_DELAYCUT && paramHi < numTicks)
					{
						startTick = paramHi;
					}
					if(rowDelay > 1 && startTick != 0 && (GetType() & (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT)))
					{
						startTick += (playState.m_nMusicSpeed + tickDelay) * (rowDelay - 1);
					}
					if(!porta) memory.chnSettings[nChn].ticksToRender = 0;

					// Panning commands have to be re-applied after a note change with potential pan change.
					if(m.command == CMD_PANNING8
						|| ((m.command == CMD_MODCMDEX || m.command == CMD_S3MCMDEX) && paramHi == 0x8)
						|| m.volcmd == VOLCMD_PANNING)
					{
						chn.nPan = setPan;
					}

					if(m.command == CMD_OFFSET)
					{
						bool isExtended = false;
						SmpLength offset = CalculateXParam(playState.m_nPattern, playState.m_nRow, nChn, &isExtended);
						if(!isExtended)
						{
							offset <<= 8;
							if(offset == 0) offset = chn.oldOffset;
							offset += static_cast<SmpLength>(chn.nOldHiOffset) << 16;
						}
						SampleOffset(chn, offset);
					} else if(m.command == CMD_OFFSETPERCENTAGE)
					{
						SampleOffset(chn, Util::muldiv_unsigned(chn.nLength, m.param, 255));
					} else if(m.command == CMD_REVERSEOFFSET && chn.pModSample != nullptr)
					{
						memory.RenderChannel(nChn, oldTickDuration);	// Re-sync what we've got so far
						ReverseSampleOffset(chn, m.param);
						startTick = playState.m_nMusicSpeed - 1;
					} else if(m.volcmd == VOLCMD_OFFSET)
					{
						if(m.vol <= CountOf(chn.pModSample->cues) && chn.pModSample != nullptr)
						{
							SmpLength offset;
							if(m.vol == 0)
								offset = chn.oldOffset;
							else
								offset = chn.oldOffset = chn.pModSample->cues[m.vol - 1];
							SampleOffset(chn, offset);
						}
					}
				}

				if(m.note == NOTE_KEYOFF || m.note == NOTE_NOTECUT || (m.note == NOTE_FADE && GetNumInstruments())
					|| ((m.command == CMD_MODCMDEX || m.command == CMD_S3MCMDEX) && (m.param & 0xF0) == 0xC0 && paramLo < numTicks)
					|| (m.command == CMD_DELAYCUT && paramLo != 0 && startTick + paramLo < numTicks)
					|| m.command == CMD_KEYOFF)
				{
					stopNote = true;
				}

				if(m.command == CMD_VOLUME)
				{
					chn.nVolume = m.param * 4;
				} else if(m.volcmd == VOLCMD_VOLUME)
				{
					chn.nVolume = m.vol * 4;
				}
				
				if(chn.pModSample && !stopNote)
				{
					// Check if we don't want to emulate some effect and thus stop processing.
					if(m.command < MAX_EFFECTS)
					{
						if(forbiddenCommands[m.command])
						{
							stopNote = true;
						} else if(m.command == CMD_MODCMDEX)
						{
							// Special case: Slides using extended commands
							switch(m.param & 0xF0)
							{
							case 0x10:
							case 0x20:
								stopNote = true;
							}
						}
					}

					if(m.volcmd < forbiddenVolCommands.size() && forbiddenVolCommands[m.volcmd])
					{
						stopNote = true;
					}
				}

				if(stopNote)
				{
					chn.Stop();
					memory.chnSettings[nChn].ticksToRender = 0;
				} else
				{
					if(oldTickDuration != tickDuration && oldTickDuration != 0)
					{
						memory.RenderChannel(nChn, oldTickDuration);	// Re-sync what we've got so far
					}

					switch(m.command)
					{
					case CMD_TONEPORTAVOL:
					case CMD_VOLUMESLIDE:
					case CMD_VIBRATOVOL:
						if(m.param || (GetType() != MOD_TYPE_MOD))
						{
							for(uint32 i = 0; i < numTicks; i++)
							{
								chn.isFirstTick = (i == 0);
								VolumeSlide(chn, m.param);
							}
						}
						break;

					case CMD_MODCMDEX:
						if((m.param & 0x0F) || (GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2)))
						{
							chn.isFirstTick = true;
							switch(m.param & 0xF0)
							{
							case 0xA0: FineVolumeUp(chn, m.param & 0x0F, false); break;
							case 0xB0: FineVolumeDown(chn, m.param & 0x0F, false); break;
							}
						}
						break;

					case CMD_S3MCMDEX:
						if(m.param == 0x9E)
						{
							// Play forward
							memory.RenderChannel(nChn, oldTickDuration);	// Re-sync what we've got so far
							chn.dwFlags.reset(CHN_PINGPONGFLAG);
						} else if(m.param == 0x9F)
						{
							// Reverse
							memory.RenderChannel(nChn, oldTickDuration);	// Re-sync what we've got so far
							chn.dwFlags.set(CHN_PINGPONGFLAG);
							if(!chn.position.GetInt() && chn.nLength && (m.IsNote() || !chn.dwFlags[CHN_LOOP]))
							{
								chn.position.Set(chn.nLength - 1, SamplePosition::fractMax);
							}
						} else if((m.param & 0xF0) == 0x70)
						{
							// TODO
							//ExtendedS3MCommands(nChn, param);
						}
						break;
					}
					chn.isFirstTick = true;
					switch(m.volcmd)
					{
					case VOLCMD_FINEVOLUP:		FineVolumeUp(chn, m.vol, m_playBehaviour[kITVolColMemory]); break;
					case VOLCMD_FINEVOLDOWN:	FineVolumeDown(chn, m.vol, m_playBehaviour[kITVolColMemory]); break;
					case VOLCMD_VOLSLIDEUP:
					case VOLCMD_VOLSLIDEDOWN:
						{
							// IT Compatibility: Volume column volume slides have their own memory
							// Test case: VolColMemory.it
							ModCommand::VOL vol = m.vol;
							if(vol == 0 && m_playBehaviour[kITVolColMemory])
							{
								vol = chn.nOldVolParam;
								if(vol == 0)
									break;
							}
							if(m.volcmd == VOLCMD_VOLSLIDEUP)
								vol <<= 4;
							for(uint32 i = 0; i < numTicks; i++)
							{
								chn.isFirstTick = (i == 0);
								VolumeSlide(chn, vol);
							}
						}
						break;
					}

					if(porta)
					{
						// Portamento needs immediate syncing, as the pitch changes on each tick
						uint32 portaTick = memory.chnSettings[nChn].ticksToRender + startTick + 1;
						memory.chnSettings[nChn].ticksToRender += numTicks;
						memory.RenderChannel(nChn, tickDuration, portaTick);
					} else
					{
						memory.chnSettings[nChn].ticksToRender += (numTicks - startTick);
					}
				}
			}
		}
		oldTickDuration = tickDuration;

		// Pattern loop is not executed in FT2 if there are any position jump or pattern break commands on the same row.
		// Pattern loop is not executed in IT if there are any position jump commands to the right (and to the left in older OpenMPT versions).
		// Test case for FT2 exception: PatLoop-Jumps.xm, PatLoop-Various.xm
		// Test case for IT: exception: LoopBreak.it, sbx-priority.it
		if(patternLoopEndedOnThisRow
		   && (!m_playBehaviour[kFT2PatternLoopWithJumps] || !(positionJumpOnThisRow || patternBreakOnThisRow))
		   && (!m_playBehaviour[kITPatternLoopWithJumpsOld] || !positionJumpOnThisRow)
		   && (!m_playBehaviour[kITPatternLoopWithJumps] || !positionJumpRightOfPatternLoop))
		{
			std::map<double, int> startTimes;
			// This is really just a simple estimation for nested pattern loops. It should handle cases correctly where all parallel loops start and end on the same row.
			// If one of them starts or ends "in between", it will most likely calculate a wrong duration.
			// For S3M files, it's also way off.
			for(CHANNELINDEX nChn = 0; nChn < GetNumChannels(); nChn++)
			{
				ModChannel &chn = playState.Chn[nChn];
				ModCommand::COMMAND command = chn.rowCommand.command;
				ModCommand::PARAM param = chn.rowCommand.param;
				if((command == CMD_S3MCMDEX && param >= 0xB1 && param <= 0xBF)
					|| (command == CMD_MODCMDEX && param >= 0x61 && param <= 0x6F))
				{
					const double start = memory.chnSettings[nChn].patLoop;
					if(!startTimes[start]) startTimes[start] = 1;
					startTimes[start] = std::lcm(startTimes[start], 1 + (param & 0x0F));
				}
			}
			for(const auto &i : startTimes)
			{
				memory.elapsedTime += (memory.elapsedTime - i.first) * (double)(i.second - 1);
				for(CHANNELINDEX nChn = 0; nChn < GetNumChannels(); nChn++)
				{
					if(memory.chnSettings[nChn].patLoop == i.first)
					{
						playState.m_lTotalSampleCount += (playState.m_lTotalSampleCount - memory.chnSettings[nChn].patLoopSmp) * (i.second - 1);
						if(m_playBehaviour[kITPatternLoopTargetReset] || (GetType() == MOD_TYPE_S3M))
						{
							memory.chnSettings[nChn].patLoop = memory.elapsedTime;
							memory.chnSettings[nChn].patLoopSmp = playState.m_lTotalSampleCount;
							memory.chnSettings[nChn].patLoopStart = playState.m_nRow + 1;
						}
						break;
					}
				}
			}
			if(GetType() == MOD_TYPE_IT)
			{
				// IT pattern loop start row update - at the end of a pattern loop, set pattern loop start to next row (for upcoming pattern loops with missing SB0)
				for(CHANNELINDEX nChn = 0; nChn < GetNumChannels(); nChn++)
				{
					ModChannel &chn = playState.Chn[nChn];
					if((chn.rowCommand.command == CMD_S3MCMDEX && chn.rowCommand.param >= 0xB1 && chn.rowCommand.param <= 0xBF))
					{
						memory.chnSettings[nChn].patLoop = memory.elapsedTime;
						memory.chnSettings[nChn].patLoopSmp = playState.m_lTotalSampleCount;
					}
				}
			}
		}
	}

	// Now advance the sample positions for sample seeking on channels that are still playing
	if(adjustSamplePos)
	{
		for(CHANNELINDEX nChn = 0; nChn < GetNumChannels(); nChn++)
		{
			if(memory.chnSettings[nChn].ticksToRender != GetLengthMemory::IGNORE_CHANNEL)
			{
				memory.RenderChannel(nChn, oldTickDuration);
			}
		}
	}

	if(retval.targetReached || target.mode == GetLengthTarget::NoTarget)
	{
		retval.lastOrder = playState.m_nCurrentOrder;
		retval.lastRow = playState.m_nRow;
	}
	retval.duration = memory.elapsedTime;
	results.push_back(retval);

	// Store final variables
	if(adjustMode & eAdjust)
	{
		if(retval.targetReached || target.mode == GetLengthTarget::NoTarget)
		{
			// Target found, or there is no target (i.e. play whole song)...
			m_PlayState = std::move(playState);
			m_PlayState.ResetGlobalVolumeRamping();
			m_PlayState.m_nNextRow = m_PlayState.m_nRow;
			m_PlayState.m_nFrameDelay = m_PlayState.m_nPatternDelay = 0;
			m_PlayState.m_nTickCount = Util::MaxValueOfType(m_PlayState.m_nTickCount) - 1;
			m_PlayState.m_bPositionChanged = true;
			for(CHANNELINDEX n = 0; n < GetNumChannels(); n++)
			{
				if(m_PlayState.Chn[n].nLastNote != NOTE_NONE)
				{
					m_PlayState.Chn[n].nNewNote = m_PlayState.Chn[n].nLastNote;
				}
				if(memory.chnSettings[n].vol != 0xFF && !adjustSamplePos)
				{
					m_PlayState.Chn[n].nVolume = std::min(memory.chnSettings[n].vol, uint8(64)) * 4;
				}
			}
			if(m_opl != nullptr) m_opl->Reset();

#ifndef NO_PLUGINS
			// If there were any PC events, update plugin parameters to their latest value.
			std::bitset<MAX_MIXPLUGINS> plugSetProgram;
			for(const auto &param : memory.plugParams)
			{
				PLUGINDEX plug = param.first.first - 1;
				IMixPlugin *plugin = m_MixPlugins[plug].pMixPlugin;
				if(plugin != nullptr)
				{
					if(!plugSetProgram[plug])
					{
						// Used for bridged plugins to avoid sending out individual messages for each parameter.
						plugSetProgram.set(plug);
						plugin->BeginSetProgram();
					}
					plugin->SetParameter(param.first.second, param.second / PlugParamValue(ModCommand::maxColumnValue));
				}
			}
			if(plugSetProgram.any())
			{
				for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
				{
					if(plugSetProgram[i])
					{
						m_MixPlugins[i].pMixPlugin->EndSetProgram();
					}
				}
			}
#endif // NO_PLUGINS
		} else if(adjustMode != eAdjustOnSuccess)
		{
			// Target not found (e.g. when jumping to a hidden sub song), reset global variables...
			m_PlayState.m_nMusicSpeed = m_nDefaultSpeed;
			m_PlayState.m_nMusicTempo = m_nDefaultTempo;
			m_PlayState.m_nGlobalVolume = m_nDefaultGlobalVolume;
		}
		// When adjusting the playback status, we will also want to update the visited rows vector according to the current position.
		if(sequence != Order.GetCurrentSequenceIndex())
		{
			Order.SetSequence(sequence);
		}
		visitedSongRows.MoveVisitedRowsFrom(visitedRows);
	}

	return results;

}


//////////////////////////////////////////////////////////////////////////////////////////////////
// Effects

// Change sample or instrument number.
void CSoundFile::InstrumentChange(ModChannel &chn, uint32 instr, bool bPorta, bool bUpdVol, bool bResetEnv) const
{
	const ModInstrument *pIns = instr <= GetNumInstruments() ? Instruments[instr] : nullptr;
	const ModSample *pSmp = &Samples[instr];
	const auto oldInsVol = chn.nInsVol;
	ModCommand::NOTE note = chn.nNewNote;

	if(note == NOTE_NONE && m_playBehaviour[kITInstrWithoutNote]) return;

	if(pIns != nullptr && ModCommand::IsNote(note))
	{
		// Impulse Tracker ignores empty slots.
		// We won't ignore them if a plugin is assigned to this slot, so that VSTis still work as intended.
		// Test case: emptyslot.it, PortaInsNum.it, gxsmp.it, gxsmp2.it
		if(pIns->Keyboard[note - NOTE_MIN] == 0 && m_playBehaviour[kITEmptyNoteMapSlot] && !pIns->HasValidMIDIChannel())
		{
			chn.pModInstrument = pIns;
			return;
		}

		if(pIns->NoteMap[note - NOTE_MIN] > NOTE_MAX) return;
		uint32 n = pIns->Keyboard[note - NOTE_MIN];
		pSmp = ((n) && (n < MAX_SAMPLES)) ? &Samples[n] : nullptr;
	} else if(GetNumInstruments())
	{
		// No valid instrument, or not a valid note.
		if (note >= NOTE_MIN_SPECIAL) return;
		if(m_playBehaviour[kITEmptyNoteMapSlot] && (pIns == nullptr || !pIns->HasValidMIDIChannel()))
		{
			// Impulse Tracker ignores empty slots.
			// We won't ignore them if a plugin is assigned to this slot, so that VSTis still work as intended.
			// Test case: emptyslot.it, PortaInsNum.it, gxsmp.it, gxsmp2.it
			chn.pModInstrument = nullptr;
			chn.nNewIns = 0;
			return;
		}
		pSmp = nullptr;
	}

	bool returnAfterVolumeAdjust = false;

	// instrumentChanged is used for IT carry-on env option
	bool instrumentChanged = (pIns != chn.pModInstrument);
	const bool sampleChanged = (chn.pModSample != nullptr) && (pSmp != chn.pModSample);

	const bool newTuning = (GetType() == MOD_TYPE_MPT && pIns && pIns->pTuning);
	// Playback behavior change for MPT: With portamento don't change sample if it is in
	// the same instrument as previous sample.
	if(bPorta && newTuning && pIns == chn.pModInstrument && sampleChanged)
		return;

	if(sampleChanged && bPorta)
	{
		// IT compatibility: No sample change (also within multi-sample instruments) during portamento when using Compatible Gxx.
		// Test case: PortaInsNumCompat.it, PortaSampleCompat.it, PortaCutCompat.it
		if(m_playBehaviour[kITPortamentoInstrument] && m_SongFlags[SONG_ITCOMPATGXX] && !chn.increment.IsZero())
		{
			pSmp = chn.pModSample;
		}

		// Special XM hack (also applies to MOD / S3M, except when playing IT-style S3Ms, such as k_vision.s3m)
		// Test case: PortaSmpChange.mod, PortaSmpChange.s3m, PortaSwap.s3m
		if((!instrumentChanged && (GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2)) && pIns)
			|| (GetType() == MOD_TYPE_PLM)
			|| (GetType() == MOD_TYPE_MOD && chn.IsSamplePlaying())
			|| (m_playBehaviour[kST3PortaSampleChange] && chn.IsSamplePlaying()))
		{
			// FT2 doesn't change the sample in this case,
			// but still uses the sample info from the old one (bug?)
			returnAfterVolumeAdjust = true;
		}
	}
	// IT compatibility: A lone instrument number should only reset sample properties to those of the corresponding sample in instrument mode.
	// C#5 01 ... <-- sample 1
	// C-5 .. g02 <-- sample 2
	// ... 01 ... <-- still sample 1, but with properties of sample 2
	// In the above example, no sample change happens on the second row. In the third row, sample 1 keeps playing but with the
	// volume and panning properties of sample 2.
	// Test case: InstrAfterMultisamplePorta.it
	if(m_nInstruments && !instrumentChanged && sampleChanged && chn.pCurrentSample != nullptr && m_playBehaviour[kITMultiSampleInstrumentNumber] && !chn.rowCommand.IsNote())
	{
		returnAfterVolumeAdjust = true;
	}

	// IT Compatibility: Envelope pickup after SCx cut (but don't do this when working with plugins, or else envelope carry stops working)
	// Test case: cut-carry.it
	if(!chn.IsSamplePlaying() && (GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) && (!pIns || !pIns->HasValidMIDIChannel()))
	{
		instrumentChanged = true;
	}

	// FT2 compatibility: new instrument + portamento = ignore new instrument number, but reload old instrument settings (the world of XM is upside down...)
	// And this does *not* happen if volume column portamento is used together with note delay... (handled in ProcessEffects(), where all the other note delay stuff is.)
	// Test case: porta-delay.xm
	if(instrumentChanged && bPorta && m_playBehaviour[kFT2PortaIgnoreInstr] && (chn.pModInstrument != nullptr || chn.pModSample != nullptr))
	{
		pIns = chn.pModInstrument;
		pSmp = chn.pModSample;
		instrumentChanged = false;
	} else
	{
		chn.pModInstrument = pIns;
	}

	// Update Volume
	if (bUpdVol && (!(GetType() & (MOD_TYPE_MOD | MOD_TYPE_S3M)) || ((pSmp != nullptr && pSmp->HasSampleData()) || chn.HasMIDIOutput())))
	{
		if(pSmp)
		{
			if(!pSmp->uFlags[SMP_NODEFAULTVOLUME])
				chn.nVolume = pSmp->nVolume;
		} else if(pIns && pIns->nMixPlug)
		{
			chn.nVolume = chn.GetVSTVolume();
		} else
		{
			chn.nVolume = 0;
		}
	}

	if(returnAfterVolumeAdjust && sampleChanged && pSmp != nullptr)
	{
		// ProTracker applies new instrument's finetune but keeps the old sample playing.
		// Test case: PortaSwapPT.mod
		if(m_playBehaviour[kMODSampleSwap])
			chn.nFineTune = pSmp->nFineTune;
		// ST3 does it similarly for middle-C speed.
		// Test case: PortaSwap.s3m, SampleSwap.s3m
		if(GetType() == MOD_TYPE_S3M && pSmp->HasSampleData())
			chn.nC5Speed = pSmp->nC5Speed;
	}

	if(returnAfterVolumeAdjust) return;

	// Instrument adjust
	chn.nNewIns = 0;

	// IT Compatiblity: NNA is reset on every note change, not every instrument change (fixes s7xinsnum.it).
	if (pIns && ((!m_playBehaviour[kITNNAReset] && pSmp) || pIns->nMixPlug || instrumentChanged))
		chn.nNNA = pIns->nNNA;

	// Update volume
	chn.UpdateInstrumentVolume(pSmp, pIns);

	// Update panning
	// FT2 compatibility: Only reset panning on instrument numbers, not notes (bUpdVol condition)
	// Test case: PanMemory.xm
	// IT compatibility: Sample and instrument panning is only applied on note change, not instrument change
	// Test case: PanReset.it
	if((bUpdVol || !(GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2))) && !m_playBehaviour[kITPanningReset])
	{
		ApplyInstrumentPanning(chn, pIns, pSmp);
	}

	// Reset envelopes
	if(bResetEnv)
	{
		// Blurb by Storlek (from the SchismTracker code):
		// Conditions experimentally determined to cause envelope reset in Impulse Tracker:
		// - no note currently playing (of course)
		// - note given, no portamento
		// - instrument number given, portamento, compat gxx enabled
		// - instrument number given, no portamento, after keyoff, old effects enabled
		// If someone can enlighten me to what the logic really is here, I'd appreciate it.
		// Seems like it's just a total mess though, probably to get XMs to play right.

		bool reset, resetAlways;

		// IT Compatibility: Envelope reset
		// Test case: EnvReset.it
		if(m_playBehaviour[kITEnvelopeReset])
		{
			const bool insNumber = (instr != 0);
			reset = (!chn.nLength
				|| (insNumber && bPorta && m_SongFlags[SONG_ITCOMPATGXX])
				|| (insNumber && !bPorta && chn.dwFlags[CHN_NOTEFADE | CHN_KEYOFF] && m_SongFlags[SONG_ITOLDEFFECTS]));
			// NOTE: IT2.14 with SB/GUS/etc. output is different. We are going after IT's WAV writer here.
			// For SB/GUS/etc. emulation, envelope carry should only apply when the NNA isn't set to "Note Cut".
			// Test case: CarryNNA.it
			resetAlways = (!chn.nFadeOutVol || instrumentChanged || chn.dwFlags[CHN_KEYOFF]);
		} else
		{
			reset = (!bPorta || !(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_DBM)) || m_SongFlags[SONG_ITCOMPATGXX]
				|| !chn.nLength || (chn.dwFlags[CHN_NOTEFADE] && !chn.nFadeOutVol));
			resetAlways = !(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_DBM)) || instrumentChanged || pIns == nullptr || chn.dwFlags[CHN_KEYOFF | CHN_NOTEFADE];
		}

		if(reset)
		{
			chn.dwFlags.set(CHN_FASTVOLRAMP);
			if(pIns != nullptr)
			{
				if(resetAlways)
				{
					chn.ResetEnvelopes();
				} else
				{
					if(!pIns->VolEnv.dwFlags[ENV_CARRY]) chn.VolEnv.Reset();
					if(!pIns->PanEnv.dwFlags[ENV_CARRY]) chn.PanEnv.Reset();
					if(!pIns->PitchEnv.dwFlags[ENV_CARRY]) chn.PitchEnv.Reset();
				}
			}

			// IT Compatibility: Autovibrato reset
			if(!m_playBehaviour[kITVibratoTremoloPanbrello])
			{
				chn.nAutoVibDepth = 0;
				chn.nAutoVibPos = 0;
			}
		} else if(pIns != nullptr && !pIns->VolEnv.dwFlags[ENV_ENABLED])
		{
			if(m_playBehaviour[kITPortamentoInstrument])
			{
				chn.VolEnv.Reset();
			} else
			{
				chn.ResetEnvelopes();
			}
		}
	}
	// Invalid sample ?
	if(pSmp == nullptr && (pIns == nullptr || !pIns->HasValidMIDIChannel()))
	{
		chn.pModSample = nullptr;
		chn.nInsVol = 0;
		return;
	}

	// Tone-Portamento doesn't reset the pingpong direction flag
	if(bPorta && pSmp == chn.pModSample && pSmp != nullptr)
	{
		// If channel length is 0, we cut a previous sample using SCx. In that case, we have to update sample length, loop points, etc...
		if(GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT) && chn.nLength != 0)
			return;
		// FT2 compatibility: Do not reset key-off status on portamento without instrument number
		// Test case: Off-Porta.xm
		if(GetType() != MOD_TYPE_XM || !m_playBehaviour[kITFT2DontResetNoteOffOnPorta] || chn.rowCommand.instr != 0)
			chn.dwFlags.reset(CHN_KEYOFF | CHN_NOTEFADE);
		chn.dwFlags = (chn.dwFlags & (CHN_CHANNELFLAGS | CHN_PINGPONGFLAG));
	} else //if(!instrumentChanged || chn.rowCommand.instr != 0 || !IsCompatibleMode(TRK_FASTTRACKER2))	// SampleChange.xm?
	{
		chn.dwFlags.reset(CHN_KEYOFF | CHN_NOTEFADE);

		// IT compatibility: Don't change bidi loop direction when no sample nor instrument is changed.
		if((m_playBehaviour[kITPingPongNoReset] || !(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))) && pSmp == chn.pModSample && !instrumentChanged)
			chn.dwFlags = (chn.dwFlags & (CHN_CHANNELFLAGS | CHN_PINGPONGFLAG));
		else
			chn.dwFlags = (chn.dwFlags & CHN_CHANNELFLAGS);

		if(pIns)
		{
			// Copy envelope flags (we actually only need the "enabled" and "pitch" flag)
			chn.VolEnv.flags = pIns->VolEnv.dwFlags;
			chn.PanEnv.flags = pIns->PanEnv.dwFlags;
			chn.PitchEnv.flags = pIns->PitchEnv.dwFlags;

			// A cutoff frequency of 0 should not be reset just because the filter envelope is enabled.
			// Test case: FilterEnvReset.it
			if((pIns->PitchEnv.dwFlags & (ENV_ENABLED | ENV_FILTER)) == (ENV_ENABLED | ENV_FILTER) && !m_playBehaviour[kITFilterBehaviour])
			{
				if(!chn.nCutOff) chn.nCutOff = 0x7F;
			}

			if(pIns->IsCutoffEnabled()) chn.nCutOff = pIns->GetCutoff();
			if(pIns->IsResonanceEnabled()) chn.nResonance = pIns->GetResonance();
		}
	}

	if(pSmp == nullptr)
	{
		chn.pModSample = nullptr;
		chn.nLength = 0;
		return;
	}

	if(bPorta && chn.nLength == 0 && (m_playBehaviour[kFT2PortaNoNote] || m_playBehaviour[kITPortaNoNote]))
	{
		// IT/FT2 compatibility: If the note just stopped on the previous tick, prevent it from restarting.
		// Test cases: PortaJustStoppedNote.xm, PortaJustStoppedNote.it
		chn.increment.Set(0);
	}

	// IT compatibility: Note-off with instrument number + Old Effects retriggers envelopes.
	// If the instrument changes, keep playing the previous sample, but load the new instrument's envelopes.
	// Test case: ResetEnvNoteOffOldFx.it
	if(chn.rowCommand.note == NOTE_KEYOFF && m_playBehaviour[kITInstrWithNoteOffOldEffects] && m_SongFlags[SONG_ITOLDEFFECTS] && sampleChanged)
	{
		if(chn.pModSample)
		{
			chn.dwFlags |= (chn.pModSample->uFlags & CHN_SAMPLEFLAGS);
		}
		chn.nInsVol = oldInsVol;
		chn.nVolume = pSmp->nVolume;
		if(pSmp->uFlags[CHN_PANNING]) chn.SetInstrumentPan(pSmp->nPan, *this);
		return;
	}

	chn.pModSample = pSmp;
	chn.nLength = pSmp->nLength;
	chn.nLoopStart = pSmp->nLoopStart;
	chn.nLoopEnd = pSmp->nLoopEnd;
	// ProTracker "oneshot" loops (if loop start is 0, play the whole sample once and then repeat until loop end)
	if(m_playBehaviour[kMODOneShotLoops] && chn.nLoopStart == 0) chn.nLoopEnd = pSmp->nLength;
	chn.dwFlags |= (pSmp->uFlags & CHN_SAMPLEFLAGS);

	// IT Compatibility: Autovibrato reset
	if(m_playBehaviour[kITVibratoTremoloPanbrello])
	{
		chn.nAutoVibDepth = 0;
		chn.nAutoVibPos = 0;
	}

	if(newTuning)
	{
		chn.nC5Speed = pSmp->nC5Speed;
		chn.m_CalculateFreq = true;
		chn.nFineTune = 0;
	} else if(!bPorta || sampleChanged || !(GetType() & (MOD_TYPE_MOD | MOD_TYPE_XM)))
	{
		// Don't reset finetune changed by "set finetune" command.
		// Test case: finetune.xm, finetune.mod
		// But *do* change the finetune if we switch to a different sample, to fix
		// Miranda`s axe by Jamson (jam007.xm).
		chn.nC5Speed = pSmp->nC5Speed;
		chn.nFineTune = pSmp->nFineTune;
	}

	chn.nTranspose = pSmp->RelativeTone;

	// FT2 compatibility: Don't reset portamento target with new instrument numbers.
	// Test case: Porta-Pickup.xm
	// ProTracker does the same.
	// Test case: PortaTarget.mod
	if(!m_playBehaviour[kFT2PortaTargetNoReset] && GetType() != MOD_TYPE_MOD)
	{
		chn.nPortamentoDest = 0;
	}
	chn.m_PortamentoFineSteps = 0;

	if(chn.dwFlags[CHN_SUSTAINLOOP])
	{
		chn.nLoopStart = pSmp->nSustainStart;
		chn.nLoopEnd = pSmp->nSustainEnd;
		if(chn.dwFlags[CHN_PINGPONGSUSTAIN]) chn.dwFlags.set(CHN_PINGPONGLOOP);
		chn.dwFlags.set(CHN_LOOP);
	}
	if(chn.dwFlags[CHN_LOOP] && chn.nLoopEnd < chn.nLength) chn.nLength = chn.nLoopEnd;

	// Fix sample position on instrument change. This is needed for IT "on the fly" sample change.
	// XXX is this actually called? In ProcessEffects(), a note-on effect is emulated if there's an on the fly sample change!
	if(chn.position.GetUInt() >= chn.nLength)
	{
		if((GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)))
		{
			chn.position.Set(0);
		}
	}
}


void CSoundFile::NoteChange(ModChannel &chn, int note, bool bPorta, bool bResetEnv, bool bManual, CHANNELINDEX channelHint) const
{
	if (note < NOTE_MIN) return;
	const ModSample *pSmp = chn.pModSample;
	const ModInstrument *pIns = chn.pModInstrument;

	const bool newTuning = (GetType() == MOD_TYPE_MPT && pIns != nullptr && pIns->pTuning);
	// save the note that's actually used, as it's necessary to properly calculate PPS and stuff
	const int realnote = note;

	if((pIns) && (note - NOTE_MIN < (int)std::size(pIns->Keyboard)))
	{
		uint32 n = pIns->Keyboard[note - NOTE_MIN];
		if((n) && (n < MAX_SAMPLES))
		{
			pSmp = &Samples[n];
		} else if(m_playBehaviour[kITEmptyNoteMapSlot] && !chn.HasMIDIOutput())
		{
			// Impulse Tracker ignores empty slots.
			// We won't ignore them if a plugin is assigned to this slot, so that VSTis still work as intended.
			// Test case: emptyslot.it, PortaInsNum.it, gxsmp.it, gxsmp2.it
			return;
		}
		note = pIns->NoteMap[note - NOTE_MIN];
	}
	// Key Off
	if(note > NOTE_MAX)
	{
		// Key Off (+ Invalid Note for XM - TODO is this correct?)
		if(note == NOTE_KEYOFF || !(GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT)))
		{
			KeyOff(chn);
			// IT compatibility: Note-off + instrument releases sample sustain but does not release envelopes or fade the instrument
			// Test case: noteoff3.it, ResetEnvNoteOffOldFx2.it
			if(!bPorta && m_playBehaviour[kITInstrWithNoteOffOldEffects] && m_SongFlags[SONG_ITOLDEFFECTS] && chn.rowCommand.instr)
				chn.dwFlags.reset(CHN_NOTEFADE | CHN_KEYOFF);
		} else // Invalid Note -> Note Fade
		{
			if(/*note == NOTE_FADE && */ GetNumInstruments())
				chn.dwFlags.set(CHN_NOTEFADE);
		}

		// Note Cut
		if (note == NOTE_NOTECUT)
		{
			if(chn.dwFlags[CHN_ADLIB] && GetType() == MOD_TYPE_S3M)
			{
				// OPL voices are not cut but enter the release portion of their envelope
				// In S3M we can still modify the volume after note-off, in legacy MPTM mode we can't
				chn.dwFlags.set(CHN_KEYOFF);
			} else
			{
				chn.dwFlags.set(CHN_NOTEFADE | CHN_FASTVOLRAMP);
				// IT compatibility: Stopping sample playback by setting sample increment to 0 rather than volume
				// Test case: NoteOffInstr.it
				if ((!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))) || (m_nInstruments != 0 && !m_playBehaviour[kITInstrWithNoteOff])) chn.nVolume = 0;
				if (m_playBehaviour[kITInstrWithNoteOff]) chn.increment.Set(0);
				chn.nFadeOutVol = 0;
			}
		}

		// IT compatibility tentative fix: Clear channel note memory.
		if(m_playBehaviour[kITClearOldNoteAfterCut])
		{
			chn.nNote = chn.nNewNote = NOTE_NONE;
		}
		return;
	}

	if(newTuning)
	{
		if(!bPorta || chn.nNote == NOTE_NONE)
			chn.nPortamentoDest = 0;
		else
		{
			chn.nPortamentoDest = pIns->pTuning->GetStepDistance(chn.nNote, chn.m_PortamentoFineSteps, static_cast<Tuning::NOTEINDEXTYPE>(note), 0);
			//Here chn.nPortamentoDest means 'steps to slide'.
			chn.m_PortamentoFineSteps = -chn.nPortamentoDest;
		}
	}

	if(!bPorta && (GetType() & (MOD_TYPE_XM | MOD_TYPE_MED | MOD_TYPE_MT2)))
	{
		if(pSmp)
		{
			chn.nTranspose = pSmp->RelativeTone;
			chn.nFineTune = pSmp->nFineTune;
		}
	}
	// IT Compatibility: Update multisample instruments frequency even if instrument is not specified (fixes the guitars in spx-shuttledeparture.it)
	// Test case: freqreset-noins.it
	if(!bPorta && pSmp && m_playBehaviour[kITMultiSampleBehaviour])
		chn.nC5Speed = pSmp->nC5Speed;

	if(bPorta && !chn.IsSamplePlaying())
	{
		if(m_playBehaviour[kFT2PortaNoNote])
		{
			// FT2 Compatibility: Ignore notes with portamento if there was no note playing.
			// Test case: 3xx-no-old-samp.xm
			chn.nPeriod = 0;
			return;
		} else if(m_playBehaviour[kITPortaNoNote])
		{
			// IT Compatibility: Ignore portamento command if no note was playing (e.g. if a previous note has faded out).
			// Test case: Fade-Porta.it
			bPorta = false;
		}
	}

	if(UseFinetuneAndTranspose())
	{
		note += chn.nTranspose;
		// RealNote = PatternNote + RelativeTone; (0..118, 0 = C-0, 118 = A#9)
		Limit(note, NOTE_MIN + 11, NOTE_MIN + 130);	// 119 possible notes
	} else
	{
		Limit(note, NOTE_MIN, NOTE_MAX);
	}
	if(m_playBehaviour[kITRealNoteMapping])
	{
		// need to memorize the original note for various effects (e.g. PPS)
		chn.nNote = static_cast<ModCommand::NOTE>(Clamp(realnote, NOTE_MIN, NOTE_MAX));
	} else
	{
		chn.nNote = static_cast<ModCommand::NOTE>(note);
	}
	chn.m_CalculateFreq = true;

	if ((!bPorta) || (GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT)))
		chn.nNewIns = 0;

	uint32 period = GetPeriodFromNote(note, chn.nFineTune, chn.nC5Speed);
	chn.nPanbrelloOffset = 0;

	// IT compatibility: Sample and instrument panning is only applied on note change, not instrument change
	// Test case: PanReset.it
	if(m_playBehaviour[kITPanningReset]) ApplyInstrumentPanning(chn, pIns, pSmp);

	if(bResetEnv && !bPorta)
	{
		chn.nVolSwing = chn.nPanSwing = 0;
		chn.nResSwing = chn.nCutSwing = 0;
		if(pIns)
		{
			// IT Compatiblity: NNA is reset on every note change, not every instrument change (fixes spx-farspacedance.it).
			if(m_playBehaviour[kITNNAReset]) chn.nNNA = pIns->nNNA;

			if(!pIns->VolEnv.dwFlags[ENV_CARRY]) chn.VolEnv.Reset();
			if(!pIns->PanEnv.dwFlags[ENV_CARRY]) chn.PanEnv.Reset();
			if(!pIns->PitchEnv.dwFlags[ENV_CARRY]) chn.PitchEnv.Reset();

			// Volume Swing
			if(pIns->nVolSwing)
			{
				chn.nVolSwing = static_cast<int16>(((mpt::random<int8>(AccessPRNG()) * pIns->nVolSwing) / 64 + 1) * (m_playBehaviour[kITSwingBehaviour] ? chn.nInsVol : ((chn.nVolume + 1) / 2)) / 199);
			}
			// Pan Swing
			if(pIns->nPanSwing)
			{
				chn.nPanSwing = static_cast<int16>(((mpt::random<int8>(AccessPRNG()) * pIns->nPanSwing * 4) / 128));
				if(!m_playBehaviour[kITSwingBehaviour] && chn.nRestorePanOnNewNote == 0)
				{
					chn.nRestorePanOnNewNote = static_cast<uint16>(chn.nPan + 1);
				}
			}
			// Cutoff Swing
			if(pIns->nCutSwing)
			{
				int32 d = ((int32)pIns->nCutSwing * (int32)(static_cast<int32>(mpt::random<int8>(AccessPRNG())) + 1)) / 128;
				chn.nCutSwing = static_cast<int16>((d * chn.nCutOff + 1) / 128);
				chn.nRestoreCutoffOnNewNote = chn.nCutOff + 1;
			}
			// Resonance Swing
			if(pIns->nResSwing)
			{
				int32 d = ((int32)pIns->nResSwing * (int32)(static_cast<int32>(mpt::random<int8>(AccessPRNG())) + 1)) / 128;
				chn.nResSwing = static_cast<int16>((d * chn.nResonance + 1) / 128);
				chn.nRestoreResonanceOnNewNote = chn.nResonance + 1;
			}
		}
	}

	if(!pSmp) return;
	if(period)
	{
		if((!bPorta) || (!chn.nPeriod)) chn.nPeriod = period;
		if(!newTuning)
		{
			// FT2 compatibility: Don't reset portamento target with new notes.
			// Test case: Porta-Pickup.xm
			// ProTracker does the same.
			// Test case: PortaTarget.mod
			// IT compatibility: Portamento target is completely cleared with new notes.
			// Test case: PortaReset.it
			if(bPorta || !(m_playBehaviour[kFT2PortaTargetNoReset] || m_playBehaviour[kITClearPortaTarget] || GetType() == MOD_TYPE_MOD))
			{
				chn.nPortamentoDest = period;
			}
		}

		if(!bPorta || (!chn.nLength && !(GetType() & MOD_TYPE_S3M)))
		{
			chn.pModSample = pSmp;
			chn.nLength = pSmp->nLength;
			chn.nLoopEnd = pSmp->nLength;
			chn.nLoopStart = 0;
			chn.position.Set(0);
			if((m_SongFlags[SONG_PT_MODE] || m_playBehaviour[kST3OffsetWithoutInstrument]) && !chn.rowCommand.instr)
			{
				chn.position.SetInt(std::min(chn.prevNoteOffset, chn.nLength - SmpLength(1)));
			} else
			{
				chn.prevNoteOffset = 0;
			}
			chn.dwFlags = (chn.dwFlags & CHN_CHANNELFLAGS) | (pSmp->uFlags & CHN_SAMPLEFLAGS);
			chn.dwFlags.reset(CHN_PORTAMENTO);
			if(chn.dwFlags[CHN_SUSTAINLOOP])
			{
				chn.nLoopStart = pSmp->nSustainStart;
				chn.nLoopEnd = pSmp->nSustainEnd;
				chn.dwFlags.set(CHN_PINGPONGLOOP, chn.dwFlags[CHN_PINGPONGSUSTAIN]);
				chn.dwFlags.set(CHN_LOOP);
				if (chn.nLength > chn.nLoopEnd) chn.nLength = chn.nLoopEnd;
			} else if(chn.dwFlags[CHN_LOOP])
			{
				chn.nLoopStart = pSmp->nLoopStart;
				chn.nLoopEnd = pSmp->nLoopEnd;
				if (chn.nLength > chn.nLoopEnd) chn.nLength = chn.nLoopEnd;
			}
			// ProTracker "oneshot" loops (if loop start is 0, play the whole sample once and then repeat until loop end)
			if(m_playBehaviour[kMODOneShotLoops] && chn.nLoopStart == 0) chn.nLoopEnd = chn.nLength = pSmp->nLength;

			if(chn.dwFlags[CHN_REVERSE])
			{
				chn.dwFlags.set(CHN_PINGPONGFLAG);
				chn.position.SetInt(chn.nLength - 1);
			}

			// Handle "retrigger" waveform type
			if(chn.nVibratoType < 4)
			{
				// IT Compatibilty: Slightly different waveform offsets (why does MPT have two different offsets here with IT old effects enabled and disabled?)
				if(!m_playBehaviour[kITVibratoTremoloPanbrello] && (GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) && !m_SongFlags[SONG_ITOLDEFFECTS])
					chn.nVibratoPos = 0x10;
				else if(GetType() == MOD_TYPE_MTM)
					chn.nVibratoPos = 0x20;
				else if(!(GetType() & (MOD_TYPE_DIGI | MOD_TYPE_DBM)))
					chn.nVibratoPos = 0;
			}
			// IT Compatibility: No "retrigger" waveform here
			if(!m_playBehaviour[kITVibratoTremoloPanbrello] && chn.nTremoloType < 4)
			{
				chn.nTremoloPos = 0;
			}
		}
		if(chn.position.GetUInt() >= chn.nLength) chn.position.SetInt(chn.nLoopStart);
	} else
	{
		bPorta = false;
	}

	if (!bPorta
		|| (!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_DBM)))
		|| (chn.dwFlags[CHN_NOTEFADE] && !chn.nFadeOutVol)
		|| (m_SongFlags[SONG_ITCOMPATGXX] && chn.rowCommand.instr != 0))
	{
		if((GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_DBM)) && chn.dwFlags[CHN_NOTEFADE] && !chn.nFadeOutVol)
		{
			chn.ResetEnvelopes();
			// IT Compatibility: Autovibrato reset
			if(!m_playBehaviour[kITVibratoTremoloPanbrello])
			{
				chn.nAutoVibDepth = 0;
				chn.nAutoVibPos = 0;
			}
			chn.dwFlags.reset(CHN_NOTEFADE);
			chn.nFadeOutVol = 65536;
		}
		if ((!bPorta) || (!m_SongFlags[SONG_ITCOMPATGXX]) || (chn.rowCommand.instr))
		{
			if ((!(GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2))) || (chn.rowCommand.instr))
			{
				chn.dwFlags.reset(CHN_NOTEFADE);
				chn.nFadeOutVol = 65536;
			}
		}
	}

	// IT compatibility: Don't reset key-off flag on porta notes unless Compat Gxx is enabled.
	// Test case: Off-Porta.it, Off-Porta-CompatGxx.it, Off-Porta.xm
	if(m_playBehaviour[kITFT2DontResetNoteOffOnPorta] && bPorta && (!m_SongFlags[SONG_ITCOMPATGXX] || chn.rowCommand.instr == 0))
		chn.dwFlags.reset(CHN_EXTRALOUD);
	else
		chn.dwFlags.reset(CHN_EXTRALOUD | CHN_KEYOFF);

	// Enable Ramping
	if(!bPorta)
	{
		chn.nLeftVU = chn.nRightVU = 0xFF;
		chn.dwFlags.reset(CHN_FILTER);
		chn.dwFlags.set(CHN_FASTVOLRAMP);

		// IT compatibility 15. Retrigger is reset in RetrigNote (Tremor doesn't store anything here, so we just don't reset this as well)
		if(!m_playBehaviour[kITRetrigger] && !m_playBehaviour[kITTremor])
		{
			// FT2 compatibility: Retrigger is reset in RetrigNote, tremor in ProcessEffects
			if(!m_playBehaviour[kFT2Retrigger] && !m_playBehaviour[kFT2Tremor])
			{
				chn.nRetrigCount = 0;
				chn.nTremorCount = 0;
			}
		}

		if(bResetEnv)
		{
			chn.nAutoVibDepth = 0;
			chn.nAutoVibPos = 0;
		}
		chn.rightVol = chn.leftVol = 0;
		bool useFilter = !m_SongFlags[SONG_MPTFILTERMODE];
		// Setup Initial Filter for this note
		if(pIns)
		{
			if(pIns->IsResonanceEnabled())
			{
				chn.nResonance = pIns->GetResonance();
				useFilter = true;
			}
			if(pIns->IsCutoffEnabled())
			{
				chn.nCutOff = pIns->GetCutoff();
				useFilter = true;
			}
			if(useFilter && (pIns->filterMode != FilterMode::Unchanged))
			{
				chn.nFilterMode = pIns->filterMode;
			}
		} else
		{
			chn.nVolSwing = chn.nPanSwing = 0;
			chn.nCutSwing = chn.nResSwing = 0;
		}
		if((chn.nCutOff < 0x7F || m_playBehaviour[kITFilterBehaviour]) && useFilter)
		{
			int cutoff = SetupChannelFilter(chn, true);
			if(cutoff >= 0 && chn.dwFlags[CHN_ADLIB] && m_opl && channelHint != CHANNELINDEX_INVALID)
				m_opl->Volume(channelHint, chn.nCutOff / 2u, true);
		}
	}
	// Special case for MPT
	if (bManual) chn.dwFlags.reset(CHN_MUTE);
	if((chn.dwFlags[CHN_MUTE] && (m_MixerSettings.MixerFlags & SNDMIX_MUTECHNMODE))
		|| (chn.pModSample != nullptr && chn.pModSample->uFlags[CHN_MUTE] && !bManual)
		|| (chn.pModInstrument != nullptr && chn.pModInstrument->dwFlags[INS_MUTE] && !bManual))
	{
		if (!bManual) chn.nPeriod = 0;
	}

	// Reset the Amiga resampler for this channel
	if(!bPorta)
	{
		chn.paulaState.Reset();
	}
}


// Apply sample or instrumernt panning
void CSoundFile::ApplyInstrumentPanning(ModChannel &chn, const ModInstrument *instr, const ModSample *smp) const
{
	int32 newPan = int32_min;
	// Default instrument panning
	if(instr != nullptr && instr->dwFlags[INS_SETPANNING])
		newPan = instr->nPan;
	// Default sample panning
	if(smp != nullptr && smp->uFlags[CHN_PANNING])
		newPan = smp->nPan;

	if(newPan != int32_min)
	{
		chn.SetInstrumentPan(newPan, *this);
		// IT compatibility: Sample and instrument panning overrides channel surround status.
		// Test case: SmpInsPanSurround.it
		if(m_playBehaviour[kPanOverride] && !m_SongFlags[SONG_SURROUNDPAN])
		{
			chn.dwFlags.reset(CHN_SURROUND);
		}
	}
}


CHANNELINDEX CSoundFile::GetNNAChannel(CHANNELINDEX nChn) const
{
	// Check for empty channel
	for(CHANNELINDEX i = m_nChannels; i < MAX_CHANNELS; i++)
	{
		const ModChannel &c = m_PlayState.Chn[i];
		// No sample and no plugin playing
		if(!c.nLength && !c.HasMIDIOutput())
			return i;
		// Plugin channel with already released note
		if(!c.nLength && c.dwFlags[CHN_KEYOFF | CHN_NOTEFADE])
			return i;
		// Stopped OPL channel
		if(c.dwFlags[CHN_ADLIB] && (!m_opl || !m_opl->IsActive(i)))
			return i;
	}

	uint32 vol = 0x800000;
	if(nChn < MAX_CHANNELS)
	{
		const ModChannel &srcChn = m_PlayState.Chn[nChn];
		if(!srcChn.nFadeOutVol && srcChn.nLength)
			return CHANNELINDEX_INVALID;
		vol = (srcChn.nRealVolume << 9) | srcChn.nVolume;
	}

	// All channels are used: check for lowest volume
	CHANNELINDEX result = CHANNELINDEX_INVALID;
	uint32 envpos = 0;
	for(CHANNELINDEX i = m_nChannels; i < MAX_CHANNELS; i++)
	{
		const ModChannel &c = m_PlayState.Chn[i];
		if(c.nLength && !c.nFadeOutVol)
			return i;
		// Use a combination of real volume [14 bit] (which includes volume envelopes, but also potentially global volume) and note volume [9 bit].
		// Rationale: We need volume envelopes in case e.g. all NNA channels are playing at full volume but are looping on a 0-volume envelope node.
		// But if global volume is not applied to master and the global volume temporarily drops to 0, we would kill arbitrary channels. Hence, add the note volume as well.
		uint32 v = (c.nRealVolume << 9) | c.nVolume;
		if(c.dwFlags[CHN_LOOP])
			v /= 2;
		if((v < vol) || ((v == vol) && (c.VolEnv.nEnvPosition > envpos)))
		{
			envpos = c.VolEnv.nEnvPosition;
			vol = v;
			result = i;
		}
	}
	return result;
}


CHANNELINDEX CSoundFile::CheckNNA(CHANNELINDEX nChn, uint32 instr, int note, bool forceCut)
{
	ModChannel &srcChn = m_PlayState.Chn[nChn];
	const ModInstrument *pIns = nullptr;
	if(!ModCommand::IsNote(static_cast<ModCommand::NOTE>(note)))
		return CHANNELINDEX_INVALID;

	// Always NNA cut - using
	if((!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_MT2)) || !m_nInstruments || forceCut) && !srcChn.HasMIDIOutput())
	{
		if(!srcChn.nLength || srcChn.dwFlags[CHN_MUTE] || !(srcChn.rightVol | srcChn.leftVol))
			return CHANNELINDEX_INVALID;

		const CHANNELINDEX nnaChn = GetNNAChannel(nChn);
		if(nnaChn == CHANNELINDEX_INVALID)
			return CHANNELINDEX_INVALID;
		ModChannel &chn = m_PlayState.Chn[nnaChn];
		// Copy Channel
		chn = srcChn;
		chn.dwFlags.reset(CHN_VIBRATO | CHN_TREMOLO | CHN_MUTE | CHN_PORTAMENTO);
		chn.nPanbrelloOffset = 0;
		chn.nMasterChn = nChn + 1;
		chn.nCommand = CMD_NONE;
		chn.rowCommand.Clear();
		// Cut the note
		chn.nFadeOutVol = 0;
		chn.dwFlags.set(CHN_NOTEFADE | CHN_FASTVOLRAMP);
		// Stop this channel
		srcChn.nLength = 0;
		srcChn.position.Set(0);
		srcChn.nROfs = srcChn.nLOfs = 0;
		srcChn.rightVol = srcChn.leftVol = 0;
		if(srcChn.dwFlags[CHN_ADLIB] && m_opl)
		{
			m_opl->NoteCut(nChn);
		}
		return nnaChn;
	}
	if(instr > GetNumInstruments())
		instr = 0;
	const ModSample *pSample = srcChn.pModSample;
	// If no instrument is given, assume previous instrument to still be valid.
	// Test case: DNA-NoInstr.it
	pIns = instr > 0 ? Instruments[instr] : srcChn.pModInstrument;
	auto dnaNote = note;
	if(pIns != nullptr)
	{
		auto smp = pIns->Keyboard[note - NOTE_MIN];
		// IT compatibility: DCT = note uses pattern notes for comparison
		// Note: This is not applied in case kITRealNoteMapping is not set to keep playback of legacy modules simple (chn.nNote is translated note in that case)
		// Test case: dct_smp_note_test.it
		if(!m_playBehaviour[kITDCTBehaviour] || !m_playBehaviour[kITRealNoteMapping])
			dnaNote = pIns->NoteMap[note - NOTE_MIN];
		if(smp > 0 && smp < MAX_SAMPLES)
		{
			pSample = &Samples[smp];
		} else if(m_playBehaviour[kITEmptyNoteMapSlot] && !pIns->HasValidMIDIChannel())
		{
			// Impulse Tracker ignores empty slots.
			// We won't ignore them if a plugin is assigned to this slot, so that VSTis still work as intended.
			// Test case: emptyslot.it, PortaInsNum.it, gxsmp.it, gxsmp2.it
			return CHANNELINDEX_INVALID;
		}
	}
	if(srcChn.dwFlags[CHN_MUTE])
		return CHANNELINDEX_INVALID;

	for(CHANNELINDEX i = nChn; i < MAX_CHANNELS; i++)
	{
		// Only apply to background channels, or the same pattern channel
		if(i < m_nChannels && i != nChn)
			continue;

		ModChannel &chn = m_PlayState.Chn[i];
		bool applyDNAtoPlug = false;
		if((chn.nMasterChn == nChn + 1 || i == nChn) && chn.pModInstrument != nullptr)
		{
			bool applyDNA = false;
			// Duplicate Check Type
			switch(chn.pModInstrument->nDCT)
			{
			case DCT_NONE:
				break;
			// Note
			case DCT_NOTE:
				if(dnaNote != NOTE_NONE && chn.nNote == dnaNote && pIns == chn.pModInstrument)
					applyDNA = true;
				if(pIns && pIns->nMixPlug)
					applyDNAtoPlug = true;
				break;
			// Sample
			case DCT_SAMPLE:
				// IT compatibility: DCT = sample only applies to same instrument
				// Test case: dct_smp_note_test.it
				if(pSample != nullptr && pSample == chn.pModSample && (pIns == chn.pModInstrument || !m_playBehaviour[kITDCTBehaviour]))
					applyDNA = true;
				break;
			// Instrument
			case DCT_INSTRUMENT:
				if(pIns == chn.pModInstrument)
					applyDNA = true;
				if(pIns && pIns->nMixPlug)
					applyDNAtoPlug = true;
				break;
			// Plugin
			case DCT_PLUGIN:
				if(pIns && (pIns->nMixPlug) && (pIns->nMixPlug == chn.pModInstrument->nMixPlug))
				{
					applyDNAtoPlug = true;
					applyDNA = true;
				}
				break;
			}
			
			// Duplicate Note Action
			if(applyDNA)
			{
#ifndef NO_PLUGINS
				if(applyDNAtoPlug && chn.nNote != NOTE_NONE)
				{
					switch(chn.pModInstrument->nDNA)
					{
					case DNA_NOTECUT:
					case DNA_NOTEOFF:
					case DNA_NOTEFADE:
						// Switch off duplicated note played on this plugin
						SendMIDINote(i, chn.GetPluginNote(m_playBehaviour[kITRealNoteMapping]) + NOTE_MAX_SPECIAL, 0);
						chn.nArpeggioLastNote = NOTE_NONE;
						break;
					}
				}
#endif // NO_PLUGINS

				switch(chn.pModInstrument->nDNA)
				{
				// Cut
				case DNA_NOTECUT:
					KeyOff(chn);
					chn.nVolume = 0;
					if(chn.dwFlags[CHN_ADLIB] && m_opl)
						m_opl->NoteCut(i);
					break;
				// Note Off
				case DNA_NOTEOFF:
					KeyOff(chn);
					if(chn.dwFlags[CHN_ADLIB] && m_opl)
						m_opl->NoteOff(i);
					break;
				// Note Fade
				case DNA_NOTEFADE:
					chn.dwFlags.set(CHN_NOTEFADE);
					if(chn.dwFlags[CHN_ADLIB] && m_opl && !m_playBehaviour[kOPLwithNNA])
						m_opl->NoteOff(i);
					break;
				}
				if(!chn.nVolume)
				{
					chn.nFadeOutVol = 0;
					chn.dwFlags.set(CHN_NOTEFADE | CHN_FASTVOLRAMP);
				}
			}
		}
	}

	// Do we need to apply New/Duplicate Note Action to a VSTi?
	bool applyNNAtoPlug = false;
#ifndef NO_PLUGINS
	IMixPlugin *pPlugin = nullptr;
	if(srcChn.HasMIDIOutput() && ModCommand::IsNote(srcChn.nNote)) // instro sends to a midi chan
	{
		PLUGINDEX plugin = GetBestPlugin(nChn, PrioritiseInstrument, RespectMutes);

		if(plugin > 0 && plugin <= MAX_MIXPLUGINS)
		{
			pPlugin =  m_MixPlugins[plugin - 1].pMixPlugin;
			if(pPlugin)
			{
				// apply NNA to this plugin iff it is currently playing a note on this tracker channel
				// (and if it is playing a note, we know that would be the last note played on this chan).
				applyNNAtoPlug = pPlugin->IsNotePlaying(srcChn.GetPluginNote(m_playBehaviour[kITRealNoteMapping]), nChn);
			}
		}
	}
#endif // NO_PLUGINS

	// New Note Action
	if(!srcChn.IsSamplePlaying() && !applyNNAtoPlug)
		return CHANNELINDEX_INVALID;

	CHANNELINDEX nnaChn = GetNNAChannel(nChn);
	if(nnaChn == CHANNELINDEX_INVALID)
		return CHANNELINDEX_INVALID;

	ModChannel &chn = m_PlayState.Chn[nnaChn];
	if(chn.dwFlags[CHN_ADLIB] && m_opl)
		m_opl->NoteCut(nnaChn);
	// Copy Channel
	chn = srcChn;
	chn.dwFlags.reset(CHN_VIBRATO | CHN_TREMOLO | CHN_PORTAMENTO);
	chn.nPanbrelloOffset = 0;

	chn.nMasterChn = nChn < GetNumChannels() ? nChn + 1 : 0;
	chn.nCommand = CMD_NONE;
#ifndef NO_PLUGINS
	if(applyNNAtoPlug && pPlugin)
	{
		switch(srcChn.nNNA)
		{
		case NNA_NOTEOFF:
		case NNA_NOTECUT:
		case NNA_NOTEFADE:
			// Switch off note played on this plugin, on this tracker channel and midi channel
			SendMIDINote(nChn, NOTE_KEYOFF, 0);
			srcChn.nArpeggioLastNote = NOTE_NONE;
			break;
		case NNA_CONTINUE:
			break;
		}
	}
#endif // NO_PLUGINS

	// Key Off the note
	switch(srcChn.nNNA)
	{
	case NNA_NOTEOFF:
		KeyOff(chn);
		if(chn.dwFlags[CHN_ADLIB] && m_opl)
		{
			m_opl->NoteOff(nChn);
			if(m_playBehaviour[kOPLwithNNA])
				m_opl->MoveChannel(nChn, nnaChn);
		}
		break;
	case NNA_NOTECUT:
		chn.nFadeOutVol = 0;
		chn.dwFlags.set(CHN_NOTEFADE);
		if(chn.dwFlags[CHN_ADLIB] && m_opl)
			m_opl->NoteCut(nChn);
		break;
	case NNA_NOTEFADE:
		chn.dwFlags.set(CHN_NOTEFADE);
		if(chn.dwFlags[CHN_ADLIB] && m_opl)
		{
			if(m_playBehaviour[kOPLwithNNA])
				m_opl->MoveChannel(nChn, nnaChn);
			else
				m_opl->NoteOff(nChn);
		}
		break;
	case NNA_CONTINUE:
		if(chn.dwFlags[CHN_ADLIB] && m_opl)
			m_opl->MoveChannel(nChn, nnaChn);
		break;
	}
	if(!chn.nVolume)
	{
		chn.nFadeOutVol = 0;
		chn.dwFlags.set(CHN_NOTEFADE | CHN_FASTVOLRAMP);
	}
	// Stop this channel
	srcChn.nLength = 0;
	srcChn.position.Set(0);
	srcChn.nROfs = srcChn.nLOfs = 0;
	
	return nnaChn;
}


bool CSoundFile::ProcessEffects()
{
	ROWINDEX nBreakRow = ROWINDEX_INVALID;		// Is changed if a break to row command is encountered
	ROWINDEX nPatLoopRow = ROWINDEX_INVALID;	// Is changed if a pattern loop jump-back is executed
	ORDERINDEX nPosJump = ORDERINDEX_INVALID;

	for(CHANNELINDEX nChn = 0; nChn < GetNumChannels(); nChn++)
	{
		ModChannel &chn = m_PlayState.Chn[nChn];
		const uint32 tickCount = m_PlayState.m_nTickCount % (m_PlayState.m_nMusicSpeed + m_PlayState.m_nFrameDelay);
		uint32 instr = chn.rowCommand.instr;
		ModCommand::VOLCMD volcmd = chn.rowCommand.volcmd;
		uint32 vol = chn.rowCommand.vol;
		ModCommand::COMMAND cmd = chn.rowCommand.command;
		uint32 param = chn.rowCommand.param;
		bool bPorta = chn.rowCommand.IsPortamento();

		uint32 nStartTick = 0;
		chn.isFirstTick = m_SongFlags[SONG_FIRSTTICK];

		// Process parameter control note.
		if(chn.rowCommand.note == NOTE_PC)
		{
#ifndef NO_PLUGINS
			const PLUGINDEX plug = chn.rowCommand.instr;
			const PlugParamIndex plugparam = chn.rowCommand.GetValueVolCol();
			const PlugParamValue value = chn.rowCommand.GetValueEffectCol() / PlugParamValue(ModCommand::maxColumnValue);

			if(plug > 0 && plug <= MAX_MIXPLUGINS && m_MixPlugins[plug - 1].pMixPlugin)
				m_MixPlugins[plug-1].pMixPlugin->SetParameter(plugparam, value);
#endif // NO_PLUGINS
		}

		// Process continuous parameter control note.
		// Row data is cleared after first tick so on following
		// ticks using channels m_nPlugParamValueStep to identify
		// the need for parameter control. The condition cmd == 0
		// is to make sure that m_nPlugParamValueStep != 0 because
		// of NOTE_PCS, not because of macro.
		if(chn.rowCommand.note == NOTE_PCS || (cmd == CMD_NONE && chn.m_plugParamValueStep != 0))
		{
#ifndef NO_PLUGINS
			const bool isFirstTick = m_SongFlags[SONG_FIRSTTICK];
			if(isFirstTick)
				chn.m_RowPlug = chn.rowCommand.instr;
			const PLUGINDEX plugin = chn.m_RowPlug;
			const bool hasValidPlug = (plugin > 0 && plugin <= MAX_MIXPLUGINS && m_MixPlugins[plugin - 1].pMixPlugin);
			if(hasValidPlug)
			{
				if(isFirstTick)
					chn.m_RowPlugParam = ModCommand::GetValueVolCol(chn.rowCommand.volcmd, chn.rowCommand.vol);
				const PlugParamIndex plugparam = chn.m_RowPlugParam;
				if(isFirstTick)
				{
					PlugParamValue targetvalue = ModCommand::GetValueEffectCol(chn.rowCommand.command, chn.rowCommand.param) / PlugParamValue(ModCommand::maxColumnValue);
					chn.m_plugParamTargetValue = targetvalue;
					chn.m_plugParamValueStep = (targetvalue - m_MixPlugins[plugin - 1].pMixPlugin->GetParameter(plugparam)) / PlugParamValue(GetNumTicksOnCurrentRow());
				}
				if(m_PlayState.m_nTickCount + 1 == GetNumTicksOnCurrentRow())
				{	// On last tick, set parameter exactly to target value.
					m_MixPlugins[plugin - 1].pMixPlugin->SetParameter(plugparam, chn.m_plugParamTargetValue);
				}
				else
					m_MixPlugins[plugin - 1].pMixPlugin->ModifyParameter(plugparam, chn.m_plugParamValueStep);
			}
#endif // NO_PLUGINS
		}

		// Apart from changing parameters, parameter control notes are intended to be 'invisible'.
		// To achieve this, clearing the note data so that rest of the process sees the row as empty row.
		if(ModCommand::IsPcNote(chn.rowCommand.note))
		{
			chn.ClearRowCmd();
			instr = 0;
			volcmd = VOLCMD_NONE;
			vol = 0;
			cmd = CMD_NONE;
			param = 0;
			bPorta = false;
		}

		// Process Invert Loop (MOD Effect, called every row if it's active)
		if(!m_SongFlags[SONG_FIRSTTICK])
		{
			InvertLoop(m_PlayState.Chn[nChn]);
		} else
		{
			if(instr) m_PlayState.Chn[nChn].nEFxOffset = 0;
		}

		// Process special effects (note delay, pattern delay, pattern loop)
		if (cmd == CMD_DELAYCUT)
		{
			//:xy --> note delay until tick x, note cut at tick x+y
			nStartTick = (param & 0xF0) >> 4;
			const uint32 cutAtTick = nStartTick + (param & 0x0F);
			NoteCut(nChn, cutAtTick, m_playBehaviour[kITSCxStopsSample]);
		} else if ((cmd == CMD_MODCMDEX) || (cmd == CMD_S3MCMDEX))
		{
			if ((!param) && (GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT)))
				param = chn.nOldCmdEx;
			else
				chn.nOldCmdEx = static_cast<ModCommand::PARAM>(param);

			// Note Delay ?
			if ((param & 0xF0) == 0xD0)
			{
				nStartTick = param & 0x0F;
				if(nStartTick == 0)
				{
					//IT compatibility 22. SD0 == SD1
					if(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))
						nStartTick = 1;
					//ST3 ignores notes with SD0 completely
					else if(GetType() == MOD_TYPE_S3M)
						continue;
				} else if(nStartTick >= (m_PlayState.m_nMusicSpeed + m_PlayState.m_nFrameDelay) && m_playBehaviour[kITOutOfRangeDelay])
				{
					// IT compatibility 08. Handling of out-of-range delay command.
					// Additional test case: tickdelay.it
					if(instr)
					{
						chn.nNewIns = static_cast<ModCommand::INSTR>(instr);
					}
					continue;
				}
			} else if(m_SongFlags[SONG_FIRSTTICK])
			{
				// Pattern Loop ?
				if((((param & 0xF0) == 0x60 && cmd == CMD_MODCMDEX)
					|| ((param & 0xF0) == 0xB0 && cmd == CMD_S3MCMDEX))
					&& !(m_playBehaviour[kST3NoMutedChannels] && ChnSettings[nChn].dwFlags[CHN_MUTE]))	// not even effects are processed on muted S3M channels
				{
					ROWINDEX nloop = PatternLoop(chn, param & 0x0F);
					if (nloop != ROWINDEX_INVALID)
					{
						// FT2 compatibility: E6x overwrites jump targets of Dxx effects that are located left of the E6x effect.
						// Test cases: PatLoop-Jumps.xm, PatLoop-Various.xm
						if(nBreakRow != ROWINDEX_INVALID && m_playBehaviour[kFT2PatternLoopWithJumps])
						{
							nBreakRow = nloop;
						}

						nPatLoopRow = nloop;
						// IT compatibility: SBx is prioritized over Position Jump (Bxx) effects that are located left of the SBx effect.
						// Test case: sbx-priority.it, LoopBreak.it
						if(m_playBehaviour[kITPatternLoopWithJumps])
							nPosJump = ORDERINDEX_INVALID;
					}

					if(GetType() == MOD_TYPE_S3M)
					{
						// ST3 doesn't have per-channel pattern loop memory, so spam all changes to other channels as well.
						for (CHANNELINDEX i = 0; i < GetNumChannels(); i++)
						{
							m_PlayState.Chn[i].nPatternLoop = chn.nPatternLoop;
							m_PlayState.Chn[i].nPatternLoopCount = chn.nPatternLoopCount;
						}
					}
				} else if ((param & 0xF0) == 0xE0)
				{
					// Pattern Delay
					// In Scream Tracker 3 / Impulse Tracker, only the first delay command on this row is considered.
					// Test cases: PatternDelays.it, PatternDelays.s3m, PatternDelays.xm
					// XXX In Scream Tracker 3, the "left" channels are evaluated before the "right" channels, which is not emulated here!
					if(!(GetType() & (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT)) || !m_PlayState.m_nPatternDelay)
					{
						if(!(GetType() & (MOD_TYPE_S3M)) || (param & 0x0F) != 0)
						{
							// While Impulse Tracker *does* count S60 as a valid row delay (and thus ignores any other row delay commands on the right),
							// Scream Tracker 3 simply ignores such commands.
							m_PlayState.m_nPatternDelay = 1 + (param & 0x0F);
						}
					}
				}
			}
		}

		if(GetType() == MOD_TYPE_MTM && cmd == CMD_MODCMDEX && (param & 0xF0) == 0xD0)
		{
			// Apparently, retrigger and note delay have the same behaviour in MultiTracker:
			// They both restart the note at tick x, and if there is a note on the same row,
			// this note is started on the first tick.
			nStartTick = 0;
			param = 0x90 | (param & 0x0F);
		}

		if(nStartTick != 0 && chn.rowCommand.note == NOTE_KEYOFF && chn.rowCommand.volcmd == VOLCMD_PANNING && m_playBehaviour[kFT2PanWithDelayedNoteOff])
		{
			// FT2 compatibility: If there's a delayed note off, panning commands are ignored. WTF!
			// Test case: PanOff.xm
			chn.rowCommand.volcmd = VOLCMD_NONE;
		}

		bool triggerNote = (m_PlayState.m_nTickCount == nStartTick);	// Can be delayed by a note delay effect
		if(m_playBehaviour[kFT2OutOfRangeDelay] && nStartTick >= m_PlayState.m_nMusicSpeed)
		{
			// FT2 compatibility: Note delays greater than the song speed should be ignored.
			// However, EEx pattern delay is *not* considered at all.
			// Test case: DelayCombination.xm, PortaDelay.xm
			triggerNote = false;
		} else if(m_playBehaviour[kRowDelayWithNoteDelay] && nStartTick > 0 && tickCount == nStartTick)
		{
			// IT compatibility: Delayed notes (using SDx) that are on the same row as a Row Delay effect are retriggered.
			// ProTracker / Scream Tracker 3 / FastTracker 2 do the same.
			// Test case: PatternDelay-NoteDelay.it, PatternDelay-NoteDelay.xm, PatternDelaysRetrig.mod
			triggerNote = true;
		}

		// IT compatibility: Tick-0 vs non-tick-0 effect distinction is always based on tick delay.
		// Test case: SlideDelay.it
		if(m_playBehaviour[kITFirstTickHandling])
		{
			chn.isFirstTick = tickCount == nStartTick;
		}

		// FT2 compatibility: Note + portamento + note delay = no portamento
		// Test case: PortaDelay.xm
		if(m_playBehaviour[kFT2PortaDelay] && nStartTick != 0)
		{
			bPorta = false;
		}

		if(m_SongFlags[SONG_PT_MODE] && instr && !m_PlayState.m_nTickCount)
		{
			// Instrument number resets the stacked ProTracker offset.
			// Test case: ptoffset.mod
			chn.prevNoteOffset = 0;
			// ProTracker compatibility: Sample properties are always loaded on the first tick, even when there is a note delay.
			// Test case: InstrDelay.mod
			if(!triggerNote && chn.IsSamplePlaying())
			{
				chn.nNewIns = static_cast<ModCommand::INSTR>(instr);
				if(instr <= GetNumSamples())
				{
					chn.nVolume = Samples[instr].nVolume;
					chn.nFineTune = Samples[instr].nFineTune;
				}
			}
		}

		// Handles note/instrument/volume changes
		if(triggerNote)
		{
			ModCommand::NOTE note = chn.rowCommand.note;
			if(instr) chn.nNewIns = static_cast<ModCommand::INSTR>(instr);

			if(ModCommand::IsNote(note) && m_playBehaviour[kFT2Transpose])
			{
				// Notes that exceed FT2's limit are completely ignored.
				// Test case: NoteLimit.xm
				int transpose = chn.nTranspose;
				if(instr && !bPorta)
				{
					// Refresh transpose
					// Test case: NoteLimit2.xm
					SAMPLEINDEX sample = SAMPLEINDEX_INVALID;
					if(GetNumInstruments())
					{
						// Instrument mode
						if(instr <= GetNumInstruments() && Instruments[instr] != nullptr)
						{
							sample = Instruments[instr]->Keyboard[note - NOTE_MIN];
						}
					} else
					{
						// Sample mode
						sample = static_cast<SAMPLEINDEX>(instr);
					}
					if(sample <= GetNumSamples())
					{
						transpose = GetSample(sample).RelativeTone;
					}
				}

				const int computedNote = note + transpose;
				if((computedNote < NOTE_MIN + 11 || computedNote > NOTE_MIN + 130))
				{
					note = NOTE_NONE;
				}
			} else if((GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_J2B)) && GetNumInstruments() != 0 && ModCommand::IsNoteOrEmpty(static_cast<ModCommand::NOTE>(note)))
			{
				// IT compatibility: Invalid instrument numbers do nothing, but they are remembered for upcoming notes and do not trigger a note in that case.
				// Test case: InstrumentNumberChange.it
				INSTRUMENTINDEX instrToCheck = static_cast<INSTRUMENTINDEX>((instr != 0) ? instr : chn.nOldIns);
				if(instrToCheck != 0 && (instrToCheck > GetNumInstruments() || Instruments[instrToCheck] == nullptr))
				{
					note = NOTE_NONE;
					instr = 0;
				}
			}

			// XM: FT2 ignores a note next to a K00 effect, and a fade-out seems to be done when no volume envelope is present (not exactly the Kxx behaviour)
			if(cmd == CMD_KEYOFF && param == 0 && m_playBehaviour[kFT2KeyOff])
			{
				note = NOTE_NONE;
				instr = 0;
			}

			bool retrigEnv = note == NOTE_NONE && instr != 0;

			// Apparently, any note number in a pattern causes instruments to recall their original volume settings - no matter if there's a Note Off next to it or whatever.
			// Test cases: keyoff+instr.xm, delay.xm
			bool reloadSampleSettings = (m_playBehaviour[kFT2ReloadSampleSettings] && instr != 0);
			// ProTracker Compatibility: If a sample was stopped before, lone instrument numbers can retrigger it
			// Test case: PTSwapEmpty.mod, PTInstrVolume.mod, SampleSwap.s3m
			bool keepInstr = (GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))
				|| m_playBehaviour[kST3SampleSwap]
				|| (m_playBehaviour[kMODSampleSwap] && !chn.IsSamplePlaying() && (chn.pModSample == nullptr || !chn.pModSample->HasSampleData()));

			// Now it's time for some FT2 crap...
			if (GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2))
			{

				// XM: Key-Off + Sample == Note Cut (BUT: Only if no instr number or volume effect is present!)
				// Test case: NoteOffVolume.xm
				if(note == NOTE_KEYOFF
					&& ((!instr && volcmd != VOLCMD_VOLUME && cmd != CMD_VOLUME) || !m_playBehaviour[kFT2KeyOff])
					&& (chn.pModInstrument == nullptr || !chn.pModInstrument->VolEnv.dwFlags[ENV_ENABLED]))
				{
					chn.dwFlags.set(CHN_FASTVOLRAMP);
					chn.nVolume = 0;
					note = NOTE_NONE;
					instr = 0;
					retrigEnv = false;
					// FT2 Compatbility: Start fading the note for notes with no delay. Only relevant when a volume command is encountered after the note-off.
					// Test case: NoteOffFadeNoEnv.xm
					if(m_SongFlags[SONG_FIRSTTICK] && m_playBehaviour[kFT2NoteOffFlags])
						chn.dwFlags.set(CHN_NOTEFADE);
				} else if(m_playBehaviour[kFT2RetrigWithNoteDelay] && !m_SongFlags[SONG_FIRSTTICK])
				{
					// FT2 Compatibility: Some special hacks for rogue note delays... (EDx with x > 0)
					// Apparently anything that is next to a note delay behaves totally unpredictable in FT2. Swedish tracker logic. :)

					retrigEnv = true;

					// Portamento + Note Delay = No Portamento
					// Test case: porta-delay.xm
					bPorta = false;

					if(note == NOTE_NONE)
					{
						// If there's a note delay but no real note, retrig the last note.
						// Test case: delay2.xm, delay3.xm
						note = static_cast<ModCommand::NOTE>(chn.nNote - chn.nTranspose);
					} else if(note >= NOTE_MIN_SPECIAL)
					{
						// Gah! Even Note Off + Note Delay will cause envelopes to *retrigger*! How stupid is that?
						// ... Well, and that is actually all it does if there's an envelope. No fade out, no nothing. *sigh*
						// Test case: OffDelay.xm
						note = NOTE_NONE;
						keepInstr = false;
						reloadSampleSettings = true;
					} else if(instr || !m_playBehaviour[kFT2NoteDelayWithoutInstr])
					{
						// Normal note (only if there is an instrument, test case: DelayVolume.xm)
						keepInstr = true;
						reloadSampleSettings = true;
					}
				}
			}

			if((retrigEnv && !m_playBehaviour[kFT2ReloadSampleSettings]) || reloadSampleSettings)
			{
				const ModSample *oldSample = nullptr;
				// Reset default volume when retriggering envelopes

				if(GetNumInstruments())
				{
					oldSample = chn.pModSample;
				} else if (instr <= GetNumSamples())
				{
					// Case: Only samples are used; no instruments.
					oldSample = &Samples[instr];
				}

				if(oldSample != nullptr)
				{
					if(!oldSample->uFlags[SMP_NODEFAULTVOLUME] && (GetType() != MOD_TYPE_S3M || oldSample->HasSampleData()))
						chn.nVolume = oldSample->nVolume;
					if(reloadSampleSettings)
					{
						// Also reload panning
						chn.SetInstrumentPan(oldSample->nPan, *this);
					}
				}
			}

			// FT2 compatibility: Instrument number disables tremor effect
			// Test case: TremorInstr.xm, TremoRecover.xm
			if(m_playBehaviour[kFT2Tremor] && instr != 0)
			{
				chn.nTremorCount = 0x20;
			}

			// IT compatibility: Envelope retriggering with instrument number based on Old Effects and Compatible Gxx flags:
			// OldFX CompatGxx Env Behaviour
			// ----- --------- -------------
			//  off     off    never reset
			//  on      off    reset on instrument without portamento
			//  off     on     reset on instrument with portamento
			//  on      on     always reset
			// Test case: ins-xx.it, ins-ox.it, ins-oc.it, ins-xc.it, ResetEnvNoteOffOldFx.it, ResetEnvNoteOffOldFx2.it, noteoff3.it
			if(GetNumInstruments() && m_playBehaviour[kITInstrWithNoteOffOldEffects]
				&& instr && !ModCommand::IsNote(note))
			{
				if((bPorta && m_SongFlags[SONG_ITCOMPATGXX])
					|| (!bPorta && m_SongFlags[SONG_ITOLDEFFECTS]))
				{
					chn.ResetEnvelopes();
					chn.dwFlags.set(CHN_FASTVOLRAMP);
					chn.nFadeOutVol = 65536;
				}
			}

			if(retrigEnv) //Case: instrument with no note data.
			{
				//IT compatibility: Instrument with no note.
				if(m_playBehaviour[kITInstrWithoutNote] || GetType() == MOD_TYPE_PLM)
				{
					// IT compatibility: Completely retrigger note after sample end to also reset portamento.
					// Test case: PortaResetAfterRetrigger.it
					bool triggerAfterSmpEnd = m_playBehaviour[kITMultiSampleInstrumentNumber] && !chn.IsSamplePlaying();
					if(GetNumInstruments())
					{
						// Instrument mode
						if(instr <= GetNumInstruments() && (chn.pModInstrument != Instruments[instr] || triggerAfterSmpEnd))
							note = chn.nNote;
					} else
					{
						// Sample mode
						if(instr < MAX_SAMPLES && (chn.pModSample != &Samples[instr] || triggerAfterSmpEnd))
							note = chn.nNote;
					}
				}

				if(GetNumInstruments() && (GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2 | MOD_TYPE_MED)))
				{
					chn.ResetEnvelopes();
					chn.dwFlags.set(CHN_FASTVOLRAMP);
					chn.dwFlags.reset(CHN_NOTEFADE);
					chn.nAutoVibDepth = 0;
					chn.nAutoVibPos = 0;
					chn.nFadeOutVol = 65536;
					// FT2 Compatbility: Reset key-off status with instrument number
					// Test case: NoteOffInstrChange.xm
					if(m_playBehaviour[kFT2NoteOffFlags])
						chn.dwFlags.reset(CHN_KEYOFF);
				}
				if (!keepInstr) instr = 0;
			}

			// Note Cut/Off/Fade => ignore instrument
			if (note >= NOTE_MIN_SPECIAL)
			{
				// IT compatibility: Default volume of sample is recalled if instrument number is next to a note-off.
				// Test case: NoteOffInstr.it, noteoff2.it
				if(m_playBehaviour[kITInstrWithNoteOff] && instr)
				{
					SAMPLEINDEX smp = static_cast<SAMPLEINDEX>(instr);
					if(GetNumInstruments())
					{
						smp = 0;
						if(instr <= GetNumInstruments() && Instruments[instr] != nullptr && ModCommand::IsNote(chn.nLastNote))
						{
							smp = Instruments[instr]->Keyboard[chn.nLastNote - NOTE_MIN];
						}
					}
					if(smp > 0 && smp <= GetNumSamples() && !Samples[smp].uFlags[SMP_NODEFAULTVOLUME])
						chn.nVolume = Samples[smp].nVolume;
				}
				// IT compatibility: Note-off with instrument number + Old Effects retriggers envelopes.
				// Test case: ResetEnvNoteOffOldFx.it
				if(!m_playBehaviour[kITInstrWithNoteOffOldEffects] || !m_SongFlags[SONG_ITOLDEFFECTS])
					instr = 0;
			}

			if(ModCommand::IsNote(note))
			{
				chn.nNewNote = chn.nLastNote = note;

				// New Note Action ?
				if(!bPorta)
				{
					CheckNNA(nChn, instr, note, false);
				}
			}

			if(note)
			{
				if(chn.nRestorePanOnNewNote > 0)
				{
					chn.nPan = (chn.nRestorePanOnNewNote & 0x7FFF) - 1;
					if(chn.nRestorePanOnNewNote & 0x8000)
						chn.dwFlags.set(CHN_SURROUND);
					chn.nRestorePanOnNewNote = 0;
				}
				if(chn.nRestoreResonanceOnNewNote > 0)
				{
					chn.nResonance = chn.nRestoreResonanceOnNewNote - 1;
					chn.nRestoreResonanceOnNewNote = 0;
				}
				if(chn.nRestoreCutoffOnNewNote > 0)
				{
					chn.nCutOff = chn.nRestoreCutoffOnNewNote - 1;
					chn.nRestoreCutoffOnNewNote = 0;
				}
			}

			// Instrument Change ?
			if(instr)
			{
				const ModSample *oldSample = chn.pModSample;
				//const ModInstrument *oldInstrument = chn.pModInstrument;

				InstrumentChange(chn, instr, bPorta, true);

				if(chn.pModSample != nullptr && chn.pModSample->uFlags[CHN_ADLIB] && m_opl)
				{
					m_opl->Patch(nChn, chn.pModSample->adlib);
				}

				// IT compatibility: Keep new instrument number for next instrument-less note even if sample playback is stopped
				// Test case: StoppedInstrSwap.it
				if(GetType() == MOD_TYPE_MOD)
				{
					// Test case: PortaSwapPT.mod
					if(!bPorta || !m_playBehaviour[kMODSampleSwap]) chn.nNewIns = 0;
				} else
				{
					if(!m_playBehaviour[kITInstrWithNoteOff] || ModCommand::IsNote(note)) chn.nNewIns = 0;
				}

				if(m_playBehaviour[kITPortamentoSwapResetsPos])
				{
					// Test cases: PortaInsNum.it, PortaSample.it
					if(ModCommand::IsNote(note) && oldSample != chn.pModSample)
					{
						//const bool newInstrument = oldInstrument != chn.pModInstrument && chn.pModInstrument->Keyboard[chn.nNewNote - NOTE_MIN] != 0;
						chn.position.Set(0);
					}
				} else if((GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) && oldSample != chn.pModSample && ModCommand::IsNote(note))
				{
					// Special IT case: portamento+note causes sample change -> ignore portamento
					bPorta = false;
				} else if(m_playBehaviour[kST3SampleSwap] && oldSample != chn.pModSample && (bPorta || !ModCommand::IsNote(note)) && chn.position.GetUInt() > chn.nLength)
				{
					// ST3 with SoundBlaster does sample swapping and continues playing the new sample where the old sample was stopped.
					// If the new sample is shorter than that, it is stopped, even if it could be looped.
					// This also applies to portamento between different samples.
					// Test case: SampleSwap.s3m
					chn.nLength = 0;
				} else if(m_playBehaviour[kMODSampleSwap] && !chn.IsSamplePlaying())
				{
					// If channel was paused and is resurrected by a lone instrument number, reset the sample position.
					// Test case: PTSwapEmpty.mod
					chn.position.Set(0);
				}
			}
			// New Note ?
			if (note != NOTE_NONE)
			{
				const bool instrChange = (!instr) && (chn.nNewIns) && ModCommand::IsNote(note);
				if(instrChange)
				{
					InstrumentChange(chn, chn.nNewIns, bPorta, chn.pModSample == nullptr && chn.pModInstrument == nullptr, !(GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2)));
					chn.nNewIns = 0;
				}
				if(chn.pModSample != nullptr && chn.pModSample->uFlags[CHN_ADLIB] && m_opl && (instrChange || !m_opl->IsActive(nChn)))
				{
					m_opl->Patch(nChn, chn.pModSample->adlib);
				}

				NoteChange(chn, note, bPorta, !(GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2)), false, nChn);
				if ((bPorta) && (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2)) && (instr))
				{
					chn.dwFlags.set(CHN_FASTVOLRAMP);
					chn.ResetEnvelopes();
					chn.nAutoVibDepth = 0;
					chn.nAutoVibPos = 0;
				}
				if(chn.dwFlags[CHN_ADLIB] && m_opl
					&& ((note == NOTE_NOTECUT || note == NOTE_KEYOFF) || (note == NOTE_FADE && !m_playBehaviour[kOPLFlexibleNoteOff])))
				{
					m_opl->NoteOff(nChn);
				}
			}
			// Tick-0 only volume commands
			if (volcmd == VOLCMD_VOLUME)
			{
				if (vol > 64) vol = 64;
				chn.nVolume = vol << 2;
				chn.dwFlags.set(CHN_FASTVOLRAMP);
			} else
			if (volcmd == VOLCMD_PANNING)
			{
				Panning(chn, vol, Pan6bit);
			}

#ifndef NO_PLUGINS
			if (m_nInstruments) ProcessMidiOut(nChn);
#endif // NO_PLUGINS
		}

		if(m_playBehaviour[kST3NoMutedChannels] && ChnSettings[nChn].dwFlags[CHN_MUTE])	// not even effects are processed on muted S3M channels
			continue;

		// Volume Column Effect (except volume & panning)
		/*	A few notes, paraphrased from ITTECH.TXT by Storlek (creator of schismtracker):
			Ex/Fx/Gx are shared with Exx/Fxx/Gxx; Ex/Fx are 4x the 'normal' slide value
			Gx is linked with Ex/Fx if Compat Gxx is off, just like Gxx is with Exx/Fxx
			Gx values: 1, 4, 8, 16, 32, 64, 96, 128, 255
			Ax/Bx/Cx/Dx values are used directly (i.e. D9 == D09), and are NOT shared with Dxx
			(value is stored into nOldVolParam and used by A0/B0/C0/D0)
			Hx uses the same value as Hxx and Uxx, and affects the *depth*
			so... hxx = (hx | (oldhxx & 0xf0))  ???
			TODO is this done correctly?
		*/
		bool doVolumeColumn = m_PlayState.m_nTickCount >= nStartTick;
		// FT2 compatibility: If there's a note delay, volume column effects are NOT executed
		// on the first tick and, if there's an instrument number, on the delayed tick.
		// Test case: VolColDelay.xm, PortaDelay.xm
		if(m_playBehaviour[kFT2VolColDelay] && nStartTick != 0)
		{
			doVolumeColumn = m_PlayState.m_nTickCount != 0 && (m_PlayState.m_nTickCount != nStartTick || (chn.rowCommand.instr == 0 && volcmd != VOLCMD_TONEPORTAMENTO));
		}
		if(volcmd > VOLCMD_PANNING && doVolumeColumn)
		{
			if (volcmd == VOLCMD_TONEPORTAMENTO)
			{
				const auto [porta, clearEffectCommand] = GetVolCmdTonePorta(chn.rowCommand, nStartTick);
				if(clearEffectCommand)
					cmd = CMD_NONE;

				TonePortamento(chn, porta);
			} else
			{
				// FT2 Compatibility: FT2 ignores some volume commands with parameter = 0.
				if(m_playBehaviour[kFT2VolColMemory] && vol == 0)
				{
					switch(volcmd)
					{
					case VOLCMD_VOLUME:
					case VOLCMD_PANNING:
					case VOLCMD_VIBRATODEPTH:
						break;
					case VOLCMD_PANSLIDELEFT:
						// FT2 Compatibility: Pan slide left with zero parameter causes panning to be set to full left on every non-row tick.
						// Test case: PanSlideZero.xm
						if(!m_SongFlags[SONG_FIRSTTICK])
						{
							chn.nPan = 0;
						}
						[[fallthrough]];
					default:
						// no memory here.
						volcmd = VOLCMD_NONE;
					}

				} else if(!m_playBehaviour[kITVolColMemory])
				{
					// IT Compatibility: Effects in the volume column don't have an unified memory.
					// Test case: VolColMemory.it
					if(vol) chn.nOldVolParam = static_cast<ModCommand::PARAM>(vol); else vol = chn.nOldVolParam;
				}

				switch(volcmd)
				{
				case VOLCMD_VOLSLIDEUP:
				case VOLCMD_VOLSLIDEDOWN:
					// IT Compatibility: Volume column volume slides have their own memory
					// Test case: VolColMemory.it
					if(vol == 0 && m_playBehaviour[kITVolColMemory])
					{
						vol = chn.nOldVolParam;
						if(vol == 0)
							break;
					} else
					{
						chn.nOldVolParam = static_cast<ModCommand::PARAM>(vol);
					}
					VolumeSlide(chn, static_cast<ModCommand::PARAM>(volcmd == VOLCMD_VOLSLIDEUP ? (vol << 4) : vol));
					break;

				case VOLCMD_FINEVOLUP:
					// IT Compatibility: Fine volume slides in the volume column are only executed on the first tick, not on multiples of the first tick in case of pattern delay
					// Test case: FineVolColSlide.it
					if(m_PlayState.m_nTickCount == nStartTick || !m_playBehaviour[kITVolColMemory])
					{
						// IT Compatibility: Volume column volume slides have their own memory
						// Test case: VolColMemory.it
						FineVolumeUp(chn, static_cast<ModCommand::PARAM>(vol), m_playBehaviour[kITVolColMemory]);
					}
					break;

				case VOLCMD_FINEVOLDOWN:
					// IT Compatibility: Fine volume slides in the volume column are only executed on the first tick, not on multiples of the first tick in case of pattern delay
					// Test case: FineVolColSlide.it
					if(m_PlayState.m_nTickCount == nStartTick || !m_playBehaviour[kITVolColMemory])
					{
						// IT Compatibility: Volume column volume slides have their own memory
						// Test case: VolColMemory.it
						FineVolumeDown(chn, static_cast<ModCommand::PARAM>(vol), m_playBehaviour[kITVolColMemory]);
					}
					break;

				case VOLCMD_VIBRATOSPEED:
					// FT2 does not automatically enable vibrato with the "set vibrato speed" command
					if(m_playBehaviour[kFT2VolColVibrato])
						chn.nVibratoSpeed = vol & 0x0F;
					else
						Vibrato(chn, vol << 4);
					break;

				case VOLCMD_VIBRATODEPTH:
					Vibrato(chn, vol);
					break;

				case VOLCMD_PANSLIDELEFT:
					PanningSlide(chn, static_cast<ModCommand::PARAM>(vol), !m_playBehaviour[kFT2VolColMemory]);
					break;

				case VOLCMD_PANSLIDERIGHT:
					PanningSlide(chn, static_cast<ModCommand::PARAM>(vol << 4), !m_playBehaviour[kFT2VolColMemory]);
					break;

				case VOLCMD_PORTAUP:
					// IT compatibility (one of the first testcases - link effect memory)
					PortamentoUp(nChn, static_cast<ModCommand::PARAM>(vol << 2), m_playBehaviour[kITVolColFinePortamento]);
					break;

				case VOLCMD_PORTADOWN:
					// IT compatibility (one of the first testcases - link effect memory)
					PortamentoDown(nChn, static_cast<ModCommand::PARAM>(vol << 2), m_playBehaviour[kITVolColFinePortamento]);
					break;

				case VOLCMD_OFFSET:
					if (triggerNote && chn.pModSample && vol <= CountOf(chn.pModSample->cues))
					{
						SmpLength offset;
						if(vol == 0)
							offset = chn.oldOffset;
						else
							offset = chn.oldOffset = chn.pModSample->cues[vol - 1];
						SampleOffset(chn, offset);
					}
					break;
				}
			}
		}

		// Effects
		if(cmd != CMD_NONE) switch (cmd)
		{
		// Set Volume
		case CMD_VOLUME:
			if(m_SongFlags[SONG_FIRSTTICK])
			{
				chn.nVolume = (param < 64) ? param * 4 : 256;
				chn.dwFlags.set(CHN_FASTVOLRAMP);
			}
			break;

		// Portamento Up
		case CMD_PORTAMENTOUP:
			if ((!param) && (GetType() & MOD_TYPE_MOD)) break;
			PortamentoUp(nChn, static_cast<ModCommand::PARAM>(param));
			break;

		// Portamento Down
		case CMD_PORTAMENTODOWN:
			if ((!param) && (GetType() & MOD_TYPE_MOD)) break;
			PortamentoDown(nChn, static_cast<ModCommand::PARAM>(param));
			break;

		// Volume Slide
		case CMD_VOLUMESLIDE:
			if (param || (GetType() != MOD_TYPE_MOD)) VolumeSlide(chn, static_cast<ModCommand::PARAM>(param));
			break;

		// Tone-Portamento
		case CMD_TONEPORTAMENTO:
			TonePortamento(chn, param);
			break;

		// Tone-Portamento + Volume Slide
		case CMD_TONEPORTAVOL:
			if ((param) || (GetType() != MOD_TYPE_MOD)) VolumeSlide(chn, static_cast<ModCommand::PARAM>(param));
			TonePortamento(chn, 0);
			break;

		// Vibrato
		case CMD_VIBRATO:
			Vibrato(chn, param);
			break;

		// Vibrato + Volume Slide
		case CMD_VIBRATOVOL:
			if ((param) || (GetType() != MOD_TYPE_MOD)) VolumeSlide(chn, static_cast<ModCommand::PARAM>(param));
			Vibrato(chn, 0);
			break;

		// Set Speed
		case CMD_SPEED:
			if(m_SongFlags[SONG_FIRSTTICK])
				SetSpeed(m_PlayState, param);
			break;

		// Set Tempo
		case CMD_TEMPO:
			if(m_playBehaviour[kMODVBlankTiming])
			{
				// ProTracker MODs with VBlank timing: All Fxx parameters set the tick count.
				if(m_SongFlags[SONG_FIRSTTICK] && param != 0) SetSpeed(m_PlayState, param);
				break;
			}
			{
				param = CalculateXParam(m_PlayState.m_nPattern, m_PlayState.m_nRow, nChn);
				if (GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))
				{
					if (param) chn.nOldTempo = static_cast<ModCommand::PARAM>(param); else param = chn.nOldTempo;
				}
				TEMPO t(param, 0);
				LimitMax(t, GetModSpecifications().GetTempoMax());
				SetTempo(t);
			}
			break;

		// Set Offset
		case CMD_OFFSET:
			if (triggerNote)
			{
				// FT2 compatibility: Portamento + Offset = Ignore offset
				// Test case: porta-offset.xm
				if(bPorta && GetType() == MOD_TYPE_XM)
				{
					break;
				}
				bool isExtended = false;
				SmpLength offset = CalculateXParam(m_PlayState.m_nPattern, m_PlayState.m_nRow, nChn, &isExtended);
				if(!isExtended)
				{
					// No X-param (normal behaviour)
					offset <<= 8;
					if (offset) chn.oldOffset = offset; else offset = chn.oldOffset;
					offset += static_cast<SmpLength>(chn.nOldHiOffset) << 16;
				}
				SampleOffset(chn, offset);
			}
			break;

		// Disorder Tracker 2 percentage offset
		case CMD_OFFSETPERCENTAGE:
			if(triggerNote)
			{
				SampleOffset(chn, Util::muldiv_unsigned(chn.nLength, param, 255));
			}
			break;

		// Arpeggio
		case CMD_ARPEGGIO:
			// IT compatibility 01. Don't ignore Arpeggio if no note is playing (also valid for ST3)
			if(m_PlayState.m_nTickCount) break;
			if((!chn.nPeriod || !chn.nNote)
				&& (chn.pModInstrument == nullptr || !chn.pModInstrument->HasValidMIDIChannel())	// Plugin arpeggio
				&& !m_playBehaviour[kITArpeggio] && (GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))) break;
			if (!param && (GetType() & (MOD_TYPE_XM | MOD_TYPE_MOD))) break;	// Only important when editing MOD/XM files (000 effects are removed when loading files where this means "no effect")
			chn.nCommand = CMD_ARPEGGIO;
			if (param) chn.nArpeggio = static_cast<ModCommand::PARAM>(param);
			break;

		// Retrig
		case CMD_RETRIG:
			if (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2))
			{
				if (!(param & 0xF0)) param |= chn.nRetrigParam & 0xF0;
				if (!(param & 0x0F)) param |= chn.nRetrigParam & 0x0F;
				param |= 0x100; // increment retrig count on first row
			}
			// IT compatibility 15. Retrigger
			if(m_playBehaviour[kITRetrigger])
			{
				if (param) chn.nRetrigParam = static_cast<uint8>(param & 0xFF);
				RetrigNote(nChn, chn.nRetrigParam, (volcmd == VOLCMD_OFFSET) ? vol + 1 : 0);
			} else
			{
				// XM Retrig
				if (param) chn.nRetrigParam = static_cast<uint8>(param & 0xFF); else param = chn.nRetrigParam;
				RetrigNote(nChn, param, (volcmd == VOLCMD_OFFSET) ? vol + 1 : 0);
			}
			break;

		// Tremor
		case CMD_TREMOR:
			if(!m_SongFlags[SONG_FIRSTTICK])
			{
				break;
			}

			// IT compatibility 12. / 13. Tremor (using modified DUMB's Tremor logic here because of old effects - http://dumb.sf.net/)
			if(m_playBehaviour[kITTremor])
			{
				if(param && !m_SongFlags[SONG_ITOLDEFFECTS])
				{
					// Old effects have different length interpretation (+1 for both on and off)
					if(param & 0xF0) param -= 0x10;
					if(param & 0x0F) param -= 0x01;
				}
				chn.nTremorCount |= 0x80; // set on/off flag
			} else if(m_playBehaviour[kFT2Tremor])
			{
				// XM Tremor. Logic is being processed in sndmix.cpp
				chn.nTremorCount |= 0x80; // set on/off flag
			}

			chn.nCommand = CMD_TREMOR;
			if (param) chn.nTremorParam = static_cast<ModCommand::PARAM>(param);

			break;

		// Set Global Volume
		case CMD_GLOBALVOLUME:
			// IT compatibility: Only apply global volume on first tick (and multiples)
			// Test case: GlobalVolFirstTick.it
			if(!m_SongFlags[SONG_FIRSTTICK])
				break;
			// ST3 applies global volume on tick 1 and does other weird things, but we won't emulate this for now.
// 			if(((GetType() & MOD_TYPE_S3M) && m_nTickCount != 1)
// 				|| (!(GetType() & MOD_TYPE_S3M) && !m_SongFlags[SONG_FIRSTTICK]))
// 			{
// 				break;
// 			}

			// FT2 compatibility: On channels that are "left" of the global volume command, the new global volume is not applied
			// until the second tick of the row. Since we apply global volume on the mix buffer rather than note volumes, this
			// cannot be fixed for now.
			// Test case: GlobalVolume.xm
// 			if(IsCompatibleMode(TRK_FASTTRACKER2) && m_SongFlags[SONG_FIRSTTICK] && m_nMusicSpeed > 1)
// 			{
// 				break;
// 			}

			if (!(GetType() & GLOBALVOL_7BIT_FORMATS)) param *= 2;

			// IT compatibility 16. ST3 and IT ignore out-of-range values.
			// Test case: globalvol-invalid.it
			if(param <= 128)
			{
				m_PlayState.m_nGlobalVolume = param * 2;
			} else if(!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_S3M)))
			{
				m_PlayState.m_nGlobalVolume = 256;
			}
			break;

		// Global Volume Slide
		case CMD_GLOBALVOLSLIDE:
			//IT compatibility 16. Saving last global volume slide param per channel (FT2/IT)
			if(m_playBehaviour[kPerChannelGlobalVolSlide])
				GlobalVolSlide(static_cast<ModCommand::PARAM>(param), chn.nOldGlobalVolSlide);
			else
				GlobalVolSlide(static_cast<ModCommand::PARAM>(param), m_PlayState.Chn[0].nOldGlobalVolSlide);
			break;

		// Set 8-bit Panning
		case CMD_PANNING8:
			if(m_SongFlags[SONG_FIRSTTICK])
			{
				Panning(chn, param, Pan8bit);
			}
			break;

		// Panning Slide
		case CMD_PANNINGSLIDE:
			PanningSlide(chn, static_cast<ModCommand::PARAM>(param));
			break;

		// Tremolo
		case CMD_TREMOLO:
			Tremolo(chn, param);
			break;

		// Fine Vibrato
		case CMD_FINEVIBRATO:
			FineVibrato(chn, param);
			break;

		// MOD/XM Exx Extended Commands
		case CMD_MODCMDEX:
			ExtendedMODCommands(nChn, static_cast<ModCommand::PARAM>(param));
			break;

		// S3M/IT Sxx Extended Commands
		case CMD_S3MCMDEX:
			if(m_playBehaviour[kST3EffectMemory] && param == 0)
			{
				param = chn.nArpeggio;	// S00 uses the last non-zero effect parameter as memory, like other effects including Arpeggio, so we "borrow" our memory there.
			}
			ExtendedS3MCommands(nChn, static_cast<ModCommand::PARAM>(param));
			break;

		// Key Off
		case CMD_KEYOFF:
			// This is how Key Off is supposed to sound... (in FT2 at least)
			if(m_playBehaviour[kFT2KeyOff])
			{
				if (m_PlayState.m_nTickCount == param)
				{
					// XM: Key-Off + Sample == Note Cut
					if(chn.pModInstrument == nullptr || !chn.pModInstrument->VolEnv.dwFlags[ENV_ENABLED])
					{
						if(param == 0 && (chn.rowCommand.instr || chn.rowCommand.volcmd != VOLCMD_NONE)) // FT2 is weird....
						{
							chn.dwFlags.set(CHN_NOTEFADE);
						}
						else
						{
							chn.dwFlags.set(CHN_FASTVOLRAMP);
							chn.nVolume = 0;
						}
					}
					KeyOff(chn);
				}
			}
			// This is how it's NOT supposed to sound...
			else
			{
				if(m_SongFlags[SONG_FIRSTTICK])
					KeyOff(chn);
			}
			break;

		// Extra-fine porta up/down
		case CMD_XFINEPORTAUPDOWN:
			switch(param & 0xF0)
			{
			case 0x10: ExtraFinePortamentoUp(chn, param & 0x0F); break;
			case 0x20: ExtraFinePortamentoDown(chn, param & 0x0F); break;
			// ModPlug XM Extensions (ignore in compatible mode)
			case 0x50:
			case 0x60:
			case 0x70:
			case 0x90:
			case 0xA0:
				if(!m_playBehaviour[kFT2RestrictXCommand]) ExtendedS3MCommands(nChn, static_cast<ModCommand::PARAM>(param));
				break;
			}
			break;

		// Set Channel Global Volume
		case CMD_CHANNELVOLUME:
			if(!m_SongFlags[SONG_FIRSTTICK]) break;
			if (param <= 64)
			{
				chn.nGlobalVol = param;
				chn.dwFlags.set(CHN_FASTVOLRAMP);
			}
			break;

		// Channel volume slide
		case CMD_CHANNELVOLSLIDE:
			ChannelVolSlide(chn, static_cast<ModCommand::PARAM>(param));
			break;

		// Panbrello (IT)
		case CMD_PANBRELLO:
			Panbrello(chn, param);
			break;

		// Set Envelope Position
		case CMD_SETENVPOSITION:
			if(m_SongFlags[SONG_FIRSTTICK])
			{
				chn.VolEnv.nEnvPosition = param;

				// FT2 compatibility: FT2 only sets the position of the panning envelope if the volume envelope's sustain flag is set
				// Test case: SetEnvPos.xm
				if(!m_playBehaviour[kFT2SetPanEnvPos] || chn.VolEnv.flags[ENV_SUSTAIN])
				{
					chn.PanEnv.nEnvPosition = param;
					chn.PitchEnv.nEnvPosition = param;
				}

			}
			break;

		// Position Jump
		case CMD_POSITIONJUMP:
			m_PlayState.m_nNextPatStartRow = 0; // FT2 E60 bug
			nPosJump = static_cast<ORDERINDEX>(CalculateXParam(m_PlayState.m_nPattern, m_PlayState.m_nRow, nChn));

			// see https://forum.openmpt.org/index.php?topic=2769.0 - FastTracker resets Dxx if Bxx is called _after_ Dxx
			// Test case: PatternJump.mod
			if((GetType() & (MOD_TYPE_MOD | MOD_TYPE_XM)) && nBreakRow != ROWINDEX_INVALID)
			{
				nBreakRow = 0;
			}
			break;

		// Pattern Break
		case CMD_PATTERNBREAK:
			{
				ROWINDEX row = PatternBreak(m_PlayState, nChn, static_cast<ModCommand::PARAM>(param));
				if(row != ROWINDEX_INVALID)
				{
					nBreakRow = row;
					if(m_SongFlags[SONG_PATTERNLOOP])
					{
						//If song is set to loop and a pattern break occurs we should stay on the same pattern.
						//Use nPosJump to force playback to "jump to this pattern" rather than move to next, as by default.
						//rewbs.to
						nPosJump = m_PlayState.m_nCurrentOrder;
					}
				}
			}
			break;

		// IMF / PTM Note Slides
		case CMD_NOTESLIDEUP:
		case CMD_NOTESLIDEDOWN:
		case CMD_NOTESLIDEUPRETRIG:
		case CMD_NOTESLIDEDOWNRETRIG:
			// Note that this command seems to be a bit buggy in Polytracker... Luckily, no tune seems to seriously use this
			// (Vic uses it e.g. in Spaceman or Perfect Reason to slide effect samples, noone will notice the difference :)
			NoteSlide(chn, param, cmd == CMD_NOTESLIDEUP || cmd == CMD_NOTESLIDEUPRETRIG, cmd == CMD_NOTESLIDEUPRETRIG || cmd == CMD_NOTESLIDEDOWNRETRIG);
			break;

		// PTM Reverse sample + offset (executed on every tick)
		case CMD_REVERSEOFFSET:
			ReverseSampleOffset(chn, static_cast<ModCommand::PARAM>(param));
			break;

#ifndef NO_PLUGINS
		// DBM: Toggle DSP Echo
		case CMD_DBMECHO:
			if(m_PlayState.m_nTickCount == 0)
			{
				uint32 chns = (param >> 4), enable = (param & 0x0F);
				if(chns > 1 || enable > 2)
				{
					break;
				}
				CHANNELINDEX firstChn = nChn, lastChn = nChn;
				if(chns == 1)
				{
					firstChn = 0;
					lastChn = m_nChannels - 1;
				}
				for(CHANNELINDEX c = firstChn; c <= lastChn; c++)
				{
					ChnSettings[c].dwFlags.set(CHN_NOFX, enable == 1);
					m_PlayState.Chn[c].dwFlags.set(CHN_NOFX, enable == 1);
				}
			}
			break;
#endif // NO_PLUGINS
		}

		if(m_playBehaviour[kST3EffectMemory] && param != 0)
		{
			UpdateS3MEffectMemory(chn, static_cast<ModCommand::PARAM>(param));
		}

		if(chn.rowCommand.instr)
		{
			// Not necessarily consistent with actually playing instrument for IT compatibility
			chn.nOldIns = chn.rowCommand.instr;
		}

	} // for(...) end

	// Navigation Effects
	if(m_SongFlags[SONG_FIRSTTICK])
	{
		const bool doPatternLoop = (nPatLoopRow != ROWINDEX_INVALID);
		const bool doBreakRow = (nBreakRow != ROWINDEX_INVALID);
		const bool doPosJump = (nPosJump != ORDERINDEX_INVALID);

		// Pattern Break / Position Jump only if no loop running
		// Exception: FastTracker 2 in all cases, Impulse Tracker in case of position jump
		// Test case for FT2 exception: PatLoop-Jumps.xm, PatLoop-Various.xm
		// Test case for IT: exception: LoopBreak.it, sbx-priority.it
		if((doBreakRow || doPosJump)
		   && (!doPatternLoop
		       || m_playBehaviour[kFT2PatternLoopWithJumps]
		       || (m_playBehaviour[kITPatternLoopWithJumps] && doPosJump)
		       || (m_playBehaviour[kITPatternLoopWithJumpsOld] && doPosJump)))
		{
			if(!doPosJump) nPosJump = m_PlayState.m_nCurrentOrder + 1;
			if(!doBreakRow) nBreakRow = 0;
			m_SongFlags.set(SONG_BREAKTOROW);

			if(nPosJump >= Order().size())
			{
				nPosJump = Order().GetRestartPos();
			}

			// IT / FT2 compatibility: don't reset loop count on pattern break.
			// Test case: gm-trippy01.it, PatLoop-Break.xm, PatLoop-Weird.xm, PatLoop-Break.mod
			if(nPosJump != m_PlayState.m_nCurrentOrder
				&& !m_playBehaviour[kITPatternLoopBreak] && !m_playBehaviour[kFT2PatternLoopWithJumps] && GetType() != MOD_TYPE_MOD)
			{
				for(CHANNELINDEX i = 0; i < GetNumChannels(); i++)
				{
					m_PlayState.Chn[i].nPatternLoopCount = 0;
				}
			}

			m_PlayState.m_nNextRow = nBreakRow;
			if(!m_SongFlags[SONG_PATTERNLOOP])
				m_PlayState.m_nNextOrder = nPosJump;
		} else if(doPatternLoop)
		{
			// Pattern Loop
			m_PlayState.m_nNextOrder = m_PlayState.m_nCurrentOrder;
			m_PlayState.m_nNextRow = nPatLoopRow;
			// FT2 skips the first row of the pattern loop if there's a pattern delay, ProTracker sometimes does it too (didn't quite figure it out yet).
			// But IT and ST3 don't do this.
			// Test cases: PatLoopWithDelay.it, PatLoopWithDelay.s3m
			if(m_PlayState.m_nPatternDelay
			   && (GetType() != MOD_TYPE_IT || !m_playBehaviour[kITPatternLoopWithJumps])
			   && GetType() != MOD_TYPE_S3M)
			{
				m_PlayState.m_nNextRow++;
			}

			// IT Compatibility: If the restart row is past the end of the current pattern
			// (e.g. when continued from a previous pattern without explicit SB0 effect), continue the next pattern.
			// Test case: LoopStartAfterPatternEnd.it
			if(nPatLoopRow >= Patterns[m_PlayState.m_nPattern].GetNumRows())
			{
				m_PlayState.m_nNextOrder++;
				m_PlayState.m_nNextRow = 0;
			}

			// As long as the pattern loop is running, mark the looped rows as not visited yet
			visitedSongRows.ResetPatternLoop(m_PlayState.m_nCurrentOrder, nPatLoopRow);
		}
	}
	return true;
}


////////////////////////////////////////////////////////////
// Channels effects


// Update the effect memory of all S3M effects that use the last non-zero effect parameter as memory (Dxy, Exx, Fxx, Ixy, Jxy, Kxy, Lxy, Qxy, Rxy, Sxy)
// Test case: ParamMemory.s3m
void CSoundFile::UpdateS3MEffectMemory(ModChannel &chn, ModCommand::PARAM param) const
{
	chn.nOldVolumeSlide = param; // Dxy / Kxy / Lxy
	chn.nOldPortaUp = param;     // Exx / Fxx
	chn.nOldPortaDown = param;   // Exx / Fxx
	chn.nTremorParam = param;    // Ixy
	chn.nArpeggio = param;       // Jxy
	chn.nRetrigParam = param;    // Qxy
	chn.nTremoloDepth = (param & 0x0F) << 2; // Rxy
	chn.nTremoloSpeed = (param >> 4) & 0x0F; // Rxy
	// Sxy is not handled here.
}


// Calculate full parameter for effects that support parameter extension at the given pattern location.
// maxCommands sets the maximum number of XParam commands to look at for this effect
// isExtended returns if the command is actually using any XParam extensions.
uint32 CSoundFile::CalculateXParam(PATTERNINDEX pat, ROWINDEX row, CHANNELINDEX chn, bool *isExtended) const
{
	if(isExtended != nullptr) *isExtended = false;
	ROWINDEX maxCommands = 4;
	const ModCommand *m = Patterns[pat].GetpModCommand(row, chn);
	uint32 val = m->param;

	switch(m->command)
	{
	case CMD_OFFSET:
		// 24 bit command
		maxCommands = 2;
		break;
	case CMD_TEMPO:
	case CMD_PATTERNBREAK:
	case CMD_POSITIONJUMP:
		// 16 bit command
		maxCommands = 1;
		break;
	default:
		return val;
	}

	const bool xmTempoFix = m->command == CMD_TEMPO && GetType() == MOD_TYPE_XM;
	ROWINDEX numRows = std::min(Patterns[pat].GetNumRows() - row - 1, maxCommands);
	while(numRows > 0)
	{
		m += Patterns[pat].GetNumChannels();
		if(m->command != CMD_XPARAM)
		{
			break;
		}
		if(xmTempoFix && val < 256)
		{
			// With XM, 0x20 is the lowest tempo. Anything below changes ticks per row.
			val -= 0x20;
		}
		val = (val << 8) | m->param;
		numRows--;
		if(isExtended != nullptr) *isExtended = true;
	}
	return val;
}


ROWINDEX CSoundFile::PatternBreak(PlayState &state, CHANNELINDEX chn, uint8 param) const
{
	if(param >= 64 && (GetType() & MOD_TYPE_S3M))
	{
		// ST3 ignores invalid pattern breaks.
		return ROWINDEX_INVALID;
	}

	state.m_nNextPatStartRow = 0; // FT2 E60 bug

	return static_cast<ROWINDEX>(CalculateXParam(state.m_nPattern, state.m_nRow, chn));
}


void CSoundFile::PortamentoUp(CHANNELINDEX nChn, ModCommand::PARAM param, const bool doFinePortamentoAsRegular)
{
	ModChannel &chn = m_PlayState.Chn[nChn];

	if(param)
	{
		// FT2 compatibility: Separate effect memory for all portamento commands
		// Test case: Porta-LinkMem.xm
		if(!m_playBehaviour[kFT2PortaUpDownMemory])
			chn.nOldPortaDown = param;
		chn.nOldPortaUp = param;
	} else
	{
		param = chn.nOldPortaUp;
	}

	const bool doFineSlides = !doFinePortamentoAsRegular && !(GetType() & (MOD_TYPE_MOD | MOD_TYPE_XM | MOD_TYPE_MT2 | MOD_TYPE_MED | MOD_TYPE_AMF0 | MOD_TYPE_DIGI | MOD_TYPE_STP | MOD_TYPE_DTM));

	// Process MIDI pitch bend for instrument plugins
	MidiPortamento(nChn, param, doFineSlides);

	if(GetType() == MOD_TYPE_MPT && chn.pModInstrument && chn.pModInstrument->pTuning)
	{
		// Portamento for instruments with custom tuning
		if(param >= 0xF0 && !doFinePortamentoAsRegular)
			PortamentoFineMPT(chn, param - 0xF0);
		else if(param >= 0xE0 && !doFinePortamentoAsRegular)
			PortamentoExtraFineMPT(chn, param - 0xE0);
		else
			PortamentoMPT(chn, param);
		return;
	} else if(GetType() == MOD_TYPE_PLM)
	{
		// A normal portamento up or down makes a follow-up tone portamento go the same direction.
		chn.nPortamentoDest = 1;
	}

	if (doFineSlides && param >= 0xE0)
	{
		if (param & 0x0F)
		{
			if ((param & 0xF0) == 0xF0)
			{
				FinePortamentoUp(chn, param & 0x0F);
				return;
			} else if ((param & 0xF0) == 0xE0 && GetType() != MOD_TYPE_DBM)
			{
				ExtraFinePortamentoUp(chn, param & 0x0F);
				return;
			}
		}
		if(GetType() != MOD_TYPE_DBM)
		{
			// DBM only has fine slides, no extra-fine slides.
			return;
		}
	}
	// Regular Slide
	if(!chn.isFirstTick
	   || (m_PlayState.m_nMusicSpeed == 1 && m_playBehaviour[kSlidesAtSpeed1])
	   || GetType() == MOD_TYPE_669
	   || (GetType() == MOD_TYPE_MED && m_SongFlags[SONG_FASTVOLSLIDES]))
	{
		DoFreqSlide(chn, -int(param) * 4);
	}
}


void CSoundFile::PortamentoDown(CHANNELINDEX nChn, ModCommand::PARAM param, const bool doFinePortamentoAsRegular)
{
	ModChannel &chn = m_PlayState.Chn[nChn];

	if(param)
	{
		// FT2 compatibility: Separate effect memory for all portamento commands
		// Test case: Porta-LinkMem.xm
		if(!m_playBehaviour[kFT2PortaUpDownMemory])
			chn.nOldPortaUp = param;
		chn.nOldPortaDown = param;
	} else
	{
		param = chn.nOldPortaDown;
	}

	const bool doFineSlides = !doFinePortamentoAsRegular && !(GetType() & (MOD_TYPE_MOD | MOD_TYPE_XM | MOD_TYPE_MT2 | MOD_TYPE_MED | MOD_TYPE_AMF0 | MOD_TYPE_DIGI | MOD_TYPE_STP | MOD_TYPE_DTM));

	// Process MIDI pitch bend for instrument plugins
	MidiPortamento(nChn, -static_cast<int>(param), doFineSlides);

	if(GetType() == MOD_TYPE_MPT && chn.pModInstrument && chn.pModInstrument->pTuning)
	{
		// Portamento for instruments with custom tuning
		if(param >= 0xF0 && !doFinePortamentoAsRegular)
			PortamentoFineMPT(chn, -static_cast<int>(param - 0xF0));
		else if(param >= 0xE0 && !doFinePortamentoAsRegular)
			PortamentoExtraFineMPT(chn, -static_cast<int>(param - 0xE0));
		else
			PortamentoMPT(chn, -static_cast<int>(param));
		return;
	} else if(GetType() == MOD_TYPE_PLM)
	{
		// A normal portamento up or down makes a follow-up tone portamento go the same direction.
		chn.nPortamentoDest = 65535;
	}

	if(doFineSlides && param >= 0xE0)
	{
		if (param & 0x0F)
		{
			if ((param & 0xF0) == 0xF0)
			{
				FinePortamentoDown(chn, param & 0x0F);
				return;
			} else if ((param & 0xF0) == 0xE0 && GetType() != MOD_TYPE_DBM)
			{
				ExtraFinePortamentoDown(chn, param & 0x0F);
				return;
			}
		}
		if(GetType() != MOD_TYPE_DBM)
		{
			// DBM only has fine slides, no extra-fine slides.
			return;
		}
	}

	if(!chn.isFirstTick
	   || (m_PlayState.m_nMusicSpeed == 1 && m_playBehaviour[kSlidesAtSpeed1])
	   || GetType() == MOD_TYPE_669
	   || (GetType() == MOD_TYPE_MED && m_SongFlags[SONG_FASTVOLSLIDES]))
	{
		DoFreqSlide(chn, int(param) * 4);
	}
}


// Send portamento commands to plugins
void CSoundFile::MidiPortamento(CHANNELINDEX nChn, int param, bool doFineSlides)
{
	int actualParam = std::abs(param);
	int pitchBend = 0;

	// Old MIDI Pitch Bends:
	// - Applied on every tick
	// - No fine pitch slides (they are interpreted as normal slides)
	// New MIDI Pitch Bends:
	// - Behaviour identical to sample pitch bends if the instrument's PWD parameter corresponds to the actual VSTi setting.

	if(doFineSlides && actualParam >= 0xE0 && !m_playBehaviour[kOldMIDIPitchBends])
	{
		if(m_PlayState.Chn[nChn].isFirstTick)
		{
			// Extra fine slide...
			pitchBend = (actualParam & 0x0F) * sgn(param);
			if(actualParam >= 0xF0)
			{
				// ... or just a fine slide!
				pitchBend *= 4;
			}
		}
	} else if(!m_PlayState.Chn[nChn].isFirstTick || m_playBehaviour[kOldMIDIPitchBends])
	{
		// Regular slide
		pitchBend = param * 4;
	}

	if(pitchBend)
	{
#ifndef NO_PLUGINS
		IMixPlugin *plugin = GetChannelInstrumentPlugin(nChn);
		if(plugin != nullptr)
		{
			int8 pwd = 13;	// Early OpenMPT legacy... Actually it's not *exactly* 13, but close enough...
			if(m_PlayState.Chn[nChn].pModInstrument != nullptr)
			{
				pwd = m_PlayState.Chn[nChn].pModInstrument->midiPWD;
			}
			plugin->MidiPitchBend(pitchBend, pwd, nChn);
		}
#endif // NO_PLUGINS
	}
}


void CSoundFile::FinePortamentoUp(ModChannel &chn, ModCommand::PARAM param) const
{
	MPT_ASSERT(!chn.HasCustomTuning());
	if(GetType() == MOD_TYPE_XM)
	{
		// FT2 compatibility: E1x / E2x / X1x / X2x memory is not linked
		// Test case: Porta-LinkMem.xm
		if(param) chn.nOldFinePortaUpDown = (chn.nOldFinePortaUpDown & 0x0F) | (param << 4); else param = (chn.nOldFinePortaUpDown >> 4);
	} else if(GetType() == MOD_TYPE_MT2)
	{
		if(param) chn.nOldFinePortaUpDown = param; else param = chn.nOldFinePortaUpDown;
	}

	if(chn.isFirstTick)
	{
		if ((chn.nPeriod) && (param))
		{
			if(m_SongFlags[SONG_LINEARSLIDES] && GetType() != MOD_TYPE_XM)
			{
				const auto oldPeriod = chn.nPeriod;
				chn.nPeriod = Util::muldivr(chn.nPeriod, GetLinearSlideUpTable(this, param & 0x0F), 65536);
				if(oldPeriod == chn.nPeriod)
				{
					if(m_playBehaviour[kHertzInLinearMode] && chn.nPeriod < Util::MaxValueOfType(chn.nPeriod))
						chn.nPeriod++;
					else if(!m_playBehaviour[kHertzInLinearMode] && chn.nPeriod > 1)
						chn.nPeriod--;
				}
			} else
			{
				chn.nPeriod -= (int)(param * 4);
				if (chn.nPeriod < 1)
				{
					chn.nPeriod = 1;
					if(GetType() == MOD_TYPE_S3M)
					{
						chn.nFadeOutVol = 0;
						chn.dwFlags.set(CHN_NOTEFADE | CHN_FASTVOLRAMP);
					}
				}
			}
		}
	}
}


void CSoundFile::FinePortamentoDown(ModChannel &chn, ModCommand::PARAM param) const
{
	MPT_ASSERT(!chn.HasCustomTuning());
	if(GetType() == MOD_TYPE_XM)
	{
		// FT2 compatibility: E1x / E2x / X1x / X2x memory is not linked
		// Test case: Porta-LinkMem.xm
		if(param) chn.nOldFinePortaUpDown = (chn.nOldFinePortaUpDown & 0xF0) | (param & 0x0F); else param = (chn.nOldFinePortaUpDown & 0x0F);
	} else if(GetType() == MOD_TYPE_MT2)
	{
		if(param) chn.nOldFinePortaUpDown = param; else param = chn.nOldFinePortaUpDown;
	}

	if(chn.isFirstTick)
	{
		if ((chn.nPeriod) && (param))
		{
			if (m_SongFlags[SONG_LINEARSLIDES] && GetType() != MOD_TYPE_XM)
			{
				const auto oldPeriod = chn.nPeriod;
				chn.nPeriod = Util::muldivr(chn.nPeriod, GetLinearSlideDownTable(this, param & 0x0F), 65536);
				if(oldPeriod == chn.nPeriod)
				{
					if(!m_playBehaviour[kHertzInLinearMode] && chn.nPeriod < Util::MaxValueOfType(chn.nPeriod))
						chn.nPeriod++;
					else if(m_playBehaviour[kHertzInLinearMode] && chn.nPeriod > 1)
						chn.nPeriod--;
				}
			} else
			{
				chn.nPeriod += (int)(param * 4);
				if (chn.nPeriod > 0xFFFF) chn.nPeriod = 0xFFFF;
			}
		}
	}
}


void CSoundFile::ExtraFinePortamentoUp(ModChannel &chn, ModCommand::PARAM param) const
{
	MPT_ASSERT(!chn.HasCustomTuning());
	if(GetType() == MOD_TYPE_XM)
	{
		// FT2 compatibility: E1x / E2x / X1x / X2x memory is not linked
		// Test case: Porta-LinkMem.xm
		if(param) chn.nOldExtraFinePortaUpDown = (chn.nOldExtraFinePortaUpDown & 0x0F) | (param << 4); else param = (chn.nOldExtraFinePortaUpDown >> 4);
	} else if(GetType() == MOD_TYPE_MT2)
	{
		if(param) chn.nOldFinePortaUpDown = param; else param = chn.nOldFinePortaUpDown;
	}

	if(chn.isFirstTick)
	{
		if ((chn.nPeriod) && (param))
		{
			if(m_SongFlags[SONG_LINEARSLIDES] && GetType() != MOD_TYPE_XM)
			{
				int oldPeriod = chn.nPeriod;
				chn.nPeriod = Util::muldivr(chn.nPeriod, GetFineLinearSlideUpTable(this, param & 0x0F), 65536);
				if(oldPeriod == chn.nPeriod) chn.nPeriod++;
			} else
			{
				chn.nPeriod -= (int)(param);
				if (chn.nPeriod < 1)
				{
					chn.nPeriod = 1;
					if(GetType() == MOD_TYPE_S3M)
					{
						chn.nFadeOutVol = 0;
						chn.dwFlags.set(CHN_NOTEFADE | CHN_FASTVOLRAMP);
					}
				}
			}
		}
	}
}


void CSoundFile::ExtraFinePortamentoDown(ModChannel &chn, ModCommand::PARAM param) const
{
	MPT_ASSERT(!chn.HasCustomTuning());
	if(GetType() == MOD_TYPE_XM)
	{
		// FT2 compatibility: E1x / E2x / X1x / X2x memory is not linked
		// Test case: Porta-LinkMem.xm
		if(param) chn.nOldExtraFinePortaUpDown = (chn.nOldExtraFinePortaUpDown & 0xF0) | (param & 0x0F); else param = (chn.nOldExtraFinePortaUpDown & 0x0F);
	} else if(GetType() == MOD_TYPE_MT2)
	{
		if(param) chn.nOldFinePortaUpDown = param; else param = chn.nOldFinePortaUpDown;
	}

	if(chn.isFirstTick)
	{
		if ((chn.nPeriod) && (param))
		{
			if(m_SongFlags[SONG_LINEARSLIDES] && GetType() != MOD_TYPE_XM)
			{
				int oldPeriod = chn.nPeriod;
				chn.nPeriod = Util::muldivr(chn.nPeriod, GetFineLinearSlideDownTable(this, param & 0x0F), 65536);
				if(oldPeriod == chn.nPeriod) chn.nPeriod--;
			} else
			{
				chn.nPeriod += (int)(param);
				if (chn.nPeriod > 0xFFFF) chn.nPeriod = 0xFFFF;
			}
		}
	}
}

// Implemented for IMF compatibility, can't actually save this in any formats
// Slide up / down every x ticks by y semitones
void CSoundFile::NoteSlide(ModChannel &chn, uint32 param, bool slideUp, bool retrig) const
{
	uint8 x, y;
	if(m_SongFlags[SONG_FIRSTTICK])
	{
		x = param & 0xF0;
		if (x)
			chn.nNoteSlideSpeed = (x >> 4);
		y = param & 0x0F;
		if (y)
			chn.nNoteSlideStep = y;
		chn.nNoteSlideCounter = chn.nNoteSlideSpeed;
	} else
	{
		if (--chn.nNoteSlideCounter == 0)
		{
			chn.nNoteSlideCounter = chn.nNoteSlideSpeed;
			// update it
			const int32 delta = (slideUp ? 1 : -1) * chn.nNoteSlideStep;
			if(chn.HasCustomTuning())
				chn.m_PortamentoFineSteps += delta * chn.pModInstrument->pTuning->GetFineStepCount();
			else
				chn.nPeriod = GetPeriodFromNote(delta + GetNoteFromPeriod(chn.nPeriod), 8363, 0);

			if(retrig)
			{
				chn.position.Set(0);
			}
		}
	}
}


std::pair<uint32, bool> CSoundFile::GetVolCmdTonePorta(const ModCommand &m, uint32 startTick) const
{
	if(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_AMS | MOD_TYPE_DMF | MOD_TYPE_DBM | MOD_TYPE_IMF | MOD_TYPE_PSM | MOD_TYPE_J2B | MOD_TYPE_ULT | MOD_TYPE_OKT | MOD_TYPE_MT2 | MOD_TYPE_MDL))
	{
		return {ImpulseTrackerPortaVolCmd[m.vol & 0x0F], false};
	} else
	{
		bool clearEffectColumn = false;
		uint32 vol = m.vol;
		if(m.command == CMD_TONEPORTAMENTO && GetType() == MOD_TYPE_XM)
		{
			// Yes, FT2 is *that* weird. If there is a Mx command in the volume column
			// and a normal 3xx command, the 3xx command is ignored but the Mx command's
			// effectiveness is doubled.
			// Test case: TonePortamentoMemory.xm
			clearEffectColumn = true;
			vol *= 2;
		}

		// FT2 compatibility: If there's a portamento and a note delay, execute the portamento, but don't update the parameter
		// Test case: PortaDelay.xm
		if(m_playBehaviour[kFT2PortaDelay] && startTick != 0)
			return {0, clearEffectColumn};
		else
			return {vol * 16, clearEffectColumn};
	}
}


// Portamento Slide
void CSoundFile::TonePortamento(ModChannel &chn, uint32 param) const
{
	chn.dwFlags.set(CHN_PORTAMENTO);

	//IT compatibility 03: Share effect memory with portamento up/down
	if((!m_SongFlags[SONG_ITCOMPATGXX] && m_playBehaviour[kITPortaMemoryShare]) || GetType() == MOD_TYPE_PLM)
	{
		if(param == 0) param = chn.nOldPortaUp;
		chn.nOldPortaUp = chn.nOldPortaDown = static_cast<uint8>(param);
	}

	if(chn.HasCustomTuning())
	{
		//Behavior: Param tells number of finesteps(or 'fullsteps'(notes) with glissando)
		//to slide per row(not per tick).
		const int32 oldPortamentoTickSlide = (m_PlayState.m_nTickCount != 0) ? chn.m_PortamentoTickSlide : 0;

		if(param)
			chn.nPortamentoSlide = param;
		else
			if(chn.nPortamentoSlide == 0)
				return;


		if((chn.nPortamentoDest > 0 && chn.nPortamentoSlide < 0) ||
			(chn.nPortamentoDest < 0 && chn.nPortamentoSlide > 0))
			chn.nPortamentoSlide = -chn.nPortamentoSlide;

		chn.m_PortamentoTickSlide = static_cast<int32>((m_PlayState.m_nTickCount + 1.0) * chn.nPortamentoSlide / m_PlayState.m_nMusicSpeed);

		if(chn.dwFlags[CHN_GLISSANDO])
		{
			chn.m_PortamentoTickSlide *= chn.pModInstrument->pTuning->GetFineStepCount() + 1;
			//With glissando interpreting param as notes instead of finesteps.
		}

		const int32 slide = chn.m_PortamentoTickSlide - oldPortamentoTickSlide;

		if(std::abs(chn.nPortamentoDest) <= std::abs(slide))
		{
			if(chn.nPortamentoDest != 0)
			{
				chn.m_PortamentoFineSteps += chn.nPortamentoDest;
				chn.nPortamentoDest = 0;
				chn.m_CalculateFreq = true;
			}
		} else
		{
			chn.m_PortamentoFineSteps += slide;
			chn.nPortamentoDest -= slide;
			chn.m_CalculateFreq = true;
		}

		return;
	}

	bool doPorta = !chn.isFirstTick
	               || (GetType() & (MOD_TYPE_DBM | MOD_TYPE_669))
	               || (m_PlayState.m_nMusicSpeed == 1 && m_playBehaviour[kSlidesAtSpeed1])
	               || (GetType() == MOD_TYPE_MED && m_SongFlags[SONG_FASTVOLSLIDES]);
	if(GetType() == MOD_TYPE_PLM && param >= 0xF0)
	{
		param -= 0xF0;
		doPorta = chn.isFirstTick;
	}

	if(param)
	{
		if(GetType() == MOD_TYPE_669)
		{
			param *= 10;
		}
		chn.nPortamentoSlide = param * 4;
	}

	if(chn.nPeriod && chn.nPortamentoDest && doPorta)
	{
		if (chn.nPeriod < chn.nPortamentoDest)
		{
			int32 delta = chn.nPortamentoSlide;
			if(m_SongFlags[SONG_LINEARSLIDES] && GetType() != MOD_TYPE_XM)
			{
				uint32 n = chn.nPortamentoSlide / 4;
				if (n > 255) n = 255;
				// Return (a*b+c/2)/c - no divide error
				// Table is 65536*2(n/192)
				delta = Util::muldivr(chn.nPeriod, LinearSlideUpTable[n], 65536) - chn.nPeriod;
				if (delta < 1) delta = 1;
			}
			chn.nPeriod += delta;
			if (chn.nPeriod > chn.nPortamentoDest) chn.nPeriod = chn.nPortamentoDest;
		} else
		if (chn.nPeriod > chn.nPortamentoDest)
		{
			int32 delta = -chn.nPortamentoSlide;
			if(m_SongFlags[SONG_LINEARSLIDES] && GetType() != MOD_TYPE_XM)
			{
				uint32 n = chn.nPortamentoSlide / 4;
				if (n > 255) n = 255;
				delta = Util::muldivr(chn.nPeriod, LinearSlideDownTable[n], 65536) - chn.nPeriod;
				if (delta > -1) delta = -1;
			}
			chn.nPeriod += delta;
			if (chn.nPeriod < chn.nPortamentoDest) chn.nPeriod = chn.nPortamentoDest;
		}
	}

	// IT compatibility 23. Portamento with no note
	// ProTracker also disables portamento once the target is reached.
	// Test case: PortaTarget.mod
	if(chn.nPeriod == chn.nPortamentoDest && (m_playBehaviour[kITPortaTargetReached] || GetType() == MOD_TYPE_MOD))
		chn.nPortamentoDest = 0;

}


void CSoundFile::Vibrato(ModChannel &chn, uint32 param) const
{
	if (param & 0x0F) chn.nVibratoDepth = (param & 0x0F) * 4;
	if (param & 0xF0) chn.nVibratoSpeed = (param >> 4) & 0x0F;
	chn.dwFlags.set(CHN_VIBRATO);
}


void CSoundFile::FineVibrato(ModChannel &chn, uint32 param) const
{
	if (param & 0x0F) chn.nVibratoDepth = param & 0x0F;
	if (param & 0xF0) chn.nVibratoSpeed = (param >> 4) & 0x0F;
	chn.dwFlags.set(CHN_VIBRATO);
	// ST3 compatibility: Do not distinguish between vibrato types in effect memory
	// Test case: VibratoTypeChange.s3m
	if(m_playBehaviour[kST3VibratoMemory] && (param & 0x0F))
	{
		chn.nVibratoDepth *= 4u;
	}
}


void CSoundFile::Panbrello(ModChannel &chn, uint32 param) const
{
	if (param & 0x0F) chn.nPanbrelloDepth = param & 0x0F;
	if (param & 0xF0) chn.nPanbrelloSpeed = (param >> 4) & 0x0F;
}


void CSoundFile::Panning(ModChannel &chn, uint32 param, PanningType panBits) const
{
	// No panning in ProTracker mode
	if(m_playBehaviour[kMODIgnorePanning])
	{
		return;
	}
	// IT Compatibility (and other trackers as well): panning disables surround (unless panning in rear channels is enabled, which is not supported by the original trackers anyway)
	if (!m_SongFlags[SONG_SURROUNDPAN] && (panBits == Pan8bit || m_playBehaviour[kPanOverride]))
	{
		chn.dwFlags.reset(CHN_SURROUND);
	}
	if(panBits == Pan4bit)
	{
		// 0...15 panning
		chn.nPan = (param * 256 + 8) / 15;
	} else if(panBits == Pan6bit)
	{
		// 0...64 panning
		if(param > 64) param = 64;
		chn.nPan = param * 4;
	} else
	{
		if(!(GetType() & (MOD_TYPE_S3M | MOD_TYPE_DSM | MOD_TYPE_AMF0 | MOD_TYPE_AMF | MOD_TYPE_MTM)))
		{
			// Real 8-bit panning
			chn.nPan = param;
		} else
		{
			// 7-bit panning + surround
			if(param <= 0x80)
			{
				chn.nPan = param << 1;
			} else if(param == 0xA4)
			{
				chn.dwFlags.set(CHN_SURROUND);
				chn.nPan = 0x80;
			}
		}
	}

	chn.dwFlags.set(CHN_FASTVOLRAMP);
	chn.nRestorePanOnNewNote = 0;
	//IT compatibility 20. Set pan overrides random pan
	if(m_playBehaviour[kPanOverride])
	{
		chn.nPanSwing = 0;
		chn.nPanbrelloOffset = 0;
	}
}


void CSoundFile::VolumeSlide(ModChannel &chn, ModCommand::PARAM param)
{
	if (param)
		chn.nOldVolumeSlide = param;
	else
		param = chn.nOldVolumeSlide;

	if((GetType() & (MOD_TYPE_MOD | MOD_TYPE_XM | MOD_TYPE_MT2 | MOD_TYPE_MED | MOD_TYPE_DIGI | MOD_TYPE_STP | MOD_TYPE_DTM)))
	{
		// MOD / XM nibble priority
		if((param & 0xF0) != 0)
		{
			param &= 0xF0;
		} else
		{
			param &= 0x0F;
		}
	}

	int newVolume = chn.nVolume;
	if(!(GetType() & (MOD_TYPE_MOD | MOD_TYPE_XM | MOD_TYPE_AMF0 | MOD_TYPE_MED | MOD_TYPE_DIGI)))
	{
		if ((param & 0x0F) == 0x0F) //Fine upslide or slide -15
		{
			if (param & 0xF0) //Fine upslide
			{
				FineVolumeUp(chn, (param >> 4), false);
				return;
			} else //Slide -15
			{
				if(chn.isFirstTick && !m_SongFlags[SONG_FASTVOLSLIDES])
				{
					newVolume -= 0x0F * 4;
				}
			}
		} else
		if ((param & 0xF0) == 0xF0) //Fine downslide or slide +15
		{
			if (param & 0x0F) //Fine downslide
			{
				FineVolumeDown(chn, (param & 0x0F), false);
				return;
			} else //Slide +15
			{
				if(chn.isFirstTick && !m_SongFlags[SONG_FASTVOLSLIDES])
				{
					newVolume += 0x0F * 4;
				}
			}
		}
	}
	if(!chn.isFirstTick || m_SongFlags[SONG_FASTVOLSLIDES] || (m_PlayState.m_nMusicSpeed == 1 && GetType() == MOD_TYPE_DBM))
	{
		// IT compatibility: Ignore slide commands with both nibbles set.
		if (param & 0x0F)
		{
			if(!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) || (param & 0xF0) == 0)
				newVolume -= (int)((param & 0x0F) * 4);
		}
		else
		{
			newVolume += (int)((param & 0xF0) >> 2);
		}
		if (GetType() == MOD_TYPE_MOD) chn.dwFlags.set(CHN_FASTVOLRAMP);
	}
	newVolume = Clamp(newVolume, 0, 256);

	chn.nVolume = newVolume;
}


void CSoundFile::PanningSlide(ModChannel &chn, ModCommand::PARAM param, bool memory)
{
	if(memory)
	{
		// FT2 compatibility: Use effect memory (lxx and rxx in XM shouldn't use effect memory).
		// Test case: PanSlideMem.xm
		if(param)
			chn.nOldPanSlide = param;
		else
			param = chn.nOldPanSlide;
	}

	if((GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2)))
	{
		// XM nibble priority
		if((param & 0xF0) != 0)
		{
			param &= 0xF0;
		} else
		{
			param &= 0x0F;
		}
	}

	int32 nPanSlide = 0;

	if(!(GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2)))
	{
		if (((param & 0x0F) == 0x0F) && (param & 0xF0))
		{
			if(m_SongFlags[SONG_FIRSTTICK])
			{
				param = (param & 0xF0) / 4u;
				nPanSlide = - (int)param;
			}
		} else if (((param & 0xF0) == 0xF0) && (param & 0x0F))
		{
			if(m_SongFlags[SONG_FIRSTTICK])
			{
				nPanSlide = (param & 0x0F) * 4u;
			}
		} else if(!m_SongFlags[SONG_FIRSTTICK])
		{
			if (param & 0x0F)
			{
				// IT compatibility: Ignore slide commands with both nibbles set.
				if(!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) || (param & 0xF0) == 0)
					nPanSlide = (int)((param & 0x0F) * 4u);
			} else
			{
				nPanSlide = -(int)((param & 0xF0) / 4u);
			}
		}
	} else
	{
		if(!m_SongFlags[SONG_FIRSTTICK])
		{
			if (param & 0xF0)
			{
				nPanSlide = (int)((param & 0xF0) / 4u);
			} else
			{
				nPanSlide = -(int)((param & 0x0F) * 4u);
			}
			// FT2 compatibility: FT2's panning slide is like IT's fine panning slide (not as deep)
			if(m_playBehaviour[kFT2PanSlide])
				nPanSlide /= 4;
		}
	}
	if (nPanSlide)
	{
		nPanSlide += chn.nPan;
		nPanSlide = Clamp(nPanSlide, 0, 256);
		chn.nPan = nPanSlide;
		chn.nRestorePanOnNewNote = 0;
	}
}


void CSoundFile::FineVolumeUp(ModChannel &chn, ModCommand::PARAM param, bool volCol) const
{
	if(GetType() == MOD_TYPE_XM)
	{
		// FT2 compatibility: EAx / EBx memory is not linked
		// Test case: FineVol-LinkMem.xm
		if(param) chn.nOldFineVolUpDown = (param << 4) | (chn.nOldFineVolUpDown & 0x0F); else param = (chn.nOldFineVolUpDown >> 4);
	} else if(volCol)
	{
		if(param) chn.nOldVolParam = param; else param = chn.nOldVolParam;
	} else
	{
		if(param) chn.nOldFineVolUpDown = param; else param = chn.nOldFineVolUpDown;
	}

	if(chn.isFirstTick)
	{
		chn.nVolume += param * 4;
		if(chn.nVolume > 256) chn.nVolume = 256;
		if(GetType() & MOD_TYPE_MOD) chn.dwFlags.set(CHN_FASTVOLRAMP);
	}
}


void CSoundFile::FineVolumeDown(ModChannel &chn, ModCommand::PARAM param, bool volCol) const
{
	if(GetType() == MOD_TYPE_XM)
	{
		// FT2 compatibility: EAx / EBx memory is not linked
		// Test case: FineVol-LinkMem.xm
		if(param) chn.nOldFineVolUpDown = param | (chn.nOldFineVolUpDown & 0xF0); else param = (chn.nOldFineVolUpDown & 0x0F);
	} else if(volCol)
	{
		if(param) chn.nOldVolParam = param; else param = chn.nOldVolParam;
	} else
	{
		if(param) chn.nOldFineVolUpDown = param; else param = chn.nOldFineVolUpDown;
	}

	if(chn.isFirstTick)
	{
		chn.nVolume -= param * 4;
		if(chn.nVolume < 0) chn.nVolume = 0;
		if(GetType() & MOD_TYPE_MOD) chn.dwFlags.set(CHN_FASTVOLRAMP);
	}
}


void CSoundFile::Tremolo(ModChannel &chn, uint32 param) const
{
	if (param & 0x0F) chn.nTremoloDepth = (param & 0x0F) << 2;
	if (param & 0xF0) chn.nTremoloSpeed = (param >> 4) & 0x0F;
	chn.dwFlags.set(CHN_TREMOLO);
}


void CSoundFile::ChannelVolSlide(ModChannel &chn, ModCommand::PARAM param) const
{
	int32 nChnSlide = 0;
	if (param) chn.nOldChnVolSlide = param; else param = chn.nOldChnVolSlide;

	if (((param & 0x0F) == 0x0F) && (param & 0xF0))
	{
		if(m_SongFlags[SONG_FIRSTTICK]) nChnSlide = param >> 4;
	} else if (((param & 0xF0) == 0xF0) && (param & 0x0F))
	{
		if(m_SongFlags[SONG_FIRSTTICK]) nChnSlide = - (int)(param & 0x0F);
	} else
	{
		if(!m_SongFlags[SONG_FIRSTTICK])
		{
			if (param & 0x0F)
			{
				if(!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_J2B | MOD_TYPE_DBM)) || (param & 0xF0) == 0)
					nChnSlide = -(int)(param & 0x0F);
			} else
			{
				nChnSlide = (int)((param & 0xF0) >> 4);
			}
		}
	}
	if (nChnSlide)
	{
		nChnSlide += chn.nGlobalVol;
		nChnSlide = Clamp(nChnSlide, 0, 64);
		chn.nGlobalVol = nChnSlide;
	}
}


void CSoundFile::ExtendedMODCommands(CHANNELINDEX nChn, ModCommand::PARAM param)
{
	ModChannel &chn = m_PlayState.Chn[nChn];
	uint8 command = param & 0xF0;
	param &= 0x0F;
	switch(command)
	{
	// E0x: Set Filter
	case 0x00:
		for(CHANNELINDEX channel = 0; channel < GetNumChannels(); channel++)
		{
			m_PlayState.Chn[channel].dwFlags.set(CHN_AMIGAFILTER, !(param & 1));
		}
		break;
	// E1x: Fine Portamento Up
	case 0x10:	if ((param) || (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2))) FinePortamentoUp(chn, param); break;
	// E2x: Fine Portamento Down
	case 0x20:	if ((param) || (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2))) FinePortamentoDown(chn, param); break;
	// E3x: Set Glissando Control
	case 0x30:	chn.dwFlags.set(CHN_GLISSANDO, param != 0); break;
	// E4x: Set Vibrato WaveForm
	case 0x40:	chn.nVibratoType = param & 0x07; break;
	// E5x: Set FineTune
	case 0x50:	if(!m_SongFlags[SONG_FIRSTTICK])
					break;
				if(GetType() & (MOD_TYPE_MOD | MOD_TYPE_DIGI | MOD_TYPE_AMF0 | MOD_TYPE_MED))
				{
					chn.nFineTune = MOD2XMFineTune(param);
					if(chn.nPeriod && chn.rowCommand.IsNote()) chn.nPeriod = GetPeriodFromNote(chn.nNote, chn.nFineTune, chn.nC5Speed);
				} else if(GetType() == MOD_TYPE_MTM)
				{
					if(chn.rowCommand.IsNote() && chn.pModSample != nullptr)
					{
						// Effect is permanent in MultiTracker
						const_cast<ModSample *>(chn.pModSample)->nFineTune = param;
						chn.nFineTune = param;
						if(chn.nPeriod) chn.nPeriod = GetPeriodFromNote(chn.nNote, chn.nFineTune, chn.nC5Speed);
					}
				} else if(chn.rowCommand.IsNote())
				{
					chn.nFineTune = MOD2XMFineTune(param - 8);
					if(chn.nPeriod) chn.nPeriod = GetPeriodFromNote(chn.nNote, chn.nFineTune, chn.nC5Speed);
				}
				break;
	// E6x: Pattern Loop
	// E7x: Set Tremolo WaveForm
	case 0x70:	chn.nTremoloType = param & 0x07; break;
	// E8x: Set 4-bit Panning
	case 0x80:
		if(m_SongFlags[SONG_FIRSTTICK])
		{
			Panning(chn, param, Pan4bit);
		}
		break;
	// E9x: Retrig
	case 0x90:	RetrigNote(nChn, param); break;
	// EAx: Fine Volume Up
	case 0xA0:	if ((param) || (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2))) FineVolumeUp(chn, param, false); break;
	// EBx: Fine Volume Down
	case 0xB0:	if ((param) || (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2))) FineVolumeDown(chn, param, false); break;
	// ECx: Note Cut
	case 0xC0:	NoteCut(nChn, param, false); break;
	// EDx: Note Delay
	// EEx: Pattern Delay
	case 0xF0:
		if(GetType() == MOD_TYPE_MOD) // MOD: Invert Loop
		{
			chn.nEFxSpeed = param;
			if(m_SongFlags[SONG_FIRSTTICK]) InvertLoop(chn);
		} else // XM: Set Active Midi Macro
		{
			chn.nActiveMacro = param;
		}
		break;
	}
}


void CSoundFile::ExtendedS3MCommands(CHANNELINDEX nChn, ModCommand::PARAM param)
{
	ModChannel &chn = m_PlayState.Chn[nChn];
	uint8 command = param & 0xF0;
	param &= 0x0F;
	switch(command)
	{
	// S0x: Set Filter
	// S1x: Set Glissando Control
	case 0x10:	chn.dwFlags.set(CHN_GLISSANDO, param != 0); break;
	// S2x: Set FineTune
	case 0x20:	if(!m_SongFlags[SONG_FIRSTTICK])
					break;
				if(GetType() != MOD_TYPE_669)
				{
					chn.nC5Speed = S3MFineTuneTable[param];
					chn.nFineTune = MOD2XMFineTune(param);
					if(chn.nPeriod)
						chn.nPeriod = GetPeriodFromNote(chn.nNote, chn.nFineTune, chn.nC5Speed);
				} else if(chn.pModSample != nullptr)
				{
					chn.nC5Speed = chn.pModSample->nC5Speed + param * 80;
				}
				break;
	// S3x: Set Vibrato Waveform
	case 0x30:	if(GetType() == MOD_TYPE_S3M)
				{
					chn.nVibratoType = param & 0x03;
				} else
				{
					// IT compatibility: Ignore waveform types > 3
					if(m_playBehaviour[kITVibratoTremoloPanbrello])
						chn.nVibratoType = (param < 0x04) ? param : 0;
					else
						chn.nVibratoType = param & 0x07;
				}
				break;
	// S4x: Set Tremolo Waveform
	case 0x40:	if(GetType() == MOD_TYPE_S3M)
				{
					chn.nTremoloType = param & 0x03;
				} else
				{
					// IT compatibility: Ignore waveform types > 3
					if(m_playBehaviour[kITVibratoTremoloPanbrello])
						chn.nTremoloType = (param < 0x04) ? param : 0;
					else
						chn.nTremoloType = param & 0x07;
				}
				break;
	// S5x: Set Panbrello Waveform
	case 0x50:
		// IT compatibility: Ignore waveform types > 3
				if(m_playBehaviour[kITVibratoTremoloPanbrello])
				{
					chn.nPanbrelloType = (param < 0x04) ? param : 0;
					chn.nPanbrelloPos = 0;
				} else
				{
					chn.nPanbrelloType = param & 0x07;
				}
				break;
	// S6x: Pattern Delay for x frames
	case 0x60:
				if(m_SongFlags[SONG_FIRSTTICK] && m_PlayState.m_nTickCount == 0)
				{
					// Tick delays are added up.
					// Scream Tracker 3 does actually not support this command.
					// We'll use the same behaviour as for Impulse Tracker, as we can assume that
					// most S3Ms that make use of this command were made with Impulse Tracker.
					// MPT added this command to the XM format through the X6x effect, so we will use
					// the same behaviour here as well.
					// Test cases: PatternDelays.it, PatternDelays.s3m, PatternDelays.xm
					m_PlayState.m_nFrameDelay += param;
				}
				break;
	// S7x: Envelope Control / Instrument Control
	case 0x70:	if(!m_SongFlags[SONG_FIRSTTICK]) break;
				switch(param)
				{
				case 0:
				case 1:
				case 2:
					{
						for (CHANNELINDEX i = m_nChannels; i < MAX_CHANNELS; i++)
						{
							ModChannel &bkChn = m_PlayState.Chn[i];
							if (bkChn.nMasterChn == nChn + 1)
							{
								if (param == 1)
								{
									KeyOff(bkChn);
									if(bkChn.dwFlags[CHN_ADLIB] && m_opl)
										m_opl->NoteOff(i);
								} else if (param == 2)
								{
									bkChn.dwFlags.set(CHN_NOTEFADE);
									if(bkChn.dwFlags[CHN_ADLIB] && m_opl)
										m_opl->NoteOff(i);
								} else
								{
									bkChn.dwFlags.set(CHN_NOTEFADE);
									bkChn.nFadeOutVol = 0;
									if(bkChn.dwFlags[CHN_ADLIB] && m_opl)
										m_opl->NoteCut(i);
								}
#ifndef NO_PLUGINS
								const ModInstrument *pIns = bkChn.pModInstrument;
								IMixPlugin *pPlugin;
								if(pIns != nullptr && pIns->nMixPlug && (pPlugin = m_MixPlugins[pIns->nMixPlug - 1].pMixPlugin) != nullptr)
								{
									pPlugin->MidiCommand(*pIns, bkChn.nNote + NOTE_MAX_SPECIAL, 0, nChn);
								}
#endif // NO_PLUGINS
							}
						}
					}
					break;
				case 3:		chn.nNNA = NNA_NOTECUT; break;
				case 4:		chn.nNNA = NNA_CONTINUE; break;
				case 5:		chn.nNNA = NNA_NOTEOFF; break;
				case 6:		chn.nNNA = NNA_NOTEFADE; break;
				case 7:		chn.VolEnv.flags.reset(ENV_ENABLED); break;
				case 8:		chn.VolEnv.flags.set(ENV_ENABLED); break;
				case 9:		chn.PanEnv.flags.reset(ENV_ENABLED); break;
				case 10:	chn.PanEnv.flags.set(ENV_ENABLED); break;
				case 11:	chn.PitchEnv.flags.reset(ENV_ENABLED); break;
				case 12:	chn.PitchEnv.flags.set(ENV_ENABLED); break;
				case 13:	// S7D: Enable pitch envelope, force to play as pitch envelope
				case 14:	// S7E: Enable pitch envelope, force to play as filter envelope
					if(GetType() == MOD_TYPE_MPT)
					{
						chn.PitchEnv.flags.set(ENV_ENABLED);
						chn.PitchEnv.flags.set(ENV_FILTER, param != 13);
					}
					break;
				}
				break;
	// S8x: Set 4-bit Panning
	case 0x80:
		if(m_SongFlags[SONG_FIRSTTICK])
		{
			Panning(chn, param, Pan4bit);
		}
		break;
	// S9x: Sound Control
	case 0x90:	ExtendedChannelEffect(chn, param); break;
	// SAx: Set 64k Offset
	case 0xA0:	if(m_SongFlags[SONG_FIRSTTICK])
				{
					chn.nOldHiOffset = static_cast<uint8>(param);
					if (!m_playBehaviour[kITHighOffsetNoRetrig] && chn.rowCommand.IsNote())
					{
						SmpLength pos = param << 16;
						if (pos < chn.nLength) chn.position.SetInt(pos);
					}
				}
				break;
	// SBx: Pattern Loop
	// SCx: Note Cut
	case 0xC0:
		if(param == 0)
		{
			//IT compatibility 22. SC0 == SC1
			if(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))
				param = 1;
			// ST3 doesn't cut notes with SC0
			else if(GetType() == MOD_TYPE_S3M)
				return;
		}
		// S3M/IT compatibility: Note Cut really cuts notes and does not just mute them (so that following volume commands could restore the sample)
		// Test case: scx.it
		NoteCut(nChn, param, m_playBehaviour[kITSCxStopsSample] || GetType() == MOD_TYPE_S3M);
		break;
	// SDx: Note Delay
	// SEx: Pattern Delay for x rows
	// SFx: S3M: Not used, IT: Set Active Midi Macro
	case 0xF0:
		if(GetType() != MOD_TYPE_S3M)
		{
			chn.nActiveMacro = static_cast<uint8>(param);
		}
		break;
	}
}


void CSoundFile::ExtendedChannelEffect(ModChannel &chn, uint32 param)
{
	// S9x and X9x commands (S3M/XM/IT only)
	if(!m_SongFlags[SONG_FIRSTTICK]) return;
	switch(param & 0x0F)
	{
	// S90: Surround Off
	case 0x00:	chn.dwFlags.reset(CHN_SURROUND);	break;
	// S91: Surround On
	case 0x01:	chn.dwFlags.set(CHN_SURROUND); chn.nPan = 128; break;

	////////////////////////////////////////////////////////////
	// ModPlug Extensions
	// S98: Reverb Off
	case 0x08:
		chn.dwFlags.reset(CHN_REVERB);
		chn.dwFlags.set(CHN_NOREVERB);
		break;
	// S99: Reverb On
	case 0x09:
		chn.dwFlags.reset(CHN_NOREVERB);
		chn.dwFlags.set(CHN_REVERB);
		break;
	// S9A: 2-Channels surround mode
	case 0x0A:
		m_SongFlags.reset(SONG_SURROUNDPAN);
		break;
	// S9B: 4-Channels surround mode
	case 0x0B:
		m_SongFlags.set(SONG_SURROUNDPAN);
		break;
	// S9C: IT Filter Mode
	case 0x0C:
		m_SongFlags.reset(SONG_MPTFILTERMODE);
		break;
	// S9D: MPT Filter Mode
	case 0x0D:
		m_SongFlags.set(SONG_MPTFILTERMODE);
		break;
	// S9E: Go forward
	case 0x0E:
		chn.dwFlags.reset(CHN_PINGPONGFLAG);
		break;
	// S9F: Go backward (and set playback position to the end if sample just started)
	case 0x0F:
		if(chn.position.IsZero() && chn.nLength && (chn.rowCommand.IsNote() || !chn.dwFlags[CHN_LOOP]))
		{
			chn.position.Set(chn.nLength - 1, SamplePosition::fractMax);
		}
		chn.dwFlags.set(CHN_PINGPONGFLAG);
		break;
	}
}


void CSoundFile::InvertLoop(ModChannel &chn)
{
	// EFx implementation for MOD files (PT 1.1A and up: Invert Loop)
	// This effect trashes samples. Thanks to 8bitbubsy for making this work. :)
	if(GetType() != MOD_TYPE_MOD || chn.nEFxSpeed == 0) return;

	// we obviously also need a sample for this
	ModSample *pModSample = const_cast<ModSample *>(chn.pModSample);
	if(pModSample == nullptr || !pModSample->HasSampleData() || !pModSample->uFlags[CHN_LOOP] || pModSample->uFlags[CHN_16BIT]) return;

	chn.nEFxDelay += ModEFxTable[chn.nEFxSpeed & 0x0F];
	if((chn.nEFxDelay & 0x80) == 0) return; // only applied if the "delay" reaches 128
	chn.nEFxDelay = 0;

	if (++chn.nEFxOffset >= pModSample->nLoopEnd - pModSample->nLoopStart)
		chn.nEFxOffset = 0;

	// TRASH IT!!! (Yes, the sample!)
	uint8 &sample = mpt::byte_cast<uint8 *>(pModSample->sampleb())[pModSample->nLoopStart + chn.nEFxOffset];
	sample = ~sample;
	pModSample->PrecomputeLoops(*this, false);
}


// Process a MIDI Macro.
// Parameters:
// [in] nChn: Mod channel to apply macro on
// [in] isSmooth: If true, internal macros are interpolated between two rows
// [in] macro: Actual MIDI Macro string
// [in] param: Parameter for parametric macros (Z00 - Z7F)
// [in] plugin: Plugin to send MIDI message to (if not specified but needed, it is autodetected)
void CSoundFile::ProcessMIDIMacro(CHANNELINDEX nChn, bool isSmooth, const char *macro, uint8 param, PLUGINDEX plugin)
{
	ModChannel &chn = m_PlayState.Chn[nChn];
	const ModInstrument *pIns = GetNumInstruments() ? chn.pModInstrument : nullptr;

	uint8 out[MACRO_LENGTH];
	uint32 outPos = 0;	// output buffer position, which also equals the number of complete bytes
	const uint8 lastZxxParam = chn.lastZxxParam;
	bool firstNibble = true;

	for(uint32 pos = 0; pos < (MACRO_LENGTH - 1) && macro[pos]; pos++)
	{
		bool isNibble = false;		// did we parse a nibble or a byte value?
		uint8 data = 0;		// data that has just been parsed

		// Parse next macro byte... See Impulse Tracker's MIDI.TXT for detailed information on each possible character.
		if(macro[pos] >= '0' && macro[pos] <= '9')
		{
			isNibble = true;
			data = static_cast<uint8>(macro[pos] - '0');
		}
		else if(macro[pos] >= 'A' && macro[pos] <= 'F')
		{
			isNibble = true;
			data = static_cast<uint8>(macro[pos] - 'A' + 0x0A);
		} else if(macro[pos] == 'c')
		{
			// MIDI channel
			isNibble = true;
			data = GetBestMidiChannel(nChn);
		} else if(macro[pos] == 'n')
		{
			// Last triggered note
			if(ModCommand::IsNote(chn.nLastNote))
			{
				data = chn.nLastNote - NOTE_MIN;
			}
		} else if(macro[pos] == 'v')
		{
			// Velocity
			// This is "almost" how IT does it - apparently, IT seems to lag one row behind on global volume or channel volume changes.
			const int swing = (m_playBehaviour[kITSwingBehaviour] || m_playBehaviour[kMPTOldSwingBehaviour]) ? chn.nVolSwing : 0;
			const int vol = Util::muldiv((chn.nVolume + swing) * m_PlayState.m_nGlobalVolume, chn.nGlobalVol * chn.nInsVol, 1 << 20);
			data = static_cast<uint8>(Clamp(vol / 2, 1, 127));
			//data = (unsigned char)std::min((chn.nVolume * chn.nGlobalVol * m_nGlobalVolume) >> (1 + 6 + 8), 127);
		} else if(macro[pos] == 'u')
		{
			// Calculated volume
			// Same note as with velocity applies here, but apparently also for instrument / sample volumes?
			const int vol = Util::muldiv(chn.nCalcVolume * m_PlayState.m_nGlobalVolume, chn.nGlobalVol * chn.nInsVol, 1 << 26);
			data = static_cast<uint8>(Clamp(vol / 2, 1, 127));
			//data = (unsigned char)std::min((chn.nCalcVolume * chn.nGlobalVol * m_nGlobalVolume) >> (7 + 6 + 8), 127);
		} else if(macro[pos] == 'x')
		{
			// Pan set
			data = static_cast<uint8>(std::min(static_cast<int>(chn.nPan / 2), 127));
		} else if(macro[pos] == 'y')
		{
			// Calculated pan
			data = static_cast<uint8>(std::min(static_cast<int>(chn.nRealPan / 2), 127));
		} else if(macro[pos] == 'a')
		{
			// High byte of bank select
			if(pIns && pIns->wMidiBank)
			{
				data = static_cast<uint8>(((pIns->wMidiBank - 1) >> 7) & 0x7F);
			}
		} else if(macro[pos] == 'b')
		{
			// Low byte of bank select
			if(pIns && pIns->wMidiBank)
			{
				data = static_cast<uint8>((pIns->wMidiBank - 1) & 0x7F);
			}
		} else if(macro[pos] == 'o')
		{
			// Offset (ignoring high offset)
			data = static_cast<uint8>((chn.oldOffset >> 8) & 0xFF);
		} else if(macro[pos] == 'h')
		{
			// Host channel number
			data = static_cast<uint8>((nChn >= GetNumChannels() ? (chn.nMasterChn - 1) : nChn) & 0x7F);
		} else if(macro[pos] == 'm')
		{
			// Loop direction (judging from the character, it was supposed to be loop type, though)
			data = chn.dwFlags[CHN_PINGPONGFLAG] ? 1 : 0;
		} else if(macro[pos] == 'p')
		{
			// Program select
			if(pIns && pIns->nMidiProgram)
			{
				data = static_cast<uint8>((pIns->nMidiProgram - 1) & 0x7F);
			}
		} else if(macro[pos] == 'z')
		{
			// Zxx parameter
			data = param & 0x7F;
			if(isSmooth && chn.lastZxxParam < 0x80
				&& (outPos < 3 || out[outPos - 3] != 0xF0 || out[outPos - 2] < 0xF0))
			{
				// Interpolation for external MIDI messages - interpolation for internal messages
				// is handled separately to allow for more than 7-bit granularity where it's possible
				data = static_cast<uint8>(CalculateSmoothParamChange(lastZxxParam, data));
			}
			chn.lastZxxParam = data;
		} else if(macro[pos] == 's')
		{
			// SysEx Checksum (not an original Impulse Tracker macro variable, but added for convenience)
			uint32 startPos = outPos;
			while(startPos > 0 && out[--startPos] != 0xF0);
			if(outPos - startPos < 5 || out[startPos] != 0xF0)
			{
				continue;
			}
			for(uint32 p = startPos + 5; p != outPos; p++)
			{
				data += out[p];
			}
			data = (~data + 1) & 0x7F;
		} else
		{
			// Unrecognized byte (e.g. space char)
			continue;
		}

		// Append parsed data
		if(isNibble)	// parsed a nibble (constant or 'c' variable)
		{
			if(firstNibble)
			{
				out[outPos] = data;
			} else
			{
				out[outPos] = (out[outPos] << 4) | data;
				outPos++;
			}
			firstNibble = !firstNibble;
		} else			// parsed a byte (variable)
		{
			if(!firstNibble)	// From MIDI.TXT: '9n' is exactly the same as '09 n' or '9 n' -- so finish current byte first
			{
				outPos++;
			}
			out[outPos++] = data;
			firstNibble = true;
		}
	}
	if(!firstNibble)
	{
		// Finish current byte
		outPos++;
	}

	// Macro string has been parsed and translated, now send the message(s)...
	uint32 sendPos = 0;
	uint8 runningStatus = 0;
	while(sendPos < outPos)
	{
		uint32 sendLen = 0;
		if(out[sendPos] == 0xF0)
		{
			// SysEx start
			if((outPos - sendPos >= 4) && (out[sendPos + 1] == 0xF0 || out[sendPos + 1] == 0xF1))
			{
				// Internal macro (normal (F0F0) or extended (F0F1)), 4 bytes long
				sendLen = 4;
			} else
			{
				// SysEx message, find end of message
				for(uint32 i = sendPos + 1; i < outPos; i++)
				{
					if(out[i] == 0xF7)
					{
						// Found end of SysEx message
						sendLen = i - sendPos + 1;
						break;
					}
				}
				if(sendLen == 0)
				{
					// Didn't find end, so "invent" end of SysEx message
					out[outPos++] = 0xF7;
					sendLen = outPos - sendPos;
				}
			}
		} else if(!(out[sendPos] & 0x80))
		{
			// Missing status byte? Try inserting running status
			if(runningStatus != 0)
			{
				sendPos--;
				out[sendPos] = runningStatus;
			} else
			{
				// No running status to re-use; skip this byte
				sendPos++;
			}
			continue;
		} else
		{
			// Other MIDI messages
			sendLen = std::min(static_cast<uint32>(MIDIEvents::GetEventLength(out[sendPos])), outPos - sendPos);
		}

		if(sendLen == 0)
		{
			break;
		}

		if(out[sendPos] < 0xF0)
		{
			runningStatus = out[sendPos];
		}
		uint32 bytesSent = SendMIDIData(nChn, isSmooth, out + sendPos, sendLen, plugin);
		// If there's no error in the macro data (e.g. unrecognized internal MIDI macro), we have sendLen == bytesSent.
		if(bytesSent > 0)
		{
			sendPos += bytesSent;
		} else
		{
			sendPos += sendLen;
		}
	}
}


// Calculate smooth MIDI macro slide parameter for current tick.
float CSoundFile::CalculateSmoothParamChange(float currentValue, float param) const
{
	MPT_ASSERT(GetNumTicksOnCurrentRow() > m_PlayState.m_nTickCount);
	const uint32 ticksLeft = GetNumTicksOnCurrentRow() - m_PlayState.m_nTickCount;
	if(ticksLeft > 1)
	{
		// Slide param
		const float step = (param - currentValue) / (float)ticksLeft;
		return (currentValue + step);
	} else
	{
		// On last tick, set exact value.
		return param;
	}
}


// Process exactly one MIDI message parsed by ProcessMIDIMacro. Returns bytes sent on success, 0 on (parse) failure.
uint32 CSoundFile::SendMIDIData(CHANNELINDEX nChn, bool isSmooth, const unsigned char *macro, uint32 macroLen, PLUGINDEX plugin)
{
	if(macroLen < 1)
	{
		return 0;
	}

	if(macro[0] == 0xFA || macro[0] == 0xFC || macro[0] == 0xFF)
	{
		// Start Song, Stop Song, MIDI Reset - both interpreted internally and sent to plugins
		for(CHANNELINDEX chn = 0; chn < GetNumChannels(); chn++)
		{
			m_PlayState.Chn[chn].nCutOff = 0x7F;
			m_PlayState.Chn[chn].nResonance = 0x00;
		}
	}

	ModChannel &chn = m_PlayState.Chn[nChn];
	if(macro[0] == 0xF0 && (macro[1] == 0xF0 || macro[1] == 0xF1))
	{
		// Internal device.
		if(macroLen < 4)
		{
			return 0;
		}
		const bool isExtended = (macro[1] == 0xF1);
		const uint8 macroCode = macro[2];
		const uint8 param = macro[3];

		if(macroCode == 0x00 && !isExtended && param < 0x80)
		{
			// F0.F0.00.xx: Set CutOff
			if(!isSmooth)
			{
				chn.nCutOff = param;
			} else
			{
				chn.nCutOff = mpt::saturate_round<uint8>(CalculateSmoothParamChange(chn.nCutOff, param));
			}
			chn.nRestoreCutoffOnNewNote = 0;
			int cutoff = SetupChannelFilter(chn, !chn.dwFlags[CHN_FILTER]);

			if(cutoff >= 0 && chn.dwFlags[CHN_ADLIB] && m_opl)
			{
				// Cutoff doubles as modulator intensity for FM instruments
				m_opl->Volume(nChn, static_cast<uint8>(cutoff / 4), true);
			}

			return 4;
		} else if(macroCode == 0x01 && !isExtended && param < 0x80)
		{
			// F0.F0.01.xx: Set Resonance
			if(!isSmooth)
			{
				chn.nResonance = param;
			} else
			{
				chn.nResonance = (uint8)CalculateSmoothParamChange((float)chn.nResonance, (float)param);
			}
			chn.nRestoreResonanceOnNewNote = 0;
			SetupChannelFilter(chn, !chn.dwFlags[CHN_FILTER]);

			return 4;
		} else if(macroCode == 0x02 && !isExtended)
		{
			// F0.F0.02.xx: Set filter mode (high nibble determines filter mode)
			if(param < 0x20)
			{
				chn.nFilterMode = static_cast<FilterMode>(param >> 4);
				SetupChannelFilter(chn, !chn.dwFlags[CHN_FILTER]);
			}

			return 4;
#ifndef NO_PLUGINS
		} else if(macroCode == 0x03 && !isExtended)
		{
			// F0.F0.03.xx: Set plug dry/wet
			const PLUGINDEX plug = (plugin != 0) ? plugin : GetBestPlugin(nChn, PrioritiseChannel, EvenIfMuted);
			if(plug > 0 && plug <= MAX_MIXPLUGINS && param < 0x80)
			{
				const float newRatio = (0x7F - (param & 0x7F)) / 127.0f;
				if(!isSmooth)
				{
					m_MixPlugins[plug - 1].fDryRatio = newRatio;
				} else
				{
					m_MixPlugins[plug - 1].fDryRatio = CalculateSmoothParamChange(m_MixPlugins[plug - 1].fDryRatio, newRatio);
				}
			}

			return 4;
		} else if((macroCode & 0x80) || isExtended)
		{
			// F0.F0.{80|n}.xx / F0.F1.n.xx: Set VST effect parameter n to xx
			const PLUGINDEX plug = (plugin != 0) ? plugin : GetBestPlugin(nChn, PrioritiseChannel, EvenIfMuted);
			const uint32 plugParam = isExtended ? (0x80 + macroCode) : (macroCode & 0x7F);
			if(plug > 0 && plug <= MAX_MIXPLUGINS)
			{
				IMixPlugin *pPlugin = m_MixPlugins[plug - 1].pMixPlugin;
				if(pPlugin && param < 0x80)
				{
					const float fParam = param / 127.0f;
					if(!isSmooth)
					{
						pPlugin->SetParameter(plugParam, fParam);
					} else
					{
						pPlugin->SetParameter(plugParam, CalculateSmoothParamChange(pPlugin->GetParameter(plugParam), fParam));
					}
				}
			}

			return 4;
#endif // NO_PLUGINS
		}

		// If we reach this point, the internal macro was invalid.

	} else
	{
#ifndef NO_PLUGINS
		// Not an internal device. Pass on to appropriate plugin.
		const CHANNELINDEX plugChannel = (nChn < GetNumChannels()) ? nChn + 1 : chn.nMasterChn;
		if(plugChannel > 0 && plugChannel <= GetNumChannels())	// XXX do we need this? I guess it might be relevant for previewing notes in the pattern... Or when using this mechanism for volume/panning!
		{
			PLUGINDEX plug = 0;
			if(!chn.dwFlags[CHN_NOFX])
			{
				plug = (plugin != 0) ? plugin : GetBestPlugin(nChn, PrioritiseChannel, EvenIfMuted);
			}

			if(plug > 0 && plug <= MAX_MIXPLUGINS)
			{
				IMixPlugin *pPlugin = m_MixPlugins[plug - 1].pMixPlugin;
				if (pPlugin != nullptr)
				{
					if(macro[0] == 0xF0)
					{
						pPlugin->MidiSysexSend(mpt::as_span(mpt::byte_cast<const std::byte*>(macro), macroLen));
					} else
					{
						uint32 len = std::min(static_cast<uint32>(MIDIEvents::GetEventLength(macro[0])), macroLen);
						uint32 curData = 0;
						memcpy(&curData, macro, len);
						pPlugin->MidiSend(curData);
					}
				}
			}
		}
#else
		MPT_UNREFERENCED_PARAMETER(plugin);
#endif // NO_PLUGINS

		return macroLen;

	}

	return 0;

}


void CSoundFile::SendMIDINote(CHANNELINDEX chn, uint16 note, uint16 volume)
{
#ifndef NO_PLUGINS
	auto &channel = m_PlayState.Chn[chn];
	const ModInstrument *pIns = channel.pModInstrument;
	// instro sends to a midi chan
	if (pIns && pIns->HasValidMIDIChannel())
	{
		PLUGINDEX plug = pIns->nMixPlug;
		if(plug > 0 && plug <= MAX_MIXPLUGINS)
		{
			IMixPlugin *pPlug = m_MixPlugins[plug - 1].pMixPlugin;
			if (pPlug != nullptr)
			{
				pPlug->MidiCommand(*pIns, note, volume, chn);
				if(note < NOTE_MIN_SPECIAL)
					channel.nLeftVU = channel.nRightVU = 0xFF;
			}
		}
	}
#endif // NO_PLUGINS
}


void CSoundFile::SampleOffset(ModChannel &chn, SmpLength param) const
{
	// ST3 compatibility: Instrument-less note recalls previous note's offset
	// Test case: OxxMemory.s3m
	if(m_playBehaviour[kST3OffsetWithoutInstrument])
		chn.prevNoteOffset = 0;
	
	chn.prevNoteOffset += param;

	if(param >= chn.nLoopEnd && (GetType() & (MOD_TYPE_S3M | MOD_TYPE_MTM)) && chn.dwFlags[CHN_LOOP] && chn.nLoopEnd > 0)
	{
		// Offset wrap-around
		// Note that ST3 only does this in GUS mode. SoundBlaster stops the sample entirely instead.
		// Test case: OffsetLoopWraparound.s3m
		param = (param - chn.nLoopStart) % (chn.nLoopEnd - chn.nLoopStart) + chn.nLoopStart;
	}

	if(GetType() == MOD_TYPE_MDL && chn.dwFlags[CHN_16BIT])
	{
		// Digitrakker really uses byte offsets, not sample offsets. WTF!
		param /= 2u;
	}

	if(chn.rowCommand.IsNote())
	{
		// IT compatibility: If this note is not mapped to a sample, ignore it.
		// Test case: empty_sample_offset.it
		if(chn.pModInstrument != nullptr)
		{
			SAMPLEINDEX smp = chn.pModInstrument->Keyboard[chn.rowCommand.note - NOTE_MIN];
			if(smp == 0 || smp > GetNumSamples())
				return;
		}

		if(m_SongFlags[SONG_PT_MODE])
		{
			// ProTracker compatbility: PT1/2-style funky 9xx offset command
			// Test case: ptoffset.mod
			chn.position.Set(chn.prevNoteOffset);
			chn.prevNoteOffset += param;
		} else
		{
			chn.position.Set(param);
		}

		if (chn.position.GetUInt() >= chn.nLength || (chn.dwFlags[CHN_LOOP] && chn.position.GetUInt() >= chn.nLoopEnd))
		{
			// Offset beyond sample size
			if(m_playBehaviour[kFT2ST3OffsetOutOfRange] || GetType() == MOD_TYPE_MTM)
			{
				// FT2 Compatibility: Don't play note if offset is beyond sample length
				// ST3 Compatibility: Don't play note if offset is beyond sample length (non-looped samples only)
				// Test cases: 3xx-no-old-samp.xm, OffsetPastSampleEnd.s3m
				chn.dwFlags.set(CHN_FASTVOLRAMP);
				chn.nPeriod = 0;
			} else if(!(GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2 | MOD_TYPE_MOD)))
			{
				// IT Compatibility: Offset
				if(m_playBehaviour[kITOffset])
				{
					if(m_SongFlags[SONG_ITOLDEFFECTS])
						chn.position.Set(chn.nLength); // Old FX: Clip to end of sample
					else
						chn.position.Set(0); // Reset to beginning of sample
				} else
				{
					chn.position.Set(chn.nLoopStart);
					if(m_SongFlags[SONG_ITOLDEFFECTS] && chn.nLength > 4)
					{
						chn.position.Set(chn.nLength - 2);
					}
				}
			} else if(GetType() == MOD_TYPE_MOD && chn.dwFlags[CHN_LOOP])
			{
				chn.position.Set(chn.nLoopStart);
			}
		}
	} else if ((param < chn.nLength) && (GetType() & (MOD_TYPE_MTM | MOD_TYPE_DMF | MOD_TYPE_MDL | MOD_TYPE_PLM)))
	{
		// Some trackers can also call offset effects without notes next to them...
		chn.position.Set(param);
	}
}


// 
void CSoundFile::ReverseSampleOffset(ModChannel &chn, ModCommand::PARAM param) const
{
	if(chn.pModSample != nullptr)
	{
		chn.dwFlags.set(CHN_PINGPONGFLAG);
		chn.dwFlags.reset(CHN_LOOP);
		chn.nLength = chn.pModSample->nLength;	// If there was a loop, extend sample to whole length.
		chn.position.Set((chn.nLength - 1) - std::min(SmpLength(param) << 8, chn.nLength - SmpLength(1)), 0);
	}
}


void CSoundFile::RetrigNote(CHANNELINDEX nChn, int param, int offset)
{
	// Retrig: bit 8 is set if it's the new XM retrig
	ModChannel &chn = m_PlayState.Chn[nChn];
	int retrigSpeed = param & 0x0F;
	int16 retrigCount = chn.nRetrigCount;
	bool doRetrig = false;

	// IT compatibility 15. Retrigger
	if(m_playBehaviour[kITRetrigger])
	{
		if(m_PlayState.m_nTickCount == 0 && chn.rowCommand.note)
		{
			chn.nRetrigCount = param & 0xf;
		} else if(!chn.nRetrigCount || !--chn.nRetrigCount)
		{
			chn.nRetrigCount = param & 0xf;
			doRetrig = true;
		}
	} else if(m_playBehaviour[kFT2Retrigger] && (param & 0x100))
	{
		// Buggy-like-hell FT2 Rxy retrig!
		// Test case: retrig.xm
		if(m_SongFlags[SONG_FIRSTTICK])
		{
			// Here are some really stupid things FT2 does on the first tick.
			// Test case: RetrigTick0.xm
			if(chn.rowCommand.instr > 0 && chn.rowCommand.IsNoteOrEmpty())
				retrigCount = 1;
			if(chn.rowCommand.volcmd == VOLCMD_VOLUME && chn.rowCommand.vol != 0)
			{
				// I guess this condition simply checked if the volume byte was != 0 in FT2.
				chn.nRetrigCount = retrigCount;
				return;
			}
		}
		if(retrigCount >= retrigSpeed)
		{
			if(!m_SongFlags[SONG_FIRSTTICK] || !chn.rowCommand.IsNote())
			{
				doRetrig = true;
				retrigCount = 0;
			}
		}
	} else
	{
		// old routines
		if (GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))
		{
			if(!retrigSpeed)
				retrigSpeed = 1;
			if(retrigCount && !(retrigCount % retrigSpeed))
				doRetrig = true;
			retrigCount++;
		} else if(GetType() == MOD_TYPE_MTM)
		{
			// In MultiTracker, E9x retriggers the last note at exactly the x-th tick of the row
			doRetrig = m_PlayState.m_nTickCount == static_cast<uint32>(param & 0x0F) && retrigSpeed != 0;
		} else
		{
			int realspeed = retrigSpeed;
			// FT2 bug: if a retrig (Rxy) occurs together with a volume command, the first retrig interval is increased by one tick
			if((param & 0x100) && (chn.rowCommand.volcmd == VOLCMD_VOLUME) && (chn.rowCommand.param & 0xF0))
				realspeed++;
			if(!m_SongFlags[SONG_FIRSTTICK] || (param & 0x100))
			{
				if(!realspeed)
					realspeed = 1;
				if(!(param & 0x100) && m_PlayState.m_nMusicSpeed && !(m_PlayState.m_nTickCount % realspeed))
					doRetrig = true;
				retrigCount++;
			} else if(GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2))
				retrigCount = 0;
			if (retrigCount >= realspeed)
			{
				if(m_PlayState.m_nTickCount || ((param & 0x100) && !chn.rowCommand.note))
					doRetrig = true;
			}
			if(m_playBehaviour[kFT2Retrigger] && param == 0)
			{
				// E90 = Retrig instantly, and only once
				doRetrig = (m_PlayState.m_nTickCount == 0);
			}
		}
	}

	// IT compatibility: If a sample is shorter than the retrig time (i.e. it stops before the retrig counter hits zero), it is not retriggered.
	// Test case: retrig-short.it
	if(chn.nLength == 0 && m_playBehaviour[kITShortSampleRetrig] && !chn.HasMIDIOutput())
		return;
	// ST3 compatibility: No retrig after Note Cut
	// Test case: RetrigAfterNoteCut.s3m
	if(m_playBehaviour[kST3RetrigAfterNoteCut] && !chn.nFadeOutVol)
		return;

	if(doRetrig)
	{
		uint32 dv = (param >> 4) & 0x0F;
		int vol = chn.nVolume;
		if(dv)
		{

			// FT2 compatibility: Retrig + volume will not change volume of retrigged notes
			if(!m_playBehaviour[kFT2Retrigger] || !(chn.rowCommand.volcmd == VOLCMD_VOLUME))
			{
				if(retrigTable1[dv])
					vol = (vol * retrigTable1[dv]) >> 4;
				else
					vol += ((int)retrigTable2[dv]) << 2;
			}
			Limit(vol, 0, 256);

			chn.dwFlags.set(CHN_FASTVOLRAMP);
		}
		uint32 note = chn.nNewNote;
		int32 oldPeriod = chn.nPeriod;
		const bool retrigAdlib = chn.dwFlags[CHN_ADLIB] && m_playBehaviour[kOPLRealRetrig];
		if(note >= NOTE_MIN && note <= NOTE_MAX && chn.nLength && retrigAdlib)
			CheckNNA(nChn, 0, note, true);
		bool resetEnv = false;
		if(GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2))
		{
			if(chn.rowCommand.instr && param < 0x100)
			{
				InstrumentChange(chn, chn.rowCommand.instr, false, false);
				resetEnv = true;
			}
			if(param < 0x100)
				resetEnv = true;
		}
		if(retrigAdlib && chn.pModSample && m_opl)
		{
			m_opl->NoteCut(nChn);
			m_opl->Patch(nChn, chn.pModSample->adlib);
		}

		const bool fading = chn.dwFlags[CHN_NOTEFADE];
		const auto oldPrevNoteOffset = chn.prevNoteOffset;
		chn.prevNoteOffset = 0;  // Retriggered notes should not use previous offset (test case: OxxMemoryWithRetrig.s3m)
		// IT compatibility: Really weird combination of envelopes and retrigger (see Storlek's q.it testcase)
		// Test case: retrig.it
		NoteChange(chn, note, m_playBehaviour[kITRetrigger], resetEnv, false, nChn);
		if(!chn.rowCommand.instr)
			chn.prevNoteOffset = oldPrevNoteOffset;
		// XM compatibility: Prevent NoteChange from resetting the fade flag in case an instrument number + note-off is present.
		// Test case: RetrigFade.xm
		if(fading && GetType() == MOD_TYPE_XM)
			chn.dwFlags.set(CHN_NOTEFADE);
		chn.nVolume = vol;
		if(m_nInstruments)
		{
			chn.rowCommand.note = static_cast<ModCommand::NOTE>(note);	// No retrig without note...
#ifndef NO_PLUGINS
			ProcessMidiOut(nChn);	//Send retrig to Midi
#endif // NO_PLUGINS
		}
		if((GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) && chn.rowCommand.note == NOTE_NONE && oldPeriod != 0)
			chn.nPeriod = oldPeriod;
		if(!(GetType() & (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT)))
			retrigCount = 0;
		// IT compatibility: see previous IT compatibility comment =)
		if(m_playBehaviour[kITRetrigger]) chn.position.Set(0);

		offset--;
		if(offset >= 0 && offset <= static_cast<int>(CountOf(chn.pModSample->cues)) && chn.pModSample != nullptr)
		{
			if(offset == 0)
				offset = chn.oldOffset;
			else
				offset = chn.oldOffset = chn.pModSample->cues[offset - 1];
			SampleOffset(chn, offset);
		}
	}

	// buggy-like-hell FT2 Rxy retrig!
	if(m_playBehaviour[kFT2Retrigger] && (param & 0x100))
		retrigCount++;

	// Now we can also store the retrig value for IT...
	if(!m_playBehaviour[kITRetrigger])
		chn.nRetrigCount = retrigCount;
}


void CSoundFile::DoFreqSlide(ModChannel &chn, int32 nFreqSlide) const
{
	if(!chn.nPeriod)
		return;
	MPT_ASSERT(!chn.HasCustomTuning());

	if(GetType() == MOD_TYPE_669)
	{
		// Like other oldskool trackers, Composer 669 doesn't have linear slides...
		// But the slides are done in Hertz rather than periods, meaning that they
		// are more effective in the lower notes (rather than the higher notes).
		nFreqSlide *= -20;
	}
	if(m_SongFlags[SONG_LINEARSLIDES] && GetType() != MOD_TYPE_XM)
	{
		// IT Linear slides
		const auto nOldPeriod = chn.nPeriod;
		uint32 n = std::abs(nFreqSlide) / 4u;
		LimitMax(n, 255u);
		if(n != 0)
		{
			chn.nPeriod = Util::muldivr(chn.nPeriod, nFreqSlide < 0 ? GetLinearSlideUpTable(this, n) : GetLinearSlideDownTable(this, n), 65536);
			if(chn.nPeriod == nOldPeriod)
			{
				const bool incPeriod = m_playBehaviour[kHertzInLinearMode] == (nFreqSlide < 0);
				if(incPeriod && chn.nPeriod < Util::MaxValueOfType(chn.nPeriod))
					chn.nPeriod++;
				else if(!incPeriod && chn.nPeriod > 1)
					chn.nPeriod--;
			}
		}
	} else
	{
		chn.nPeriod += nFreqSlide;
	}
	if (chn.nPeriod < 1)
	{
		chn.nPeriod = 1;
		if(GetType() == MOD_TYPE_S3M)
		{
			chn.nFadeOutVol = 0;
			chn.dwFlags.set(CHN_NOTEFADE | CHN_FASTVOLRAMP);
		}
	}
}


void CSoundFile::NoteCut(CHANNELINDEX nChn, uint32 nTick, bool cutSample)
{
	if (m_PlayState.m_nTickCount == nTick)
	{
		ModChannel &chn = m_PlayState.Chn[nChn];
		if(cutSample)
		{
			chn.increment.Set(0);
			chn.nFadeOutVol = 0;
			chn.dwFlags.set(CHN_NOTEFADE);
		} else
		{
			chn.nVolume = 0;
		}
		chn.dwFlags.set(CHN_FASTVOLRAMP);

		// instro sends to a midi chan
		SendMIDINote(nChn, /*chn.nNote+*/NOTE_MAX_SPECIAL, 0);
		
		if(chn.dwFlags[CHN_ADLIB] && m_opl)
		{
			m_opl->NoteCut(nChn, false);
		}
	}
}


void CSoundFile::KeyOff(ModChannel &chn) const
{
	const bool keyIsOn = !chn.dwFlags[CHN_KEYOFF];
	chn.dwFlags.set(CHN_KEYOFF);
	if(chn.pModInstrument != nullptr && !chn.VolEnv.flags[ENV_ENABLED])
	{
		chn.dwFlags.set(CHN_NOTEFADE);
	}
	if (!chn.nLength) return;
	if (chn.dwFlags[CHN_SUSTAINLOOP] && chn.pModSample && keyIsOn)
	{
		const ModSample *pSmp = chn.pModSample;
		if(pSmp->uFlags[CHN_LOOP])
		{
			if (pSmp->uFlags[CHN_PINGPONGLOOP])
				chn.dwFlags.set(CHN_PINGPONGLOOP);
			else
				chn.dwFlags.reset(CHN_PINGPONGLOOP | CHN_PINGPONGFLAG);
			chn.dwFlags.set(CHN_LOOP);
			chn.nLength = pSmp->nLength;
			chn.nLoopStart = pSmp->nLoopStart;
			chn.nLoopEnd = pSmp->nLoopEnd;
			if (chn.nLength > chn.nLoopEnd) chn.nLength = chn.nLoopEnd;
			if(chn.position.GetUInt() > chn.nLength)
			{
				// Test case: SusAfterLoop.it
				chn.position.Set(chn.nLoopStart + ((chn.position.GetInt() - chn.nLoopStart) % (chn.nLoopEnd - chn.nLoopStart)));
			}
		} else
		{
			chn.dwFlags.reset(CHN_LOOP | CHN_PINGPONGLOOP | CHN_PINGPONGFLAG);
			chn.nLength = pSmp->nLength;
		}
	}

	if (chn.pModInstrument)
	{
		const ModInstrument *pIns = chn.pModInstrument;
		if((pIns->VolEnv.dwFlags[ENV_LOOP] || (GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2 | MOD_TYPE_MDL))) && pIns->nFadeOut != 0)
		{
			chn.dwFlags.set(CHN_NOTEFADE);
		}

		if (pIns->VolEnv.nReleaseNode != ENV_RELEASE_NODE_UNSET && chn.VolEnv.nEnvValueAtReleaseJump == NOT_YET_RELEASED)
		{
			chn.VolEnv.nEnvValueAtReleaseJump = pIns->VolEnv.GetValueFromPosition(chn.VolEnv.nEnvPosition, 256);
			chn.VolEnv.nEnvPosition = pIns->VolEnv[pIns->VolEnv.nReleaseNode].tick;
		}
	}
}


//////////////////////////////////////////////////////////
// CSoundFile: Global Effects


void CSoundFile::SetSpeed(PlayState &playState, uint32 param) const
{
#ifdef MODPLUG_TRACKER
	// FT2 appears to be decrementing the tick count before checking for zero,
	// so it effectively counts down 65536 ticks with speed = 0 (song speed is a 16-bit variable in FT2)
	if(GetType() == MOD_TYPE_XM && !param)
	{
		playState.m_nMusicSpeed = uint16_max;
	}
#endif	// MODPLUG_TRACKER
	if(param > 0) playState.m_nMusicSpeed = param;
	if(GetType() == MOD_TYPE_STM && param > 0)
	{
		playState.m_nMusicSpeed = std::max(param >> 4, uint32(1));
		playState.m_nMusicTempo = ConvertST2Tempo(static_cast<uint8>(param));
	}
}


// Convert a ST2 tempo byte to classic tempo and speed combination
TEMPO CSoundFile::ConvertST2Tempo(uint8 tempo)
{
	static constexpr uint8 ST2TempoFactor[] = { 140, 50, 25, 15, 10, 7, 6, 4, 3, 3, 2, 2, 2, 2, 1, 1 };
	static constexpr uint32 st2MixingRate = 23863; // Highest possible setting in ST2

	// This underflows at tempo 06...0F, and the resulting tick lengths depend on the mixing rate.
	int32 samplesPerTick = st2MixingRate / (49 - ((ST2TempoFactor[tempo >> 4u] * (tempo & 0x0F)) >> 4u));
	if(samplesPerTick <= 0)
		samplesPerTick += 65536;
	return TEMPO().SetRaw(Util::muldivrfloor(st2MixingRate, 5 * TEMPO::fractFact, samplesPerTick * 2));
}


void CSoundFile::SetTempo(TEMPO param, bool setFromUI)
{
	const CModSpecifications &specs = GetModSpecifications();

	// Anything lower than the minimum tempo is considered to be a tempo slide
	const TEMPO minTempo = (GetType() == MOD_TYPE_MDL) ? TEMPO(1, 0) : TEMPO(32, 0);

	if(setFromUI)
	{
		// Set tempo from UI - ignore slide commands and such.
		m_PlayState.m_nMusicTempo = Clamp(param, specs.GetTempoMin(), specs.GetTempoMax());
	} else if(param >= minTempo && m_SongFlags[SONG_FIRSTTICK] == !m_playBehaviour[kMODTempoOnSecondTick])
	{
		// ProTracker sets the tempo after the first tick.
		// Note: The case of one tick per row is handled in ProcessRow() instead.
		// Test case: TempoChange.mod
		m_PlayState.m_nMusicTempo = std::min(param, specs.GetTempoMax());
	} else if(param < minTempo && !m_SongFlags[SONG_FIRSTTICK])
	{
		// Tempo Slide
		TEMPO tempDiff(param.GetInt() & 0x0F, 0);
		if((param.GetInt() & 0xF0) == 0x10)
			m_PlayState.m_nMusicTempo += tempDiff;
		else
			m_PlayState.m_nMusicTempo -= tempDiff;

		TEMPO tempoMin = specs.GetTempoMin(), tempoMax = specs.GetTempoMax();
		if(m_playBehaviour[kTempoClamp])	// clamp tempo correctly in compatible mode
		{
			tempoMax.Set(255);
		}
		Limit(m_PlayState.m_nMusicTempo, tempoMin, tempoMax);
	}
}


ROWINDEX CSoundFile::PatternLoop(ModChannel &chn, uint32 param)
{
	if (param)
	{
		// Loop Repeat
		if(chn.nPatternLoopCount)
		{
			// There's a loop left
			chn.nPatternLoopCount--;
			if(!chn.nPatternLoopCount)
			{
				// IT compatibility 10. Pattern loops (+ same fix for S3M files)
				// When finishing a pattern loop, the next loop without a dedicated SB0 starts on the first row after the previous loop.
				if(m_playBehaviour[kITPatternLoopTargetReset] || (GetType() == MOD_TYPE_S3M))
				{
					chn.nPatternLoop = m_PlayState.m_nRow + 1;
				}

				return ROWINDEX_INVALID;
			}
		} else
		{
			// First time we get into the loop => Set loop count.

			// IT compatibility 10. Pattern loops (+ same fix for XM / MOD / S3M files)
			if(!m_playBehaviour[kITFT2PatternLoop] && !(GetType() & (MOD_TYPE_MOD | MOD_TYPE_S3M)))
			{
				ModChannel *p = m_PlayState.Chn;
				for(CHANNELINDEX i = 0; i < GetNumChannels(); i++, p++) if (p != &chn)
				{
					// Loop on other channel
					if(p->nPatternLoopCount) return ROWINDEX_INVALID;
				}
			}
			chn.nPatternLoopCount = static_cast<uint8>(param);
		}
		m_PlayState.m_nNextPatStartRow = chn.nPatternLoop; // Nasty FT2 E60 bug emulation!
		return chn.nPatternLoop;
	} else
	{
		// Loop Start
		chn.nPatternLoop = m_PlayState.m_nRow;
	}
	return ROWINDEX_INVALID;
}


void CSoundFile::GlobalVolSlide(ModCommand::PARAM param, uint8 &nOldGlobalVolSlide)
{
	int32 nGlbSlide = 0;
	if (param) nOldGlobalVolSlide = param; else param = nOldGlobalVolSlide;

	if((GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2)))
	{
		// XM nibble priority
		if((param & 0xF0) != 0)
		{
			param &= 0xF0;
		} else
		{
			param &= 0x0F;
		}
	}

	if (((param & 0x0F) == 0x0F) && (param & 0xF0))
	{
		if(m_SongFlags[SONG_FIRSTTICK]) nGlbSlide = (param >> 4) * 2;
	} else
	if (((param & 0xF0) == 0xF0) && (param & 0x0F))
	{
		if(m_SongFlags[SONG_FIRSTTICK]) nGlbSlide = - (int)((param & 0x0F) * 2);
	} else
	{
		if(!m_SongFlags[SONG_FIRSTTICK])
		{
			if (param & 0xF0)
			{
				// IT compatibility: Ignore slide commands with both nibbles set.
				if(!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_IMF | MOD_TYPE_J2B | MOD_TYPE_MID | MOD_TYPE_AMS | MOD_TYPE_DBM)) || (param & 0x0F) == 0)
					nGlbSlide = (int)((param & 0xF0) >> 4) * 2;
			} else
			{
				nGlbSlide = -(int)((param & 0x0F) * 2);
			}
		}
	}
	if (nGlbSlide)
	{
		if(!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_IMF | MOD_TYPE_J2B | MOD_TYPE_MID | MOD_TYPE_AMS | MOD_TYPE_DBM))) nGlbSlide *= 2;
		nGlbSlide += m_PlayState.m_nGlobalVolume;
		Limit(nGlbSlide, 0, 256);
		m_PlayState.m_nGlobalVolume = nGlbSlide;
	}
}


//////////////////////////////////////////////////////
// Note/Period/Frequency functions

// Find lowest note which has same or lower period as a given period (i.e. the note has the same or higher frequency)
uint32 CSoundFile::GetNoteFromPeriod(uint32 period, int32 nFineTune, uint32 nC5Speed) const
{
	if(!period) return 0;
	if(m_playBehaviour[kFT2Periods])
	{
		// FT2's "RelocateTon" function actually rounds up and down, while GetNoteFromPeriod normally just truncates.
		nFineTune += 64;
	}
	// This essentially implements std::lower_bound, with the difference that we don't need an iterable container.
	uint32 minNote = NOTE_MIN, maxNote = NOTE_MAX, count = maxNote - minNote + 1;
	const bool periodIsFreq = PeriodsAreFrequencies();
	while(count > 0)
	{
		const uint32 step = count / 2, midNote = minNote + step;
		uint32 n = GetPeriodFromNote(midNote, nFineTune, nC5Speed);
		if((n > period && !periodIsFreq) || (n < period && periodIsFreq) || !n)
		{
			minNote = midNote + 1;
			count -= step + 1;
		} else
		{
			count = step;
		}
	}
	return minNote;
}


uint32 CSoundFile::GetPeriodFromNote(uint32 note, int32 nFineTune, uint32 nC5Speed) const
{
	if (note == NOTE_NONE || (note >= NOTE_MIN_SPECIAL)) return 0;
	note -= NOTE_MIN;
	if (!UseFinetuneAndTranspose())
	{
		if(GetType() & (MOD_TYPE_MDL | MOD_TYPE_DTM))
		{
			// MDL uses non-linear slides, but their effectiveness does not depend on the middle-C frequency.
			return (FreqS3MTable[note % 12u] << 4) >> (note / 12);
		}
		if(m_SongFlags[SONG_LINEARSLIDES] || GetType() == MOD_TYPE_669)
		{
			// In IT linear slide mode, directly use frequency in Hertz rather than periods.
			if(m_playBehaviour[kHertzInLinearMode] || GetType() == MOD_TYPE_669)
				return Util::muldiv_unsigned(nC5Speed, LinearSlideUpTable[(note % 12u) * 16u] << (note / 12u), 65536 << 5);
			else
				return (FreqS3MTable[note % 12u] << 5) >> (note / 12);
		} else
		{
			if (!nC5Speed)
				nC5Speed = 8363;
			LimitMax(nC5Speed, uint32_max >> (note / 12u));
			//(a*b)/c
			return Util::muldiv_unsigned(8363, (FreqS3MTable[note % 12u] << 5), nC5Speed << (note / 12u));
			//8363 * freq[note%12] / nC5Speed * 2^(5-note/12)
		}
	} else if (GetType() & (MOD_TYPE_XM | MOD_TYPE_MTM))
	{
		if (note < 12) note = 12;
		note -= 12;

		if(GetType() == MOD_TYPE_MTM)
		{
			nFineTune *= 16;
		} else if(m_playBehaviour[kFT2FinetunePrecision])
		{
			// FT2 Compatibility: The lower three bits of the finetune are truncated.
			// Test case: Finetune-Precision.xm
			nFineTune &= ~7;
		}

		if(m_SongFlags[SONG_LINEARSLIDES])
		{
			int l = ((NOTE_MAX - note) << 6) - (nFineTune / 2);
			if (l < 1) l = 1;
			return static_cast<uint32>(l);
		} else
		{
			int finetune = nFineTune;
			uint32 rnote = (note % 12) << 3;
			uint32 roct = note / 12;
			int rfine = finetune / 16;
			int i = rnote + rfine + 8;
			Limit(i , 0, 103);
			uint32 per1 = XMPeriodTable[i];
			if(finetune < 0)
			{
				rfine--;
				finetune = -finetune;
			} else rfine++;
			i = rnote+rfine+8;
			if (i < 0) i = 0;
			if (i >= 104) i = 103;
			uint32 per2 = XMPeriodTable[i];
			rfine = finetune & 0x0F;
			per1 *= 16-rfine;
			per2 *= rfine;
			return ((per1 + per2) << 1) >> roct;
		}
	} else
	{
		nFineTune = XM2MODFineTune(nFineTune);
		if ((nFineTune) || (note < 24) || (note >= 24 + std::size(ProTrackerPeriodTable)))
			return (ProTrackerTunedPeriods[nFineTune * 12u + note % 12u] << 5) >> (note / 12u);
		else
			return (ProTrackerPeriodTable[note - 24] << 2);
	}
}


// Converts period value to sample frequency. Return value is fixed point, with FREQ_FRACBITS fractional bits.
uint32 CSoundFile::GetFreqFromPeriod(uint32 period, uint32 c5speed, int32 nPeriodFrac) const
{
	if (!period) return 0;
	if (GetType() & (MOD_TYPE_XM | MOD_TYPE_MTM))
	{
		if(m_playBehaviour[kFT2Periods])
		{
			// FT2 compatibility: Period is a 16-bit value in FT2, and it overflows happily.
			// Test case: FreqWraparound.xm
			period &= 0xFFFF;
		}
		if(m_SongFlags[SONG_LINEARSLIDES])
		{
			uint32 octave;
			if(m_playBehaviour[kFT2Periods])
			{
				// Under normal circumstances, this calculation returns the same values as the non-compatible one.
				// However, once the 12 octaves are exceeded (through portamento slides), the octave shift goes
				// crazy in FT2, meaning that the frequency wraps around randomly...
				// The entries in FT2's conversion table are four times as big, hence we have to do an additional shift by two bits.
				// Test case: FreqWraparound.xm
				// 12 octaves * (12 * 64) LUT entries = 9216, add 767 for rounding
				uint32 div = ((9216u + 767u - period) / 768);
				octave = ((14 - div) & 0x1F);
			} else
			{
				octave = (period / 768) + 2;
			}
			return (XMLinearTable[period % 768] << (FREQ_FRACBITS + 2)) >> octave;
		} else
		{
			if(!period) period = 1;
			return ((8363 * 1712L) << FREQ_FRACBITS) / period;
		}
	} else if (UseFinetuneAndTranspose())
	{
		return ((3546895L * 4) << FREQ_FRACBITS) / period;
	} else if(GetType() == MOD_TYPE_669)
	{
		// We only really use c5speed for the finetune pattern command. All samples in 669 files have the same middle-C speed (imported as 8363 Hz).
		return (period + c5speed - 8363) << FREQ_FRACBITS;
	} else if(GetType() & (MOD_TYPE_MDL | MOD_TYPE_DTM))
	{
		LimitMax(period, Util::MaxValueOfType(period) >> 8);
		if (!c5speed) c5speed = 8363;
		return Util::muldiv_unsigned(c5speed, (1712L << 7) << FREQ_FRACBITS, (period << 8) + nPeriodFrac);
	} else
	{
		LimitMax(period, Util::MaxValueOfType(period) >> 8);
		if(m_SongFlags[SONG_LINEARSLIDES])
		{
			if(m_playBehaviour[kHertzInLinearMode])
			{
				// IT linear slides already use frequencies instead of periods.
				static_assert(FREQ_FRACBITS <= 8, "Check this shift operator");
				return uint32(((uint64(period) << 8) + nPeriodFrac) >> (8 - FREQ_FRACBITS));
			} else
			{
				if (!c5speed) c5speed = 8363;
				return Util::muldiv_unsigned(c5speed, (1712L << 8) << FREQ_FRACBITS, (period << 8) + nPeriodFrac);
			}
		} else
		{
			return Util::muldiv_unsigned(8363, (1712L << 8) << FREQ_FRACBITS, (period << 8) + nPeriodFrac);
		}
	}
}


PLUGINDEX CSoundFile::GetBestPlugin(CHANNELINDEX nChn, PluginPriority priority, PluginMutePriority respectMutes) const
{
	if (nChn >= MAX_CHANNELS)		//Check valid channel number
	{
		return 0;
	}

	//Define search source order
	PLUGINDEX plugin = 0;
	switch (priority)
	{
		case ChannelOnly:
			plugin = GetChannelPlugin(nChn, respectMutes);
			break;
		case InstrumentOnly:
			plugin  = GetActiveInstrumentPlugin(nChn, respectMutes);
			break;
		case PrioritiseInstrument:
			plugin  = GetActiveInstrumentPlugin(nChn, respectMutes);
			if(!plugin || plugin > MAX_MIXPLUGINS)
			{
				plugin = GetChannelPlugin(nChn, respectMutes);
			}
			break;
		case PrioritiseChannel:
			plugin  = GetChannelPlugin(nChn, respectMutes);
			if(!plugin || plugin > MAX_MIXPLUGINS)
			{
				plugin = GetActiveInstrumentPlugin(nChn, respectMutes);
			}
			break;
	}

	return plugin; // 0 Means no plugin found.
}


PLUGINDEX CSoundFile::GetChannelPlugin(CHANNELINDEX nChn, PluginMutePriority respectMutes) const
{
	const ModChannel &channel = m_PlayState.Chn[nChn];

	PLUGINDEX plugin;
	if((respectMutes == RespectMutes && channel.dwFlags[CHN_MUTE]) || channel.dwFlags[CHN_NOFX])
	{
		plugin = 0;
	} else
	{
		// If it looks like this is an NNA channel, we need to find the master channel.
		// This ensures we pick up the right ChnSettings.
		// NB: nMasterChn == 0 means no master channel, so we need to -1 to get correct index.
		if (nChn >= m_nChannels && channel.nMasterChn > 0)
		{
			nChn = channel.nMasterChn - 1;
		}

		if(nChn < MAX_BASECHANNELS)
		{
			plugin = ChnSettings[nChn].nMixPlugin;
		} else
		{
			plugin = 0;
		}
	}
	return plugin;
}


PLUGINDEX CSoundFile::GetActiveInstrumentPlugin(CHANNELINDEX nChn, PluginMutePriority respectMutes) const
{
	// Unlike channel settings, pModInstrument is copied from the original chan to the NNA chan,
	// so we don't need to worry about finding the master chan.

	PLUGINDEX plug = 0;
	if(m_PlayState.Chn[nChn].pModInstrument != nullptr)
	{
		if(respectMutes == RespectMutes && m_PlayState.Chn[nChn].pModSample && m_PlayState.Chn[nChn].pModSample->uFlags[CHN_MUTE])
		{
			plug = 0;
		} else
		{
			plug = m_PlayState.Chn[nChn].pModInstrument->nMixPlug;
		}
	}
	return plug;
}


// Retrieve the plugin that is associated with the channel's current instrument.
// No plugin is returned if the channel is muted or if the instrument doesn't have a MIDI channel set up,
// As this is meant to be used with instrument plugins.
IMixPlugin *CSoundFile::GetChannelInstrumentPlugin(CHANNELINDEX chn) const
{
#ifndef NO_PLUGINS
	if(m_PlayState.Chn[chn].dwFlags[CHN_MUTE | CHN_SYNCMUTE])
	{
		// Don't process portamento on muted channels. Note that this might have a side-effect
		// on other channels which trigger notes on the same MIDI channel of the same plugin,
		// as those won't be pitch-bent anymore.
		return nullptr;
	}

	if(m_PlayState.Chn[chn].HasMIDIOutput())
	{
		const ModInstrument *pIns = m_PlayState.Chn[chn].pModInstrument;
		// Instrument sends to a MIDI channel
		if(pIns->nMixPlug != 0 && pIns->nMixPlug <= MAX_MIXPLUGINS)
		{
			return m_MixPlugins[pIns->nMixPlug - 1].pMixPlugin;
		}
	}
#else
	MPT_UNREFERENCED_PARAMETER(chn);
#endif // NO_PLUGINS
	return nullptr;
}


// Get the MIDI channel currently associated with a given tracker channel
uint8 CSoundFile::GetBestMidiChannel(CHANNELINDEX trackerChn) const
{
	if(trackerChn >= std::size(m_PlayState.Chn))
	{
		return 0;
	}

	const ModChannel &chn = m_PlayState.Chn[trackerChn];
	const ModInstrument *ins = chn.pModInstrument;
	if(ins != nullptr)
	{
		if(ins->nMidiChannel == MidiMappedChannel)
		{
			// For mapped channels, return their pattern channel, modulo 16 (because there are only 16 MIDI channels)
			return static_cast<uint8>((chn.nMasterChn ? (chn.nMasterChn - 1u) : trackerChn) % 16u);
		} else if(ins->HasValidMIDIChannel())
		{
			return (ins->nMidiChannel - 1u) % 16u;
		}
	}
	return 0;
}


#ifdef MODPLUG_TRACKER
void CSoundFile::HandlePatternTransitionEvents()
{
	// MPT sequence override
	if(m_PlayState.m_nSeqOverride != ORDERINDEX_INVALID && m_PlayState.m_nSeqOverride < Order().size())
	{
		if(m_SongFlags[SONG_PATTERNLOOP])
		{
			m_PlayState.m_nPattern = Order()[m_PlayState.m_nSeqOverride];
		}
		m_PlayState.m_nCurrentOrder = m_PlayState.m_nSeqOverride;
		m_PlayState.m_nSeqOverride = ORDERINDEX_INVALID;
	}

	// Channel mutes
	for (CHANNELINDEX chan = 0; chan < GetNumChannels(); chan++)
	{
		if (m_bChannelMuteTogglePending[chan])
		{
			if(GetpModDoc())
			{
				GetpModDoc()->MuteChannel(chan, !GetpModDoc()->IsChannelMuted(chan));
			}
			m_bChannelMuteTogglePending[chan] = false;
		}
	}
}
#endif // MODPLUG_TRACKER


// Update time signatures (global or pattern-specific). Don't forget to call this when changing the RPB/RPM settings anywhere!
void CSoundFile::UpdateTimeSignature()
{
	if(!Patterns.IsValidIndex(m_PlayState.m_nPattern) || !Patterns[m_PlayState.m_nPattern].GetOverrideSignature())
	{
		m_PlayState.m_nCurrentRowsPerBeat = m_nDefaultRowsPerBeat;
		m_PlayState.m_nCurrentRowsPerMeasure = m_nDefaultRowsPerMeasure;
	} else
	{
		m_PlayState.m_nCurrentRowsPerBeat = Patterns[m_PlayState.m_nPattern].GetRowsPerBeat();
		m_PlayState.m_nCurrentRowsPerMeasure = Patterns[m_PlayState.m_nPattern].GetRowsPerMeasure();
	}
}


void CSoundFile::PortamentoMPT(ModChannel &chn, int param)
{
	//Behavior: Modifies portamento by param-steps on every tick.
	//Note that step meaning depends on tuning.

	chn.m_PortamentoFineSteps += param;
	chn.m_CalculateFreq = true;
}


void CSoundFile::PortamentoFineMPT(ModChannel &chn, int param)
{
	//Behavior: Divides portamento change between ticks/row. For example
	//if Ticks/row == 6, and param == +-6, portamento goes up/down by one tuning-dependent
	//fine step every tick.

	if(m_PlayState.m_nTickCount == 0)
		chn.nOldFinePortaUpDown = 0;

	const int tickParam = static_cast<int>((m_PlayState.m_nTickCount + 1.0) * param / m_PlayState.m_nMusicSpeed);
	chn.m_PortamentoFineSteps += (param >= 0) ? tickParam - chn.nOldFinePortaUpDown : tickParam + chn.nOldFinePortaUpDown;
	if(m_PlayState.m_nTickCount + 1 == m_PlayState.m_nMusicSpeed)
		chn.nOldFinePortaUpDown = static_cast<int8>(std::abs(param));
	else
		chn.nOldFinePortaUpDown = static_cast<int8>(std::abs(tickParam));

	chn.m_CalculateFreq = true;
}


void CSoundFile::PortamentoExtraFineMPT(ModChannel &chn, int param)
{
	// This kinda behaves like regular fine portamento.
	// It changes the pitch by n finetune steps on the first tick.

	if(chn.isFirstTick)
	{
		chn.m_PortamentoFineSteps += param;
		chn.m_CalculateFreq = true;
	}
}


OPENMPT_NAMESPACE_END
