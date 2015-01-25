/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "cnst.h"
#include "rom.h"
#include "proto.h"

/*--------------------------------------------------------------------------*/
/*  Function  itda                                                          */
/*  ~~~~~~~~~~~~~~                                                          */
/*                                                                          */
/*  Inverse time domain aliasing                                            */
/*--------------------------------------------------------------------------*/
/*  float     *in    (i)   input vector                                     */
/*  float     *out   (o)   output vector                                    */
/*--------------------------------------------------------------------------*/
void itda(float *in, float *out)
{
   short i;

   for (i = 0; i < MLT960_LENGTH_DIV_2; i++)
   {
      out[i]                          = in[MLT960_LENGTH_DIV_2 + i];
      out[MLT960_LENGTH_DIV_2 + i]    = -in[MLT960_LENGTH - 1 - i];
      out[MLT960_LENGTH + i]          = -in[MLT960_LENGTH_DIV_2 - 1 - i];
      out[3*MLT960_LENGTH_DIV_2 + i]  = -in[i];
   }
}

/*--------------------------------------------------------------------------*/
/*  Function  imdct_short                                                   */
/*  ~~~~~~~~~~~~~~~~~~~~~                                                   */
/*                                                                          */
/*  Inverse MDCT for short frames                                           */
/*--------------------------------------------------------------------------*/
/*  float     *in    (i)   input vector                                     */
/*  float     *out   (o)   output vector                                    */
/*--------------------------------------------------------------------------*/
void imdct_short(float *in, float *out)
{
   float alias[MAX_SEGMENT_LENGTH];
   short i;

   dct4_240(in, alias);

   for (i = 0; i < MAX_SEGMENT_LENGTH/4; i++)
   {
      out[i]                          = alias[MAX_SEGMENT_LENGTH/4 + i];
      out[MAX_SEGMENT_LENGTH/4 + i]   = -alias[MAX_SEGMENT_LENGTH/2 - 1 - i];
      out[MAX_SEGMENT_LENGTH/2 + i]   = -alias[MAX_SEGMENT_LENGTH/4 - 1 - i];
      out[3*MAX_SEGMENT_LENGTH/4 + i] = -alias[i];
   }
}

/*--------------------------------------------------------------------------*/
/*  Function  inverse_transform                                             */
/*  ~~~~~~~~~~~~~~~~~~~~~~~~~~~                                             */
/*                                                                          */
/*  Inverse transform from the DCT domain to time domain                    */
/*--------------------------------------------------------------------------*/
/*  float     *in_mdct       (i)   input MDCT vector                        */
/*  float     *out           (o)   output vector                            */
/*  int       *is_transient  (o)   transient flag                           */
/*--------------------------------------------------------------------------*/
void inverse_transform(float *in_mdct, float *out, int is_transient)
{
   float out_alias[FRAME_LENGTH];
   float alias[MAX_SEGMENT_LENGTH];
   float *in_segment;
   float *out_segment;
   float tmp;
   
   short ta, seg;

   if (is_transient)  
   {   
      for (ta = 0; ta < FRAME_LENGTH; ta++) 
      {
         out_alias[ta] = 0.0f;
      }

      out_segment = out_alias - MAX_SEGMENT_LENGTH / 4;
      in_segment  = in_mdct + 0;
      
      imdct_short(in_segment, alias);

      for (ta = MAX_SEGMENT_LENGTH /4; ta < MAX_SEGMENT_LENGTH /2; ta++) 
      {
         out_segment[ta] = alias[ta];
      }

      for (ta = MAX_SEGMENT_LENGTH /2; ta < MAX_SEGMENT_LENGTH; ta++) 
      {
         out_segment[ta] = alias[ta]*short_window[ta];
      }
                  
      out_segment = out_segment + MAX_SEGMENT_LENGTH/2;
      in_segment  = in_segment  + MAX_SEGMENT_LENGTH/2;

      for (seg = 1 ; seg <  NUM_TIME_SWITCHING_BLOCKS-1; seg++) 
      {
         imdct_short(in_segment, alias);

         for (ta = 0; ta < MAX_SEGMENT_LENGTH; ta++) 
         {
            out_segment[ta] = out_segment[ta] + alias[ta] * short_window[ta];
         }

         in_segment  = in_segment  + MAX_SEGMENT_LENGTH/2;
         out_segment = out_segment + MAX_SEGMENT_LENGTH/2;
      } 

      imdct_short(in_segment, alias);

      for (ta = 0; ta < MAX_SEGMENT_LENGTH /2; ta++) 
      {
         out_segment[ta] = out_segment[ta] + alias[ta] * short_window[ta];
      }

      for (ta = MAX_SEGMENT_LENGTH /2; ta < MAX_SEGMENT_LENGTH /4+MAX_SEGMENT_LENGTH /2; ta++) 
      {
         out_segment[ta] = alias[ta];
      }
            
      for (ta = 0; ta < FRAME_LENGTH/2; ta++) 
      {
         tmp          = out_alias[ta];
         out_alias[ta] = out_alias[FRAME_LENGTH-1-ta];
         out_alias[FRAME_LENGTH-1-ta] = tmp;
      }
   }
   else
   {
      dct4_960(in_mdct, out_alias);
   }

   itda(out_alias, out);
}
