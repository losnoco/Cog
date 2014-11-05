/*
 * SSEQ Player - SDAT SBNK (Sound Bank) structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-09-08
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#pragma once

#include "SWAR.h"
#include "INFOEntry.h"
#include "common.h"

struct SBNKInstrumentRange
{
	uint8_t lowNote;
	uint8_t highNote;
	uint16_t record;
	uint16_t swav;
	uint16_t swar;
	uint8_t noteNumber;
	uint8_t attackRate;
	uint8_t decayRate;
	uint8_t sustainLevel;
	uint8_t releaseRate;
	uint8_t pan;

	SBNKInstrumentRange(uint8_t lowerNote, uint8_t upperNote, int recordType);

	void Read(PseudoFile &file);
};

struct SBNKInstrument
{
	uint8_t record;
	std::vector<SBNKInstrumentRange> ranges;

	SBNKInstrument();

	void Read(PseudoFile &file, uint32_t startOffset);
};

struct SBNK
{
	std::string filename;
	std::vector<SBNKInstrument> instruments;

	const SWAR *waveArc[4];
	INFOEntryBANK info;

	SBNK(const std::string &fn = "");
	SBNK(const SBNK &sbnk);
	SBNK &operator=(const SBNK &sbnk);

	void Read(PseudoFile &file);
};
