/* Copyright  (C) 2010-2021 The RetroArch team
 *
 * ---------------------------------------------------------------------------------------
 * The following license statement only applies to this file (s32_to_float.c).
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#include <boolean.h>
#include <features/features_cpu.h>
#include <audio/conversion/s32_to_float.h>

#if (defined(__ARM_NEON__) || defined(HAVE_NEON))
static bool s32_to_float_neon_enabled = false;

#include <arm_neon.h>

void convert_s32_to_float(float *out,
      const int32_t *in, size_t samples, float gain)
{
   unsigned i      = 0;

   if (s32_to_float_neon_enabled)
   {
      float        gf        = gain / (1 << 31);
      float32x4_t vgf        = {gf, gf, gf, gf};
      while (samples >= 8)
      {
         float32x4x2_t oreg;
         int32x4x2_t inreg   = vld1q_s32_x2(in);
         oreg.val[0]         = vmulq_f32(vcvtq_f32_s32(inreg.val[0]), vgf);
         oreg.val[1]         = vmulq_f32(vcvtq_f32_s32(inreg.val[1]), vgf);
         vst2q_f32(out, oreg);
         in                 += 8;
         out                += 8;
         samples            -= 8;
      }
   }

   gain /= 0x80000000L;

   for (; i < samples; i++)
      out[i] = (float)in[i] * gain;
}

void convert_s32_to_float_init_simd(void)
{
   uint64_t cpu = cpu_features_get();

   if (cpu & RETRO_SIMD_NEON)
      s32_to_float_neon_enabled = true;
}
#else
void convert_s32_to_float(float *out,
      const int32_t *in, size_t samples, float gain)
{
   unsigned i      = 0;

#if defined(__SSE2__)
   float fgain   = gain / UINT32_C(0x80000000);
   __m128 factor = _mm_set1_ps(fgain);

   for (i = 0; i + 8 <= samples; i += 8, in += 8, out += 8)
   {
      __m128i input    = _mm_loadu_si128((const __m128i *)in);
      __m128i input2   = _mm_loadu_si128((const __m128i *)(in + 4));
      __m128 output_l  = _mm_mul_ps(_mm_cvtepi32_ps(input), factor);
      __m128 output_r  = _mm_mul_ps(_mm_cvtepi32_ps(input2), factor);

      _mm_storeu_ps(out + 0, output_l);
      _mm_storeu_ps(out + 4, output_r);
   }

   samples = samples - i;
   i       = 0;
#endif

   gain   /= 0x80000000L;

   for (; i < samples; i++)
      out[i] = (float)in[i] * gain;
}

void convert_s32_to_float_init_simd(void) { }
#endif

