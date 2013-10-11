/*
 * SSEQ Player - SDAT SWAR (Wave Archive) structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-21
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#ifndef SSEQPLAYER_SWAR_H
#define SSEQPLAYER_SWAR_H

#include <map>
#include "SWAV.h"
#include "INFOEntry.h"
#include "common.h"

/*
 * The size has been left out of this structure as it is unused by this player.
 */
struct SWAR
{
	std::string filename;
	std::map<uint32_t, SWAV> swavs;

	INFOEntryWAVEARC info;

	SWAR(const std::string &fn = "");

	void Read(PseudoFile &file);
};

#endif
