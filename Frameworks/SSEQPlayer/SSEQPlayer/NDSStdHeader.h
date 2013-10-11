/*
 * SSEQ Player - Nintendo DS Standard Header structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-21
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#ifndef SSEQPLAYER_NDSSTDHEADER_H
#define SSEQPLAYER_NDSSTDHEADER_H

#include "common.h"

struct NDSStdHeader
{
	int8_t type[4];
	uint32_t magic;

	NDSStdHeader();

	void Read(PseudoFile &file);
	void Verify(const std::string &typeToCheck, uint32_t magicToCheck);
};

#endif
