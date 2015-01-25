/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "rom.h"

/*--------------------------------------------------------------------------*/
/*  Function  codesearch                                                    */
/*  ~~~~~~~~~~~~~~~~~~~~                                                    */
/*                                                                          */
/*  Finding the closest point of lattice                                    */
/*--------------------------------------------------------------------------*/
/*  float       *x  (i)  arbitrary vector in Qv                             */
/*  short       *C  (o)  point of lattice                                   */
/*  Word16      R   (i)  number of bits per coefficient                     */
/*--------------------------------------------------------------------------*/
void codesearch(short *x, short *C, short R)
{
    short i, j, sum;
    short e[8], em, temp;


    sum = 0;
    for (i=0; i<8; i++)
    {
       temp = x[i] & FacLVQ2Mask[R];
       C[i] = x[i] >> FacLVQ2Qv[R];
       if ((temp>FacLVQ2HalfQv[R]) || ((temp==FacLVQ2HalfQv[R]) && (x[i]<0)))
       {
         C[i] += 1;
       }
       sum += C[i];
    }
    if (sum&1)
    {
      j = 0;
      em = 0;
      for (i=0; i<8; i++)
      {
         temp = C[i] << FacLVQ2Qv[R];
         e[i] = x[i] - temp;
         temp = e[i];
         if (e[i]<0)
         {
           temp = -e[i];
         }
         if (em<temp)
         {
           em = temp;
           j = i;
         }
      }
      if (e[j]>=0)
      {
        C[j] += 1;
      }
      else
      {
        C[j] -= 1;
      }
    }

    return;
}
