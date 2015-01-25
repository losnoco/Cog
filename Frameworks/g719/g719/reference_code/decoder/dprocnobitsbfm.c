/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "proto.h"
#include "cnst.h"

/*--------------------------------------------------------------------------*/
/*  Function  dprocnobitsbfm                                                */
/*  ~~~~~~~~~~~~~~~~~~~~~~~~                                                */
/*                                                                          */
/*  Decoding for sub-vectors originally allocated with 0 bits               */
/*--------------------------------------------------------------------------*/
/*  short     *R            (i/o) number of bits per coefficinet            */
/*  short     *idx          (i)   reordered sub-vector indices              */
/*  short     *ynrm         (i)   quantization indices for norms            */
/*  short     *ycof         (i)   quantization indices for MLT coefficients */
/*  short     **ppbits      (i/o) pointer to bitstream                      */
/*  float     *coefsq       (o)   reconstructed MLT coefficients            */
/*  float     *coefsq_norm  (o)   reconstructed normalized MLT coefficients */
/*  short     nb_sfm        (i)   number of sub-vectors to be coded         */
/*  short     diff          (i)   number of available bits                  */
/*--------------------------------------------------------------------------*/
void dprocnobitsbfm(
short *R,
short *idx,
short *ynrm,
short *ycof,
short **ppbits,
float *coefsq,
float *coefsq_norm,
short nb_sfm,
short diff
)
{
    short i, k, m, im, offset;
    short *pbits;

    pbits = *ppbits;

    im = MAX16B;
    k = nb_sfm - 1;
    for (i=k; i>0; i--)
    {
       if (R[idx[i]]==0)
       {
         im = i;
       }
    }
    for (i=im; i<nb_sfm; i++)
    {
      m = idx[i];
      if (R[m]==0)
      {
        if (m<SFM_G1)
        {
          if (diff>=WID_G1)
          {
            R[m] = 1;
            offset = m * WID_G1;
            dprocnf(&ycof[offset], pbits, ynrm[m], NB_VECT1, &coefsq[offset], &coefsq_norm[offset]);
            pbits += WID_G1;
            diff -= WID_G1;
          }
        }
        else if (m<SFM_G1G2)
        {
          if (diff>=WID_G2)
          {
            R[m] = 1;
            offset = NUMC_G1 + (m - SFM_G1) * WID_G2;
            dprocnf(&ycof[offset], pbits, ynrm[m], NB_VECT2, &coefsq[offset], &coefsq_norm[offset]);
            pbits += WID_G2;
            diff -= WID_G2;
          }
        }
        else if (m<SFM_N)
        {
          if (diff>=WID_G3)
          {
            R[m] = 1;
            offset = NUMC_G1G2 + (m - SFM_G1G2) * WID_G3;
            dprocnf(&ycof[offset], pbits, ynrm[m], NB_VECT3, &coefsq[offset], &coefsq_norm[offset]);
            pbits += WID_G3;
            diff -= WID_G3;
          }
        }
        else
        {
          if (diff>=WID_GX)
          {
            R[m] = 1;
            offset = NUMC_N + (m - SFM_N) * WID_GX;
            dprocnf(&ycof[offset], pbits, ynrm[m], NB_VECTX, &coefsq[offset], &coefsq_norm[offset]);
            pbits += WID_GX;
            diff -= WID_GX;
          }
        }
      }
    }

    *ppbits = pbits;
    return;
}
