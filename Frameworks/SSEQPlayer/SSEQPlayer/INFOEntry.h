/*
 * SSEQ Player - SDAT INFO Entry structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-21
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#ifndef SSEQPLAYER_INFOENTRY_H
#define SSEQPLAYER_INFOENTRY_H

#include "common.h"

struct INFOEntry
{
	virtual ~INFOEntry()
	{
	}

	virtual void Read(PseudoFile &file) = 0;
};

struct INFOEntrySEQ : INFOEntry
{
	uint16_t fileID;
	uint16_t bank;
	uint8_t vol;

	INFOEntrySEQ();

	void Read(PseudoFile &file);
};

struct INFOEntryBANK : INFOEntry
{
	uint16_t fileID;
	uint16_t waveArc[4];

	INFOEntryBANK();

	void Read(PseudoFile &file);
};

struct INFOEntryWAVEARC : INFOEntry
{
	uint16_t fileID;

	INFOEntryWAVEARC();

	void Read(PseudoFile &file);
};

#endif
