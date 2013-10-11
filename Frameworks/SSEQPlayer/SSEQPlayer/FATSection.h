/*
 * SSEQ Player - SDAT FAT (File Allocation Table) Section structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-21
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#ifndef SSEQPLAYER_FATSECTION_H
#define SSEQPLAYER_FATSECTION_H

#include "common.h"

struct FATRecord
{
	uint32_t offset;

	FATRecord();

	void Read(PseudoFile &file);
};

struct FATSection
{
	std::vector<FATRecord> records;

	FATSection();

	void Read(PseudoFile &file);
};

#endif
