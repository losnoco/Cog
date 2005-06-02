/*
  wrapper for dcts
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */

//changes 8/4/2002 (by Hauke Duden):
//	- added some explicit casts to remove compilation warnings

#include "common.h"
#include "dct.h"


ATTR_ALIGN(64) static REAL hsec_12[3];
ATTR_ALIGN(64) static REAL cos2_6=(REAL)cos(PI/6.0*2.0);
ATTR_ALIGN(64) static REAL cos1_6=(REAL)cos(PI/6.0*1.0);
ATTR_ALIGN(64) static REAL hsec_36[9];
ATTR_ALIGN(64) static REAL cos_18[9];


/**
   This was some time ago a standalone dct class,
   but to get more speed I made it an inline dct 
   int the filter classes
*/

static int dct36_12Init=false;

void initialize_dct12_dct36() {
  if (dct36_12Init) {
    return;
  }
  dct36_12Init=true;


  int i;

  for(i=0;i<3;i++)
    hsec_12[i]=(REAL)(0.5/cos(double(i*2+1)* PI_12));

  for(i=0;i<9;i++)
    hsec_36[i]=(REAL)(0.5/cos(double(i*2+1)* PI_36));

  for(i=0;i<9;i++)
    cos_18[i]=(REAL)(cos(PI_18*double(i)));

}


void dct36(REAL *inbuf,REAL *prevblk1,
		  REAL *prevblk2,REAL *wi,REAL *out){

#define MACRO0(v) {                                 \
    REAL tmp;                                       \
    out2[9+(v)]=(tmp=sum0+sum1)*wi[27+(v)];         \
    out2[8-(v)]=tmp * wi[26-(v)];  }                \
    sum0-=sum1;                                     \
    ts[SBLIMIT*(8-(v))]=out1[8-(v)]+sum0*wi[8-(v)]; \
    ts[SBLIMIT*(9+(v))]=out1[9+(v)]+sum0*wi[9+(v)]; 
#define MACRO1(v) { \
    REAL sum0,sum1; \
    sum0=tmp1a+tmp2a; \
    sum1=(tmp1b+tmp2b)*hsec_36[(v)]; \
    MACRO0(v); }
#define MACRO2(v) {                    \
    REAL sum0,sum1;                    \
    sum0=tmp2a-tmp1a;                  \
    sum1=(tmp2b-tmp1b) * hsec_36[(v)]; \
    MACRO0(v); }

  {
    REAL *in = inbuf;

    in[17]+=in[16];in[16]+=in[15];in[15]+=in[14];in[14]+=in[13]; 
    in[13]+=in[12];in[12]+=in[11];in[11]+=in[10];in[10]+=in[ 9];
    in[ 9]+=in[ 8];in[ 8]+=in[ 7];in[ 7]+=in[ 6];in[ 6]+=in[ 5];
    in[ 5]+=in[ 4];in[ 4]+=in[ 3];in[ 3]+=in[ 2];in[ 2]+=in[ 1];
    in[ 1]+=in[ 0];


    in[17]+=in[15];in[15]+=in[13];in[13]+=in[11];in[11]+=in[ 9];
    in[ 9]+=in[ 7];in[7] +=in[ 5];in[ 5]+=in[ 3];in[ 3]+=in[ 1];

    {
      REAL *c = cos_18;
      REAL *out2 = prevblk2;
      REAL *out1 = prevblk1;
      REAL *ts = out;
      
      REAL ta33,ta66,tb33,tb66;

      ta33=in[2*3+0]*c[3];
      ta66=in[2*6+0]*c[6];
      tb33=in[2*3+1]*c[3];
      tb66=in[2*6+1]*c[6];

      { 
	REAL tmp1a,tmp2a,tmp1b,tmp2b;
	tmp1a=          in[2*1+0]*c[1]+ta33          +in[2*5+0]*c[5]+in[2*7+0]*c[7];
	tmp1b=          in[2*1+1]*c[1]+tb33          +in[2*5+1]*c[5]+in[2*7+1]*c[7];
	tmp2a=in[2*0+0]+in[2*2+0]*c[2]+in[2*4+0]*c[4]+ta66          +in[2*8+0]*c[8];
	tmp2b=in[2*0+1]+in[2*2+1]*c[2]+in[2*4+1]*c[4]+tb66          +in[2*8+1]*c[8];
	MACRO1(0);
	MACRO2(8);
      }

      {
	REAL tmp1a,tmp2a,tmp1b,tmp2b;
	tmp1a=(in[2*1+0]-in[2*5+0]-in[2*7+0])*c[3];
	tmp1b=(in[2*1+1]-in[2*5+1]-in[2*7+1])*c[3];
	tmp2a=(in[2*2+0]-in[2*4+0]-in[2*8+0])*c[6]-in[2*6+0]+in[2*0+0];
	tmp2b=(in[2*2+1]-in[2*4+1]-in[2*8+1])*c[6]-in[2*6+1]+in[2*0+1];
	MACRO1(1);
	MACRO2(7);
      }

      {
	REAL tmp1a,tmp2a,tmp1b,tmp2b;
	tmp1a=          in[2*1+0]*c[5]-ta33          -in[2*5+0]*c[7]+in[2*7+0]*c[1];
	tmp1b=          in[2*1+1]*c[5]-tb33          -in[2*5+1]*c[7]+in[2*7+1]*c[1];
	tmp2a=in[2*0+0]-in[2*2+0]*c[8]-in[2*4+0]*c[2]+ta66          +in[2*8+0]*c[4];
	tmp2b=in[2*0+1]-in[2*2+1]*c[8]-in[2*4+1]*c[2]+tb66          +in[2*8+1]*c[4];
	MACRO1(2);
	MACRO2(6);
      }

      {
	REAL tmp1a,tmp2a,tmp1b,tmp2b;
	tmp1a=          in[2*1+0]*c[7]-ta33          +in[2*5+0]*c[1]-in[2*7+0]*c[5];
	tmp1b=          in[2*1+1]*c[7]-tb33          +in[2*5+1]*c[1]-in[2*7+1]*c[5];
	tmp2a=in[2*0+0]-in[2*2+0]*c[4]+in[2*4+0]*c[8]+ta66          -in[2*8+0]*c[2];
	tmp2b=in[2*0+1]-in[2*2+1]*c[4]+in[2*4+1]*c[8]+tb66          -in[2*8+1]*c[2];
	MACRO1(3);
	MACRO2(5);
      }

      {
        REAL sum0,sum1;
    	sum0= in[2*0+0]-in[2*2+0]+in[2*4+0]-in[2*6+0]+in[2*8+0];
    	sum1=(in[2*0+1]-in[2*2+1]+in[2*4+1]-in[2*6+1]+in[2*8+1])*hsec_36[4];
	MACRO0(4);
      }
    }
  }


}


void dct12(REAL *in,REAL *prevblk1,REAL *prevblk2,REAL *wi,REAL *out) {

#define DCT12_PART1   \
        in5=in[5*3];  \
  in5+=(in4=in[4*3]); \
  in4+=(in3=in[3*3]); \
  in3+=(in2=in[2*3]); \
  in2+=(in1=in[1*3]); \
  in1+=(in0=in[0*3]); \
                      \
  in5+=in3;in3+=in1;  \
                      \
  in2*=cos1_6;        \
  in3*=cos1_6;

#define DCT12_PART2         \
  in0+=in4*cos2_6;          \
                            \
  in4=in0+in2;              \
  in0-=in2;                 \
                            \
  in1+=in5*cos2_6;          \
                            \
  in5=(in1+in3)*hsec_12[0]; \
  in1=(in1-in3)*hsec_12[2]; \
                            \
  in3=in4+in5;              \
  in4-=in5;                 \
                            \
  in2=in0+in1;              \
  in0-=in1;

  {
    REAL in0,in1,in2,in3,in4,in5;
    register REAL *pb1=prevblk1;
    out[SBLIMIT*0]=pb1[0];out[SBLIMIT*1]=pb1[1];out[SBLIMIT*2]=pb1[2];
    out[SBLIMIT*3]=pb1[3];out[SBLIMIT*4]=pb1[4];out[SBLIMIT*5]=pb1[5];
 
    DCT12_PART1;
    
    {
      REAL tmp0,tmp1=(in0-in4);
      {
	register REAL tmp2=(in1-in5)*hsec_12[1];
	tmp0=tmp1+tmp2;
	tmp1-=tmp2;
      }
      out[(17-1)*SBLIMIT]=pb1[17-1]+tmp0*wi[11-1];
      out[(12+1)*SBLIMIT]=pb1[12+1]+tmp0*wi[ 6+1];
      out[(6 +1)*SBLIMIT]=pb1[6 +1]+tmp1*wi[ 1  ];
      out[(11-1)*SBLIMIT]=pb1[11-1]+tmp1*wi[ 5-1];
    }

    DCT12_PART2;
    out[(17-0)*SBLIMIT]=pb1[17-0]+in2*wi[11-0];
    out[(12+0)*SBLIMIT]=pb1[12+0]+in2*wi[ 6+0];
    out[(12+2)*SBLIMIT]=pb1[12+2]+in3*wi[ 6+2];
    out[(17-2)*SBLIMIT]=pb1[17-2]+in3*wi[11-2];

    out[( 6+0)*SBLIMIT]=pb1[ 6+0]+in0*wi[0];
    out[(11-0)*SBLIMIT]=pb1[11-0]+in0*wi[5-0];
    out[( 6+2)*SBLIMIT]=pb1[ 6+2]+in4*wi[2];
    out[(11-2)*SBLIMIT]=pb1[11-2]+in4*wi[5-2];
  }

  in++;
  {
    REAL in0,in1,in2,in3,in4,in5;
    register REAL *pb2 = prevblk2;
 
    DCT12_PART1;

    {
      REAL tmp0,tmp1=(in0-in4);
      {
	REAL tmp2=(in1-in5)*hsec_12[1];
	tmp0=tmp1+tmp2;
	tmp1-=tmp2;
      }
      pb2[5-1]=tmp0*wi[11-1];
      pb2[0+1]=tmp0*wi[6+1];
      out[(12+1)*SBLIMIT]+=tmp1*wi[1];
      out[(17-1)*SBLIMIT]+=tmp1*wi[5-1];
    }

    DCT12_PART2;

    pb2[5-0]=in2*wi[11-0];
    pb2[0+0]=in2*wi[6+0];
    pb2[0+2]=in3*wi[6+2];
    pb2[5-2]=in3*wi[11-2];

    out[(12+0)*SBLIMIT]+=in0*wi[0];
    out[(17-0)*SBLIMIT]+=in0*wi[5-0];
    out[(12+2)*SBLIMIT]+=in4*wi[2];
    out[(17-2)*SBLIMIT]+=in4*wi[5-2];
  }

  in++; 
  {
    REAL in0,in1,in2,in3,in4,in5;
    register REAL *pb2 = prevblk2;
    pb2[12]=pb2[13]=pb2[14]=pb2[15]=pb2[16]=pb2[17]=0.0;

    DCT12_PART1;

    {
      REAL tmp0,tmp1=(in0-in4);
      {
	REAL tmp2=(in1-in5)*hsec_12[1];
	tmp0=tmp1+tmp2;
	tmp1-=tmp2;
      }
      pb2[11-1]=tmp0*wi[11-1];
      pb2[ 6+1]=tmp0*wi[6+1];
      pb2[ 0+1]+=tmp1*wi[1];
      pb2[ 5-1]+=tmp1*wi[5-1];
    }

    DCT12_PART2;
    pb2[11-0]=in2*wi[11-0];
    pb2[ 6+0]=in2*wi[ 6+0];
    pb2[ 6+2]=in3*wi[ 6+2];
    pb2[11-2]=in3*wi[11-2];

    pb2[ 0+0]+=in0*wi[0  ];
    pb2[ 5-0]+=in0*wi[5-0];
    pb2[ 0+2]+=in4*wi[2  ];
    pb2[ 5-2]+=in4*wi[5-2];
  }


}


