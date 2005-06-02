/*
  header for synthesis
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */



#ifndef __SYNTHESIS_H
#define __SYNTHESIS_H

#include "common.h"
#include "dct.h"

#define CALCBUFFERSIZE     512
#define FRAMESIZE         (2*2*2*32*18)


class Synthesis {

  //
  // Subbandsynthesis two calcbuffers for every channel, and two channels.
  // calcbufferL[0]=calcbuffer[0]
  // calcbufferL[1]=calcbuffer[1]
  // calcbufferR[0]=calcbuffer[2]
  // calcbufferR[1]=calcbuffer[3]
  ATTR_ALIGN(64) REAL calcbuffer[2][2][CALCBUFFERSIZE];
  ATTR_ALIGN(64) int  currentcalcbuffer,calcbufferoffset;
  ATTR_ALIGN(64) static const REAL filter[512];
  ATTR_ALIGN(64) REAL out[FRAMESIZE];
  int outpos;

 public:
  Synthesis();
  ~Synthesis();
  
  // mpeg1,2
  void doSynth(int lDownSample,int lOutputStereo,
	       REAL *fractionL,REAL *fractionR);

  void doMP3Synth(int lDownSample,int lOutputStereo,
		  REAL in[2][SSLIMIT][SBLIMIT]);

  // put mpeg to raw
  inline void putraw(REAL val) {
    out[outpos++]=val;
  }

  inline REAL* getOutputData() { return out; }
  inline void clearrawdata() { outpos=0; }
  inline int getLen() { return outpos; }
  

 private:
  void synth_Down(int lOutputStereo,REAL *fractionL,REAL *fractionR);
  void synth_Std(int lOutputStereo,REAL *fractionL,REAL *fractionR);
  
  void synthMP3_Down(int lOutputStereo,REAL hout [2][SSLIMIT][SBLIMIT]);
  void synthMP3_Std(int lOutputStereo,REAL hout [2][SSLIMIT][SBLIMIT]);

  inline void nextOffset() {
    calcbufferoffset++;
    calcbufferoffset&=0xf;
    /*
    if (calcbufferoffset<15) {
      calcbufferoffset++;
    } else {
      calcbufferoffset=0;
    }
    */
    currentcalcbuffer^=1;
  }

  
  void computebuffer_Std(REAL *fraction,REAL buffer[2][CALCBUFFERSIZE]);
  void generate_Std(void);
  void generatesingle_Std(void);
  
  void computebuffer_Down(REAL *fraction,REAL buffer[2][CALCBUFFERSIZE]);
  void generate_Down(void);
  void generatesingle_Down(void);
  

};

#endif
