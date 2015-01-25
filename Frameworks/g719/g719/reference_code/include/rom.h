/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#ifndef _ROM_H
#define _ROM_H

#include "complxop.h"

extern float fft120cnst[144];
extern complex ptwdf[480];
extern complex dct480_table_1[480];
extern complex dct480_table_2[480];
extern complex dct120_table_1[120];
extern complex dct120_table_2[120];
extern int indxPre[120];
extern int indxPost[120];
extern float wscw16q15[];

extern short sfm_start[44];
extern short sfm_end[44];
extern short sfmsize[44];

extern float short_window[480];
extern float window[1920];

extern float dicn[40];
extern float thren[39];
extern short dicnlg2[40];

extern short RV[10];
extern short FacLVQ2Qv[10];
extern short FacLVQ2Mask[10];
extern short FacLVQ2HalfQv[10];

extern short dic0[10][8];
extern short dic1[10][8];
extern short dic2[10];
extern short dic3[256];
extern short dic4[256][8];

extern short huffnorm[32];
extern short huffsizn[32];
extern short huffcoef[60];
extern short huffsizc[60];
extern short huffoffset[6];

extern short sfm_width[20];
extern short a[20];

#endif
