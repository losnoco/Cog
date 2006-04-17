/*
  pcm frame description.
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */

//changes 8/4/2002 (by Hauke Duden):
//	- removed cout and exit stuff 
//	- added #include <new> to ensure that bad_alloc will be thrown on mem error

#include "pcmFrame.h"
#include <stdlib.h>

#include <new>
#include <limits.h>

#include <string.h>

//anscheinend ist die range der Werte -1,1
/*#define convMacro(in,dtemp,tmp)		\
	tmp=(int)(in[0]*32768*0.9f);			\
	in++;							\
	if(tmp>32767)					\
	  tmp=32767;					\
	else if(tmp<-32768)				\
	  tmp=-32768;*/


#define convMacro(in,dtemp,tmp)                                              \
    in[0]*=SCALFACTOR;                                                       \
    dtemp = ((((65536.0 * 65536.0 * 16)+(65536.0 * 0.5))* 65536.0))+(in[0]); \
    tmp = ((*(int *)&dtemp) - 0x80000000);                                   \
    in++;                                                                    \
    if(tmp>32767) {                                                          \
      tmp=32767;                                                             \
    } else if (tmp<-32768) {                                                 \
      tmp =-0x8000;                                                          \
    }                                                                        



PCMFrame::PCMFrame(int size) {
  data=new short int[size];
  len=0;
  this->size=size;
  // this format has a sampleSize of 16, signed, endian==machine
  this->sampleSize=sizeof(short int)*8;
  this->lSigned=true;
  this->lBigEndian=AUDIOFRAME_BIGENDIAN;
  setFrameType(_FRAME_AUDIO_PCM);
}


PCMFrame::~PCMFrame() {
  delete data;
}


void PCMFrame::putFloatData(float* left,float* right,int copyLen) {
  int destSize=0;
  if (left  != NULL) destSize++;
  if (right != NULL) destSize++;
  destSize*=copyLen;
  /*if ((len+destSize) > size) {
    cout << "cannot copy putFloatData L/R version . Does not fit"<<endl;
    exit(0);
  }*/
  double dtemp; 
  int tmp;
  int i=copyLen;
  switch(getStereo()) {
  case 1:
    while(i > 0) {
      convMacro(left,dtemp,tmp);
      data[len++]=(short int)tmp;
      convMacro(right,dtemp,tmp);
      data[len++]=(short int)tmp;
      i--;
    } 
    break;
  case 0:
    if (left != NULL) {
      int i=copyLen;
      while(i > 0) {
	convMacro(left,dtemp,tmp);
	data[len++]=(short int)tmp;
	i--;
	// right channel empty
	len++;
      } 
    }
    if (right != NULL) {
      int i=copyLen;
      len=len-destSize;
      while(i > 0) {
	// select right channel
	len++;
	convMacro(right,dtemp,tmp);
	data[len++]=(short int)tmp;
	i--;
	// left channel already copied
      }  
    }
  default:
    /*cout << "unknown stereo value in pcmFrame"<<endl;
    exit(0);*/
	  break;
  }
}

void PCMFrame::putFloatData(float* in,int lenCopy) {

  /*if ((len+lenCopy) > size) {
    cout << "cannot copy putFloatData. Does not fit"<<endl;
    exit(0);
  }*/
  double dtemp; 
  int tmp;
  while(lenCopy > 0) {
    convMacro(in,dtemp,tmp);
    data[len++]=(short int)tmp;
    lenCopy--;
  }

}

void PCMFrame::putInt16Data(short* pData,int nLength)
{
	memcpy(data,pData,nLength*sizeof(short));
	len=nLength;
}

void PCMFrame::putSilence(int nLength)
{
	memset(data,0,nLength*sizeof(short));
	len=nLength;
}
