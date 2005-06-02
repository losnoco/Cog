/*
  bitwindow class
  Copyright (C) 2000  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */

//changes 8/11/2002 (by Hauke Duden):
//	- removed unnecessary includes


#ifndef __MPEGBITWINDOW_H
#define __MPEGBITWINDOW_H


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//#include <iostream.h>

#ifndef WORDS_BIGENDIAN
#define _KEY 0
#else
#define _KEY 3
#endif

#define WINDOWSIZE    4096
#define BITWINDOWSIZE    (WINDOWSIZE*8)


class MpegAudioBitWindow {

  int  point,bitindex;
  char buffer[2*WINDOWSIZE];

 public:
  MpegAudioBitWindow(){bitindex=point=0;}

  inline void initialize(void)  {bitindex=point=0;}
  inline int  gettotalbit(void) const {return bitindex;}

  inline void putbyte(int c)    {buffer[point&(WINDOWSIZE-1)]=c;point++;}
  void wrap(void);
  inline void rewind(int bits)  {bitindex-=bits;}
  inline void forward(int bits) {bitindex+=bits;}

  // returns number of bits which can safley read
  int  getCanReadBits();


  //
  // Ugly bitgetting inline functions for higher speed
  //


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
    u.store[_KEY]=buffer[(bitindex>>3)&(WINDOWSIZE-1)]<<bi;
    //u.store[_KEY]=buffer[bitindex>>3]<<bi;
    bi=8-bi;
    bitindex+=bi;
    
    while(bits) {
      if(!bi) {
	u.store[_KEY]=buffer[(bitindex>>3)&(WINDOWSIZE-1)];
	//u.store[_KEY]=buffer[bitindex>>3];
	bitindex+=8;
	bi=8;
      }
      
      if(bits>=bi) {
	u.current<<=bi;
	bits-=bi;
	bi=0;
      }
      else {
	u.current<<=bits;
	bi-=bits;
	bits=0;
      }
    }
    bitindex-=bi;
    
    return (u.current>>8);
  }


  int getbit(void) {
    register int r=(buffer[(bitindex>>3)&(WINDOWSIZE-1)]>>(7-(bitindex&7)))&1;
    //register int r=(buffer[bitindex>>3]>>(7-(bitindex&7)))&1;
    bitindex++;
    return r;
  }

  // no range check version
  inline int getbits9_f(int bits) {
    register unsigned short a;
    {
      int offset=bitindex>>3;
      a=(((unsigned char)buffer[offset])<<8)|((unsigned char)buffer[offset+1]);
    }
    a<<=(bitindex&7);
    bitindex+=bits;
    return (int)((unsigned int)(a>>(16-bits)));
  }    

  // range check version
  int getbits9(int bits) {
    register unsigned short a;
    {
      int offset=(bitindex>>3)&(WINDOWSIZE-1);
      a=(((unsigned char)buffer[offset])<<8)|((unsigned char)buffer[offset+1]);
    }

    a<<=(bitindex&7);
    bitindex+=bits;
    return (int)((unsigned int)(a>>(16-bits)));
  }

  int  peek8() {
    int offset = (bitindex>>3)&(WINDOWSIZE-1), a;
    a=(((unsigned char)buffer[offset])<<8) | ((unsigned char)buffer[offset+1]);
    return (a >> (8-(bitindex&7))) & 0xff;
  }  
  

};
#endif
