/*
  unrolled operations, for better Pentium FPU scheduling
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */



#ifndef __OP_H
#define __OP_H

/**
   The Pentium has two pipelined FPUs which makes it possible
   to do two operations in one cycle.
   (If you are lucky)

*/

#define PTR_DIST (1024)

#define OS r1=vp1[0] * dp[0]; \
           r2=vp1[PTR_DIST-0] * dp[0]; \
           dp++;

#define XX1 vp1+=15;dp++;

#define XX2 r1+=vp1[0] * dp[-1]; \
	    r2+=vp1[PTR_DIST-0] * dp[-1];

#define OP_END(val) vp1-=val;dp+=val;

#define OP_END_1(vVal,dVal) vp1+=(vVal-dVal),dp+=dVal


// this is OP_END(x);XX1; together:
#define OP_END_2(vVal)      vp1+=(15-vVal),dp+=vVal+1


// check this to test pipelining

#define SCHEDULE1(op,r1,r2)       r1;op;r2;
#define SCHEDULE2(op,r1,r2)       op;r1;r2;

#define SCHEDULE(a,b,c)    SCHEDULE2(a,b,c);


#define OP r1+=vp1[-1] * dp[0]; \
	   r2+=vp1[PTR_DIST-1] * dp[0];



#define OP2 SCHEDULE(OP   ,r1+=vp1[-2]  * dp[1] ,r2+=vp1[PTR_DIST-2]  *dp[1]);
#define OP3 SCHEDULE(OP2  ,r1+=vp1[-3]  * dp[2] ,r2+=vp1[PTR_DIST-3]  *dp[2]);
#define OP4 SCHEDULE(OP3  ,r1+=vp1[-4]  * dp[3] ,r2+=vp1[PTR_DIST-4]  *dp[3]);
#define OP5 SCHEDULE(OP4  ,r1+=vp1[-5]  * dp[4] ,r2+=vp1[PTR_DIST-5]  *dp[4]);
#define OP6 SCHEDULE(OP5  ,r1+=vp1[-6]  * dp[5] ,r2+=vp1[PTR_DIST-6]  *dp[5]);
#define OP7 SCHEDULE(OP6  ,r1+=vp1[-7]  * dp[6] ,r2+=vp1[PTR_DIST-7]  *dp[6]);
#define OP8 SCHEDULE(OP7  ,r1+=vp1[-8]  * dp[7] ,r2+=vp1[PTR_DIST-8]  *dp[7]);
#define OP9 SCHEDULE(OP8  ,r1+=vp1[-9]  * dp[8] ,r2+=vp1[PTR_DIST-9]  *dp[8]);
#define OP10 SCHEDULE(OP9 ,r1+=vp1[-10] * dp[9] ,r2+=vp1[PTR_DIST-10] *dp[9]);
#define OP11 SCHEDULE(OP10,r1+=vp1[-11] * dp[10],r2+=vp1[PTR_DIST-11] *dp[10]);
#define OP12 SCHEDULE(OP11,r1+=vp1[-12] * dp[11],r2+=vp1[PTR_DIST-12] *dp[11]);
#define OP13 SCHEDULE(OP12,r1+=vp1[-13] * dp[12],r2+=vp1[PTR_DIST-13] *dp[12]);
#define OP14 SCHEDULE(OP13,r1+=vp1[-14] * dp[13],r2+=vp1[PTR_DIST-14] *dp[13]);
#define OP15 SCHEDULE(OP14,r1+=vp1[-15] * dp[14],r2+=vp1[PTR_DIST-15] *dp[14]);


/*
#define OP r1+=vp1[-1] * dp[0]; \
	   r2+=vp2[-1] * dp[0];



#define OP2 SCHEDULE(OP   ,r1+=vp1[-2]  * dp[1] ,r2+=vp2[-2]  * dp[1]);
#define OP3 SCHEDULE(OP2  ,r1+=vp1[-3]  * dp[2] ,r2+=vp2[-3]  * dp[2]);
#define OP4 SCHEDULE(OP3  ,r1+=vp1[-4]  * dp[3] ,r2+=vp2[-4]  * dp[3]);
#define OP5 SCHEDULE(OP4  ,r1+=vp1[-5]  * dp[4] ,r2+=vp2[-5]  * dp[4]);
#define OP6 SCHEDULE(OP5  ,r1+=vp1[-6]  * dp[5] ,r2+=vp2[-6]  * dp[5]);
#define OP7 SCHEDULE(OP6  ,r1+=vp1[-7]  * dp[6] ,r2+=vp2[-7]  * dp[6]);
#define OP8 SCHEDULE(OP7  ,r1+=vp1[-8]  * dp[7] ,r2+=vp2[-8]  * dp[7]);
#define OP9 SCHEDULE(OP8  ,r1+=vp1[-9]  * dp[8] ,r2+=vp2[-9]  * dp[8]);
#define OP10 SCHEDULE(OP9 ,r1+=vp1[-10] * dp[9] ,r2+=vp2[-10] * dp[9]);
#define OP11 SCHEDULE(OP10,r1+=vp1[-11] * dp[10],r2+=vp2[-11] * dp[10]);
#define OP12 SCHEDULE(OP11,r1+=vp1[-12] * dp[11],r2+=vp2[-12] * dp[11]);
#define OP13 SCHEDULE(OP12,r1+=vp1[-13] * dp[12],r2+=vp2[-13] * dp[12]);
#define OP14 SCHEDULE(OP13,r1+=vp1[-14] * dp[13],r2+=vp2[-14] * dp[13]);
#define OP15 SCHEDULE(OP14,r1+=vp1[-15] * dp[14],r2+=vp2[-15] * dp[14]);
*/

#endif
