/*
 * SSEQ Player - SDAT INFO Entry structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-21
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#include "INFOEntry.h"

INFOEntrySEQ::INFOEntrySEQ() : fileID(0), bank(0), vol(0)
{
}

void INFOEntrySEQ::Read(PseudoFile &file)
{
	this->fileID = file.ReadLE<uint16_t>();
	file.ReadLE<uint16_t>(); // unknown
	this->bank = file.ReadLE<uint16_t>();
	this->vol = file.ReadLE<uint8_t>();
	if (!this->vol)
		this->vol = 0x7F; // Prevents nothing for volume
	file.ReadLE<uint8_t>(); // cpr
	file.ReadLE<uint8_t>(); // ppr
	file.ReadLE<uint8_t>(); // ply
}

INFOEntryBANK::INFOEntryBANK() : fileID(0)
{
	memset(this->waveArc, 0, sizeof(this->waveArc));
}

void INFOEntryBANK::Read(PseudoFile &file)
{
	this->fileID = file.ReadLE<uint16_t>();
	file.ReadLE<uint16_t>(); // unknown
	file.ReadLE(this->waveArc);
}

INFOEntryWAVEARC::INFOEntryWAVEARC() : fileID(0)
{
}

void INFOEntryWAVEARC::Read(PseudoFile &file)
{
	this->fileID = file.ReadLE<uint16_t>();
}
