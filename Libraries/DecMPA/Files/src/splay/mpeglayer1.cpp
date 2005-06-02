/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Mpeglayer1.cc
// It's for MPEG Layer 1

//changes 8/4/2002 (by Hauke Duden):
//	- added VC6 pragma to prevent some warnings from being reported

#include "mpegsound.h"
#include "synthesis.h"

#ifdef _MSC_VER
#pragma warning(disable : 4305)
#endif

// Tables for layer 1
static const REAL factortable[15] = 
{
  0.0, 
  (1.0/2.0)    * (4.0/3.0),         (1.0/4.0)     * (8.0/7.0),
  (1.0/8.0)    * (16.0/15.0),       (1.0/16.0)    * (32.0/31.0),
  (1.0/32.0)   * (64.0/63.0),       (1.0/64.0)    * (128.0/127.0),
  (1.0/128.0)  * (256.0/255.0),     (1.0/256.0)   * (512.0/511.0),
  (1.0/512.0)  * (1024.0/1023.0),   (1.0/1024.0)  * (2048.0/2047.0),
  (1.0/2048.0) * (4096.0/4095.0),   (1.0/4096.0)  * (8192.0/8191.0),
  (1.0/8192.0) * (16384.0/16383.0), (1.0/16384.0) * (32768.0/32767.0)
};

static const REAL offsettable[15] = 
{
  0.0, 
  ((1.0/2.0)-1.0)    * (4.0/3.0),         ((1.0/4.0)-1.0)     * (8.0/7.0), 
  ((1.0/8.0)-1.0)    * (16.0/15.0),       ((1.0/16.0)-1.0)    * (32.0/31.0),
  ((1.0/32.0)-1.0)   * (64.0/63.0),       ((1.0/64.0)-1.0)    * (128.0/127.0),
  ((1.0/128.0)-1.0)  * (256.0/255.0),     ((1.0/256.0)-1.0)   * (512.0/511.0),
  ((1.0/512.0)-1.0)  * (1024.0/1023.0),   ((1.0/1024.0)-1.0)  * (2048.0/2047.0),
  ((1.0/2048.0)-1.0) * (4096.0/4095.0),   ((1.0/4096.0)-1.0)  * (8192.0/8191.0),
  ((1.0/8192.0)-1.0) * (16384.0/16383.0), ((1.0/16384.0)-1.0) * (32768.0/32767.0)
};

#ifdef _MSC_VER
#pragma warning(default : 4305)
#endif

// Mpeg layer 1
void Mpegtoraw::extractlayer1(void)
{
  int inputstereo=mpegAudioHeader->getInputstereo();
  int stereobound=mpegAudioHeader->getStereobound();
  REAL fraction[MAXCHANNEL][MAXSUBBAND];
  REAL scalefactor[MAXCHANNEL][MAXSUBBAND];

  int bitalloc[MAXCHANNEL][MAXSUBBAND],
      sample[MAXCHANNEL][MAXSUBBAND];

  register int i,j;
  int s=stereobound,l;


// Bitalloc
  for(i=0;i<s;i++)
  {
    bitalloc[LS][i]=getbits(4);
    bitalloc[RS][i]=getbits(4);
  }
  for(;i<MAXSUBBAND;i++)
    bitalloc[LS][i]=
    bitalloc[RS][i]=getbits(4);

// Scale index
  if(inputstereo)
    for(i=0;i<MAXSUBBAND;i++)
    {
      if(bitalloc[LS][i])scalefactor[LS][i]=scalefactorstable[getbits(6)];
      if(bitalloc[RS][i])scalefactor[RS][i]=scalefactorstable[getbits(6)];
    }
  else
    for(i=0;i<MAXSUBBAND;i++)
      if(bitalloc[LS][i])scalefactor[LS][i]=scalefactorstable[getbits(6)];

  for(l=0;l<SCALEBLOCK;l++)
  {
    // Sample
    for(i=0;i<s;i++)
    {
      if((j=bitalloc[LS][i]))sample[LS][i]=getbits(j+1);
      if((j=bitalloc[RS][i]))sample[RS][i]=getbits(j+1);
    }
    for(;i<MAXSUBBAND;i++)
      if((j=bitalloc[LS][i]))sample[LS][i]=sample[RS][i]=getbits(j+1);


    // Fraction
    if(lOutputStereo)
      for(i=0;i<MAXSUBBAND;i++)
      {
	if((j=bitalloc[LS][i]))
	  fraction[LS][i]=(REAL(sample[LS][i])*factortable[j]+offsettable[j])
			  *scalefactor[LS][i];
	else fraction[LS][i]=0.0;
	if((j=bitalloc[RS][i]))
	  fraction[RS][i]=(REAL(sample[RS][i])*factortable[j]+offsettable[j])
			  *scalefactor[RS][i];
	else fraction[RS][i]=0.0;
      }
    else
      for(i=0;i<MAXSUBBAND;i++)
	if((j=bitalloc[LS][i]))
	  fraction[LS][i]=(REAL(sample[LS][i])*factortable[j]+offsettable[j])
			  *scalefactor[LS][i];
	else fraction[LS][i]=0.0;

    synthesis->doSynth(lDownSample,lOutputStereo,
		       fraction[LS],fraction[RS]);
  }
}
