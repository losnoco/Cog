/*
  base class for frames
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */



#include "frame.h"


Frame::Frame() {
  type=_FRAME_UNK;
}


Frame::~Frame() {
}


const char* Frame::getMajorFrameName(int type) {
  int majorID=type >> 12;
  switch(majorID) {
  case _FRAME_UNK:
    return "_FRAME_UNK";
  case _FRAME_RAW:
    return "_FRAME_RAW";
  case _FRAME_AUDIO:
    return "_FRAME_AUDIO";
  case _FRAME_VIDEO:
    return "_FRAME_VIDEO";
  case _FRAME_PAKET:
    return "_FRAME_PAKET";
  default:
    return "unknown major frameType";
  }
  return "never happens Frame::getMajorFrameName";
}

    

const char* Frame::getFrameName(int type) {
  switch(type) {
    // Raw
  case _FRAME_RAW_BASE:
    return "_FRAME_RAW_BASE";
  case _FRAME_RAW_OGG:
    return "_FRAME_RAW_OGG";


    // Audio
  case _FRAME_AUDIO_BASE:
    return "_FRAME_AUDIO_BASE";
  case _FRAME_AUDIO_PCM:
    return "_FRAME_AUDIO_PCM";
  case _FRAME_AUDIO_FLOAT:
    return "_FRAME_AUDIO_FLOAT";



    // Rest
  default:
    return "cannot find name";
  }
  return "never happens Frame::getFrameName";
}

