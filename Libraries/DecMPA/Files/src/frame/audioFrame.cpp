/*
  abstract definition of an audio frame
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */

//changes 8/4/2002 (by Hauke Duden):
//	- removed cout and exit stuff 


#include "audioFrame.h"


AudioFrame::AudioFrame() {
  
  stereo=-1;
  frequencyHZ=-1;

  sampleSize=-1;
  lBigEndian=-1;
  lSigned=-1;
  setFrameType(_FRAME_AUDIO_BASE);
}


AudioFrame::~AudioFrame() {
}


int AudioFrame::getLen() {
  //cout << "direct virtual call AudioFrame::getLen"<<endl;
  return 0;
}


void AudioFrame::setLen(int ) {
  //cout << "direct virtual call AudioFrame::setLen"<<endl;
}


int AudioFrame::getSize() {
  //cout << "direct virtual call AudioFrame::getSize"<<endl;
  return 0;
}


void AudioFrame::putFloatData(float* ,int ) {
  //cout << "direct virtual call AudioFrame::putFloatData"<<endl;
}

void AudioFrame::putFloatData(float* ,float* ,int ) {
  //cout << "direct virtual call AudioFrame::putFloatData L/R version"<<endl;
}

void AudioFrame::putInt16Data(short* pdata,int nLength)
{
}

void AudioFrame::putSilence(int nLength)
{
}


void AudioFrame::clearrawdata() {
  //cout << "direct virtual call AudioFrame::clearrawdata"<<endl;
}

void AudioFrame::setFrameFormat(int stereo,int freq) {
  this->stereo=stereo;
  this->frequencyHZ=freq;
}



int AudioFrame::isFormatEqual(AudioFrame* compare) {
  if(compare->getStereo() != stereo) {
    return false;
  }
  if(compare->getSampleSize() != sampleSize) {
    return false;
  }
  if(compare->getBigEndian() != lBigEndian) {
    return false;
  }
  if(compare->getFrequenceHZ() != frequencyHZ) {
    return false;
  }
  if(compare->getSigned() != lSigned) {
    return false;
  }
  return true;
}

void AudioFrame::print(const char* msg) {
  /*cout << "PCMFrame::print:"<<msg<<endl;
  cout << "stereo:"<<stereo<<endl;
  cout << "sampleSize:"<<sampleSize<<endl;
  cout << "lBigEndian:"<<lBigEndian<<endl;
  cout << "frequencyHZ:"<<frequencyHZ<<endl;
  cout << "lSigned:"<<lSigned<<endl;*/
}


void AudioFrame::copyFormat(AudioFrame* dest) {
  //if (dest->getFrameType() != _FRAME_AUDIO_BASE) {
  //  cout << "cannot copy frameFormat into frametype!= _FRAME_AUDIO_BASE"<<endl;
  //  exit(0);
  //}
  dest->setFrameFormat(getStereo(),getFrequenceHZ());
  dest->sampleSize=getSampleSize();
  dest->lBigEndian=getBigEndian();
  dest->lSigned=getSigned();
}
