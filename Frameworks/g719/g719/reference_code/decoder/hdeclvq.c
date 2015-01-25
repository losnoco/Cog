/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "cnst.h"

/*--------------------------------------------------------------------------*/
/*  Function  hdec2blvq                                                     */
/*  ~~~~~~~~~~~~~~~~~~~                                                     */
/*                                                                          */
/*  Huffman decoding for LVQ2 quantization indices                          */
/*--------------------------------------------------------------------------*/
/*  short       *bitstream  (i)    Huffman code                             */
/*  short       N           (i)    number of coefficients                   */
/*  short       *index      (o)    LVQ2 quantization indices                */
/*--------------------------------------------------------------------------*/
void hdec2blvq(short *bitstream, short N, short *index)
{
  short i;
  short temp;
  short *pbits, *pidx;


  pbits = bitstream;
  pidx  = index;

  for (i=0; i<N; i++)
  {
     temp = 0;
     if ((*pbits++)==G192_BIT1)
     {
       temp = 3;
       if ((*pbits++)==G192_BIT1)
       {
         temp = 1;
         if ((*pbits++)==G192_BIT1)
         {
           temp++;
         }
       }
     }
     *pidx++ = temp;
  }

  return;
}

/*--------------------------------------------------------------------------*/
/*  Function  hdec2blvq                                                     */
/*  ~~~~~~~~~~~~~~~~~~~                                                     */
/*                                                                          */
/*  Huffman decoding for indices of 3-bit LVQ2                              */
/*--------------------------------------------------------------------------*/
/*  short       *bitstream  (i)    Huffman code                             */
/*  short       N           (i)    number of coefficients                   */
/*  short       *index      (o)    LVQ2 quantization indices                */
/*--------------------------------------------------------------------------*/
void hdec3blvq(short *bitstream, short N, short *index)
{
  short i, j, k, m;
  short temp;
  short *pbits, *pidx;


  pbits = bitstream;
  pidx  = index;

  for (i=0; i<N; i++)
  {
     j = 0;
     if ((*pbits++)==G192_BIT1)
     {
       j = 1;
     }
     k = j * 2;
     if ((*pbits++)==G192_BIT1)
     {
       k++;
     }
     temp = j * 4 + k;
     if (k==2)
     {
       j = 0;
       if ((*pbits++)==G192_BIT1)
       {
         j = 1;
       }
       m = j * 2;
       if ((*pbits++)==G192_BIT1)
       {
         m++;
       }
       temp = j * 2 + m + 1;
       if (m==0)
       {
         temp = 3;
         if ((*pbits++)==G192_BIT1)
         {
           temp++;
         }
       }
     }
     *pidx++ = temp;
  }

  return;
}


/*--------------------------------------------------------------------------*/
/*  Function  hdec2blvq                                                     */
/*  ~~~~~~~~~~~~~~~~~~~                                                     */
/*                                                                          */
/*  Huffman decoding for indices of 4-bit LVQ2                              */
/*--------------------------------------------------------------------------*/
/*  short       *bitstream  (i)    Huffman code                             */
/*  short       N           (i)    number of coefficients                   */
/*  short       *index      (o)    LVQ2 quantization indices                */
/*--------------------------------------------------------------------------*/
void hdec4blvq(short *bitstream, short N, short *index)
{
  short i, j, k, m;
  short temp;
  short *pbits, *pidx;


  pbits = bitstream;
  pidx  = index;

  for (i=0; i<N; i++)
  {
     k = 0;
     if ((*pbits++)==G192_BIT1)
     {
       k = 2;
     }
     if ((*pbits++)==G192_BIT1)
     {
       k++;
     }
     temp = 0;
     if (k!=0)
     {
       j = 0;
       if ((*pbits++)==G192_BIT1)
       {
         j = 1;
       }
       temp = 1;
       if (j!=0)
       {
         temp = 15;
       }
       if (k!=3)
       {
         m = j * 2;
         if ((*pbits++)==G192_BIT1)
         {
           m++;
         }
         temp = m;
         if (j==0)
         {
           temp = m + 13;
         }
         if (k!=1)
         {
           m = m * 2;
           if ((*pbits++)==G192_BIT1)
           {
             m++;
           }
           temp = m;
           if (j==0)
           {
             temp = m + 9;
           }
           if (m==7)
           {
             temp = m;
             if ((*pbits++)==G192_BIT1)
             {
               temp = m + 1;
             }
           }
         }
       }
     }
     *pidx++ = temp;
  }

  return;
}

/*--------------------------------------------------------------------------*/
/*  Function  hdec2blvq                                                     */
/*  ~~~~~~~~~~~~~~~~~~~                                                     */
/*                                                                          */
/*  Huffman decoding for indices of 5-bit LVQ2                              */
/*--------------------------------------------------------------------------*/
/*  short       *bitstream  (i)    Huffman code                             */
/*  short       N           (i)    number of coefficients                   */
/*  short       *index      (o)    LVQ2 quantization indices                */
/*--------------------------------------------------------------------------*/
void hdec5blvq(short *bitstream, short N, short *index)
{
  short i, j, k, m, n;
  short temp;
  short *pbits, *pidx;


  pbits = bitstream;
  pidx  = index;

  for (i=0; i<N; i++)
  {
     n = 0;
     if ((*pbits++)==G192_BIT1)
     {
       n = 2;
     }
     if ((*pbits++)==G192_BIT1)
     {
       n++;
     }
     temp = 0;
     if (n!=0)
     {
       j = 0;
       if ((*pbits++)==G192_BIT1)
       {
         j = 1;
       }
       temp = 1;
       if (j!=0)
       {
         temp = 31;
       }
       if (n!=1)
       {
         k = 0;
         if ((*pbits++)==G192_BIT1)
         {
           k = 1;
         }
         if (n==2)
         {
           temp = 2;
           if (k!=0)
           {
             temp = 30;
           }
           if (j!=0)
           {
             temp = 3;
             if ((*pbits++)==G192_BIT1)
             {
               temp++;
             }
             if (k!=0)
             {
               temp += 25;
             }
           }
         }
         else
         {
           m = 0;
           if ((*pbits++)==G192_BIT1)
           {
             m = 2;
           }
           if ((*pbits++)==G192_BIT1)
           {
             m++;
           }
           temp = m + 5;
           if (k!=0)
           {
             temp += 19;
           }
           if (j!=0)
           {
             m = k * 4 + m;
             temp = 23;
             if (m!=7)
             {
               m *= 2;
               if ((*pbits++)==G192_BIT1)
               {
                 m++;
               }
               temp = m + 9;
             }
           }
         }
       }
     }
     *pidx++ = temp;
  }

  return;
}
