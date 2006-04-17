// MPEG/WAVE Sound library

//   (C) 1997 by Woo-jae Jung 

// Mpegsound.h
//   This is typeset for functions in MPEG/WAVE Sound library.
//   Now, it's for only linux-pc-?86

//changes 8/4/2002 (by Hauke Duden):
//	- removed dump stuff


/************************************/
/* Include default library packages */
/************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif



#include "mpegAudioStream.h"
#include "common.h"

class Synthesis;
class AudioFrame;


#ifndef _L__SOUND__
#define _L__SOUND__

#include "../mpegAudioFrame/mpegAudioHeader.h"
#include "mpegAudioBitWindow.h"


//#define DEBUG_LAYER(x) x
#define DEBUG_LAYER(x)


/**************************/
/* Define values for MPEG */
/**************************/
#define SCALEBLOCK         12
#define MAXSUBBAND         32
#define MAXCHANNEL         2
#define RAWDATASIZE        (2*2*2*32*SSLIMIT)



// Huffmancode
#define HTN 34



#define MODE_MONO   0
#define MODE_STEREO 1

/********************/
/* Type definitions */
/********************/

typedef struct {
  bool         generalflag;
  unsigned int part2_3_length;
  unsigned int big_values;
  unsigned int global_gain;
  unsigned int scalefac_compress;
  unsigned int window_switching_flag;
  unsigned int block_type;
  unsigned int mixed_block_flag;
  unsigned int table_select[3];
  unsigned int subblock_gain[3];
  unsigned int region0_count;
  unsigned int region1_count;
  unsigned int preflag;
  unsigned int scalefac_scale;
  unsigned int count1table_select;
}layer3grinfo;

typedef struct {
  unsigned main_data_begin;
  unsigned private_bits;
  struct {
    unsigned scfsi[4];
    layer3grinfo gr[2];
  }ch[2];
}layer3sideinfo;

typedef struct {
  int l[23];            /* [cb] */
  int s[3][13];         /* [window][cb] */
}layer3scalefactor;     /* [ch] */

typedef struct {
  int tablename;
  unsigned int xlen,ylen;
  unsigned int linbits;
  unsigned int treelen;
  const unsigned int (*val)[2];
}HUFFMANCODETABLE;







class DCT;
//class Dump;

// Class for converting mpeg format to raw format
class Mpegtoraw {
  /*****************************/
  /* Constant tables for layer */
  /*****************************/
private:
  static const int bitrate[2][3][15];
  static const int frequencies[2][3];
  static const REAL scalefactorstable[64];
  ATTR_ALIGN(64) static const HUFFMANCODETABLE ht[HTN];


  friend class HuffmanLookup;

  /*************************/
  /* MPEG header variables */
  /*************************/
  
  // comes from constructor, decoder works on them
  MpegAudioStream* mpegAudioStream;
  MpegAudioHeader* mpegAudioHeader;
  AudioFrame* audioFrame;
  //Dump* dump;
  Synthesis* synthesis;

  /***************************************/
  /* Interface for setting music quality */
  /***************************************/

  int lWantStereo;
  int lOutputStereo;
  int lDownSample;

public:
  Mpegtoraw(MpegAudioStream* mpegAudioStream,
	    MpegAudioHeader* mpegAudioHeader);


  ~Mpegtoraw();
  int decode(AudioFrame* audioFrame);


  void setStereo(int lStereo);
  int  getStereo();

  void setDownSample(int lDownSample);
  int getDownSample();



private:
  void initialize();
  

  
  /*****************************/
  /* Loading MPEG-Audio stream */
  /*****************************/

  union
  {
    unsigned char store[4];
    unsigned int  current;
  }u;


  int getbyte()            { return mpegAudioStream->getbyte(); }
  int getbits(int bits)    { return mpegAudioStream->getbits(bits); }
  int getbits9(int bits)   { return mpegAudioStream->getbits9(bits); }
  int getbits8()           { return mpegAudioStream->getbits8(); }
  int getbit()             { return mpegAudioStream->getbit(); }


  void sync()              { mpegAudioStream->sync(); }
  bool issync()            { return mpegAudioStream->issync()!=0; }


  /********************/
  /* Global variables */
  /********************/

  // optimisation from maplay12+
  // 0/1: nonzero for channel 0/1 2: max position for both
  int nonzero[3];

  // for Layer3

  int layer3framestart;
  int layer3part2start;

  ATTR_ALIGN(64) REAL prevblck[2][2][SBLIMIT][SSLIMIT];
  int currentprevblock;
  ATTR_ALIGN(64) layer3sideinfo sideinfo;
  ATTR_ALIGN(64) layer3scalefactor scalefactors[2];

  ATTR_ALIGN(64) MpegAudioBitWindow bitwindow;
  MpegAudioBitWindow lastValidBitwindow;
  
  int wgetbit(void);
  int wgetbits9(int bits);
  int wgetbits(int bits);
  int wgetCanReadBits();

  /*************************************/
  /* Decoding functions for each layer */
  /*************************************/

  // Extractor
  void extractlayer1(void);    // MPEG-1
  void extractlayer2(void);
  void extractlayer3(void);
  void extractlayer3_2(void);  // MPEG-2


  // Functions for layer 3
  void layer3initialize(void);
  bool layer3getsideinfo(void);
  bool layer3getsideinfo_2(void);
  void layer3getscalefactors(int ch,int gr);
  void layer3getscalefactors_2(int ch);
  void layer3huffmandecode(int ch,int gr,int out[SBLIMIT][SSLIMIT]);
  REAL layer3twopow2(int scale,int preflag,int pretab_offset,int l);
  REAL layer3twopow2_1(int a,int b,int c);
  void layer3dequantizesample(int ch,int gr,int   in[SBLIMIT][SSLIMIT],
			                    REAL out[SBLIMIT][SSLIMIT]);
  void adjustNonZero(REAL in[2][SBLIMIT][SSLIMIT]);
  void layer3fixtostereo(int gr,REAL  in[2][SBLIMIT][SSLIMIT]);
  void layer3reorderandantialias(int ch,int gr,REAL  in[SBLIMIT][SSLIMIT],
				               REAL out[SBLIMIT][SSLIMIT]);

  void layer3hybrid(int ch,int gr,REAL in[SBLIMIT][SSLIMIT],
		                  REAL out[SSLIMIT][SBLIMIT]);
  
  void huffmandecoder_1(const HUFFMANCODETABLE *h,int *x,int *y);
  void huffmandecoder_2(const HUFFMANCODETABLE *h,int *x,int *y,int *v,int *w);

};



#endif
