/*-----------------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                             */
/* © 2008 Ericsson AB. and Polycom Inc. */
/* All rights reserved.                                                              */
/*-----------------------------------------------------------------------------------*/

#include "proto.h"
#include "cnst.h"
#include "rom.h"

/*---------------------------------------------------------------------------*/
/*  Function  dprocnf                                                        */
/*  ~~~~~~~~~~~~~~~~~                                                        */
/*                                                                           */
/*  De-quantization for sub-vectors originally allocated with 0 bits         */
/*---------------------------------------------------------------------------*/
/*  short     *y           (i)   indices of the selected codevectors         */
/*  short     *pbits       (i)   pointer to bitstream                        */
/*  short     idxnrm       (i)   indices of quantized norms                  */
/*  short     nb_vecs      (i)   number of 8-D vectors in current sub-vector */
/*  float     *coefs       (o)   MLT coefficients                            */
/*  float     *coefs_norm  (o)   normalized MLT coefficients                 */
/*---------------------------------------------------------------------------*/
void dprocnf(short *y, short *pbits, short idxnrm, short nb_vecs, float *coefs, float* coefs_norm)
{
   short i, j;
   short pre_idx;
   float normq;


   normq = dicn[idxnrm];
   pre_idx = MAX16B;
   for (i=0; i<nb_vecs; i++)
   {
      bits2idxc(pbits, 8, 1, y);
      pbits += 8;
      if ((pre_idx<128) && (*y<16))
      {
        for (j=0; j<8; j++)
        {
           *coefs_norm = OFFSETf;
           *coefs++ = (*coefs_norm++) * normq;
        }
      }
      else
      {
        for (j=0; j<8; j++)
        {
           *coefs_norm = (float)dic4[*y][j] / FCT_LVQ1f + OFFSETf;
           *coefs++ = (*coefs_norm++) * normq;
        }
      }
      pre_idx = *y;
      y += 8;
   }


   return;
}
