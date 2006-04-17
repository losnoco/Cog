/*
  abstract definition of an audio frame
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */




#ifndef __AUDIOFRAME_H
#define __AUDIOFRAME_H


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef WORDS_BIGENDIAN
#define AUDIOFRAME_BIGENDIAN 1
#else
#define AUDIOFRAME_BIGENDIAN 0
#endif

#include "frame.h"

#define SCALFACTOR           SHRT_MAX
#define MP3FRAMESIZE         (2*2*2*32*18)


class AudioFrame : public Frame {

  int stereo;
  int frequencyHZ;
  
 public:
  AudioFrame();
  virtual ~AudioFrame();

  // info about "import" data
  void setFrameFormat(int stereo,int freq);

  inline int getStereo()      { return stereo;      }
  inline int getFrequenceHZ() { return frequencyHZ; }

  // these return values depend on the implementation
  // how the data is stored internally after "import"
  inline int getSampleSize()  { return sampleSize;  }
  inline int getBigEndian()   { return lBigEndian;  }
  inline int getSigned()      { return lSigned;     }

  // info about output
  virtual int getLen();
  virtual void setLen(int len);
  virtual int getSize();
  virtual void clearrawdata();

  // data import
  virtual void putFloatData(float* data,int len);
  virtual void putFloatData(float* left,float* right,int len);

  virtual void putInt16Data(short* pdata,int nLength);

  virtual void putSilence(int nLength);

   int isFormatEqual(AudioFrame* compare);
  // Note: this can only be called with _real_ AudioFrame's as dest
  void copyFormat(AudioFrame* dest);

  void print(const char* msg);

 protected:
  int sampleSize;
  int lBigEndian;
  int lSigned;

};


#endif
