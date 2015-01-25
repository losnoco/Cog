/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "cnst.h"
#include "rom.h"

/*--------------------------------------------------------------------------*/
/*  Function  bitalloc                                                      */
/*  ~~~~~~~~~~~~~~~~~~~~~                                                   */
/*                                                                          */
/*  Adaptive bit allocation for 20kHz audio codec                           */
/*--------------------------------------------------------------------------*/
/*  short       *y     (i)    reordered norm of sub-vectors                 */
/*  short       *idx   (i)    reordered sub-vector indices                  */
/*  short       sum    (i)    number of available bits                      */
/*  short       N      (i)    number of norms                               */
/*  short       M      (i)    maximum number of bits per dimension          */
/*  short       *r     (o)    bit-allacation vector                         */
/*--------------------------------------------------------------------------*/
void bitalloc(short *y, short *idx, short sum, short N, short M, short *r)
{
    short i, j, k, n, m, v, im;
    short diff, temp;


    im = 1;
    diff = sum;
    n = sum >> 3;
    for (i=0; i<n; i++)
    {
       k = 0;
       temp = y[0];
       for (m=1; m<=im; m++)
       {
          if (temp<y[m])
          {
            temp = y[m];
            k = m;
          }
       }
       if (k==im)
       {
         im++;
       }
       j = idx[k];
       if ((sum>=sfmsize[j]) && (r[j]<M))
       {
         y[k] -= 2;
         r[j]++;
         if (r[j]>=M)
         {
           y[k] = MIN16B;
         }
         sum -= sfmsize[j];
       }
       else
       {
         y[k] = MIN16B;
         k++;
         if (k==im)
         {
           im++;
         }
       }
       if ((sum<WID_G1) || (diff==sum))
       {
         break;
       }
       diff = sum;
       v = N - 2;
       if (k>v)
       {
         for (i=0; i<N; i++)
         {
            if (y[i]>MIN16B)
            {
              im = i + 1;
              break;
            }
         }
       }
    }
    if (sum>=WID_G2)
    {
      for (i=0; i<N; i++)
      {
         j = idx[i];
         if ((j>=SFM_G1) && (j<SFM_G1G2) && (r[j]==0))
         {
           r[j] = 1;
           sum -= WID_G2;
           if (sum<WID_G2)
           {
             break;
           }
         }
      }
    }
    if (sum>=WID_G2)
    {
      for (i=0; i<N; i++)
      {
         j = idx[i];
         if ((j>=SFM_G1) && (j<SFM_G1G2) && (r[j]==1))
         {
           r[j] = 2;
           sum -= WID_G2;
           if (sum<WID_G2)
           {
             break;
           }
         }
      }
    }
    if (sum>=WID_G1)
    {
      for (i=0; i<N; i++)
      {
         j = idx[i];
         if ((j<SFM_G1) && (r[j]==0))
         {
           r[j] = 1;
           sum -= WID_G1;
           if (sum<WID_G1)
           {
             break;
           }
         }
      }
    }
    if (sum>=WID_G1)
    {
      for (i=0; i<N; i++)
      {
         j = idx[i];
         if ((j<SFM_G1) && (r[j]==1))
         {
           r[j] = 2;
           sum -= WID_G1;
           if (sum<WID_G1)
           {
             break;
           }
         }
      }
    }

    return;
}
