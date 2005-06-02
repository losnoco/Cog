/*
  std synth implementation
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


//#include "dct64.cpp"

inline void Synthesis::computebuffer_Std(REAL *fraction,
					 REAL buffer[2][CALCBUFFERSIZE]) {
  REAL *out1,*out2;
  out1=buffer[currentcalcbuffer]+calcbufferoffset;
  out2=buffer[currentcalcbuffer^1]+calcbufferoffset;
  dct64(out1,out2,fraction);
}


#define SAVE putraw(r);
#define OS  r=*vp * *dp++
#define XX  vp+=15;r+=*vp * *dp++
#define OP  r+=*--vp * *dp++


inline void Synthesis::generatesingle_Std(void) {
  int i;
  register REAL r, *vp;
  register const REAL *dp;

  i=32;
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


#define SAVE putraw(r1); putraw(r2);

#define OS r1=*vp1 * *dp; \
           r2=*vp2 * *dp++ 
#define XX vp1+=15;r1+=*vp1 * *dp; \
           vp2+=15;r2+=*vp2 * *dp++
#define OP r1+=*--vp1 * *dp; \
           r2+=*--vp2 * *dp++

/*
inline void Synthesis::generate_old(void)
{
  int i;
  REAL r1,r2;
  register REAL *vp1,*vp2;
  register const REAL *dp;

  dp=filter;
  vp1=calcbuffer[LS][currentcalcbuffer]+calcbufferoffset;
  vp2=calcbuffer[RS][currentcalcbuffer]+calcbufferoffset;
// actual_v+actual_write_pos;

  i=32;
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
*/
#undef OS
#undef XX
#undef OP
#undef SAVE

#include "op.h"
#define SAVE putraw(r1); putraw(r2);

inline void Synthesis::generate_Std(void)
{
  int i;
  REAL r1,r2;

  register REAL *vp1;
  register const REAL *dp;

  dp=filter;
  vp1=calcbuffer[LS][currentcalcbuffer]+calcbufferoffset;
  // we calculate cp2 from vp1 because they are both
  // in the same array. code was:
  // register REAL* vp2
  //vp2=calcbuffer[RS][currentcalcbuffer]+calcbufferoffset;

  i=32;
  switch (calcbufferoffset)
  {
    case  0:for(;i;i--,OP_END_1(15,14)){
              OS;XX1;XX2;OP14;
	      SAVE;}break;
    case  1:for(;i;i--,OP_END_1(15,13)){
              OS;OP;OP_END_2(1);XX2;OP13;
	      SAVE;}break;
    case  2:for(;i;i--,OP_END_1(15,12)){
              OS;OP2;OP_END_2(2);XX2;OP12;
	      SAVE;}break;
    case  3:for(;i;i--,OP_END_1(15,11)){ 
              OS;OP3;OP_END_2(3);XX2;OP11;
	      SAVE;}break;
    case  4:for(;i;i--,OP_END_1(15,10)){
              OS;OP4;OP_END_2(4);XX2;OP10;
	      SAVE;}break;
    case  5:for(;i;i--,OP_END_1(15,9)){
              OS;OP5;OP_END_2(5);XX2;OP9;
	      SAVE;}break;
    case  6:for(;i;i--,OP_END_1(15,8)){
              OS;OP6;OP_END_2(6);XX2;OP8;
	      SAVE;}break;
    case  7:for(;i;i--,OP_END_1(15,7)){
              OS;OP7;OP_END_2(7);XX2;OP7;
	      SAVE;}break;
    case  8:for(;i;i--,OP_END_1(15,6)){
              OS;OP8;OP_END_2(8);XX2;OP6;
	      SAVE;}break;
    case  9:for(;i;i--,OP_END_1(15,5)){
              OS;OP9;OP_END_2(9);XX2;OP5;
	      SAVE;}break;
    case 10:for(;i;i--,OP_END_1(15,4)){
              OS;OP10;OP_END_2(10);XX2;OP4;
	      SAVE;}break;
    case 11:for(;i;i--,OP_END_1(15,3)){
              OS;OP11;OP_END_2(11);XX2;OP3;
	      SAVE;}break;
    case 12:for(;i;i--,OP_END_1(15,2)){
              OS;OP12;OP_END_2(12);XX2;OP2;
	      SAVE;}break;
    case 13:for(;i;i--,OP_END_1(15,1)){
              OS;OP13;OP_END_2(13);XX2;OP;
	      SAVE;}break;
    case 14:for(;i;i--,vp1+=15){
	      OS;OP14;OP_END_2(14);XX2;
	      SAVE;}break;
    case 15:for(;i;i--,OP_END_1(31,15)){
              OS;OP15;
	      SAVE;}break;
  }
}
#undef OP_END_1
#undef OP_END_2
#undef OP
#undef OP1
#undef OP2
#undef OP3
#undef OP4
#undef OP5
#undef OP6
#undef OP7
#undef OP8
#undef OP9
#undef OP10
#undef OP11
#undef OP12
#undef OP13
#undef OP14
#undef OP15
#undef OS
#undef XX1
#undef XX2
#undef SCHEDULE
#undef SCHEDULE1
#undef SCHEDULE2
#undef SAVE

void Synthesis::synth_Std(int lOutputStereo,REAL *fractionL,REAL *fractionR) {
  switch(lOutputStereo) {
  case true:
    computebuffer_Std(fractionL,calcbuffer[LS]);
    computebuffer_Std(fractionR,calcbuffer[RS]);
    generate_Std();
    nextOffset();
    break;
  case false:
    computebuffer_Std(fractionL,calcbuffer[LS]);
    generatesingle_Std();
    nextOffset();
    break;
  default:
    /*cout << "unknown lOutputStereo in Synthesis::synth_Std"<<endl;
    exit(0);*/
	  break;
  }
}


void Synthesis::synthMP3_Std(int lOutputStereo,
			     REAL hout [2][SSLIMIT][SBLIMIT]) {
  int ss;
  switch(lOutputStereo) {
  case true:
    for(ss=0;ss<SSLIMIT;ss++) {
      computebuffer_Std(hout[LS][ss],calcbuffer[LS]);
      computebuffer_Std(hout[RS][ss],calcbuffer[RS]);
      generate_Std();
      nextOffset();
    }
    break;
  case false:
    for(ss=0;ss<SSLIMIT;ss++) {
      computebuffer_Std(hout[LS][ss],calcbuffer[LS]);
      generatesingle_Std();
      nextOffset();
    }
    break;
  default:
    /*cout << "unknown lOutputStereo in Synthesis::synth_Std"<<endl;
    exit(0);*/
	  break;
  }
}
