/*
 * SSEQ Player - Constants/Macros
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-09-08
 *
 * Adapted from source code of FeOS Sound System
 * By fincs
 * https://github.com/fincs/FSS
 *
 * Some constants/macros also taken from libdns, part of the devkitARM portion of devkitPro
 * http://devkitpro.org/
 */

#pragma once

#include <cstdint>

const uint32_t ARM7_CLOCK = 33513982;
const double SecondsPerClockCycle = 64.0 * 2728.0 / ARM7_CLOCK;

inline uint32_t BIT(uint32_t n) { return 1 << n; }

enum { TS_ALLOCBIT, TS_NOTEWAIT, TS_PORTABIT, TS_TIEBIT, TS_END, TS_BITS };

enum { TUF_VOL, TUF_PAN, TUF_TIMER, TUF_MOD, TUF_LEN, TUF_BITS };

enum { CS_NONE, CS_START, CS_ATTACK, CS_DECAY, CS_SUSTAIN, CS_RELEASE };

enum { CF_UPDVOL, CF_UPDPAN, CF_UPDTMR, CF_BITS };

enum { TYPE_PCM, TYPE_PSG, TYPE_NOISE };

const int FSS_TRACKCOUNT = 16;
const int FSS_MAXTRACKS = 32;
const int FSS_TRACKSTACKSIZE = 3;
const int AMPL_K = 723;
const int AMPL_MIN = -AMPL_K;
const int AMPL_THRESHOLD = AMPL_MIN << 7;

inline int SOUND_FREQ(int n) { return -0x1000000 / n; }

inline uint32_t SOUND_VOL(int n) { return n; }
inline uint32_t SOUND_VOLDIV(int n) { return n << 8; }
inline uint32_t SOUND_PAN(int n) { return n << 16; }
inline uint32_t SOUND_DUTY(int n) { return n << 24; }
const uint32_t SOUND_REPEAT = BIT(27);
const uint32_t SOUND_ONE_SHOT = BIT(28);
inline uint32_t SOUND_LOOP(bool a) { return a ? SOUND_REPEAT : SOUND_ONE_SHOT; }
const uint32_t SOUND_FORMAT_PSG = 3 << 29;
inline uint32_t SOUND_FORMAT(int n) { return n << 29; }
const uint32_t SCHANNEL_ENABLE = BIT(31);

enum Interpolation
{
	INTERPOLATION_NONE,
	INTERPOLATION_LINEAR,
	INTERPOLATION_4POINTLEGRANGE,
	INTERPOLATION_6POINTLEGRANGE,
	INTERPOLATION_SINC
};
