/* Copyright  (C) 2010-2020 The RetroArch team
 *
 * ---------------------------------------------------------------------------------------
 * The following license statement only applies to this file (sinc_resampler.c).
 * ---------------------------------------------------------------------------------------
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Bog-standard windowed SINC implementation. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <retro_environment.h>
#include <retro_inline.h>
#include <filters.h>
#include <memalign.h>

#include <audio/audio_resampler.h>
#include <filters.h>

#ifdef __SSE__
#include <xmmintrin.h>
#endif

#if defined(__AVX__)
#include <immintrin.h>
#endif

/* Rough SNR values for upsampling:
 * LOWEST: 40 dB
 * LOWER: 55 dB
 * NORMAL: 70 dB
 * HIGHER: 110 dB
 * HIGHEST: 140 dB
 */

/* TODO, make all this more configurable. */

enum sinc_window
{
   SINC_WINDOW_NONE   = 0,
   SINC_WINDOW_KAISER,
   SINC_WINDOW_LANCZOS
};

/* For the little amount of taps we're using,
 * SSE1 is faster than AVX for some reason.
 * AVX code is kept here though as by increasing number
 * of sinc taps, the AVX code is clearly faster than SSE1.
 */

typedef struct rarch_sinc_resampler
{
   /* A buffer for phase_table, buffer_l and buffer_r
    * are created in a single calloc().
    * Ensure that we get as good cache locality as we can hope for. */
   float *main_buffer;
   float *phase_table;
   float **buffer;
   unsigned phase_bits;
   unsigned subphase_bits;
   unsigned subphase_mask;
   unsigned taps;
   unsigned ptr;
   size_t channels;
   uint32_t time;
   float subphase_mod;
   float kaiser_beta;
} rarch_sinc_resampler_t;

#if (defined(__ARM_NEON__) || defined(HAVE_NEON))

#include <arm_neon.h>

/* Assumes that taps >= 8, and that taps is a multiple of 8.
 * Not bothering to reimplement this one for the external .S
 */
static void resampler_sinc_process_neon_kaiser(void *re_, struct resampler_data *data)
{
   rarch_sinc_resampler_t *resamp = (rarch_sinc_resampler_t*)re_;
   unsigned phases                = 1 << (resamp->phase_bits + resamp->subphase_bits);
   uint32_t ratio                 = phases / data->ratio;
   const float *input             = data->data_in;
   float *output                  = data->data_out;
   size_t frames                  = data->input_frames;
   size_t out_frames              = 0;
   size_t channel;
   size_t channels                = resamp->channels;
   while (frames)
   {
      while (frames && resamp->time >= phases)
      {
         /* Push in reverse to make filter more obvious. */
         if (!resamp->ptr)
            resamp->ptr = resamp->taps;
         resamp->ptr--;

         for (channel = 0; channel < channels; ++channel)
         {
            resamp->buffer[channel][resamp->ptr + resamp->taps] =
               resamp->buffer[channel][resamp->ptr]             = *input++;
         }

         resamp->time                                -= phases;
         frames--;
      }

      {
         const float *buffer[channels];
         
         for (channel = 0; channel < channels; ++channel)
         {
            buffer[channel] = resamp->buffer[channel] + resamp->ptr;
         }
         unsigned taps = resamp->taps;
         while (resamp->time < phases)
         {
            unsigned phase           = resamp->time >> resamp->subphase_bits;
            const float *phase_table = resamp->phase_table + phase * taps * 2;
            const float *delta_table = phase_table + taps;
            float32x4_t delta        = vdupq_n_f32((resamp->time & resamp->subphase_mask) * resamp->subphase_mod);
            unsigned i;
            float32x4_t outp[channels];
            for (channel = 0; channel < channels; channel++)
            {
               outp[channel] = vdupq_n_f32(0);
            }

            for (i = 0; i < taps; i += 8)
            {
               float32x4x2_t coeff8  = vld2q_f32(&phase_table[i]);
               float32x4x2_t delta8  = vld2q_f32(&delta_table[i]);

               coeff8.val[0] = vmlaq_f32(coeff8.val[0], delta8.val[0], delta);
               coeff8.val[1] = vmlaq_f32(coeff8.val[1], delta8.val[1], delta);
                
               for (channel = 0; channel < channels; ++channel)
               {
                   float32x4x2_t samples = vld2q_f32(&buffer[channel][i]);
                   outp[channel] = vmlaq_f32(outp[channel], samples.val[0], coeff8.val[0]);
                   outp[channel] = vmlaq_f32(outp[channel], samples.val[1], coeff8.val[1]);
               }
            }

            for (channel = 0; channel < channels; ++channel)
            {
               float32x2_t r = vadd_f32(vget_low_f32(outp[channel]), vget_high_f32(outp[channel]));
               output[channel] = vget_lane_f32(vpadd_f32(r, r), 0);
            }
            output                 += channels;
            out_frames++;
            resamp->time           += ratio;
         }
      }
   }

   data->output_frames = out_frames;
}

/* Assumes that taps >= 8, and that taps is a multiple of 8. */
static void resampler_sinc_process_neon(void *re_, struct resampler_data *data)
{
   rarch_sinc_resampler_t *resamp = (rarch_sinc_resampler_t*)re_;
   unsigned phases                = 1 << (resamp->phase_bits + resamp->subphase_bits);
   uint32_t ratio                 = phases / data->ratio;
   const float *input             = data->data_in;
   float *output                  = data->data_out;
   size_t frames                  = data->input_frames;
   size_t out_frames              = 0;
   size_t channel;
   size_t channels                = resamp->channels;
   while (frames)
   {
      while (frames && resamp->time >= phases)
      {
         /* Push in reverse to make filter more obvious. */
         if (!resamp->ptr)
            resamp->ptr = resamp->taps;
         resamp->ptr--;

         for (channel = 0; channel < channels; ++channel)
         {
            resamp->buffer[channel][resamp->ptr + resamp->taps] =
               resamp->buffer[channel][resamp->ptr]                = *input++;
         }

         resamp->time                                -= phases;
         frames--;
      }

      {
         const float *buffer[channels];
         for (channel = 0; channel < channels; ++channel)
         {
            buffer[channel] = resamp->buffer[channel] + resamp->ptr;
         }
         unsigned taps            = resamp->taps;
         while (resamp->time < phases)
         {
            unsigned phase           = resamp->time >> resamp->subphase_bits;
            const float *phase_table = resamp->phase_table + phase * taps;
            unsigned i;
            float32x4_t outp[channels];
            for (channel = 0; channel < channels; channel++)
            {
               outp[channel] = vdupq_n_f32(0);
            }

            for (i = 0; i < taps; i += 8)
            {
               float32x4x2_t coeff8  = vld2q_f32(&phase_table[i]);
                
               for (channel = 0; channel < channels; ++channel)
               {
                  float32x4x2_t sample  = vld2q_f32(&buffer[channel][i]);
                   
                  outp[channel] = vmlaq_f32(outp[channel], sample.val[0], coeff8.val[0]);
                  outp[channel] = vmlaq_f32(outp[channel], sample.val[1], coeff8.val[1]);
               }
            }

            for (channel = 0; channel < channels; ++channel)
            {
               float32x2_t sample = vadd_f32(vget_low_f32(outp[channel]), vget_high_f32(outp[channel]));
               output[channel] = vget_lane_f32(vpadd_f32(sample, sample), 0);
            }
            output                 += channels;
            out_frames++;
            resamp->time           += ratio;
         }
      }
   }

   data->output_frames = out_frames;
}
#endif

#if defined(__AVX__)
#pragma clang attribute push (__attribute__((target("avx"))), apply_to=function)
static void resampler_sinc_process_avx_kaiser(void *re_, struct resampler_data *data)
{
   rarch_sinc_resampler_t *resamp = (rarch_sinc_resampler_t*)re_;
   unsigned phases                = 1 << (resamp->phase_bits + resamp->subphase_bits);

   uint32_t ratio                 = phases / data->ratio;
   const float *input             = data->data_in;
   float *output                  = data->data_out;
   size_t frames                  = data->input_frames;
   size_t out_frames              = 0;
   size_t channel;
   size_t channels                = resamp->channels;

   {
      while (frames)
      {
         while (frames && resamp->time >= phases)
         {
            /* Push in reverse to make filter more obvious. */
            if (!resamp->ptr)
               resamp->ptr = resamp->taps;
            resamp->ptr--;

            for (channel = 0; channel < channels; ++channel)
            {
               resamp->buffer[channel][resamp->ptr + resamp->taps] =
                  resamp->buffer[channel][resamp->ptr]             = *input++;
            }

            resamp->time                                -= phases;
            frames--;
         }

         {
            const float *buffer[channels];
            for (channel = 0; channel < channels; ++channel)
            {
               buffer[channel] = resamp->buffer[channel] + resamp->ptr;
            }
            unsigned taps            = resamp->taps;
            while (resamp->time < phases)
            {
               unsigned i;
               unsigned phase           = resamp->time >> resamp->subphase_bits;

               float *phase_table       = resamp->phase_table + phase * taps * 2;
               float *delta_table       = phase_table + taps;
               __m256 delta             = _mm256_set1_ps((float)
                     (resamp->time & resamp->subphase_mask) * resamp->subphase_mod);

               __m256 sums[channels];
               for (channel = 0; channel < channels; channel++)
               {
                  sums[channel] = _mm256_setzero_ps();
               }

               for (i = 0; i < taps; i += 8)
               {
                  __m256 deltas = _mm256_load_ps(delta_table + i);
                  __m256 sinc   = _mm256_add_ps(_mm256_load_ps((const float*)phase_table + i),
                        _mm256_mul_ps(deltas, delta));

                  for (channel = 0; channel < channels; ++channel)
                  {
                     __m256 buf = _mm256_loadu_ps(buffer[channel] + i);
                     sums[channel] = _mm256_add_ps(sums[channel], _mm256_mul_ps(buf, sinc));
                  }
               }

               /* hadd on AVX is weird, and acts on low-lanes
                * and high-lanes separately. */
               for (channel = 0; channel < channels; ++channel)
               {
                  __m256 res = _mm256_hadd_ps(sums[channel], sums[channel]);
                  res        = _mm256_hadd_ps(res, res);
                  res        = _mm256_add_ps(_mm256_permute2f128_ps(res, res, 1), res);

                  /* This is optimized to mov %xmmN, [mem].
                   * There doesn't seem to be any _mm256_store_ss intrinsic. */
                  _mm_store_ss(output + channel, _mm256_extractf128_ps(res, 0));
               }

               output += channels;
               out_frames++;
               resamp->time += ratio;
            }
         }
      }
   }

   data->output_frames = out_frames;
}

static void resampler_sinc_process_avx(void *re_, struct resampler_data *data)
{
   rarch_sinc_resampler_t *resamp = (rarch_sinc_resampler_t*)re_;
   unsigned phases                = 1 << (resamp->phase_bits + resamp->subphase_bits);

   uint32_t ratio                 = phases / data->ratio;
   const float *input             = data->data_in;
   float *output                  = data->data_out;
   size_t frames                  = data->input_frames;
   size_t out_frames              = 0;
   size_t channel;
   size_t channels                = resamp->channels;

   {
      while (frames)
      {
         while (frames && resamp->time >= phases)
         {
            /* Push in reverse to make filter more obvious. */
            if (!resamp->ptr)
               resamp->ptr = resamp->taps;
            resamp->ptr--;

            for (channel = 0; channel < channels; ++channel)
            {
               resamp->buffer[channel][resamp->ptr + resamp->taps] =
                  resamp->buffer[channel][resamp->ptr]             = *input++;
            }

            resamp->time                                -= phases;
            frames--;
         }

         {
            const float *buffer[channels];
            for (channel = 0; channel < channels; ++channel)
            {
               buffer[channel] = resamp->buffer[channel] + resamp->ptr;
            }
            unsigned taps            = resamp->taps;
            while (resamp->time < phases)
            {
               unsigned i;
               __m256 delta;
               unsigned phase           = resamp->time >> resamp->subphase_bits;
               float *phase_table       = resamp->phase_table + phase * taps;

               __m256 sums[channels];
               for (channel = 0; channel < channels; channel++)
               {
                  sums[channel] = _mm256_setzero_ps();
               }

               for (i = 0; i < taps; i += 8)
               {
                  __m256 sinc   = _mm256_load_ps((const float*)phase_table + i);

                  for (channel = 0; channel < channels; ++channel)
                  {
                     __m256 buf = _mm256_loadu_ps(buffer[channel] + i);
                     sums[channel] = _mm256_add_ps(sums[channel], _mm256_mul_ps(buf, sinc));
                  }
               }

               /* hadd on AVX is weird, and acts on low-lanes
                * and high-lanes separately. */
               for (channel = 0; channel < channels; ++channel)
               {
                  __m256 res = _mm256_hadd_ps(sums[channel], sums[channel]);
                  res        = _mm256_hadd_ps(res, res);
                  res        = _mm256_add_ps(_mm256_permute2f128_ps(res, res, 1), res);

                  /* This is optimized to mov %xmmN, [mem].
                   * There doesn't seem to be any _mm256_store_ss intrinsic. */
                  _mm_store_ss(output + channel, _mm256_extractf128_ps(res, 0));
               }

               output += channels;
               out_frames++;
               resamp->time += ratio;
            }
         }
      }
   }

   data->output_frames = out_frames;
}
#pragma clang attribute pop
#endif

#if defined(__SSE__)
static void resampler_sinc_process_sse_kaiser(void *re_, struct resampler_data *data)
{
   rarch_sinc_resampler_t *resamp = (rarch_sinc_resampler_t*)re_;
   unsigned phases                = 1 << (resamp->phase_bits + resamp->subphase_bits);

   uint32_t ratio                 = phases / data->ratio;
   const float *input             = data->data_in;
   float *output                  = data->data_out;
   size_t frames                  = data->input_frames;
   size_t out_frames              = 0;
   size_t channel;
   size_t channels                = resamp->channels;

   {
      while (frames)
      {
         while (frames && resamp->time >= phases)
         {
            /* Push in reverse to make filter more obvious. */
            if (!resamp->ptr)
               resamp->ptr = resamp->taps;
            resamp->ptr--;

            for (channel = 0; channel < channels; ++channel)
            {
               resamp->buffer[channel][resamp->ptr + resamp->taps] =
                  resamp->buffer[channel][resamp->ptr]             = *input++;
            }

            resamp->time                                -= phases;
            frames--;
         }

         {
            const float *buffer[channels];
            for (channel = 0; channel < channels; ++channel)
            {
               buffer[channel] = resamp->buffer[channel] + resamp->ptr;
            }
            unsigned taps            = resamp->taps;
            while (resamp->time < phases)
            {
               unsigned i;
               unsigned phase           = resamp->time >> resamp->subphase_bits;
               float *phase_table       = resamp->phase_table + phase * taps * 2;
               float *delta_table       = phase_table + taps;
               __m128 delta             = _mm_set1_ps((float)
                     (resamp->time & resamp->subphase_mask) * resamp->subphase_mod);

               __m128 sums[channels];
               for (channel = 0; channel < channels; channel++)
               {
                  sums[channel] = _mm_setzero_ps();
               }

               for (i = 0; i < taps; i += 4)
               {
                  __m128 deltas = _mm_load_ps(delta_table + i);
                  __m128 _sinc  = _mm_add_ps(_mm_load_ps((const float*)phase_table + i),
                        _mm_mul_ps(deltas, delta));
                  for (channel = 0; channel < channels; ++channel)
                  {
                     __m128 buf = _mm_loadu_ps(buffer[channel] + i);
                     sums[channel] = _mm_add_ps(sums[channel], _mm_mul_ps(buf, _sinc));
                  }
               }
                
               for (channel = 0; channel < channels; ++channel)
               {
                  __m128 v = sums[channel];
                  __m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1));
                  __m128 sums = _mm_add_ps(v, shuf);
                  shuf = _mm_movehl_ps(shuf, sums);
                  sums = _mm_add_ps(sums, shuf);
                  output[channel] = _mm_cvtss_f32(sums);
               }

               output += channels;
               out_frames++;
               resamp->time += ratio;
            }
         }
      }
   }

   data->output_frames = out_frames;
}

static void resampler_sinc_process_sse(void *re_, struct resampler_data *data)
{
   rarch_sinc_resampler_t *resamp = (rarch_sinc_resampler_t*)re_;
   unsigned phases                = 1 << (resamp->phase_bits + resamp->subphase_bits);

   uint32_t ratio                 = phases / data->ratio;
   const float *input             = data->data_in;
   float *output                  = data->data_out;
   size_t frames                  = data->input_frames;
   size_t out_frames              = 0;
   size_t channel;
   size_t channels                = resamp->channels;

   {
      while (frames)
      {
         while (frames && resamp->time >= phases)
         {
            /* Push in reverse to make filter more obvious. */
            if (!resamp->ptr)
               resamp->ptr = resamp->taps;
            resamp->ptr--;

            for (channel = 0; channel < channels; ++channel)
            {
               resamp->buffer[channel][resamp->ptr + resamp->taps] =
                  resamp->buffer[channel][resamp->ptr]             = *input++;
            }

            resamp->time                                -= phases;
            frames--;
         }

         {
            const float *buffer[channels];
            for (channel = 0; channel < channels; ++channel)
            {
               buffer[channel] = resamp->buffer[channel] + resamp->ptr;
            }
            unsigned taps            = resamp->taps;
            while (resamp->time < phases)
            {
               unsigned i;
               unsigned phase           = resamp->time >> resamp->subphase_bits;
               float *phase_table       = resamp->phase_table + phase * taps;

               __m128 sums[channels];
               for (channel = 0; channel < channels; channel++)
               {
                  sums[channel] = _mm_setzero_ps();
               }

               for (i = 0; i < taps; i += 4)
               {
                  __m128 _sinc = _mm_load_ps((const float*)phase_table + i);
                  for (channel = 0; channel < channels; ++channel)
                  {
                     __m128 buf = _mm_loadu_ps(buffer[channel] + i);
                     sums[channel] = _mm_add_ps(sums[channel], _mm_mul_ps(buf, _sinc));
                  }
               }

               for (channel = 0; channel < channels; ++channel)
               {
                  __m128 v = sums[channel];
                  __m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1));
                  __m128 sums = _mm_add_ps(v, shuf);
                  shuf = _mm_movehl_ps(shuf, sums);
                  sums = _mm_add_ps(sums, shuf);
                  output[channel] = _mm_cvtss_f32(sums);
               }

               output += 2;
               out_frames++;
               resamp->time += ratio;
            }
         }
      }
   }

   data->output_frames = out_frames;
}
#endif

static void resampler_sinc_process_c_kaiser(void *re_, struct resampler_data *data)
{
   rarch_sinc_resampler_t *resamp = (rarch_sinc_resampler_t*)re_;
   unsigned phases                = 1 << (resamp->phase_bits + resamp->subphase_bits);

   uint32_t ratio                 = phases / data->ratio;
   const float *input             = data->data_in;
   float *output                  = data->data_out;
   size_t frames                  = data->input_frames;
   size_t out_frames              = 0;
   size_t channel;
   size_t channels                = resamp->channels;

   {
      while (frames)
      {
         while (frames && resamp->time >= phases)
         {
            /* Push in reverse to make filter more obvious. */
            if (!resamp->ptr)
               resamp->ptr = resamp->taps;
            resamp->ptr--;

            for (channel = 0; channel < channels; channel++)
            {
               resamp->buffer[channel][resamp->ptr + resamp->taps]    =
                  resamp->buffer[channel][resamp->ptr]                = *input++;
            }

            resamp->time                                   -= phases;
            frames--;
         }

         {
            const float *buffer[channels];
            for (channel = 0; channel < channels; channel++)
            {
               buffer[channel] = resamp->buffer[channel] + resamp->ptr;
            }
            unsigned taps            = resamp->taps;
            while (resamp->time < phases)
            {
               unsigned i;
               float sums[channels];
               unsigned phase           = resamp->time >> resamp->subphase_bits;
               float *phase_table       = resamp->phase_table + phase * taps * 2;
               float *delta_table       = phase_table + taps;
               float delta              = (float)
                  (resamp->time & resamp->subphase_mask) * resamp->subphase_mod;
                
               for (channel = 0; channel < channels; channel++)
               {
                  sums[channel] = 0.0f;
               }

               for (i = 0; i < taps; i++)
               {
                  float sinc_val        = phase_table[i] + delta_table[i] * delta;

                  for (channel = 0; channel < channels; channel++)
                  {
                     sums[channel]     += buffer[channel][i] * sinc_val;
                  }
               }

               for (channel = 0; channel < channels; channel++)
               {
                  output[channel] = sums[channel];
               }

               output                  += channels;
               out_frames++;
               resamp->time            += ratio;
            }
         }

      }
   }

   data->output_frames = out_frames;
}

static void resampler_sinc_process_c(void *re_, struct resampler_data *data)
{
   rarch_sinc_resampler_t *resamp = (rarch_sinc_resampler_t*)re_;
   unsigned phases                = 1 << (resamp->phase_bits + resamp->subphase_bits);

   uint32_t ratio                 = phases / data->ratio;
   const float *input             = data->data_in;
   float *output                  = data->data_out;
   size_t frames                  = data->input_frames;
   size_t out_frames              = 0;
   size_t channel;
   size_t channels                = resamp->channels;

   {
      while (frames)
      {
         while (frames && resamp->time >= phases)
         {
            /* Push in reverse to make filter more obvious. */
            if (!resamp->ptr)
               resamp->ptr = resamp->taps;
            resamp->ptr--;

            for (channel = 0; channel < channels; channel++)
            {
               resamp->buffer[channel][resamp->ptr + resamp->taps]    =
                  resamp->buffer[channel][resamp->ptr]                = *input++;
            }

            resamp->time                                   -= phases;
            frames--;
         }

         {
            const float *buffer[channels];
            for (channel = 0; channel < channels; channel++)
            {
               buffer[channel] = resamp->buffer[channel] + resamp->ptr;
            }
            unsigned taps            = resamp->taps;
            while (resamp->time < phases)
            {
               unsigned i;
               float sums[channels];
               unsigned phase           = resamp->time >> resamp->subphase_bits;
               float *phase_table       = resamp->phase_table + phase * taps;

               for (channel = 0; channel < channels; channel++)
               {
                  sums[channel] = 0.0f;
               }

               for (i = 0; i < taps; i++)
               {
                  float sinc_val        = phase_table[i];

                  for (channel = 0; channel < channels; channel++)
                  {
                     sums[channel]     += buffer[channel][i] * sinc_val;
                  }
               }

               for (channel = 0; channel < channels; channel++)
               {
                  output[channel]       = sums[channel];
               }

               output                  += channels;
               out_frames++;
               resamp->time            += ratio;
            }
         }

      }
   }

   data->output_frames = out_frames;
}

static size_t resampler_sinc_latency(void *data)
{
   rarch_sinc_resampler_t *resamp = (rarch_sinc_resampler_t*)data;
   return resamp->taps / 2;
}

static void resampler_sinc_free(const retro_resampler_t *resampler, void *data)
{
   rarch_sinc_resampler_t *resamp = (rarch_sinc_resampler_t*)data;
   if (resamp) {
      memalign_free(resamp->main_buffer);
      free(resamp->buffer);
   }
   free(resamp);
   if (resampler && resampler != &sinc_resampler) {
      free((void*)resampler);
   }
}

static void sinc_init_table_kaiser(rarch_sinc_resampler_t *resamp,
      double cutoff,
      float *phase_table, int phases, int taps, bool calculate_delta)
{
   int i, j;
   double    window_mod = kaiser_window_function(0.0, resamp->kaiser_beta); /* Need to normalize w(0) to 1.0. */
   int           stride = calculate_delta ? 2 : 1;
   double     sidelobes = taps / 2.0;

   for (i = 0; i < phases; i++)
   {
      for (j = 0; j < taps; j++)
      {
         double sinc_phase;
         float val;
         int               n = j * phases + i;
         double window_phase = (double)n / (phases * taps); /* [0, 1). */
         window_phase        = 2.0 * window_phase - 1.0; /* [-1, 1) */
         sinc_phase          = sidelobes * window_phase;
         val                 = cutoff * sinc(M_PI * sinc_phase * cutoff) *
            kaiser_window_function(window_phase, resamp->kaiser_beta) / window_mod;
         phase_table[i * stride * taps + j] = val;
      }
   }

   if (calculate_delta)
   {
      int phase;
      int p;

      for (p = 0; p < phases - 1; p++)
      {
         for (j = 0; j < taps; j++)
         {
            float delta = phase_table[(p + 1) * stride * taps + j] -
               phase_table[p * stride * taps + j];
            phase_table[(p * stride + 1) * taps + j] = delta;
         }
      }

      phase = phases - 1;
      for (j = 0; j < taps; j++)
      {
         float val, delta;
         double sinc_phase;
         int n               = j * phases + (phase + 1);
         double window_phase = (double)n / (phases * taps); /* (0, 1]. */
         window_phase        = 2.0 * window_phase - 1.0; /* (-1, 1] */
         sinc_phase          = sidelobes * window_phase;

         val                 = cutoff * sinc(M_PI * sinc_phase * cutoff) *
            kaiser_window_function(window_phase, resamp->kaiser_beta) / window_mod;
         delta = (val - phase_table[phase * stride * taps + j]);
         phase_table[(phase * stride + 1) * taps + j] = delta;
      }
   }
}

static void sinc_init_table_lanczos(
      rarch_sinc_resampler_t *resamp, double cutoff,
      float *phase_table, int phases, int taps, bool calculate_delta)
{
   int i, j;
   double    window_mod = lanzcos_window_function(0.0); /* Need to normalize w(0) to 1.0. */
   int           stride = calculate_delta ? 2 : 1;
   double     sidelobes = taps / 2.0;

   for (i = 0; i < phases; i++)
   {
      for (j = 0; j < taps; j++)
      {
         double sinc_phase;
         float val;
         int               n = j * phases + i;
         double window_phase = (double)n / (phases * taps); /* [0, 1). */
         window_phase        = 2.0 * window_phase - 1.0; /* [-1, 1) */
         sinc_phase          = sidelobes * window_phase;
         val                 = cutoff * sinc(M_PI * sinc_phase * cutoff) *
            lanzcos_window_function(window_phase) / window_mod;
         phase_table[i * stride * taps + j] = val;
      }
   }

   if (calculate_delta)
   {
      int phase;
      int p;

      for (p = 0; p < phases - 1; p++)
      {
         for (j = 0; j < taps; j++)
         {
            float delta = phase_table[(p + 1) * stride * taps + j] -
               phase_table[p * stride * taps + j];
            phase_table[(p * stride + 1) * taps + j] = delta;
         }
      }

      phase = phases - 1;
      for (j = 0; j < taps; j++)
      {
         float val, delta;
         double sinc_phase;
         int n               = j * phases + (phase + 1);
         double window_phase = (double)n / (phases * taps); /* (0, 1]. */
         window_phase        = 2.0 * window_phase - 1.0; /* (-1, 1] */
         sinc_phase          = sidelobes * window_phase;

         val                 = cutoff * sinc(M_PI * sinc_phase * cutoff) *
            lanzcos_window_function(window_phase) / window_mod;
         delta = (val - phase_table[phase * stride * taps + j]);
         phase_table[(phase * stride + 1) * taps + j] = delta;
      }
   }
}

static void *resampler_sinc_new(const struct resampler_config *config,
      const retro_resampler_t **_resampler, double bandwidth_mod,
      enum resampler_quality quality, size_t channels, resampler_simd_mask_t mask)
{
   double cutoff                  = 0.0;
   size_t phase_elems             = 0;
   size_t elems                   = 0;
   size_t channel;
   unsigned enable_avx            = 0;
   unsigned sidelobes             = 0;
   enum sinc_window window_type   = SINC_WINDOW_NONE;
   rarch_sinc_resampler_t *re     = (rarch_sinc_resampler_t*)
      calloc(1, sizeof(*re));
   
   retro_resampler_t *resampler   = (retro_resampler_t*)*_resampler;

   if (!re)
      return NULL;

   if (resampler == &sinc_resampler)
   {
      resampler = malloc(sizeof(*resampler));
      if (!resampler)
      {
         free(re);
         return NULL;
      }
      memcpy(resampler, *_resampler, sizeof(*resampler));
      *_resampler = (const retro_resampler_t*)resampler;
   }

   switch (quality)
   {
      case RESAMPLER_QUALITY_LOWEST:
         cutoff            = 0.98;
         sidelobes         = 2;
         re->phase_bits    = 12;
         re->subphase_bits = 10;
         window_type       = SINC_WINDOW_LANCZOS;
         break;
      case RESAMPLER_QUALITY_LOWER:
         cutoff            = 0.98;
         sidelobes         = 4;
         re->phase_bits    = 12;
         re->subphase_bits = 10;
         window_type       = SINC_WINDOW_LANCZOS;
         break;
      case RESAMPLER_QUALITY_HIGHER:
         cutoff            = 0.90;
         sidelobes         = 32;
         re->phase_bits    = 10;
         re->subphase_bits = 14;
         window_type       = SINC_WINDOW_KAISER;
         re->kaiser_beta   = 10.5;
         enable_avx        = 1;
         break;
      case RESAMPLER_QUALITY_HIGHEST:
         cutoff            = 0.962;
         sidelobes         = 128;
         re->phase_bits    = 10;
         re->subphase_bits = 14;
         window_type       = SINC_WINDOW_KAISER;
         re->kaiser_beta   = 14.5;
         enable_avx        = 1;
         break;
      case RESAMPLER_QUALITY_NORMAL:
      case RESAMPLER_QUALITY_DONTCARE:
         cutoff            = 0.825;
         sidelobes         = 8;
         re->phase_bits    = 8;
         re->subphase_bits = 16;
         window_type       = SINC_WINDOW_KAISER;
         re->kaiser_beta   = 5.5;
         break;
   }

   re->subphase_mask = (1 << re->subphase_bits) - 1;
   re->subphase_mod  = 1.0f / (1 << re->subphase_bits);
   re->taps          = sidelobes * 2;
   re->channels      = channels;

   /* Downsampling, must lower cutoff, and extend number of
    * taps accordingly to keep same stopband attenuation. */
   if (bandwidth_mod < 1.0)
   {
      cutoff *= bandwidth_mod;
      re->taps = (unsigned)ceil(re->taps / bandwidth_mod);
   }

   /* Be SIMD-friendly. */
#if defined(__AVX__)
   if (enable_avx)
      re->taps  = (re->taps + 7) & ~7;
   else
#endif
   {
#if (defined(__ARM_NEON__) || defined(HAVE_NEON))
      re->taps     = (re->taps + 7) & ~7;
#else
      re->taps     = (re->taps + 3) & ~3;
#endif
   }

   phase_elems     = ((1 << re->phase_bits) * re->taps);
   if (window_type == SINC_WINDOW_KAISER)
      phase_elems  = phase_elems * 2;
   elems           = phase_elems + 2 * re->taps * channels;

   re->main_buffer = (float*)memalign_alloc(128, sizeof(float) * elems);
   if (!re->main_buffer)
      goto error;
    
   re->buffer      = (float**)malloc(sizeof(float*) * channels);
   if (!re->buffer)
      goto error;

   memset(re->main_buffer, 0, sizeof(float) * elems);

   re->phase_table     = re->main_buffer;
   re->buffer[0]       = re->main_buffer + phase_elems;
   for (channel = 1; channel < channels; ++channel)
   {
      re->buffer[channel] = re->buffer[channel - 1] + 2 * re->taps;
   }

   switch (window_type)
   {
      case SINC_WINDOW_LANCZOS:
         sinc_init_table_lanczos(re, cutoff, re->phase_table,
               1 << re->phase_bits, re->taps, false);
         break;
      case SINC_WINDOW_KAISER:
         sinc_init_table_kaiser(re, cutoff, re->phase_table,
               1 << re->phase_bits, re->taps, true);
         break;
      case SINC_WINDOW_NONE:
         goto error;
   }

   resampler->process = resampler_sinc_process_c;
   if (window_type == SINC_WINDOW_KAISER)
      resampler->process    = resampler_sinc_process_c_kaiser;

   if (mask & RESAMPLER_SIMD_AVX && enable_avx)
   {
#if defined(__AVX__)
      resampler->process    = resampler_sinc_process_avx;
      if (window_type == SINC_WINDOW_KAISER)
         resampler->process = resampler_sinc_process_avx_kaiser;
#endif
   }
   else if (mask & RESAMPLER_SIMD_SSE)
   {
#if defined(__SSE__)
      resampler->process = resampler_sinc_process_sse;
      if (window_type == SINC_WINDOW_KAISER)
         resampler->process = resampler_sinc_process_sse_kaiser;
#endif
   }
   else if (mask & RESAMPLER_SIMD_NEON)
   {
#if (defined(__ARM_NEON__) || defined(HAVE_NEON))
      resampler->process = resampler_sinc_process_neon;
      if (window_type == SINC_WINDOW_KAISER)
         resampler->process = resampler_sinc_process_neon_kaiser;
#endif
   }

   return re;

error:
   resampler_sinc_free(resampler, re);
   return NULL;
}

retro_resampler_t sinc_resampler = {
   resampler_sinc_new,
   resampler_sinc_process_c,
   resampler_sinc_free,
   resampler_sinc_latency,
   RESAMPLER_API_VERSION,
   "sinc",
   "sinc"
};
