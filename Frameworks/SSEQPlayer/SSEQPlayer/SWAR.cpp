/*
 * SSEQ Player - SDAT SWAR (Wave Archive) structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#include <vector>
#include "SWAR.h"
#include "NDSStdHeader.h"

SWAR::SWAR(const std::string &fn) : filename(fn), swavs(), info()
{
}

void SWAR::Read(PseudoFile &file)
{
	uint32_t startOfSWAR = file.pos;
	NDSStdHeader header;
	header.Read(file);
	header.Verify("SWAR", 0x0100FEFF);
	int8_t type[4];
	file.ReadLE(type);
	if (!VerifyHeader(type, "DATA"))
		throw std::runtime_error("SWAR DATA structure invalid");
	file.ReadLE<uint32_t>(); // size
	uint32_t reserved[8];
	file.ReadLE(reserved);
	uint32_t count = file.ReadLE<uint32_t>();
	auto offsets = std::vector<uint32_t>(count);
	file.ReadLE(offsets);
	for (uint32_t i = 0; i < count; ++i)
		if (offsets[i])
		{
			file.pos = startOfSWAR + offsets[i];
			this->swavs[i] = SWAV();
			this->swavs[i].Read(file);
		}
}
