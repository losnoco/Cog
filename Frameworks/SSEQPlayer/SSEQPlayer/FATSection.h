/*
 * SSEQ Player - SDAT FAT (File Allocation Table) Section structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-09-08
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#pragma once

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
