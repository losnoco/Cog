/*
  initializer/resyncer/frame detection etc.. for mpeg audio
  Copyright (C) 2000  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */



#ifndef __MPEGAUDIOSTREAM_H
#define __MPEGAUDIOSTREAM_H

// we include this for the big_endian define
//#include "mpegAudioBitWindow.h"

#ifndef WORDS_BIGENDIAN
#define _KEY 0
#else
#define _KEY 3
#endif



#define _MAX_MPEG_BUFFERSIZE    4096


/**
   Here we go from the frame to the bitlevel.


*/

class MpegAudioStream {

  char* buffer;
  int len;
  int bitindex;

 public:
  MpegAudioStream();
  ~MpegAudioStream();

  void setFrame(unsigned char* prt,int len);

  // Bit functions

  inline char* getBuffer()       { return buffer; }
  inline int   getBufferSize()   { return _MAX_MPEG_BUFFERSIZE ;}
  inline void sync()             { bitindex=(bitindex+7)&0xFFFFFFF8; }
  inline int issync()            { return (bitindex&7);};
  
  /**
     Now follow ugly inline function. The performance gain is 1.5 %
     on a 400 MHz AMD
  */

  inline int getbyte()  {
    int r=(unsigned char)buffer[bitindex>>3];
    bitindex+=8;
    return r;
  }

  inline int getbits9(int bits) {
    register unsigned short a;
    {
      int offset=bitindex>>3;
      
      a=(((unsigned char)buffer[offset])<<8) | 
	((unsigned char)buffer[offset+1]);
    }
    
    a<<=(bitindex&7);
    bitindex+=bits;
    return (int)((unsigned int)(a>>(16-bits))); 
  }

  inline int getbits8() {
    register unsigned short a;
    
    {
      int offset=bitindex>>3;
      
      a=(((unsigned char)buffer[offset])<<8) | 
	((unsigned char)buffer[offset+1]);
    }
    
    a<<=(bitindex&7);
    bitindex+=8;
    return (int)((unsigned int)(a>>8));
  }

  inline int getbit() {
     register int r=(buffer[bitindex>>3]>>(7-(bitindex&7)))&1;

     bitindex++;
     return r;
  }

  inline int getbits(int bits) {
    union
    {
      char store[4];
      int current;
    }u;
    int bi;
    
    if(!bits)return 0;
    
    u.current=0;
    bi=(bitindex&7);
    u.store[_KEY]=buffer[bitindex>>3]<<bi;
    bi=8-bi;
    bitindex+=bi;
    
    while(bits) {
	if(!bi)  {
	  u.store[_KEY]=buffer[bitindex>>3];
	  bitindex+=8;
	  bi=8;
	}
	if(bits>=bi) {
	  u.current<<=bi;
	  bits-=bi;
	  bi=0;
	} else {
	  u.current<<=bits;
	  bi-=bits;
	  bits=0;
	}
    }
    bitindex-=bi;
    return (u.current>>8);
  }




};


#endif
