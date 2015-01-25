/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "proto.h"
#include "cnst.h"
#include "rom.h"

/*--------------------------------------------------------------------------*/
/*  Function  dqcoefs                                                       */
/*  ~~~~~~~~~~~~~~~~~~~                                                     */
/*                                                                          */
/*  Vector de-quantization for normalized MLT coefficients                  */
/*--------------------------------------------------------------------------*/
/*  short     *y           (i)   indices of the selected codevectors        */
/*  short     *idxnrm      (i)   indices of quantized norms                 */
/*  short     *R           (i)   number of bits per coefficient             */
/*  short     N1           (i)   beginning sub-vector's number in the group */
/*  short     N2           (i)   ending sub-vector's number in the group    */
/*  short     L            (i)   number of coefficients in each sub-vector  */
/*  float     *coefs       (o)   MLT coefficients                           */
/*  float     *coefs_norm  (o)   normalized MLT coefficients                */
/*--------------------------------------------------------------------------*/
void dqcoefs(short *y, 
             short *idxnrm,
             short *R,
             short N1,
             short N2,
             short L,
             float *coefs,
             float *coefs_norm)
{
   short i, j, n, v, rv;
   short nb_vecs, pre_idx;
   short x[8];
   float normq, factor;
   short *pidx;
   float *pcoefs, *pcoefs_norm;


   pidx = y;
   pcoefs = coefs;
   pcoefs_norm = coefs_norm;
   nb_vecs = L >> 3;
   for (n=N1; n<N2; n++)
   {
      normq = dicn[idxnrm[n]];
      v = R[n];
      if (v>1)
      {
         rv = RV[v];
         factor = FCT_LVQ2f / (float)rv;
         for (i=0; i<nb_vecs; i++)
         {
            idx2code(pidx, x, v);
            for (j=0; j<8; j++)
            {
               *pcoefs_norm = x[j] * factor + OFFSETf;
               *pcoefs++ = (*pcoefs_norm++) * normq;
            }
            pidx += 8;
         }
      }
      else if (v==1)
      {
         pre_idx = MAX16B;
         for (i=0; i<nb_vecs; i++)
         {
            if ((pre_idx<128) && (*pidx<16))
            {
              for (j=0; j<8; j++)
              {
                 *pcoefs_norm = OFFSETf;
                 *pcoefs++ = (*pcoefs_norm++) * normq;
              }
            }
            else
            {
              for (j=0; j<8; j++)
              {
                 *pcoefs_norm = (float)dic4[*pidx][j] / FCT_LVQ1f + OFFSETf;
                 *pcoefs++ = (*pcoefs_norm++) * normq;
              }
            }
            pre_idx = *pidx;
            pidx += 8;
         }
      }
      else
      {
         for (i=0; i<L; i++)
         {
            *pcoefs_norm++ = 0.0;
            *pcoefs++ = 0.0;
         }
         pidx += L;
      }
   }


   return;
}
