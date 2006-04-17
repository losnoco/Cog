/*
  wrapper for window functions
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */

//changes 8/4/2002 (by Hauke Duden):
//	- added VC6 pragma to prevent some warnings from being reported 

#include "mpegsound.h"


static int windowInit=0;

ATTR_ALIGN(64) REAL win[4][36];
ATTR_ALIGN(64) REAL winINV[4][36];


#ifdef _MSC_VER
#pragma warning(disable: 4244)
#endif

void initialize_win() {
  if (windowInit) {
    return;
  }
  windowInit=true;

  int i;
  
  for(i=0;i<18;i++) {
    /*
      win[0][i]=win[1][i]=0.5*sin(PI_72*(double)(2*i+1))/
      cos(PI_72*(double)(2*i+19));
    */
    win[0][i]=win[1][i]=0.5*sin(PI_72*(double)(2*(i+0)+1))/cos(MY_PI * (double) (2*(i+0) +19) / 72.0 );
    win[0][i+18] = win[3][i+18] = 0.5 * sin( MY_PI / 72.0 * (double) (2*(i+18)+1) ) / cos ( MY_PI * (double) (2*(i+18)+19) / 72.0 );
  }
  /*
    for(;i<36;i++) {
    win[0][i]=win[3][i]=0.5*sin(PI_72*(double)(2*i+1))/cos(PI_72*(double)(2*i+19));
    
    }
  */
  for(i=0;i<6;i++) {
    win[1][i+18]=0.5/cos(MY_PI*(double)(2*(i+18)+19)/72.0);
    win[3][i+12]=0.5/cos(MY_PI*(double)(2*(i+12)+19)/72.0);
    win[1][i+24]=0.5*sin(PI_24*(double)(2*i+13))/cos(MY_PI*(double)(2*(i+24)+19)/72.0);
    win[1][i+30]=win[3][i]=0.0;
    win[3][i+6 ]=0.5*sin(PI_24*(double)(2*i+1))/cos(MY_PI*(double)(2*(i+6)+19)/72.0);
  }
  for(i=0;i<12;i++)
    win[2][i]=0.5*sin(PI_24*(double)(2*i+1))/cos(MY_PI*(double)(2*i+7)/24.0);

  int j;

  for(j=0;j<4;j++) {
    int len[4] = { 36,36,12,36 };
    for(i=0;i<len[j];i+=2)
      winINV[j][i] = + win[j][i];
    for(i=1;i<len[j];i+=2)
      winINV[j][i] = - win[j][i];
  }


}

#ifdef _MSC_VER
#pragma warning(default: 4244)
#endif


