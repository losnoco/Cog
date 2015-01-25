/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "proto.h"
#include "cnst.h"
#include "rom.h"

/*--------------------------------------------------------------------------*/
/*  Function  unpackc                                                       */
/*  ~~~~~~~~~~~~~~~~~                                                       */
/*                                                                          */
/*  Huffman decoding and unpacking indices for quantized coefficients       */
/*--------------------------------------------------------------------------*/
/*  short     *R      (i)   number of bits per coefficinet                  */
/*  short     *pbits  (i)   pointer to bitstream                            */
/*  short     flag    (i)   Huffman code flag                               */
/*  short     rv      (i)   offset for index of quantized coefficients      */
/*  short     N1      (i)   beginning sub-vector's number in the group      */
/*  short     N2      (i)   ending sub-vector's number in the group         */
/*  short     L       (o)   number of coefficients in each sub-vector       */
/*  short     *y      (o)   indices of the selected codevectors             */
/*--------------------------------------------------------------------------*/
/*  short     return  (o)   length of Huffman code                          */
/*--------------------------------------------------------------------------*/
short unpackc(short *R, short *pbits, short flag, short rv, short N1, short N2, short L, short *y)
{
    short i, j, k, n, r, v, hcode_l, offset, sum;
    short nb_vecs, length;

    nb_vecs = L >> 3;
    length = 0;
    if (flag==NOHUFCODE)
    {
      for (n=N1; n<N2; n++)
      {
         v = R[n];
         if (v>1)
         {
           bits2idxc(pbits, L, v, &y[rv]);
           sum = v * L;
           pbits += sum;
           length += sum;
         }
         else if (v==1)
         {
           k = rv;
           for (i=0; i<nb_vecs; i++)
           {
              bits2idxc(pbits, 8, 1, &y[k]);
              pbits += 8;
              k += 8;
           }
           length += L;
         }
         rv += L;
      }
    }
    else
    {
      r = 0;
      hcode_l = 0;
      for (n=N1; n<N2; n++)
      {
         v = R[n];
         if (v>QBIT_MAX1)
         {
           bits2idxc(pbits, L, v, &y[rv]);
           sum = v * L;
           pbits += sum;
           r += sum;
         }
         else if (v>1)
         {
           if (v==2)
           {
             hdec2blvq(pbits, L, &y[rv]);
           }
           else if (v==3)
           {
             hdec3blvq(pbits, L, &y[rv]);
           }
           else if (v==4)
           {
             hdec4blvq(pbits, L, &y[rv]);
           }
           else
           {
             hdec5blvq(pbits, L, &y[rv]);
           }
           offset = huffoffset[v];
           for (i=0; i<L; i++)
           {
              k = rv + i;
              j = offset + y[k];
              hcode_l += huffsizc[j];
              pbits += huffsizc[j];
           }
         }
         else if (v==1)
         {
           k = rv;
           for (i=0; i<nb_vecs; i++)
           {
              bits2idxc(pbits, 8, 1, &y[k]);
              pbits += 8;
              k += 8;
           }
           r += L;
         }
         rv += L;
      }
      length = hcode_l + r;
    }

    return(length);
}
