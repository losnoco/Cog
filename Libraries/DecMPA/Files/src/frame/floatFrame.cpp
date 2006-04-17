/*
  stores frames as floats.
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */

//changes 8/4/2002 (by Hauke Duden):
//	- removed cout and exit stuff 
//	- added #include <new> to ensure that bad_alloc will be thrown on mem error

#include "floatFrame.h"

#include <new>
#include <memory.h>


FloatFrame::FloatFrame(int size) {
  data=new float[size];
  len=0;
  this->size=size;
  this->sampleSize=sizeof(float)*8;
  this->lSigned=true;
  this->lBigEndian=AUDIOFRAME_BIGENDIAN;
  setFrameType(_FRAME_AUDIO_FLOAT);
}


FloatFrame::~FloatFrame() {
  delete data;
}


void FloatFrame::putFloatData(float* in,int lenCopy) {
  //if ((len+lenCopy) > size) {
  //  cout << "cannot copy putFloatData. Does not fit"<<endl;
  //  exit(0);
  //}
  memcpy(data+len,in,lenCopy*sizeof(float));
  len+=lenCopy;


}


void FloatFrame::putFloatData(float* left,float* right,int len) {
  //cout << "not yet implemented"<<endl;
}


void FloatFrame::putInt16Data(short* pData,int nLength)
{
	for(int i=0;i<nLength;i++)
		data[i]=((float)pData[i])/32768.0f;

	len=nLength;
}

void FloatFrame::putSilence(int nLength)
{
	for(int i=0;i<nLength;i++)
		data[i]=0.0f;

	len=nLength;
}

