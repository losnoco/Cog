/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (c) Reinier van Vliet - https://bitbucket.org/rhinoid/ */
/* Copyright (c) Christopher Snowhill */

#ifndef JXS_H
#define JXS_H
#include <stdint.h>
#include <Syntrax_c/jaytrax.h>

#define J3457_CHANS_SUBSONG (16)
#define J3457_ORDERS_SUBSONG (256)
#define J3457_ROWS_PAT (64)
#define J3457_EFF_INST (4)
#define J3457_WAVES_INST (16)
#define J3457_SAMPS_WAVE (256)
#define J3457_ARPS_SONG (16)
#define J3457_STEPS_ARP (16)

// All packed fields are little endian
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

#if defined(__GNUC__) || defined(__clang__)
#define ATTPACK __attribute__((__packed__))
#endif

#ifndef ATTPACK
#define ATTPACK
#endif

typedef struct f_JT1Order f_JT1Order;
struct f_JT1Order {
	int16_t         patnr;      // which pattern to play...
	int16_t         patlen;     // 0/16/32/48
} ATTPACK;

typedef struct f_JT1Row f_JT1Row;
struct f_JT1Row {
	uint8_t         srcnote;
	uint8_t         dstnote;
	uint8_t         inst;
	int8_t          param;
	uint8_t         script;
} ATTPACK;

typedef struct f_JT1Header f_JT1Header;
struct f_JT1Header {
	int16_t         mugiversion;//version of mugician this song was saved with
	int16_t         PAD00;
	int32_t         nrofpats;   //number of patterns available
	int32_t         nrofsongs;  //number of available subsongs
	int32_t         nrofinst;   //number of instruments used
	int32_t         PAD01;
	int16_t         PAD02;
	int16_t         PAD03;
	int16_t         PAD04;
	int16_t         PAD05;
	int16_t         PAD06;
	int16_t         PAD07;
	int16_t         PAD08;
	int16_t         PAD09;
	int16_t         PAD0A;
	int16_t         PAD0B;
	int16_t         PAD0C;
	int16_t         PAD0D;
	int16_t         PAD0E;
	int16_t         PAD0F;
	int16_t         PAD10;
	int16_t         PAD11;
} ATTPACK;

typedef struct f_JT1Subsong f_JT1Subsong;
struct f_JT1Subsong {
	int32_t         PAD00[J3457_CHANS_SUBSONG];
	uint8_t         mute[J3457_CHANS_SUBSONG];   // which channels are muted? (1=muted)
	int32_t         songspd;    // delay between pattern steps
	int32_t         groove;     // groove value... 0 = nothing, 1 = swing, 2 = shuffle
	int32_t         songpos;    // where does the song start? (what length?)
	int32_t         songstep;   // which patternpos offset? (1/64 of length)
	int32_t         endpos;     // where does the song stop? (what length?)
	int32_t         endstep;    // which patternpos offset? (1/64 of length)
	int32_t         looppos;    // where does the song loop? (what length?)
	int32_t         loopstep;   // which patternpos offset? (1/64 of length)
	int16_t         songloop;   // if true, the song loops in between looppos and endpos
	char            name[32];   // name of subsong
	int16_t         nrofchans;  // nr of channels used
	uint16_t        delaytime; // the delay time (for the echo effect)
	uint8_t         delayamount[J3457_CHANS_SUBSONG]; // amount per channel for the echo-effect
	int16_t         amplification; //extra amplification factor (20 to 1000)
	int16_t         PAD01;
	int16_t         PAD02;
	int16_t         PAD03;
	int16_t         PAD04;
	int16_t         PAD05;
	int16_t         PAD06;
	f_JT1Order      orders[J3457_CHANS_SUBSONG][J3457_ORDERS_SUBSONG];
} ATTPACK;

typedef struct f_JT1Effect f_JT1Effect;
struct f_JT1Effect {
	int32_t         dsteffect;
	int32_t         srceffect1;
	int32_t         srceffect2;
	int32_t         osceffect;
	int32_t         effectvar1;
	int32_t         effectvar2;
	int32_t         effectspd;
	int32_t         oscspd;
	int32_t         effecttype;
	int8_t          oscflg;
	int8_t          reseteffect;
	int16_t         PAD00;
} ATTPACK;

// inst is the structure which has the entire instrument definition.
typedef struct f_JT1Inst f_JT1Inst;
struct f_JT1Inst {
	int16_t         mugiversion;
	char            instname[32];
	int16_t         waveform;
	int16_t         wavelength;
	int16_t         mastervol;
	int16_t         amwave;
	int16_t         amspd;
	int16_t         amlooppoint;
	int16_t         finetune;
	int16_t         fmwave;
	int16_t         fmspd;
	int16_t         fmlooppoint;
	int16_t         fmdelay;
	int16_t         arpeggio;
	int8_t          resetwave[J3457_WAVES_INST];
	int16_t         panwave;
	int16_t         panspd;
	int16_t         panlooppoint;
	int16_t         PAD00;
	int16_t         PAD01;
	int16_t         PAD02;
	int16_t         PAD03;
	int16_t         PAD04;
	int16_t         PAD05;
	f_JT1Effect     fx[J3457_EFF_INST];
	char            samplename[192]; // path to the sample used (was _MAX_PATH long... now truncated to 192) (even shorter in the future?)
	int32_t         PAD06;
	int32_t         PAD07;
	int32_t         PAD08;
	int32_t         PAD09;
	int32_t         PAD0A;
	int32_t         PAD0B;
	int32_t         PAD0C;
	int32_t         PAD0D;
	int32_t         PAD0E;
	int32_t         PAD0F;
	int32_t         PAD10;
	int32_t         PAD11;
	int16_t         PAD12;
	int16_t         sharing;    // sample sharing! sharing contains instr nr of shared sanpledata (0=no sharing)
	int16_t         loopflg;    //does the sample loop or play one/shot? (0=1shot)
	int16_t         bidirecflg; // does the sample loop birdirectional? (0=no)
	int32_t         startpoint;
	int32_t         looppoint;
	int32_t         endpoint;
	int32_t         hasSampData;     // pointer to the sample (may be 0)
	int32_t         samplelength;      // length of sample
	//int16_t         waves[J3457_WAVES_INST * J3457_SAMPS_WAVE];
} ATTPACK;

#ifdef _MSC_VER
#pragma pack(pop)
#endif

#undef ATTPACK

//---------------------JXS3458


//---------------------

#ifdef __cplusplus
extern "C" {
#endif

int jxsfile_readSong(const char* path, JT1Song** sngOut);
int jxsfile_readSongMem(const uint8_t* data, size_t size, JT1Song** sngOut);
void jxsfile_freeSong(JT1Song* song);

#ifdef __cplusplus
}
#endif
#endif
