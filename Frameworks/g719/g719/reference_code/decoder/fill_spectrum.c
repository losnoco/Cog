/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "cnst.h"
#include "state.h"
#include "rom.h"
#include "proto.h"
#include <math.h>

/*--------------------------------------------------------------------------*/
/*  Function  fill_spectrum                                                 */
/*  ~~~~~~~~~~~~~~~~~~~~~~~                                                 */
/*                                                                          */
/*  Fill the spectrum which has been quantized with 0 bits                  */
/*--------------------------------------------------------------------------*/
/*  float     *coeff        (i)   normalized MLT coefficients               */
/*  float     *coeff_out    (o)   MLT coefficients                          */
/*  short     *R            (i)   number of bits per coefficient            */
/*  short     is_transient  (i)   transient flag                            */
/*  short     norm          (i)   quantization indices for norms            */
/*  short     nf_idx        (i)   noise fill index                          */
/*--------------------------------------------------------------------------*/
void fill_spectrum(float *coeff, 
                   float *coeff_out, 
                   short *R, 
                   short is_transient, 
                   short norm[],
                   short nf_idx)
{
   float CodeBook[FREQ_LENGTH];
   float *src, *dst, *end;
   float normq;
   short cb_size, cb_pos;
   short sfm, j, i;
   
   short last_sfm;
   short first_coeff;
   short low_coeff;

   /* Build codebook */
   cb_size = 0;
   for (sfm = 0; sfm < NB_SFM; sfm++)
   {
      if (R[sfm] != 0)
      {
         for (j = sfm_start[sfm]; j < sfm_end[sfm]; j++)
         {
            CodeBook[cb_size] = coeff[j];
            cb_size++;
         }
      }
   }

   /* detect last sfm */
   last_sfm = NB_SFM - 1;
   if (is_transient == 0)
   {
      for (sfm = NB_SFM-1; sfm >= 0; sfm--) 
      {
         if (R[sfm] != 0) 
         {
            last_sfm = sfm;
            break;
         }
      }
   }

   if (cb_size != 0) 
   {
      /* Read from codebook */
      cb_pos = 0;
      for (sfm = 0; sfm <= last_sfm; sfm++)
      {
         if (R[sfm] == 0)
         {
            for (j = sfm_start[sfm]; j < sfm_end[sfm]; j++)
            {
               coeff[j] = CodeBook[cb_pos];
               cb_pos++;
               cb_pos = (cb_pos>=cb_size) ? 0 : cb_pos;
            }
         }
      }

      if (is_transient == 0)
      {
         low_coeff = sfm_end[last_sfm] >> 1;
         src       = coeff + sfm_end[last_sfm] - 1;

         first_coeff = sfm_end[last_sfm];
         dst = coeff + sfm_end[last_sfm];
         end = coeff + sfm_end[NB_SFM-1];

         while (dst < end) 
         {
            while (dst < end && src >= &coeff[low_coeff]) 
            {
               *dst++ = *src--;
            }

            src++;

            while (dst < end && src < &coeff[first_coeff]) 
            {
               *dst++ = *src++;
            }
         }
      }
      
      for (sfm = 0; sfm <= last_sfm; sfm++)
      {
         if (R[sfm] == 0)
         {
            for (j = sfm_start[sfm]; j < sfm_end[sfm]; j++)
            {
               /* Scale NoiseFill */
               coeff[j] = (float)(coeff[j]/pow(2,nf_idx));
            }
         }
      }
   }

   /*  shape the spectrum */
   for (sfm = 0; sfm < NB_SFM; sfm++) 
   {
      normq = dicn[norm[sfm]];
      for (i = sfm_start[sfm]; i < sfm_end[sfm]; i++)
      {
         coeff_out[i] = coeff[i]*normq;
      }
   }
}
