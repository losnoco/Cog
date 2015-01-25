/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "cnst.h"

/*--------------------------------------------------------------------------*/
/*  Function  idx2bitsn                                                     */
/*  ~~~~~~~~~~~~~~~~~~~                                                     */
/*                                                                          */
/*  Conversion from norm index into ITU bit stream                          */
/*--------------------------------------------------------------------------*/
/*  short      x   (i)  index of quantized norm                             */
/*  short      N   (i)  bits per norm                                       */
/*  short      *y  (o)  ITU bitstream                                       */
/*--------------------------------------------------------------------------*/
void idx2bitsn(short x, short N, short *y)
{
    short i;
    short temp;


    y += N;
    for (i=0; i<N; i++)
    {
       temp = (x >> i) & 1;
       y--;
       *y = G192_BIT1;
       if (temp==0)
       {
         *y = G192_BIT0;
       }
    }

    return;
}


/*--------------------------------------------------------------------------*/
/*  Function  idx2bitsc                                                     */
/*  ~~~~~~~~~~~~~~~~~~~                                                     */
/*                                                                          */
/*  Conversion from quantization index into ITU bit stream                  */
/*--------------------------------------------------------------------------*/
/*  short       *x  (i)  quantization index                                 */
/*  short       N   (i)  vector dimensions                                  */
/*  short       L   (i)  bits per coefficient                               */
/*  short       *y  (o)  ITU bitstream                                      */
/*--------------------------------------------------------------------------*/
void idx2bitsc(short *x, short N, short L, short *y)
{
    short i, j, m, n;
    short temp;
    short *pty;


    if (L==1)
    {
      n = 1;
      m = N;
    }
    else
    {
      n = N;
      m = L;
    }

    for (j=0; j<n; j++)
    {
       y += m;
       pty = y;
       for (i=0; i<m; i++)
       {
          temp = (x[j] >> i) & 1;
          pty--;
          *pty = G192_BIT1;
          if (temp==0)
          {
            *pty = G192_BIT0;
          }
       }
    }

    return;
}

/*--------------------------------------------------------------------------*/
/*  Function  bits2idxn                                                     */
/*  ~~~~~~~~~~~~~~~~~~~                                                     */
/*                                                                          */
/*  Conversion from ITU bit stream into norm index                          */
/*--------------------------------------------------------------------------*/
/*  short       *y   i)  ITU bitstream                                      */
/*  short       N   (i)  bits per norm                                      */
/*  short       *x  (o)  index of quantized norm                            */
/*--------------------------------------------------------------------------*/
void bits2idxn(short *y, short N, short *x)
{
    short i;


    *x = 0;
    for (i=0; i<N; i++)
    {
       *x <<= 1;
       if ((*y++)==G192_BIT1)
       {
         *x += 1;
       }
    }

    return;
}

/*--------------------------------------------------------------------------*/
/*  Function  bits2idxc                                                     */
/*  ~~~~~~~~~~~~~~~~~~~                                                     */
/*                                                                          */
/*  Conversion from ITU bit stream into coefficient index                   */
/*--------------------------------------------------------------------------*/
/*  short       *y  (i)  ITU bitstream                                      */
/*  short       N   (i)  vector dimensions                                  */
/*  short       L   (i)  bits per coefficient                               */
/*  short       *x  (o)  index of quantized coefficient                     */
/*--------------------------------------------------------------------------*/
void bits2idxc(short *y, short N, short L, short *x)
{
    short i, k, m, n;
    short temp;

    if (L==1)
    {
      n = 1;
      m = N;
    }
    else
    {
      n = N;
      m = L;
    }

    for (k=0; k<n; k++)
    {
       x[k] = 0;
       for (i=0; i<m; i++)
       {
          temp = x[k] << 1;
          if ((*y++)==G192_BIT1)
          {
            temp += 1;
          }
          x[k] = temp;
       }
    }

    return;
}
