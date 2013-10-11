/*
 * SSEQ Player - Nintendo DS Standard Header structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-21
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#include <stdexcept>
#include "NDSStdHeader.h"

NDSStdHeader::NDSStdHeader() : magic(0)
{
	memset(this->type, 0, sizeof(this->type));
}

void NDSStdHeader::Read(PseudoFile &file)
{
	file.ReadLE(this->type);
	this->magic = file.ReadLE<uint32_t>();
	file.ReadLE<uint32_t>(); // file size
	file.ReadLE<uint16_t>(); // structure size
	file.ReadLE<uint16_t>(); // # of blocks
}

void NDSStdHeader::Verify(const std::string &typeToCheck, uint32_t magicToCheck)
{
	if (!VerifyHeader(this->type, typeToCheck) || this->magic != magicToCheck)
		throw std::runtime_error("NDS Standard Header for " + typeToCheck + " invalid");
}
