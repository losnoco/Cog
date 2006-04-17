/*
  decoder interface for the splay mp3 decoder.
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */

//changes 8/4/2002 (by Hauke Duden):
//	- added #include <new> to ensure that bad_alloc will be thrown on mem error
//	- removed dump stuff
//	- added function to access header from outside

#define _FROM_SOURCE
#include "../mpegAudioFrame/dxHead.h"
#include "splayDecoder.h"
#include "mpegsound.h"

#include <new>
#include <string.h>


SplayDecoder::SplayDecoder() {
  header = new MpegAudioHeader();
  stream = new MpegAudioStream();
  server = new Mpegtoraw(stream,header);

  xHeadData=new XHEADDATA();
  xHeadData->toc=new unsigned char[101];
  //dump=new Dump();
}


SplayDecoder::~SplayDecoder() {
  delete (xHeadData->toc);
  delete xHeadData;
  
  delete server;
  delete header;
  delete stream;
  //delete dump;
}



int SplayDecoder::decode(unsigned char* ptr, int len,AudioFrame* dest) {
  int back;
  // fist setup the stream and the 4 bytes header info;
  //dump->dump((char*)ptr,len);
  if (header->parseHeader(ptr) == false) {
    return false;
  }
  // maybe a Xing Header?
  if (len >= 152+4) {
    int lXing=GetXingHeader(xHeadData,(unsigned char*)ptr);
    if (lXing) {
      return false;
    }
  }
  stream->setFrame(ptr+4,len-4);
  back=server->decode(dest);

  return back;

}


void SplayDecoder::config(const char* key,const char* val,void* ) {
  if (strcmp(key,"2")==0) {
    server->setDownSample(atoi(val));
  }
  if (strcmp(key,"m")==0) {
    server->setStereo(atoi(val));
  }
}

MpegAudioHeader* SplayDecoder::GetMPEGAudioHeader()
{
	return header;
}

