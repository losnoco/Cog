/*
 * SSEQ Player - SDAT SSEQ (Sequence) structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-09-08
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#pragma once

#include "SBNK.h"
#include "INFOEntry.h"
#include "common.h"

struct SSEQ
{
	std::string filename;
	std::vector<uint8_t> data;

	const SBNK *bank;
	INFOEntrySEQ info;

	SSEQ(const std::string &fn = "");
	SSEQ(const SSEQ &sseq);
	SSEQ &operator=(const SSEQ &sseq);

	void Read(PseudoFile &file);
};
