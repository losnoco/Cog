/*
 * SSEQ Player - SDAT INFO Section structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#include <vector>
#include "INFOSection.h"

template<typename T> INFORecord<T>::INFORecord() : entries()
{
}

template<typename T> void INFORecord<T>::Read(PseudoFile &file, uint32_t startOffset)
{
	uint32_t count = file.ReadLE<uint32_t>();
	auto entryOffsets = std::vector<uint32_t>(count);
	file.ReadLE(entryOffsets);
	for (uint32_t i = 0; i < count; ++i)
		if (entryOffsets[i])
		{
			file.pos = startOffset + entryOffsets[i];
			this->entries[i] = T();
			this->entries[i].Read(file);
		}
}

INFOSection::INFOSection() : SEQrecord(), BANKrecord(), WAVEARCrecord()
{
}

void INFOSection::Read(PseudoFile &file)
{
	uint32_t startOfINFO = file.pos;
	int8_t type[4];
	file.ReadLE(type);
	if (!VerifyHeader(type, "INFO"))
		throw std::runtime_error("SDAT INFO Section invalid");
	file.ReadLE<uint32_t>(); // size
	uint32_t recordOffsets[8];
	file.ReadLE(recordOffsets);
	if (recordOffsets[REC_SEQ])
	{
		file.pos = startOfINFO + recordOffsets[REC_SEQ];
		this->SEQrecord.Read(file, startOfINFO);
	}
	if (recordOffsets[REC_BANK])
	{
		file.pos = startOfINFO + recordOffsets[REC_BANK];
		this->BANKrecord.Read(file, startOfINFO);
	}
	if (recordOffsets[REC_WAVEARC])
	{
		file.pos = startOfINFO + recordOffsets[REC_WAVEARC];
		this->WAVEARCrecord.Read(file, startOfINFO);
	}
}
