/*
 * SSEQ Player - SDAT INFO Section structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-09-08
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#pragma once

#include <map>
#include "INFOEntry.h"
#include "common.h"

template<typename T> struct INFORecord
{
	std::map<uint32_t, T> entries;

	INFORecord();

	void Read(PseudoFile &file, uint32_t startOffset);
};

struct INFOSection
{
	INFORecord<INFOEntrySEQ> SEQrecord;
	INFORecord<INFOEntryBANK> BANKrecord;
	INFORecord<INFOEntryWAVEARC> WAVEARCrecord;

	INFOSection();

	void Read(PseudoFile &file);
};
