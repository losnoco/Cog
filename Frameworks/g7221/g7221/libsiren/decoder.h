/*
 * Siren Encoder/Decoder library
 *
 *   @author: Youness Alaoui <kakaroto@kakaroto.homelinux.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#ifndef _SIREN_DECODER_H
#define _SIREN_DECODER_H

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "dct4.h"
#include "rmlt.h"
#include "huffman.h"
#include "common.h"


typedef struct stSirenDecoder {
	int sample_rate;
	int flag;
#ifdef __WAV_HEADER__
	PCMWavHeader WavHeader;
#endif
	float context[640];
	float backup_frame[640];
	int dw1;
	int dw2;
	int dw3;
	int dw4;
	int absolute_region_power_index[28];
	float decoder_standard_deviation[28];
	int power_categories[28];
	int category_balance[32];
} * SirenDecoder;


/* MUST be 16000 to be compatible with MSN Voice clips (I think) */
extern SirenDecoder Siren7_NewDecoder(int sample_rate, int flag); /* flag = 1 for Siren7 and 2 for Siren14 */
extern void Siren7_CloseDecoder(SirenDecoder decoder);
extern void Siren7_ResetDecoder(SirenDecoder decoder);
/* Input bytes / output samples */
/* Siren7: 60 / 320 */
/* Siren14: 120 / 640 */
extern int Siren7_DecodeFrame(SirenDecoder decoder, unsigned char *DataIn, unsigned char *DataOut);

#endif /* _SIREN_DECODER_H */
