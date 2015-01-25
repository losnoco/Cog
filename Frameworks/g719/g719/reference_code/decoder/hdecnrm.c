/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "cnst.h"

/*--------------------------------------------------------------------------*/
/*  Function  hdecnrm                                                       */
/*  ~~~~~~~~~~~~~~~~~                                                       */
/*                                                                          */
/*  Huffman decoding for indices of quantized norms                         */
/*--------------------------------------------------------------------------*/
/*  short       *bitstream  (i)    Huffman code                             */
/*  short       N           (i)    number of norms                          */
/*  short       *index      (o)    indices of quantized norms               */
/*--------------------------------------------------------------------------*/
void hdecnrm(short *bitstream, short N, short *index)
{
  short i, j, k, n, m;
  short temp;
  short *pbits, *pidx;

  pbits = bitstream;
  pidx  = index;

  m = N - 1;
  for (i=0; i<m; i++)
  {
     j = 0;
     k = 0;
     if ((*pbits++)==G192_BIT1)
     {
       j = 1;
     }
     if ((*pbits++)==G192_BIT1)
     {
       k = 1;
     }
     n = j * 2 + k;
     j = j * 4;
     temp = 16 + n - j;
     if ((*pbits++)==G192_BIT1)
     {
       temp = 12 + n + j;
       if ((*pbits++)==G192_BIT1)
       {
         j = 0;
         if ((*pbits++)==G192_BIT1)
         {
           j = 1;
         }
         temp = 8 + n;
         if (j!=0)
         {
           temp += 12;
         }
         if ((*pbits++)==G192_BIT1)
         {
           temp = n;
           if ((*pbits++)==G192_BIT1)
           {
             temp = n + 4;
           }
           if (j!=0)
           {
             temp += 24;
           }
         }
       }
     }
     *pidx++ = temp;
  }

  return;
}
