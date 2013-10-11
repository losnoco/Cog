/*
 * SSEQ Player - SDAT SYMB (Symbol/Filename) Section structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-21
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#ifndef SSEQPLAYER_SYMBSECTION_H
#define SSEQPLAYER_SYMBSECTION_H

#include <map>
#include "common.h"

struct SYMBRecord
{
	std::map<uint32_t, std::string> entries;

	SYMBRecord();

	void Read(PseudoFile &file, uint32_t startOffset);
};

/*
 * The size has been left out of this structure as it is unused by this player.
 */
struct SYMBSection
{
	SYMBRecord SEQrecord;
	SYMBRecord BANKrecord;
	SYMBRecord WAVEARCrecord;

	SYMBSection();

	void Read(PseudoFile &file);
};

#endif
