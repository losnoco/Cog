/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Mpeglayer2.cc
// It's for MPEG Layer 2

//changes 8/4/2002 (by Hauke Duden):
//	- added some explicit casts to remove compilation warnings
//	- added VC6 pragma to prevent some warnings from being reported
 
#include "mpegsound.h"
#include "synthesis.h"


#define BUGFIX
#include "mpeg2tables.h"


// workaround for buggy mpeg2 streams.
// tested with 12 monkey cdi, worgked fine.
// problem was: the stream produced ints
// with access out of the tables
// if we have such an access we set it to a zero entry
#ifdef BUGFIX
static int checkCodeRange(int code,const REAL* group) {
  int back=0;
  if (group == NULL) {
    //cout << "group null"<<endl;
    return 0;
  }
  back=code;

  if (group == group5bits) {
    if (back > 27*3) {
      // redirect to zero value
      back=3;
    }
    return back;
  }
  if (group == group7bits) {
    if (back > 125*3) {
      back=6;
    }
    return back;
  }  
  if (group == group10bits) {
    if (back > 729*3) {
      back=12;
    }
    return back;
  }
  //DEBUG_LAYER(cout << "unknown group found!"<<endl;)
  return -1;
}


#endif








// Mpeg layer 2
void Mpegtoraw::extractlayer2(void) {
  int inputstereo=mpegAudioHeader->getInputstereo();
  int tableindex=mpegAudioHeader->getTableindex();
  int subbandnumber=mpegAudioHeader->getSubbandnumber();
  int stereobound=mpegAudioHeader->getStereobound();

  REAL fraction[MAXCHANNEL][3][MAXSUBBAND];
  unsigned int bitalloc[MAXCHANNEL][MAXSUBBAND],
               scaleselector[MAXCHANNEL][MAXSUBBAND];
  REAL scalefactor[2][3][MAXSUBBAND];

  const REAL *group[MAXCHANNEL][MAXSUBBAND];
  unsigned int codelength[MAXCHANNEL][MAXSUBBAND];
  REAL factor[MAXCHANNEL][MAXSUBBAND];
  REAL c[MAXCHANNEL][MAXSUBBAND],d[MAXCHANNEL][MAXSUBBAND];

  int s=stereobound,n=subbandnumber;
  

  // Bitalloc
  {
    register int i;
    register const int *t=bitalloclengthtable[tableindex];
    for(i=0;i<s;i++,t++)
    {
      bitalloc[LS][i]=getbits(*t);
      bitalloc[RS][i]=getbits(*t);
    }
    for(;i<n;i++,t++) {
      bitalloc[LS][i]=bitalloc[RS][i]=getbits(*t);
    }
  }



  // Scale selector
  if(inputstereo)
    for(register int i=0;i<n;i++)
    {
      if(bitalloc[LS][i])scaleselector[LS][i]=getbits(2);
      if(bitalloc[RS][i])scaleselector[RS][i]=getbits(2);
    }
  else
    for(register int i=0;i<n;i++)
      if(bitalloc[LS][i])scaleselector[LS][i]=getbits(2);

  // Scale index
  {
    register int i,j;


    for(i=0;i<n;i++)
    {
      if((j=bitalloc[LS][i]))
      {
	if(!tableindex)
	{
	  group[LS][i]=grouptableA[j];
	  codelength[LS][i]=codelengthtableA[j];
	  factor[LS][i]=factortableA[j];
	  c[LS][i]=ctableA[j];
	  d[LS][i]=dtableA[j];
	}
	else
	{
	  if(i<=2)
	  {
	    group[LS][i]=grouptableB1[j];
	    codelength[LS][i]=codelengthtableB1[j];
	    factor[LS][i]=factortableB1[j];
	    c[LS][i]=ctableB1[j];
	    d[LS][i]=dtableB1[j];
	  }
	  else
	  {
	    group[LS][i]=grouptableB234[j];
	    if(i<=10)
	    {
	      codelength[LS][i]=codelengthtableB2[j];
	      factor[LS][i]=factortableB2[j];
	      c[LS][i]=ctableB2[j];
	      d[LS][i]=dtableB2[j];
	    }
	    else if(i<=22)
	    {
	      codelength[LS][i]=codelengthtableB3[j];
	      factor[LS][i]=factortableB3[j];
	      c[LS][i]=ctableB3[j];
	      d[LS][i]=dtableB3[j];
	    }
	    else
	    {
	      codelength[LS][i]=codelengthtableB4[j];
	      factor[LS][i]=factortableB4[j];
	      c[LS][i]=ctableB4[j];
	      d[LS][i]=dtableB4[j];
	    }
	  }
	}


	switch(scaleselector[LS][i])
	{
	  case 0:scalefactor[LS][0][i]=scalefactorstable[getbits(6)];
		 scalefactor[LS][1][i]=scalefactorstable[getbits(6)];
		 scalefactor[LS][2][i]=scalefactorstable[getbits(6)];
		 break;
	  case 1:scalefactor[LS][0][i]=
		 scalefactor[LS][1][i]=scalefactorstable[getbits(6)];
		 scalefactor[LS][2][i]=scalefactorstable[getbits(6)];
		 break;
	  case 2:scalefactor[LS][0][i]=
		 scalefactor[LS][1][i]=
		 scalefactor[LS][2][i]=scalefactorstable[getbits(6)];
		 break;
	  case 3:scalefactor[LS][0][i]=scalefactorstable[getbits(6)];
		 scalefactor[LS][1][i]=
		 scalefactor[LS][2][i]=scalefactorstable[getbits(6)];
		 break;
	default:
	  //cout << "scaleselector left default never happens"<<endl;
	  break;
	}
      }


      if(inputstereo && (j=bitalloc[RS][i]))
      {
	if(!tableindex)
	{
	  group[RS][i]=grouptableA[j];
	  codelength[RS][i]=codelengthtableA[j];
	  factor[RS][i]=factortableA[j];
	  c[RS][i]=ctableA[j];
	  d[RS][i]=dtableA[j];
	}
	else
	{
	  if(i<=2)
	  {
	    group[RS][i]=grouptableB1[j];
	    codelength[RS][i]=codelengthtableB1[j];
	    factor[RS][i]=factortableB1[j];
	    c[RS][i]=ctableB1[j];
	    d[RS][i]=dtableB1[j];
	  }
	  else
	  {
	    group[RS][i]=grouptableB234[j];
	    if(i<=10)
	    {
	      codelength[RS][i]=codelengthtableB2[j];
	      factor[RS][i]=factortableB2[j];
	      c[RS][i]=ctableB2[j];
	      d[RS][i]=dtableB2[j];
	    }
	    else if(i<=22)
	    {
	      codelength[RS][i]=codelengthtableB3[j];
	      factor[RS][i]=factortableB3[j];
	      c[RS][i]=ctableB3[j];
	      d[RS][i]=dtableB3[j];
	    }
	    else
	    {
	      codelength[RS][i]=codelengthtableB4[j];
	      factor[RS][i]=factortableB4[j];
	      c[RS][i]=ctableB4[j];
	      d[RS][i]=dtableB4[j];
	    }
	  }
	}

  
	switch(scaleselector[RS][i])
	{
	  case 0 : scalefactor[RS][0][i]=scalefactorstable[getbits(6)];
		   scalefactor[RS][1][i]=scalefactorstable[getbits(6)];
		   scalefactor[RS][2][i]=scalefactorstable[getbits(6)];
		   break;
	  case 1 : scalefactor[RS][0][i]=
		   scalefactor[RS][1][i]=scalefactorstable[getbits(6)];
		   scalefactor[RS][2][i]=scalefactorstable[getbits(6)];
		   break;
	  case 2 : scalefactor[RS][0][i]=
		   scalefactor[RS][1][i]=
		   scalefactor[RS][2][i]=scalefactorstable[getbits(6)];
		   break;
	  case 3 : scalefactor[RS][0][i]=scalefactorstable[getbits(6)];
		   scalefactor[RS][1][i]=
		   scalefactor[RS][2][i]=scalefactorstable[getbits(6)];
		   break;
	default:
	  //cout << "scaleselector right default never happens"<<endl;
	  break;
	}


      }
    }
  }



// Read Sample
  {
    register int i;

    for(int l=0;l<SCALEBLOCK;l++)
    {
      // Read Sample
      for(i=0;i<s;i++)
      {
	if(bitalloc[LS][i])
	{
	  if(group[LS][i])
	  {
	    register const REAL *s;
	    int code=getbits(codelength[LS][i]);



	    code+=code<<1;
#ifdef BUGFIX
	    // bugfix for bad streams
	    code=checkCodeRange(code,group[LS][i]);
	    if (code == -1) return;
#endif
	    s=group[LS][i]+code;

	    fraction[LS][0][i]=s[0];
	    fraction[LS][1][i]=s[1];
	    fraction[LS][2][i]=s[2];
	  }
	  else
	  {
	    fraction[LS][0][i]=(REAL)
	      (REAL(getbits(codelength[LS][i]))*factor[LS][i]-1.0);
	    fraction[LS][1][i]=(REAL)
	      (REAL(getbits(codelength[LS][i]))*factor[LS][i]-1.0);
	    fraction[LS][2][i]=(REAL)
	      (REAL(getbits(codelength[LS][i]))*factor[LS][i]-1.0);
	  }
	}
	else fraction[LS][0][i]=fraction[LS][1][i]=fraction[LS][2][i]=0.0;


	if(inputstereo && bitalloc[RS][i])
	{
	  if(group[RS][i])
	  {
	    const REAL *s;
	    int code=getbits(codelength[RS][i]);

	    code+=code<<1;
#ifdef BUGFIX
	    // bugfix for bad streams
	    code=checkCodeRange(code,group[RS][i]);
	    if (code == -1) return;
#endif
	    s=group[RS][i]+code;

	    fraction[RS][0][i]=s[0];
	    fraction[RS][1][i]=s[1];
	    fraction[RS][2][i]=s[2];
	  }
	  else
	  {
	    fraction[RS][0][i]=(REAL)
	      (REAL(getbits(codelength[RS][i]))*factor[RS][i]-1.0);
	    fraction[RS][1][i]=(REAL)
	      (REAL(getbits(codelength[RS][i]))*factor[RS][i]-1.0);
	    fraction[RS][2][i]=(REAL)
	      (REAL(getbits(codelength[RS][i]))*factor[RS][i]-1.0);
	  }
	}
	else fraction[RS][0][i]=fraction[RS][1][i]=fraction[RS][2][i]=0.0;
      }


     for(;i<n;i++)
      {
	if(bitalloc[LS][i])
	{
	  if(group[LS][i])
	  {
	    register const REAL *s;
	    int code=getbits(codelength[LS][i]);

	    code+=code<<1;
#ifdef BUGFIX
	    // bugfix for bad streams
	    code=checkCodeRange(code,group[LS][i]);
	    if (code == -1) return;
#endif
	    s=group[LS][i]+code;

	    fraction[LS][0][i]=fraction[RS][0][i]=s[0];
	    fraction[LS][1][i]=fraction[RS][1][i]=s[1];
	    fraction[LS][2][i]=fraction[RS][2][i]=s[2];
	  }
	  else
	  {
	    fraction[LS][0][i]=fraction[RS][0][i]=(REAL)
	      (REAL(getbits(codelength[LS][i]))*factor[LS][i]-1.0);
	    fraction[LS][1][i]=fraction[RS][1][i]=(REAL)
	      (REAL(getbits(codelength[LS][i]))*factor[LS][i]-1.0);
	    fraction[LS][2][i]=fraction[RS][2][i]=(REAL)
	      (REAL(getbits(codelength[LS][i]))*factor[LS][i]-1.0);
	  }
	}
	else fraction[LS][0][i]=fraction[LS][1][i]=fraction[LS][2][i]=
	     fraction[RS][0][i]=fraction[RS][1][i]=fraction[RS][2][i]=0.0;
      }




      //Fraction
      if(lOutputStereo)
	for(i=0;i<n;i++)
	{
	  if(bitalloc[LS][i])
	  {
	    if(!group[LS][i])
	    {
	      fraction[LS][0][i]=(fraction[LS][0][i]+d[LS][i])*c[LS][i];
	      fraction[LS][1][i]=(fraction[LS][1][i]+d[LS][i])*c[LS][i];
	      fraction[LS][2][i]=(fraction[LS][2][i]+d[LS][i])*c[LS][i];
	    }

	    register REAL t=scalefactor[LS][l>>2][i];
	    fraction[LS][0][i]*=t;
	    fraction[LS][1][i]*=t;
	    fraction[LS][2][i]*=t;
	  }

	  if(bitalloc[RS][i])
	  {
	    if(!group[RS][i])
	    {
	      fraction[RS][0][i]=(fraction[RS][0][i]+d[RS][i])*c[LS][i];
	      fraction[RS][1][i]=(fraction[RS][1][i]+d[RS][i])*c[LS][i];
	      fraction[RS][2][i]=(fraction[RS][2][i]+d[RS][i])*c[LS][i];
	    }

	    register REAL t=scalefactor[RS][l>>2][i];
	    fraction[RS][0][i]*=t;
	    fraction[RS][1][i]*=t;
	    fraction[RS][2][i]*=t;
	  }
	}
      else
	for(i=0;i<n;i++)
	  if(bitalloc[LS][i])
	  {
	    if(!group[LS][i])
	    {
	      fraction[LS][0][i]=(fraction[LS][0][i]+d[LS][i])*c[LS][i];
	      fraction[LS][1][i]=(fraction[LS][1][i]+d[LS][i])*c[LS][i];
	      fraction[LS][2][i]=(fraction[LS][2][i]+d[LS][i])*c[LS][i];
	    }

	    register REAL t=scalefactor[LS][l>>2][i];
	    fraction[LS][0][i]*=t;
	    fraction[LS][1][i]*=t;
	    fraction[LS][2][i]*=t;
	  }


      for(;i<MAXSUBBAND;i++)
	fraction[LS][0][i]=fraction[LS][1][i]=fraction[LS][2][i]=
	fraction[RS][0][i]=fraction[RS][1][i]=fraction[RS][2][i]=0.0;

      for(i=0;i<3;i++) {
	synthesis->doSynth(lDownSample,lOutputStereo,
			   fraction[LS][i],fraction[RS][i]);
      }

    }
  }
}
