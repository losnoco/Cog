/*
  stores information after we found a header.
  Copyright (C) 2000  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */

//changes 8/4/2002 (by Hauke Duden):
//	- added bitratekbps

//changes 8/11/2002 (by Hauke Duden):
//	- removed unnecessary includes


#ifndef __MPEGHEADERINFO_H
#define __MPEGHEADERINFO_H


//#include <stdio.h>
//#include <iostream.h>
//#include <stdlib.h>
//#include <string.h>

#define _FREQUENCY_44100 0
#define _FREQUENCY_48000 1
#define _FREQUENCY_32000 2

#define _MODE_FULLSTEREO 0
#define _MODE_JOINT      1
#define _MODE_DUAL       2
#define _MODE_SINGLE     3

#define _VERSION_1       0
#define _VERSION_2       1


#define MAXSUBBAND         32
#define SCALEBLOCK         12



class MpegAudioHeader {

  int protection;
  int layer;
  int version;
  int padding;
  int frequency;
  int frequencyHz;
  int bitrateindex;
  int extendedmode;
  int mode;
  int inputstereo;
  int channelbitrate;
  int tableindex;
  int subbandnumber;
  int stereobound;
  int framesize;
  int layer3slots;
  int lmpeg25;
  unsigned char header[4];

  int m_nBitRateKbps;
  double m_AvgFrameSize;
  
 public:
  MpegAudioHeader();
  ~MpegAudioHeader();

  int parseHeader(unsigned char* buf);
  
  inline int getProtection()     { return protection; }
  inline int getLayer()          { return layer; }
  inline int getVersion()        { return version; }
  inline int getPadding()        { return padding; }
  inline int getFrequency()      { return frequency; }
  inline int getFrequencyHz()    { return frequencyHz; }
  inline int getBitrateindex()   { return bitrateindex; }
  inline int getExtendedmode()   { return extendedmode; }
  inline int getMode()           { return mode; }
  inline int getInputstereo()    { return inputstereo; }
  inline int getFramesize()      { return framesize; }
  inline int getLayer25()        { return lmpeg25; } 

  // MPEG layer 2
  inline int getTableindex()     { return tableindex; }
  // MPEG layer 1/2
  inline int getSubbandnumber()  { return subbandnumber; }
  inline int getStereobound()    { return stereobound; }
  // MPEG layer 3
  inline int getLayer3slots()    { return layer3slots; }
  
  int getChannelbitrate();


  inline unsigned char* getHeader() { return header; }
  int  getpcmperframe();


  void copyTo(MpegAudioHeader* dest);

  void print(const char* name);
  void printStates(const char* name);

  inline int GetBitRateKbps(){ return m_nBitRateKbps; }
  inline double GetAvgFrameSize()      { return m_AvgFrameSize; }
};
#endif
