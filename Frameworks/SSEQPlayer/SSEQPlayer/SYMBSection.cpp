/*
 * SSEQ Player - SDAT SYMB (Symbol/Filename) Section structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#include <vector>
#include "SYMBSection.h"

SYMBRecord::SYMBRecord() : entries()
{
}

void SYMBRecord::Read(PseudoFile &file, uint32_t startOffset)
{
	uint32_t count = file.ReadLE<uint32_t>();
	auto entryOffsets = std::vector<uint32_t>(count);
	file.ReadLE(entryOffsets);
	for (uint32_t i = 0; i < count; ++i)
		if (entryOffsets[i])
		{
			file.pos = startOffset + entryOffsets[i];
			this->entries[i] = file.ReadNullTerminatedString();
		}
}

SYMBSection::SYMBSection() : SEQrecord(), BANKrecord(), WAVEARCrecord()
{
}

void SYMBSection::Read(PseudoFile &file)
{
	uint32_t startOfSYMB = file.pos;
	int8_t type[4];
	file.ReadLE(type);
	if (!VerifyHeader(type, "SYMB"))
		throw std::runtime_error("SDAT SYMB Section invalid");
	file.ReadLE<uint32_t>(); // size
	uint32_t recordOffsets[8];
	file.ReadLE(recordOffsets);
	if (recordOffsets[REC_SEQ])
	{
		file.pos = startOfSYMB + recordOffsets[REC_SEQ];
		this->SEQrecord.Read(file, startOfSYMB);
	}
	if (recordOffsets[REC_BANK])
	{
		file.pos = startOfSYMB + recordOffsets[REC_BANK];
		this->BANKrecord.Read(file, startOfSYMB);
	}
	if (recordOffsets[REC_WAVEARC])
	{
		file.pos = startOfSYMB + recordOffsets[REC_WAVEARC];
		this->WAVEARCrecord.Read(file, startOfSYMB);
	}
}
