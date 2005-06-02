/*
  stores information after we found a header.
  Copyright (C) 2000  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */

//changes 8/4/2002 (by Hauke Duden):
//	- removed cout and exit stuff 
//	- added bitratekbps variable

#include "mpegAudioHeader.h"

#define DEBUG_HEADER(x)
//#define DEBUG_HEADER(x)  x



static const int frequencies[3][3]= {
    {44100,48000,32000},  // MPEG 1
    {22050,24000,16000},  // MPEG 2
    {11025,12000,8000}    // MPEG 2.5
};

static int translate[3][2][16] = { { { 2,0,0,2,2,2,3,3,3,3,3,3,3,3,3,2 } ,
				     { 2,0,0,0,0,0,0,2,2,2,3,3,3,3,3,2 } } ,
				   { { 2,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2 } ,
				     { 2,0,0,0,0,0,0,2,2,2,2,2,2,2,2,2 } } ,
				   { { 2,1,1,2,2,2,3,3,3,3,3,3,3,3,3,2 } ,
				     { 2,1,1,1,1,1,1,2,2,2,3,3,3,3,3,2 } } };


static int sblims[5] = { 8 , 12 , 27, 30 , 30 };


static const int bitrate[2][3][15]= {
  // MPEG 1
  {{0,32,64,96,128,160,192,224,256,288,320,352,384,416,448},
   {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384},
   {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320}},

  // MPEG 2
  {{0,32,48,56,64,80,96,112,128,144,160,176,192,224,256},
   {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160},
   {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160}}
};


MpegAudioHeader::MpegAudioHeader() {
  
}


MpegAudioHeader::~MpegAudioHeader() {
}


int MpegAudioHeader::getChannelbitrate() {
  //cout << "getChannelbitrate not implemented"<<endl;
  return 0;
}

int MpegAudioHeader::parseHeader(unsigned char* buf){

  int c;
  int mpeg25=false;
  

  // Analyzing
  header[0]=buf[0];
  header[1]=buf[1];
  header[2]=buf[2];
  header[3]=buf[3];

  c=buf[1];
  lmpeg25=false;
  if ( (c&0xf0) == 0xe0) {
    lmpeg25=true;
  } 

  c&=0xf;
  protection=c&1;
  layer=4-((c>>1)&3);
  // we catch the layer==4 error later, for now go on with parsing
  version=(int)(((c>>3)&1)^1);
  if ((version==0) && lmpeg25) {
    //DEBUG_HEADER(cout << "wrong lsf/mpeg25 combination"<<endl;)
    return false;
  }
  c=buf[2];

  c=((c))>>1;
  padding=(c&1);
  c>>=1;
  frequency=(int)(c&3);
  c>>=2;
  bitrateindex=(int)c;
  if(bitrateindex>=15) {
    //DEBUG_HEADER(cout << "bitrateindex error"<<endl;)
    return false;
  }
  c=buf[3];

  c=((unsigned int)(c))>>4;
  extendedmode=c&3;
  mode=(int)(c>>2);


  // Making information
  inputstereo= (mode==_MODE_SINGLE)?0:1;

  //
  // frequency can be 0,1 or 2 but the mask above allows 3 as well
  // check now.
  if (frequency > 2) {
    //DEBUG_HEADER(cout << "frequency value out of range"<<endl;)
    return false;
  }

  //
  // does not belong here should be in the layer specific parts. [START]
  //

  switch(layer) {
  case 3: 
    subbandnumber=0;
    stereobound=0;
    tableindex=0;
    break;
  case 2:
    tableindex    = translate[frequency][inputstereo][bitrateindex];
    subbandnumber = sblims[tableindex];
    stereobound   = subbandnumber;
    /*
      Now merge the tableindex, for the bitalloclengthtable
    */
    tableindex=tableindex>>1;
    if(mode==_MODE_SINGLE)stereobound=0;
    if(mode==_MODE_JOINT)stereobound=(extendedmode+1)<<2;

    break;
  case 1:
    subbandnumber=MAXSUBBAND;
    stereobound=subbandnumber;
    tableindex=0;
    if(mode==_MODE_SINGLE)stereobound=0;
    if(mode==_MODE_JOINT)stereobound=(extendedmode+1)<<2;
    break;
  default:
    //DEBUG_HEADER(cout <<"unknown layer"<<endl;)
    return false;
  }

  //
  // does not belong here should be in the layer specific parts. [END]
  //
  frequencyHz=frequencies[version+lmpeg25][frequency];
  // framesize & slots
  if(layer==1)
  {
    if (frequencyHz <= 0)
      return false;
	
	m_AvgFrameSize=(12000.0*bitrate[version][0][bitrateindex]*4.0)/frequencyHz;

	framesize=(12000*bitrate[version][0][bitrateindex])/frequencyHz;
  
    if(frequency==_FREQUENCY_44100 && padding)
		framesize++;
    framesize<<=2;
  }
  else
  {
    int freq=frequencyHz<<version;
    if (freq <= 0)
      return false;

	m_AvgFrameSize=(144000.0*bitrate[version][layer-1][bitrateindex])/freq;

    //the padding seems to be used to increase the size of some
	//frames so that the bitrate has exactly the specified value.
	//In other words, it is used to compensate for the rounding of the
	//frame sizes to full bytes
	framesize=(144000*bitrate[version][layer-1][bitrateindex])/freq; 
    if(padding)
		framesize++;

    if(layer==3)
	{
      if(version)
        layer3slots=framesize-((mode==_MODE_SINGLE)?9:17)  -(protection?0:2)  -4;
      else
        layer3slots=framesize-((mode==_MODE_SINGLE)?17:32) -(protection?0:2) -4;
    }
  }
  if (framesize <= 0)
  {
    //DEBUG_HEADER(cout << "framesize negative"<<endl;)
    return false;
  }

  m_nBitRateKbps=bitrate[version][layer-1][bitrateindex];

  return true;

}
  

int MpegAudioHeader::getpcmperframe() {
  int s;
  
  s=32;
  if(layer==3) {
    s*=18;
    if(version==0)s*=2;
  }
  else {
    s*=SCALEBLOCK;
    if(layer==2)s*=3;
  }

  return s;
}


void MpegAudioHeader::copyTo(MpegAudioHeader* dest) {
  dest->protection=protection;
  dest->layer=layer;
  dest->version=version;
  dest->padding=padding;
  dest->frequency=frequency;
  dest->frequencyHz=frequencyHz;
  dest->bitrateindex=bitrateindex;
  dest->extendedmode=extendedmode;
  dest->mode=mode;
  dest->inputstereo=inputstereo;
  dest->channelbitrate=channelbitrate;
  dest->tableindex=tableindex;
  dest->subbandnumber=subbandnumber;
  dest->stereobound=stereobound;
  dest->framesize=framesize;
  dest->layer3slots=layer3slots;
  dest->lmpeg25=lmpeg25;
}


void MpegAudioHeader::print(const char* name) {
/*  cout << "MpegAudioHeader [START]:"<<name<<endl;
  printf("header:%1x%1x%1x%1x\n",header[0],header[1],header[2],header[3]);
  cout << "getProtection:"<<getProtection()<<endl;
  cout << "getLayer:"<<getLayer()<<endl;
  cout << "getVersion:"<<getVersion()<<endl;
  cout << "getPadding:"<<getPadding()<<endl;
  cout << "getFrequency:"<<getFrequency()<<endl;
  cout << "getFrequencyHz:"<<getFrequencyHz()<<endl;
  cout << "getBitrateindex:"<<getBitrateindex()<<endl;
  cout << "getExtendedmode:"<<getExtendedmode()<<endl;
  cout << "getMode():"<<getMode()<<endl;
  cout << "getInputstereo:"<<getInputstereo()<<endl;
  cout << "getChannelbitrate:"<<getChannelbitrate()<<endl;
  cout << "getTableindex:"<<getTableindex()<<endl;
  cout << "getSubbandnumber:"<<getSubbandnumber()<<endl;
  cout << "getStereobound:"<<getStereobound()<<endl;
  cout << "getFramesize:"<<getFramesize()<<endl;
  cout << "getLayer3slots:"<<getLayer3slots()<<endl;
  cout << "getpcmperframe:"<<getpcmperframe()<<endl;
  cout << "MpegAudioHeader [END]:"<<name<<endl;*/
 
}







