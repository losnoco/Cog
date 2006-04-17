/*
  decoder interface for the splay mp3 decoder.
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */

//changes 8/4/2002 (by Hauke Duden):
//	- removed dump stuff
//	- added function to access header from outside

//changes 8/11/2002 (by Hauke Duden):
//	- removed unnecessary includes

#ifndef __SPLAYDECODER_H
#define __SPLAYDECODER_H

// state definitions for splay decoder

#define _SPLAY_RESET              0
#define _SPLAY_EOF                1
#define _SPLAY_FIRSTINIT          2
#define _SPLAY_REINIT             3
#define _SPLAY_DECODE             4
#define _SPLAY_FRAME              5


#include "../frame/audioFrame.h"
//#include "dump.h"
//#include <string.h>

class Mpegtoraw;
class MpegAudioStream;
class MpegAudioHeader;


/**
   The decoder interface.
   The decoder expects an mpeg audio frame.
   The call to decode is "atomic", after that you have
   a PCMFrame to play.

*/



class SplayDecoder {

  MpegAudioStream* stream;
  MpegAudioHeader* header;
  Mpegtoraw* server;
  //Dump* dump;
#ifdef _FROM_SOURCE
  XHEADDATA* xHeadData;
#else
  void* xHeadData;
#endif


 public:
  SplayDecoder();
  ~SplayDecoder();

  int decode(unsigned char* ptr, int len,AudioFrame* dest);
  void config(const char* key,const char* val,void* ret);

  MpegAudioHeader* GetMPEGAudioHeader();

};
#endif
