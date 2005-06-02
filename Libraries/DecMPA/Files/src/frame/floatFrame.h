/*
  stores frames as floats.
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */



#ifndef __FLOATFRAME_H
#define __FLOATFRAME_H

#include "audioFrame.h"

// this format has a sampleSize of sizeof(float), signed, endian==machine

class FloatFrame : public AudioFrame {

  float* data;
  int len;
  int size;


 public:
  FloatFrame(int size);
  ~FloatFrame();

  int        getLen()                            { return len; }
  void       setLen(int len)                     { this->len=len; }
  int        getSize()                           { return size; }
  float*     getData()                           { return data; }

  void       putFloatData(float* data,int len);
  void       putFloatData(float* left,float* right,int len);

  void       putInt16Data(short* pdata,int nLength);
  void putSilence(int nLength);

  void       clearrawdata()                      { len=0; }
 


};
#endif
