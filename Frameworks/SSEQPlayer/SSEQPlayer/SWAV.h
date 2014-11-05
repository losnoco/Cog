/*
 * SSEQ Player - SDAT SWAV (Waveform/Sample) structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-09-08
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#pragma once

#include "common.h"

struct SWAV
{
	uint8_t waveType;
	uint8_t loop;
	uint16_t sampleRate;
	uint16_t time;
	uint32_t loopOffset;
	uint32_t nonLoopLength;
	std::vector<int16_t> data;
	const int16_t *dataptr;

	SWAV();

	void Read(PseudoFile &file);
	void DecodeADPCM(const uint8_t *origData, uint32_t len);
};
