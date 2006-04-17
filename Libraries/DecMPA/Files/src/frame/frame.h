/*
  base class for frames
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */

//changes 8/11/2002 (by Hauke Duden):
//	- removed unnecessary includes

#ifndef __FRAME_H
#define __FRAME_H


//#include <iostream.h>
//#include <stdio.h>
//#include <limits.h>
//#include <stdlib.h>
//#include <string.h>



/**
   The base class for frames. Every derived class from this class
   must belong to some "major" class type and it must have an unique
   id for itsself. Even if it is a base class it must have a unique id.

   How does this work. We have an int for the Frame id. In the int
   itsself we but the majorid as well.
   The Start codes are all multiple of 2 so for example
   0..127   belongs to FRAME UNK
   128..255 belongs to FRAME RAW

   So we can with a simple shift operation find out the major class
*/
#define _FRAME_SHIFT         7 
#define _FRAME_ID_MAX        128            //(2^_FRAME_SHIFT)        


// Major Frame classes
#define _FRAME_UNK           0
#define _FRAME_RAW           1
#define _FRAME_AUDIO         2
#define _FRAME_VIDEO         3
#define _FRAME_PAKET         4

// start ids of minor classes

#define _FRAME_UNK_START     (0)           
#define _FRAME_RAW_START     (_FRAME_ID_MAX)         
#define _FRAME_AUDIO_START   (_FRAME_ID_MAX*2)       
#define _FRAME_VIDEO_START   (_FRAME_ID_MAX*2*2)     
#define _FRAME_PAKET_START   (_FRAME_ID_MAX*2*2*2)   

// Minor Frame type IDs


// Raw
#define _FRAME_RAW_BASE                          (_FRAME_RAW_START+1)
#define _FRAME_RAW_OGG                           (_FRAME_RAW_START+2)
#define _FRAME_RAW_MPEG_I_VIDEO                  (_FRAME_RAW_START+3)
#define _FRAME_RAW_MPEG_SYSTEM                   (_FRAME_RAW_START+4)


// Audio:
#define _FRAME_AUDIO_BASE                        (_FRAME_AUDIO_START+1)
#define _FRAME_AUDIO_PCM                         (_FRAME_AUDIO_START+2)
#define _FRAME_AUDIO_FLOAT                       (_FRAME_AUDIO_START+3)

// Video:
#define _FRAME_VIDEO_BASE                        (_FRAME_VIDEO_START+1)
#define _FRAME_VIDEO_YUV                         (_FRAME_VIDEO_START+2)
#define _FRAME_VIDEO_RGB_8                       (_FRAME_VIDEO_START+3)
#define _FRAME_VIDEO_RGB_16                      (_FRAME_VIDEO_START+4)
#define _FRAME_VIDEO_RGB_32                      (_FRAME_VIDEO_START+5)

// Packet:
#define _FRAME_PACKET_SYNC                       (_FRAME_PAKET_START+1)
#define _FRAME_PACKET_PACKET_CONTAINER           (_FRAME_PAKET_START+2)




class Frame {
  int type;
  
 public:
  Frame();
  ~Frame();
  inline int  getMajorFrameType()              { return (type>>_FRAME_SHIFT);}
  inline int  getFrameType()                   { return type;      }
  inline void setFrameType(int type)           { this->type=type;  }


  static const char* getMajorFrameName(int type);
  static const char* getFrameName(int type);
};
#endif
