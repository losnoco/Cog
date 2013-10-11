/*
 * SSEQ Player - Track structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-04-01
 *
 * Adapted from source code of FeOS Sound System
 * By fincs
 * https://github.com/fincs/FSS
 */

#ifndef SSEQPLAYER_TRACK_H
#define SSEQPLAYER_TRACK_H

#include <bitset>
#include "consts.h"

struct Player;

struct Track
{
	int8_t trackId;

	std::bitset<TS_BITS> state;
	uint8_t num, prio;
	Player *ply;

	const uint8_t *startPos;
	const uint8_t *pos;
	const uint8_t *stack[FSS_TRACKSTACKSIZE];
	uint8_t stackPos;
	uint8_t loopCount[FSS_TRACKSTACKSIZE];

	int wait;
	uint16_t patch;
	uint8_t portaKey, portaTime;
	int16_t sweepPitch;
	uint8_t vol, expr;
	int8_t pan; // -64..63
	uint8_t pitchBendRange;
	int8_t pitchBend;
	int8_t transpose;

	uint8_t a, d, s, r;

	uint8_t modType, modSpeed, modDepth, modRange;
	uint16_t modDelay;

	std::bitset<TUF_BITS> updateFlags;

	Track();

	void Init(uint8_t handle, Player *ply, const uint8_t *pos, int n);
	void Zero();
	void ClearState();
	void Free();
	int NoteOn(int key, int vel, int len);
	int NoteOnTie(int key, int vel);
	void ReleaseAllNotes();
	void Run();
};

#endif
