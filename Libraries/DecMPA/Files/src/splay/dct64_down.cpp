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


ATTR_ALIGN(64) static REAL hcos_64_down[16];
ATTR_ALIGN(64) static REAL hcos_32_down[8];
ATTR_ALIGN(64) static REAL hcos_16_down[4];
ATTR_ALIGN(64) static REAL hcos_8_down[2];
ATTR_ALIGN(64) static REAL hcos_4_down;

/**
   This was some time ago a standalone dct class,
   but to get more speed I made it an inline dct 
   int the filter classes
*/

static int dctInit=false;

void initialize_dct64_downsample() {
  if (dctInit) {
    return;
  }
  dctInit=true;

  int i;

  for(i=0;i<16;i++) {
    hcos_64_down[i]=(REAL)(1.0/(2.0*cos(MY_PI*double(i*2+1)/64.0)));
  }
  for(i=0;i< 8;i++) {
    hcos_32_down[i]=(REAL)(1.0/(2.0*cos(MY_PI*double(i*2+1)/32.0)));
  }
  for(i=0;i< 4;i++) {
    hcos_16_down[i]=(REAL)(1.0/(2.0*cos(MY_PI*double(i*2+1)/16.0)));
  }
  for(i=0;i< 2;i++) {
    hcos_8_down[i]=(REAL)(1.0/(2.0*cos(MY_PI*double(i*2+1)/ 8.0)));
  }
  hcos_4_down=(REAL)(1.0/(2.0*cos(MY_PI*1.0/4.0)));

}

void dct64_downsample(REAL* out1,REAL* out2,REAL *fraction) {
  REAL p0,p1,p2,p3,p4,p5,p6,p7,p8,p9,pa,pb,pc,pd,pe,pf;
  REAL q0,q1,q2,q3,q4,q5,q6,q7,q8,q9,qa,qb,qc,qd,qe,qf;

#define OUT1(v,t) out1[(32-(v))*16]   =(-(out1[(v)*16]=t))
#define OUT2(v)   out2[(96-(v)-32)*16]=out2[((v)-32)*16]

  // compute new values via a fast cosine transform:
  /*  {
    register REAL *x=fraction;

    p0=x[ 0]+x[31];p1=x[ 1]+x[30];p2=x[ 2]+x[29];p3=x[ 3]+x[28];
    p4=x[ 4]+x[27];p5=x[ 5]+x[26];p6=x[ 6]+x[25];p7=x[ 7]+x[24];
    p8=x[ 8]+x[23];p9=x[ 9]+x[22];pa=x[10]+x[21];pb=x[11]+x[20];
    pc=x[12]+x[19];pd=x[13]+x[18];pe=x[14]+x[17];pf=x[15]+x[16];
  }

  q0=p0+pf;q1=p1+pe;q2=p2+pd;q3=p3+pc;
  q4=p4+pb;q5=p5+pa;q6=p6+p9;q7=p7+p8;
  q8=hcos_32_down[0]*(p0-pf);q9=hcos_32_down[1]*(p1-pe);
  qa=hcos_32_down[2]*(p2-pd);qb=hcos_32_down[3]*(p3-pc);
  qc=hcos_32_down[4]*(p4-pb);qd=hcos_32_down[5]*(p5-pa);
  qe=hcos_32_down[6]*(p6-p9);qf=hcos_32_down[7]*(p7-p8); */

  {

    register REAL *x=fraction;

    q0=x[ 0]+x[15];q1=x[ 1]+x[14];q2=x[ 2]+x[13];q3=x[ 3]+x[12];
    q4=x[ 4]+x[11];q5=x[ 5]+x[10];q6=x[ 6]+x[ 9];q7=x[ 7]+x[ 8];

    q8=hcos_32_down[0]*(x[ 0]-x[15]);q9=hcos_32_down[1]*(x[ 1]-x[14]);
    qa=hcos_32_down[2]*(x[ 2]-x[13]);qb=hcos_32_down[3]*(x[ 3]-x[12]);
    qc=hcos_32_down[4]*(x[ 4]-x[11]);qd=hcos_32_down[5]*(x[ 5]-x[10]);
    qe=hcos_32_down[6]*(x[ 6]-x[ 9]);qf=hcos_32_down[7]*(x[ 7]-x[ 8]);
  }

  p0=q0+q7;p1=q1+q6;p2=q2+q5;p3=q3+q4;
  p4=hcos_16_down[0]*(q0-q7);p5=hcos_16_down[1]*(q1-q6);
  p6=hcos_16_down[2]*(q2-q5);p7=hcos_16_down[3]*(q3-q4);
  p8=q8+qf;p9=q9+qe;pa=qa+qd;pb=qb+qc;
  pc=hcos_16_down[0]*(q8-qf);pd=hcos_16_down[1]*(q9-qe);
  pe=hcos_16_down[2]*(qa-qd);pf=hcos_16_down[3]*(qb-qc);

  q0=p0+p3;q1=p1+p2;q2=hcos_8_down[0]*(p0-p3);q3=hcos_8_down[1]*(p1-p2);
  q4=p4+p7;q5=p5+p6;q6=hcos_8_down[0]*(p4-p7);q7=hcos_8_down[1]*(p5-p6);
  q8=p8+pb;q9=p9+pa;qa=hcos_8_down[0]*(p8-pb);qb=hcos_8_down[1]*(p9-pa);
  qc=pc+pf;qd=pd+pe;qe=hcos_8_down[0]*(pc-pf);qf=hcos_8_down[1]*(pd-pe);

  p0=q0+q1;p1=hcos_4_down*(q0-q1);p2=q2+q3;p3=hcos_4_down*(q2-q3);
  p4=q4+q5;p5=hcos_4_down*(q4-q5);p6=q6+q7;p7=hcos_4_down*(q6-q7);
  p8=q8+q9;p9=hcos_4_down*(q8-q9);pa=qa+qb;pb=hcos_4_down*(qa-qb);
  pc=qc+qd;pd=hcos_4_down*(qc-qd);pe=qe+qf;pf=hcos_4_down*(qe-qf);

  {
    register REAL tmp;

    tmp=p6+p7;
    OUT2(36)=-(p5+tmp);
    OUT2(44)=-(p4+tmp);
    tmp=pb+pf;
    OUT1(10,tmp);
    OUT1(6,pd+tmp);
    tmp=pe+pf;
    OUT2(46)=-(p8+pc+tmp);
    OUT2(34)=-(p9+pd+tmp);
    tmp+=pa+pb;
    OUT2(38)=-(pd+tmp);
    OUT2(42)=-(pc+tmp);
    OUT1(2,p9+pd+pf);
    OUT1(4,p5+p7);
    OUT2(48)=-p0;
    out2[0]=-(out1[0]=p1);
    OUT1( 8,p3);
    OUT1(12,p7);
    OUT1(14,pf);
    OUT2(40)=-(p2+p3);
  }

  {
    register REAL *x=fraction;

    /*    p0=hcos_64_down[ 0]*(x[ 0]-x[31]);p1=hcos_64_down[ 1]*(x[ 1]-x[30]);
    p2=hcos_64_down[ 2]*(x[ 2]-x[29]);p3=hcos_64_down[ 3]*(x[ 3]-x[28]);
    p4=hcos_64_down[ 4]*(x[ 4]-x[27]);p5=hcos_64_down[ 5]*(x[ 5]-x[26]);
    p6=hcos_64_down[ 6]*(x[ 6]-x[25]);p7=hcos_64_down[ 7]*(x[ 7]-x[24]);
    p8=hcos_64_down[ 8]*(x[ 8]-x[23]);p9=hcos_64_down[ 9]*(x[ 9]-x[22]);
    pa=hcos_64_down[10]*(x[10]-x[21]);pb=hcos_64_down[11]*(x[11]-x[20]);
    pc=hcos_64_down[12]*(x[12]-x[19]);pd=hcos_64_down[13]*(x[13]-x[18]);
    pe=hcos_64_down[14]*(x[14]-x[17]);pf=hcos_64_down[15]*(x[15]-x[16]); */

    p0=hcos_64_down[ 0]*x[ 0];p1=hcos_64_down[ 1]*x[ 1];
    p2=hcos_64_down[ 2]*x[ 2];p3=hcos_64_down[ 3]*x[ 3];
    p4=hcos_64_down[ 4]*x[ 4];p5=hcos_64_down[ 5]*x[ 5];
    p6=hcos_64_down[ 6]*x[ 6];p7=hcos_64_down[ 7]*x[ 7];
    p8=hcos_64_down[ 8]*x[ 8];p9=hcos_64_down[ 9]*x[ 9];
    pa=hcos_64_down[10]*x[10];pb=hcos_64_down[11]*x[11];
    pc=hcos_64_down[12]*x[12];pd=hcos_64_down[13]*x[13];
    pe=hcos_64_down[14]*x[14];pf=hcos_64_down[15]*x[15];
  }

  q0=p0+pf;q1=p1+pe;q2=p2+pd;q3=p3+pc;
  q4=p4+pb;q5=p5+pa;q6=p6+p9;q7=p7+p8;
  q8=hcos_32_down[0]*(p0-pf);q9=hcos_32_down[1]*(p1-pe);
  qa=hcos_32_down[2]*(p2-pd);qb=hcos_32_down[3]*(p3-pc);
  qc=hcos_32_down[4]*(p4-pb);qd=hcos_32_down[5]*(p5-pa);
  qe=hcos_32_down[6]*(p6-p9);qf=hcos_32_down[7]*(p7-p8);

  p0=q0+q7;p1=q1+q6;p2=q2+q5;p3=q3+q4;
  p4=hcos_16_down[0]*(q0-q7);p5=hcos_16_down[1]*(q1-q6);
  p6=hcos_16_down[2]*(q2-q5);p7=hcos_16_down[3]*(q3-q4);
  p8=q8+qf;p9=q9+qe;pa=qa+qd;pb=qb+qc;
  pc=hcos_16_down[0]*(q8-qf);pd=hcos_16_down[1]*(q9-qe);
  pe=hcos_16_down[2]*(qa-qd);pf=hcos_16_down[3]*(qb-qc);

  q0=p0+p3;q1=p1+p2;q2=hcos_8_down[0]*(p0-p3);q3=hcos_8_down[1]*(p1-p2);
  q4=p4+p7;q5=p5+p6;q6=hcos_8_down[0]*(p4-p7);q7=hcos_8_down[1]*(p5-p6);
  q8=p8+pb;q9=p9+pa;qa=hcos_8_down[0]*(p8-pb);qb=hcos_8_down[1]*(p9-pa);
  qc=pc+pf;qd=pd+pe;qe=hcos_8_down[0]*(pc-pf);qf=hcos_8_down[1]*(pd-pe);

  p0=q0+q1;p1=hcos_4_down*(q0-q1);
  p2=q2+q3;p3=hcos_4_down*(q2-q3);
  p4=q4+q5;p5=hcos_4_down*(q4-q5);
  p6=q6+q7;p7=hcos_4_down*(q6-q7);
  p8=q8+q9;p9=hcos_4_down*(q8-q9);
  pa=qa+qb;pb=hcos_4_down*(qa-qb);
  pc=qc+qd;pd=hcos_4_down*(qc-qd);
  pe=qe+qf;pf=hcos_4_down*(qe-qf);

  {
    REAL tmp;

    tmp=pd+pf;
    OUT1(5,p5+p7+pb+tmp);
    tmp+=p9;
    OUT1(1,p1+tmp);
    OUT2(33)=-(p1+pe+tmp);
    tmp+=p5+p7;
    OUT1(3,tmp);
    OUT2(35)=-(p6+pe+tmp);
    tmp=pa+pb+pc+pd+pe+pf;
    OUT2(39)=-(p2+p3+tmp-pc);
    OUT2(43)=-(p4+p6+p7+tmp-pd);
    OUT2(37)=-(p5+p6+p7+tmp-pc);
    OUT2(41)=-(p2+p3+tmp-pd);
    tmp=p8+pc+pe+pf;
    OUT2(47)=-(p0+tmp);
    OUT2(45)=-(p4+p6+p7+tmp);
    tmp=pb+pf;
    OUT1(11,p7+tmp);
    tmp+=p3;
    OUT1( 9,tmp);
    OUT1( 7,pd+tmp);
    OUT1(13,p7+pf);
    OUT1(15,pf);
  }
}

