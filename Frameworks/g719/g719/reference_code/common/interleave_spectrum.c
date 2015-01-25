/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "cnst.h"

/*--------------------------------------------------------------------------*/
/*  Function  interleave_spectrum                                           */
/*  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                                           */
/*                                                                          */
/*  Interleave the spectrum                                                 */
/*--------------------------------------------------------------------------*/
/*  float     *coefs  (i/o)   input and output coefficients                 */
/*--------------------------------------------------------------------------*/
void interleave_spectrum(float *coefs)
{
   short   i, j;
   float   *p1a, *p1b, *p2a, *p2b, *p3a, *p3b, *p3c, *p4a, *p4b;
   float   coefs_short[STOP_BAND];
   float   *pcoefs, *pcoefs1, *pcoefs2;

   p1a    = coefs_short;
   pcoefs = coefs;

   for (i = 0; i < 4; i++) 
   {      
      for (j = 0; j < STOP_BAND4; j++) 
      {
         p1a[j] = pcoefs[j];
      }
      p1a += STOP_BAND4;
      pcoefs += FRAME_LENGTH/4;   
   }

   p1a = coefs;
   p1b = coefs + 64;
   p2a = coefs + NUMC_G1;
   p2b = coefs + NUMC_G1 + 64;
   p3a = coefs + NUMC_G23;
   p3b = coefs + NUMC_G23 + 96;
   p3c = coefs + NUMC_G23 + 192;
   p4a = coefs + NUMC_N;
   p4b = coefs + NUMC_N + 128;
   for (i = 0; i < STOP_BAND; i += STOP_BAND4)
   {
      pcoefs  = coefs_short + i;
      pcoefs1 = coefs_short + 16 + i;
      for (j = 0; j < 16; j++)
      {
         *p1a++ = *pcoefs++;
         *p1b++ = *pcoefs1++;
      }

      pcoefs  = coefs_short + NUMC_G1SUB + i;
      pcoefs1 = coefs_short + NUMC_G1SUB + 16 + i;
      for (j = 0; j < 16; j++)
      {
         *p2a++ = *pcoefs++;
         *p2b++ = *pcoefs1++;
      }

      pcoefs  = coefs_short + NUMC_G1G2SUB + i;
      pcoefs1 = coefs_short + NUMC_G1G2SUB + WID_G3 + i;
      pcoefs2 = coefs_short + NUMC_G1G2SUB + 2 * WID_G3 + i;
      for (j = 0; j < WID_G3; j++)
      {
         *p3a++ = *pcoefs++;
         *p3b++ = *pcoefs1++;
         *p3c++ = *pcoefs2++;
      }

      pcoefs  = coefs_short + NUMC_G1G2G3SUB + i;
      pcoefs1 = coefs_short + NUMC_G1G2G3SUB + WID_GX + i;
      for (j = 0; j < WID_GX; j++)
      {
         *p4a++ = *pcoefs++;
         *p4b++ = *pcoefs1++;
      }
   }

}

/*--------------------------------------------------------------------------*/
/*  Function  de_interleave_spectrum                                        */
/*  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                                        */
/*                                                                          */
/*  Deinterleave the spectrum                                               */
/*--------------------------------------------------------------------------*/
/*  float     *coefs  (i/o)   input and output coefficients                 */
/*--------------------------------------------------------------------------*/
void de_interleave_spectrum(float *coefs)
{
   short   i, j;
   float   coefs_short[STOP_BAND];
   float   *p1a, *p1b, *p2a, *p2b, *p3a, *p3b, *p3c, *p4a, *p4b;
   float   *pcoefs, *pcoefs1, *pcoefs2;


   p1a = coefs;
   p1b = coefs + 64;
   p2a = coefs + NUMC_G1;
   p2b = coefs + NUMC_G1 + 64;
   p3a = coefs + NUMC_G23;
   p3b = coefs + NUMC_G23 + 96;
   p3c = coefs + NUMC_G23 + 192;
   p4a = coefs + NUMC_N;
   p4b = coefs + NUMC_N + 128;
   for (i=0; i<STOP_BAND; i+=STOP_BAND4)
   {
      pcoefs  = coefs_short + i;
      pcoefs1 = coefs_short + 16 + i;
      for (j=0; j<16; j++)
      {
         *pcoefs++  = *p1a++;
         *pcoefs1++ = *p1b++;
      }

      pcoefs  = coefs_short + NUMC_G1SUB + i;
      pcoefs1 = coefs_short + NUMC_G1SUB + 16 + i;
      for (j=0; j<16; j++)
      {
         *pcoefs++  = *p2a++;
         *pcoefs1++ = *p2b++;
      }

      pcoefs  = coefs_short + NUMC_G1G2SUB + i;
      pcoefs1 = coefs_short + NUMC_G1G2SUB + WID_G3 + i;
      pcoefs2 = coefs_short + NUMC_G1G2SUB + 2 * WID_G3 + i;
      for (j=0; j<WID_G3; j++)
      {
         *pcoefs++  = *p3a++;
         *pcoefs1++ = *p3b++;
         *pcoefs2++ = *p3c++;
      }

      pcoefs  = coefs_short + NUMC_G1G2G3SUB + i;
      pcoefs1 = coefs_short + NUMC_G1G2G3SUB + WID_GX + i;
      for (j=0; j<WID_GX; j++)
      {
         *pcoefs++  = *p4a++;
         *pcoefs1++ = *p4b++;
      }
   }


   /* unpack the spectrum */
   p1a    = coefs_short;
   pcoefs = coefs;
   for (i=0; i<4; i++) 
   {      
      for (j=0; j<STOP_BAND4; j++) 
      {
         pcoefs[j] = p1a[j];
      }
      for (j=STOP_BAND4; j<FRAME_LENGTH/4; j++) 
      {
         pcoefs[j] = 0;
      }
      p1a += STOP_BAND4;
      pcoefs += FRAME_LENGTH/4;   
   }

}
