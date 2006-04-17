/*
  header for synthesis
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */

//changes 8/4/2002 (by Hauke Duden):
//	- removed cout and exit stuff 


#include "synthesis.h"

Synthesis::Synthesis() {

  int i;
  outpos=0;
  calcbufferoffset=15;
  currentcalcbuffer=0;

  for(i=CALCBUFFERSIZE-1;i>=0;i--)
    calcbuffer[LS][0][i]=calcbuffer[LS][1][i]=
    calcbuffer[RS][0][i]=calcbuffer[RS][1][i]=0.0;
  
  initialize_dct64();
  initialize_dct64_downsample();
}


Synthesis::~Synthesis() {
}


void Synthesis::doSynth(int lDownSample,int lOutputStereo,
			REAL *fractionL,REAL *fractionR) {
  switch(lDownSample) {
  case false:
    synth_Std(lOutputStereo,fractionL,fractionR);
    break;
  case true:
    synth_Down(lOutputStereo,fractionL,fractionR);
    break;
  default:
    /*cout << "unknown downsample parameter"<<lDownSample<<endl;
    exit(0);*/
	  break;
  } 
}

  
void Synthesis::doMP3Synth(int lDownSample,int lOutputStereo,
			   REAL in[2][SSLIMIT][SBLIMIT]) {

  switch(lDownSample) {
  case false:
    synthMP3_Std(lOutputStereo,in);
    break;
  case true:
    synthMP3_Down(lOutputStereo,in);
    break;
  default:
    /*cout << "unknown downsample parameter:"<<lDownSample<<endl;
    exit(0);*/
	  break;
  }
}


