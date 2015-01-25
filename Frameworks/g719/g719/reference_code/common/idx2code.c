/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "proto.h"
#include "cnst.h"
#include "rom.h"

/*--------------------------------------------------------------------------*/
/*  Function  idx2code                                                      */
/*  ~~~~~~~~~~~~~~~~~~                                                      */
/*                                                                          */
/*  Finding a codevector from its index                                     */
/*--------------------------------------------------------------------------*/
/*  short     *k  (i)   index of the codevector                             */
/*  short     *y  (o)   codevector                                          */
/*  short     R   (i)   number of bits per coefficient                      */
/*--------------------------------------------------------------------------*/
void idx2code(short *k, short *y, short R)
{
    short i, m, tmp;
    short v[8], z[8];


    tmp = FacLVQ2Qv[R] - R;
    m = k[0] << 1;
    for (i=1; i<8; i++)
    {
       m += k[i];
    }
    if (tmp<0)
    {
      z[0] = m >> (-tmp);
    }
    else
    {
      z[0] = m << tmp;
    }
    for (i=1; i<8; i++)
    {
       if (tmp<0)
       {
         z[i] = k[i] >> (-tmp);
       }
       else
       {
         z[i] = k[i] << tmp;
       }
    }
    codesearch(z, v, R);
    y[0] = m - (v[0] << R);
    for (i=1; i<8; i++)
    {
       y[i] = k[i] - (v[i] << R);
    }

    return;
}
