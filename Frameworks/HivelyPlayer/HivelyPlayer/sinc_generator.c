#include <stdio.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>

#ifndef M_PI
#define M_PI 3.141592653589793
#endif

enum { half_width  = 8 };
enum { phase_bits  = 5 };
enum { phase_count = 1 << phase_bits };
enum { sinc_samples = phase_count * half_width };

static float sinc_lut[sinc_samples + 1];
static float window_lut[sinc_samples + 1];
static int bl_step[phase_count + 1] [half_width];

static int fEqual(const float b, const float a)
{
    return fabs(a - b) < 1.0e-6;
}

static float sinc(float x)
{
    return fEqual(x, 0.0) ? 1.0 : sin(x * M_PI) / (x * M_PI);
}

int main(void)
{
    unsigned i;
    int k;
    double dx = (float)(half_width) / sinc_samples, x = 0.0;
    for (i = 0; i < sinc_samples + 1; ++i, x += dx)
    {
        float y = x / half_width;
#if 0
        // Blackman
        float window = 0.42659 - 0.49656 * cos(M_PI + M_PI * y) + 0.076849 * cos(2.0 * M_PI * y);
#elif 1
        // Nuttal 3 term
        float window = 0.40897 + 0.5 * cos(M_PI * y) + 0.09103 * cos(2.0 * M_PI * y);
#elif 0
        // C.R.Helmrich's 2 term window
        float window = 0.79445 * cos(0.5 * M_PI * y) + 0.20555 * cos(1.5 * M_PI * y);
#elif 0
        // Lanczos
        float window = sinc(y);
#endif
        sinc_lut[i] = fabs(x) < half_width ? sinc(x) : 0.0;
        window_lut[i] = window;
    }

    for (i = 0; i <= phase_count / 2; ++i)
    {
        float kernel_sum = 0;
        float kernel[half_width * 2];
        for (k = half_width; k >= -half_width + 1; --k)
        {
            int pos = k * phase_count;
            int abs_pos = abs(i - pos);
            kernel_sum += kernel[k + half_width - 1] = sinc_lut[abs_pos] * window_lut[abs_pos];
        }
	    kernel_sum = (1.0 / kernel_sum) * 32768.0;
        for (k = 0; k < half_width; ++k)
            bl_step[i][k] = (int)(kernel[k] * kernel_sum);
        for (k = 0; k < half_width; ++k)
            bl_step[phase_count - i][half_width - k - 1] = (int)(kernel[half_width + k] * kernel_sum);
    }

    printf( "static int const bl_step [phase_count + 1] [half_width] =\n{\n" );

    for (i = 0; i <= phase_count; ++i)
    {
        printf("{");
        for (k = 0; k < half_width; ++k)
        {
            printf("%5d", bl_step[i][k]);
            if (k < half_width - 1) printf(",");
        }
        printf("}"); if (i < phase_count) printf(",");
        printf("\n");
    }
    
    printf( "};\n" );

    return 0;
}
