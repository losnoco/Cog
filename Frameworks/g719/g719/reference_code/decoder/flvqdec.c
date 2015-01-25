/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "proto.h"
#include "cnst.h"
#include "rom.h"

/*--------------------------------------------------------------------------*/
/*  Function  flvqdec                                                       */
/*  ~~~~~~~~~~~~~~~~~                                                       */
/*                                                                          */
/*  Decoding of Fast Lattice Vector Quantization (FLVQ)                     */
/*--------------------------------------------------------------------------*/
/*  short     **bitstream     (i)   bit-stream vector                       */
/*  float     *coefsq         (o)   MLT coefficient vector                  */
/*  float     *coefsq_norm    (o)   normalized MLT coefficient vector       */
/*  short     R               (o)   bit-allocation vector                   */
/*  short     NumSpectumBits  (i)   number of available bits                */
/*  short     *ynrm           (o)   norm quantization index vector          */
/*  short     is_transient    (i)   transient flag                          */
/*--------------------------------------------------------------------------*/
void flvqdec(
   short **bitstream,
   float *coefsq,
   float *coefsq_norm,
   short *R,
   short NumSpectumBits,
   short *ynrm,
   short is_transient
)
{
    short i, j, k, v, nb_sfm;
    short diff;
    short hcode_l, FlagL, FlagN, FlagC;
    short idx[NB_SFM], normqlg2[NB_SFM], wnorm[NB_SFM], idxbuf[NB_SFM];
    short ycof[STOP_BAND];
    short *pbits;
    short **ppbits;


    pbits = *bitstream;
    /*** Unpacking bit stream to flags ***/
    FlagL = 0;
    if ((*pbits++)==G192_BIT1)
    {
      FlagL = 1;
    }
    FlagN = 0;
    if ((*pbits++)==G192_BIT1)
    {
      FlagN = 1;
    }
    FlagC = 0;
    if ((*pbits++)==G192_BIT1)
    {
      FlagC = 1;
    }

    /*** Unpacking bit stream and Huffman decoding for indices of quantized norms ***/
    if (FlagL==NOALLGROUPS)
    {
      nb_sfm = SFM_N;
    }
    else
    {
      nb_sfm = NB_SFM;
    }
    bits2idxn(pbits, NORM0_BITS, ynrm);
    pbits += NORM0_BITS;
    if (FlagN==HUFCODE)
    {
      hdecnrm(pbits, NB_SFM, &ynrm[1]);
      hcode_l = 0;
      for (i=1; i<NB_SFM; i++)
      {
         hcode_l += huffsizn[ynrm[i]];
      }
      pbits += hcode_l;
    }
    else
    {
      for (i=1; i<NB_SFM; i++)
      {
         bits2idxn(pbits, NORMI_BITS, &ynrm[i]);
         pbits += NORMI_BITS;
      }
      hcode_l = NUMNRMIBITS;
    }

    /*** De-quantization of norms ***/
    /* First sub-frame */
    normqlg2[0] = dicnlg2[ynrm[0]];
    /* Other sub-frames */
    if (is_transient)
    {
      /* Recover quantization indices and quantized norms */
      idxbuf[0] = ynrm[0];
      for (i=1; i<NB_SFM; i++)
      {
         idxbuf[i] = ynrm[i] + idxbuf[i-1] - 15;
      }
      recovernorm(idxbuf, ynrm, normqlg2);
    }
    else
    {
      for (i=1; i<NB_SFM; i++)
      {
         j = i - 1;
         k = ynrm[j] - 15;
         ynrm[i] += k;
         normqlg2[i] = dicnlg2[ynrm[i]];
      }
    }

    /*** Bit allocation ***/
    for (i=0; i<nb_sfm; i++)
    {
       idx[i] = i;
    }
    map_quant_weight(normqlg2, wnorm, is_transient);
    reordvct(wnorm, nb_sfm, idx);    
    for (i=0; i<NB_SFM; i++)
    {
       R[i] = 0;
    }
    diff = NumSpectumBits - FLAGS_BITS - NORM0_BITS;
    v = diff - hcode_l;
    diff = v;
    bitalloc(wnorm, idx, diff, nb_sfm, QBIT_MAX2, R);


    /*** Unpacking bit stream and Huffman decoding for indices of quantized coefficients ***/
    /* First group */
    hcode_l = unpackc(R, pbits, FlagC, 0, 0, SFM_G1, WID_G1, ycof);
    pbits += hcode_l;

    /* Second group */
    k = unpackc(R, pbits, FlagC, NUMC_G1, SFM_G1, SFM_G1G2, WID_G2, ycof);
    pbits += k;
    hcode_l += k;

    /* Third group */
    k = unpackc(R, pbits, FlagC, NUMC_G1G2, SFM_G1G2, SFM_N, WID_G3, ycof);
    pbits += k;
    hcode_l += k;

    /* Forth group */
    if (nb_sfm>SFM_N)
    {
      k = unpackc(R, pbits, FlagC, NUMC_N, SFM_N, NB_SFM, WID_GX, ycof);
      pbits += k;
      hcode_l += k;
    }
    diff = v - hcode_l;


    /*** Lattice Vector De-quantization for normalized MLT coefficients ***/
    /* First group */
    dqcoefs(&ycof[0], ynrm, R, 0, SFM_G1, WID_G1, &coefsq[0], &coefsq_norm[0]);

    /* Second group */
    dqcoefs(&ycof[NUMC_G1], ynrm, R, SFM_G1, SFM_G1G2, WID_G2, &coefsq[NUMC_G1], &coefsq_norm[NUMC_G1]);

    /* Third group */
    dqcoefs(&ycof[NUMC_G1G2], ynrm, R, SFM_G1G2, SFM_N, WID_G3, &coefsq[NUMC_G1G2], &coefsq_norm[NUMC_G1G2]);

    /* Forth group */
    dqcoefs(&ycof[NUMC_N], ynrm, R, SFM_N, NB_SFM, WID_GX, &coefsq[NUMC_N], &coefsq_norm[NUMC_N]);


    /*** Processing for noise-filling sub-vectors ***/
    ppbits = &pbits;
    dprocnobitsbfm(R, idx, ynrm, ycof, ppbits, coefsq, coefsq_norm, nb_sfm, diff);

    *bitstream = pbits;
    return;
}
