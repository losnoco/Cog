/*
 * SSEQ Player - SDAT FAT (File Allocation Table) Section structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-21
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#include <stdexcept>
#include "FATSection.h"

FATRecord::FATRecord() : offset(0)
{
}

void FATRecord::Read(PseudoFile &file)
{
	this->offset = file.ReadLE<uint32_t>();
	file.ReadLE<uint32_t>(); // size
	uint32_t reserved[2];
	file.ReadLE(reserved);
}

FATSection::FATSection() : records()
{
}

void FATSection::Read(PseudoFile &file)
{
	int8_t type[4];
	file.ReadLE(type);
	if (!VerifyHeader(type, "FAT "))
		throw std::runtime_error("SDAT FAT Section invalid");
	file.ReadLE<uint32_t>(); // size
	uint32_t count = file.ReadLE<uint32_t>();
	this->records.resize(count);
	for (uint32_t i = 0; i < count; ++i)
		this->records[i].Read(file);
}
