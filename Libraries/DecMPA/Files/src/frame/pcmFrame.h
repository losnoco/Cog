/*
  pcm frame description.
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */



#ifndef __PCMFRAME_H
#define __PCMFRAME_H


#include "audioFrame.h"

// this format has a sampleSize of 16, signed, endian==machine

class PCMFrame : public AudioFrame {

  short int* data;
  int len;
  int size;

 public:
  PCMFrame(int size);
  ~PCMFrame();

  int        getLen()                            { return len; }
  void       setLen(int len)                     { this->len=len; }
  int        getSize()                           { return size; }
  short int* getData()                           { return data; }

  void       putFloatData(float* data,int len);
  void       putFloatData(float* left,float* right,int len);

  void putInt16Data(short* pdata,int nLength);

  void putSilence(int nLength);
  

  void       clearrawdata()                      { len=0; }
 

};
#endif
