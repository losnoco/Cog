/*
  downsample implementation
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */

//changes 8/4/2002 (by Hauke Duden):
//	- removed cout and exit stuff 

#include "synthesis.h"
#include "dct.h"

void Synthesis::computebuffer_Down(REAL *fraction,
				   REAL buffer[2][CALCBUFFERSIZE]){
  REAL *out1,*out2;

  out1=buffer[currentcalcbuffer]+calcbufferoffset;
  out2=buffer[currentcalcbuffer^1]+calcbufferoffset;
  dct64_downsample(out1,out2,fraction);
}



#define SAVE putraw(r); \
        dp+=16;vp+=15+(15-14)
#define OS   r=*vp * *dp++
#define XX   vp+=15;r+=*vp * *dp++
#define OP   r+=*--vp * *dp++

inline void Synthesis::generatesingle_Down(void)
{
  int i;
  register REAL r, *vp;
  register const REAL *dp;

  i=32/2;
  dp=filter;
  vp=calcbuffer[LS][currentcalcbuffer]+calcbufferoffset; 
// actual_v+actual_write_pos;

  switch (calcbufferoffset)
  {
    case  0:for(;i;i--,vp+=15){
              OS;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  1:for(;i;i--,vp+=15){
	      OS;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  2:for(;i;i--,vp+=15){
              OS;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  3:for(;i;i--,vp+=15){
	      OS;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  4:for(;i;i--,vp+=15){
              OS;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  5:for(;i;i--,vp+=15){
	      OS;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  6:for(;i;i--,vp+=15){
	      OS;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  7:for(;i;i--,vp+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  8:for(;i;i--,vp+=15){
              OS;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  9:for(;i;i--,vp+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case 10:for(;i;i--,vp+=15){
              OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;
	      SAVE;}break;
    case 11:for(;i;i--,vp+=15){
              OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;
	      SAVE;}break;
    case 12:for(;i;i--,vp+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;
	      SAVE;}break;
    case 13:for(;i;i--,vp+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;
	      SAVE;}break;
    case 14:for(;i;i--,vp+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;
	      SAVE;}break;
    case 15:for(;i;i--,vp+=31){
              OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
  }
}

#undef OS
#undef XX
#undef OP
#undef SAVE

#define SAVE \
	putraw(r1); \
	putraw(r2); \
        dp+=16;vp1+=15+(15-14);vp2+=15+(15-14)
#define OS r1=*vp1 * *dp; \
           r2=*vp2 * *dp++ 
#define XX vp1+=15;r1+=*vp1 * *dp; \
	   vp2+=15;r2+=*vp2 * *dp++
#define OP r1+=*--vp1 * *dp; \
	   r2+=*--vp2 * *dp++


inline void Synthesis::generate_Down(void)
{
  int i;
  REAL r1,r2;
  register REAL *vp1,*vp2;
  register const REAL *dp;

  dp=filter;
  vp1=calcbuffer[LS][currentcalcbuffer]+calcbufferoffset;
  vp2=calcbuffer[RS][currentcalcbuffer]+calcbufferoffset;
// actual_v+actual_write_pos;

  i=32/2;
  switch (calcbufferoffset)
  {
    case  0:for(;i;i--,vp1+=15,vp2+=15){
              OS;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  1:for(;i;i--,vp1+=15,vp2+=15){
	      OS;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  2:for(;i;i--,vp1+=15,vp2+=15){
              OS;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  3:for(;i;i--,vp1+=15,vp2+=15){
	      OS;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  4:for(;i;i--,vp1+=15,vp2+=15){
              OS;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  5:for(;i;i--,vp1+=15,vp2+=15){
	      OS;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  6:for(;i;i--,vp1+=15,vp2+=15){
	      OS;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  7:for(;i;i--,vp1+=15,vp2+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  8:for(;i;i--,vp1+=15,vp2+=15){
              OS;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  9:for(;i;i--,vp1+=15,vp2+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case 10:for(;i;i--,vp1+=15,vp2+=15){
              OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;
	      SAVE;}break;
    case 11:for(;i;i--,vp1+=15,vp2+=15){
              OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;
	      SAVE;}break;
    case 12:for(;i;i--,vp1+=15,vp2+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;
	      SAVE;}break;
    case 13:for(;i;i--,vp1+=15,vp2+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;
	      SAVE;}break;
    case 14:for(;i;i--,vp1+=15,vp2+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;
	      SAVE;}break;
    case 15:for(;i;i--,vp1+=31,vp2+=31){
              OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
  }
}



void Synthesis::synth_Down(int lOutputStereo,REAL *fractionL,REAL *fractionR) {
  switch(lOutputStereo) {
  case true:
    computebuffer_Down(fractionL,calcbuffer[LS]);
    computebuffer_Down(fractionR,calcbuffer[RS]);
    generate_Down();
    nextOffset();
    break;
  case false:
    computebuffer_Down(fractionL,calcbuffer[LS]);
    generatesingle_Down();
    nextOffset();
    break;
  default:
    /*cout << "unknown lOutputStereo in Synthesis::synth_Std"<<endl;
    exit(0);*/
	  break;
  }
}


void Synthesis::synthMP3_Down(int lOutputStereo,
			      REAL hout [2][SSLIMIT][SBLIMIT]) {
  int ss;
  switch(lOutputStereo) {
  case true:
    for(ss=0;ss<SSLIMIT;ss++) {
      computebuffer_Down(hout[LS][ss],calcbuffer[LS]);
      computebuffer_Down(hout[RS][ss],calcbuffer[RS]);
      generate_Down();
      nextOffset();
    }
    break;
  case false:
    for(ss=0;ss<SSLIMIT;ss++) {
      computebuffer_Down(hout[LS][ss],calcbuffer[LS]);
      generatesingle_Down();
      nextOffset();
    }
    break;
  default:
    /*cout << "unknown lOutputStereo in Synthesis::synth_Std"<<endl;
    exit(0);*/
	  break;
  }
}

