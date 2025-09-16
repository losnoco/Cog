/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (c) Reinier van Vliet - https://bitbucket.org/rhinoid/ */
/* Copyright (c) Christopher Snowhill */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jaytrax.h"
#include "jxs.h"
#include "ioutil.h"

//---------------------ENDIANNESS!

// Sure, let's support 16 and 32 bit values having different endianness!

static int isStaticInit = 0;
static int isBigEndian16 = 0;
static int isBigEndian32 = 0;

static void initEndian(void) {
	if(!isStaticInit) {
		union {
			uint32_t dword;
			uint8_t bytes[4];
		} tester32;
		union {
			uint16_t word;
			uint8_t bytes[2];
		} tester16;
		tester16.word = 0x00FF;
		tester32.dword = 0x000000FF;
		isBigEndian16 = tester16.bytes[1] == 0xFF;
		isBigEndian32 = tester32.bytes[3] == 0xFF;
		isStaticInit = 1;
	}
}

static uint16_t readuint16(uint16_t in) {
	if(isBigEndian16) {
#if defined(__GNUC__) || defined(__clang__)
		return __builtin_bswap16(in);
#else
		uint8_t *p = (uint8_t *)&in;
		return p[1] + (uint16_t)(p[0]) * 256;
#endif
	} else {
		return in;
	}
}

static int16_t readint16(int16_t in) {
	return (int16_t)readuint16((uint16_t)in);
}

static void swapint16(int16_t *data, size_t count) {
	if(isBigEndian16) {
		for(size_t i = 0; i < count; ++i) {
#if defined(__GNUC__) || defined(__clang__)
			data[i] = __builtin_bswap16(data[i]);
#else
			data[i] = readint16(data[i]);
#endif
		}
	}
}

static uint32_t readuint32(uint32_t in) {
	if(isBigEndian32) {
#if defined(__GNUC__) || defined(__clang__)
		return __builtin_bswap32(in);
#else
		uint8_t *p = (uint8_t *)&in;
		return p[3] + (uint32_t)(p[2]) * 256 + (uint32_t)(p[1]) * 256 * 256 + (uint32_t)(p[0]) * 256 * 256 * 256;
#endif
	} else {
		return in;
	}
}

static int32_t readint32(int32_t in) {
	return (int32_t)readuint32((uint32_t)in);
}

//---------------------readers

struct memdata {
	const uint8_t* data;
	size_t remain;
	int error;
};

static void memread(void* buf, size_t size, size_t count, void* _data) {
	struct memdata* data = (struct memdata*)_data;
	size_t unread = 0;
	size *= count;
	if (size > data->remain) {
		unread = size - data->remain;
		size = data->remain;
		data->error = 1;
	}
	memcpy(buf, data->data, size);
	data->data += size;
	data->remain -= size;
	if (unread)
		memset(((uint8_t*)buf) + size, 0, unread);
}

static int memerror(void* _data) {
	struct memdata* data = (struct memdata*)_data;
	return data->error;
}

static void fileread(void* buf, size_t size, size_t count, void* _file) {
	FILE* file = (FILE*)_file;
	fread(buf, size, count, file);
}

static int fileerror(void* _file) {
	FILE* file = (FILE*)_file;
	return ferror(file);
}

typedef void (*thereader)(void*, size_t, size_t, void*);
typedef int (*theerror)(void*);

//---------------------JXS3457

static int struct_readHeader(JT1Song* dest, thereader reader, theerror iferror, void* fin) {
	f_JT1Header t;

	reader(&t, sizeof(f_JT1Header), 1, fin);
	dest->mugiversion = readint16(t.mugiversion);
	dest->nrofpats    = readint32(t.nrofpats);
	dest->nrofsongs   = readint32(t.nrofsongs);
	dest->nrofinst    = readint32(t.nrofinst);
	return iferror(fin);
}

static int struct_readSubsong(JT1Subsong* dest, size_t len, thereader reader, theerror iferror, void* fin) {
	uint32_t i, j, k;
	f_JT1Subsong t;

	for (i=0; i < len; i++) {
		reader(&t, sizeof(f_JT1Subsong), 1, fin);
		for (j = 0; j < J3457_CHANS_SUBSONG; j++) dest[i].mute[j] = t.mute[j];
		dest[i].songspd         = readint32(t.songspd);
		dest[i].groove          = readint32(t.groove);
		dest[i].songpos         = readint32(t.songpos);
		dest[i].songstep        = readint32(t.songstep);
		dest[i].endpos          = readint32(t.endpos);
		dest[i].endstep         = readint32(t.endstep);
		dest[i].looppos         = readint32(t.looppos);
		dest[i].loopstep        = readint32(t.loopstep);
		dest[i].songloop        = readint16(t.songloop);
		memcpy(dest[i].name, &t.name, 32);
		dest[i].name[32] = '\0';
		dest[i].nrofchans       = readint16(t.nrofchans);
		dest[i].delaytime       = readuint16(t.delaytime);
		for (j=0; j < J3457_CHANS_SUBSONG; j++) {
			dest[i].delayamount[j] = t.delayamount[j];
		}
		dest[i].amplification   = readint16(t.amplification);
		for (j=0; j < J3457_CHANS_SUBSONG; j++) {
			for (k=0; k < J3457_ORDERS_SUBSONG; k++) {
				dest[i].orders[j][k].patnr  = readint16(t.orders[j][k].patnr);
				dest[i].orders[j][k].patlen = readint16(t.orders[j][k].patlen);
			}
		}
	}
	return iferror(fin);
}

static int struct_readPat(JT1Row* dest, size_t len, thereader reader, theerror iferror, void* fin) {
	uint32_t i, j;
	f_JT1Row t[J3457_ROWS_PAT];

	for (i = 0; i < len; i++) {
		reader(&t, sizeof(f_JT1Row)*J3457_ROWS_PAT, 1, fin);
		for (j = 0; j < J3457_ROWS_PAT; j++) {
			uint32_t off = i * J3457_ROWS_PAT + j;
			dest[off].srcnote = t[j].srcnote;
			dest[off].dstnote = t[j].dstnote;
			dest[off].inst    = t[j].inst;
			dest[off].param   = t[j].param;
			dest[off].script  = t[j].script;
		}
	}
	return iferror(fin);
}

static int struct_readInst(JT1Inst* dest, size_t len, thereader reader, theerror iferror, void* fin) {
	uint32_t i, j;
	f_JT1Inst t;
	for (i = 0; i < len; i++) {
		reader(&t, sizeof(f_JT1Inst), 1, fin);
		dest[i].mugiversion     = readint16(t.mugiversion);
		memcpy(dest[i].instname, &t.instname, 32);
		dest[i].instname[32] = '\0';
		dest[i].waveform        = readint16(t.waveform);
		dest[i].wavelength      = readint16(t.wavelength);
		dest[i].mastervol       = readint16(t.mastervol);
		dest[i].amwave          = readint16(t.amwave);
		dest[i].amspd           = readint16(t.amspd);
		dest[i].amlooppoint     = readint16(t.amlooppoint);
		dest[i].finetune        = readint16(t.finetune);
		dest[i].fmwave          = readint16(t.fmwave);
		dest[i].fmspd           = readint16(t.fmspd);
		dest[i].fmlooppoint     = readint16(t.fmlooppoint);
		dest[i].fmdelay         = readint16(t.fmdelay);
		dest[i].arpeggio        = readint16(t.arpeggio);
		for (j=0; j < J3457_WAVES_INST; j++) {
			dest[i].resetwave[j] = t.resetwave[j];
		}
		dest[i].panwave         = readint16(t.panwave);
		dest[i].panspd          = readint16(t.panspd);
		dest[i].panlooppoint    = readint16(t.panlooppoint);
		for (j=0; j < J3457_EFF_INST; j++) {
			dest[i].fx[j].dsteffect     = readint32(t.fx[j].dsteffect);
			dest[i].fx[j].srceffect1    = readint32(t.fx[j].srceffect1);
			dest[i].fx[j].srceffect2    = readint32(t.fx[j].srceffect2);
			dest[i].fx[j].osceffect     = readint32(t.fx[j].osceffect);
			dest[i].fx[j].effectvar1    = readint32(t.fx[j].effectvar1);
			dest[i].fx[j].effectvar2    = readint32(t.fx[j].effectvar2);
			dest[i].fx[j].effectspd     = readint32(t.fx[j].effectspd);
			dest[i].fx[j].oscspd        = readint32(t.fx[j].oscspd);
			dest[i].fx[j].effecttype    = readint32(t.fx[j].effecttype);
			dest[i].fx[j].oscflg        = t.fx[j].oscflg;
			dest[i].fx[j].reseteffect   = t.fx[j].reseteffect;
		}
		memcpy(dest[i].samplename, &t.samplename, 192);
		dest[i].samplename[192] = '\0';
		//exFnameFromPath(&dest[i].samplename, &t.samplename, SE_NAMELEN);
		dest[i].sharing       = readint16(t.sharing);
		dest[i].loopflg       = readint16(t.loopflg);
		dest[i].bidirecflg    = readint16(t.bidirecflg);
		dest[i].startpoint    = readint32(t.startpoint);
		dest[i].looppoint     = readint32(t.looppoint);
		dest[i].endpoint      = readint32(t.endpoint);
		dest[i].hasSampData   = t.hasSampData ? 1 : 0; //this was a sampdata pointer in original jaytrax
		dest[i].samplelength  = readint32(t.samplelength);
		//memcpy(&dest[i].waves, &t.waves, J3457_WAVES_INST * J3457_SAMPS_WAVE * sizeof(int16_t));
		reader(dest->waves, 2, J3457_WAVES_INST * J3457_SAMPS_WAVE, fin);
		swapint16(dest->waves, J3457_WAVES_INST * J3457_SAMPS_WAVE);
	}
	return iferror(fin);
}

//---------------------JXS3458

/* Soon! */

//---------------------

static int jxsfile_readSongCb(thereader reader, theerror iferror, void* fin, JT1Song** sngOut) {
#define FAIL(x) {error=(x); goto _ERR;}
	JT1Song* song;
	int i;
	int error;

	initEndian();

	//song
	if((song = (JT1Song*)calloc(1, sizeof(JT1Song)))) {
		int version;

		if (struct_readHeader(song, reader, iferror, fin)) FAIL(ERR_BADSONG);
		//version magic
		version = song->mugiversion;
		if (version >= 3456 && version <= 3457) {
			int nrSubsongs = song->nrofsongs;
			int nrPats     = song->nrofpats;
			int nrRows     = J3457_ROWS_PAT * nrPats;
			int nrInst     = song->nrofinst;

			//subsongs
			if ((song->subsongs = (JT1Subsong**)calloc(nrSubsongs, sizeof(JT1Subsong*)))) {
				for (i = 0; i < nrSubsongs; i++) {
					if ((song->subsongs[i] = (JT1Subsong*)calloc(1, sizeof(JT1Subsong)))) {
						if (struct_readSubsong(song->subsongs[i], 1, reader, iferror, fin)) FAIL(ERR_BADSONG);
					} else FAIL(ERR_MALLOC);
				}
			} else FAIL(ERR_MALLOC);

			//patterns
			if ((song->patterns = (JT1Row*)calloc(nrRows, sizeof(JT1Row)))) {
				if (struct_readPat(song->patterns, nrPats, reader, iferror, fin)) FAIL(ERR_BADSONG);
			} else FAIL(ERR_MALLOC);

			//pattern names. Length includes \0
			if ((song->patNames = (char**)calloc(nrPats, sizeof(char*)))) {
				for (i=0; i < nrPats; i++) {
					int32_t nameLen = 0;

					reader(&nameLen, 4, 1, fin);
                    if ((song->patNames[i] = (char*)calloc(nameLen + 1, sizeof(char)))) {
                        reader(song->patNames[i], nameLen, 1, fin);
						song->patNames[i][nameLen] = '\0'; // Just in case
					} else FAIL(ERR_MALLOC);
				}

				if (iferror(fin)) FAIL(ERR_BADSONG);
			} else FAIL(ERR_MALLOC);

			//instruments
			if ((song->instruments = (JT1Inst**)calloc(nrInst, sizeof(JT1Inst*)))) {
				if (!(song->samples = (uint8_t**)calloc(nrInst, sizeof(uint8_t*)))) FAIL(ERR_MALLOC);
				for (i = 0; i < nrInst; i++) {
					if ((song->instruments[i] = (JT1Inst*)calloc(1, sizeof(JT1Inst)))) {
						JT1Inst* inst = song->instruments[i];
						if (struct_readInst(inst, 1, reader, iferror, fin)) FAIL(ERR_BADSONG);

						//patch old instrument to new
						if (version == 3456) {
							inst->sharing = 0;
							inst->loopflg = 0;
							inst->bidirecflg = 0;
							inst->startpoint = 0;
							inst->looppoint = 0;
							inst->endpoint = 0;
							//silly place to put a pointer
							if (inst->hasSampData) {
								inst->startpoint=0;
								inst->endpoint=(inst->samplelength/2);
								inst->looppoint=0;
							}
						}

						//sample data
						if (inst->hasSampData) {
							//inst->samplelength is in bytes, not samples
							if(!(song->samples[i] = (uint8_t*)calloc(inst->samplelength, sizeof(uint8_t)))) FAIL(ERR_MALLOC);
							reader(song->samples[i], 1, inst->samplelength, fin);
							if (iferror(fin)) FAIL(ERR_BADSONG);
							swapint16((int16_t *)(song->samples[i]), inst->samplelength / 2);
						} else {
							song->samples[i] = NULL;
						}
					} else FAIL(ERR_MALLOC);
				}
			} else FAIL(ERR_MALLOC);

			//arpeggio table
			reader(&song->arpTable, J3457_STEPS_ARP, J3457_ARPS_SONG, fin);

			if (iferror(fin)) FAIL(ERR_BADSONG);
		} else if (version == 3458) {
			//Soon enough!
			FAIL(ERR_BADSONG);
		} else FAIL(ERR_BADSONG);
	} else FAIL(ERR_MALLOC);

	*sngOut = song;
	return ERR_OK;
	#undef FAIL
_ERR:
	jxsfile_freeSong(song);
	*sngOut = NULL;
	return error;
}

int jxsfile_readSong(const char* path, JT1Song** sngOut) {
	char buf[BUFSIZ];
	FILE *fin;
	int error;

	if (!(fin = fopen(path, "rb"))) return ERR_FILEIO;
	setbuf(fin, buf);

	error = jxsfile_readSongCb(fileread, fileerror, fin, sngOut);

	fclose(fin);
	return error;

#if 0 // not actually used above
_ERR:
	if (fin) fclose(fin);
	return error;
#endif
}

int jxsfile_readSongMem(const uint8_t* data, size_t size, JT1Song** sngOut) {
	struct memdata fin;
	fin.data = data;
	fin.remain = size;
	fin.error = 0;

	return jxsfile_readSongCb(memread, memerror, &fin, sngOut);
}

void jxsfile_freeSong(JT1Song* song) {
	if (song) {
		int i;
		int nrSubsongs = song->nrofsongs;
		int nrPats     = song->nrofpats;
		int nrInst     = song->nrofinst;

		if (song->subsongs) {
			for (i = 0; i < nrSubsongs; i++) {
				if (song->subsongs[i])
					free(song->subsongs[i]);
			}

			free(song->subsongs);
		}

		if (song->patterns) {
			free(song->patterns);
		}

		if (song->patNames) {
			for (i=0; i < nrPats; i++) {
				if (song->patNames[i])
					free(song->patNames[i]);
			}

			free(song->patNames);
		}

		if (song->instruments && song->samples) {
			for (i = 0; i < nrInst; i++) {
				if (song->instruments[i])
					free(song->instruments[i]);
				if (song->samples[i])
					free(song->samples[i]);
			}
		}

		if (song->samples) {
			free(song->samples);
		}

		if (song->instruments) {
			free(song->instruments);
		}

		free(song);
	}
}
