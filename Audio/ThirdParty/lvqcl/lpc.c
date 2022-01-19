/*
 * Copyright (c) 2013, 2018 lvqcl
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <memory.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lpc.h"

static void apply_window(float * const data, const size_t data_len);
static void compute_autocorr(const float * const data, const size_t data_len, double * const autoc, const int m);
static int compute_lpc(const double * const autoc, double * const lpc, const int lpc_order);
static void lpc_extrapolate_data(float * const data0, const size_t data_len, const size_t extra, const double * const lpc, const int order, const bool invdir);

void lpc_extrapolate2(float * const data, const size_t data_len, const int nch, const int lpc_order, const size_t extra_bkwd, const size_t extra_fwd, void ** extrapolate_buffer, size_t * extrapolate_buffer_size)
{
    const size_t tdata0_size = sizeof(float) * (extra_bkwd + data_len + extra_fwd);
    const size_t autoc_size = sizeof(double) * (lpc_order + 1);
    const size_t lpc_size = sizeof(double) * lpc_order;
    
    const size_t new_size = tdata0_size + autoc_size + lpc_size;
    
    if (new_size > *extrapolate_buffer_size)
    {
        *extrapolate_buffer = realloc(*extrapolate_buffer, new_size);
        *extrapolate_buffer_size = new_size;
    }
    
    float* tdata0 = (float*)(*extrapolate_buffer);                   // for 1 channel only
    float* const tdata = tdata0 + extra_bkwd;                        // for 1 channel only

    double* autoc = (double*)(*extrapolate_buffer + tdata0_size);
    double* lpc = (double*)(*extrapolate_buffer + tdata0_size + autoc_size);
    int max_order;

    for(int c = 0; c < nch; c++)
    {
        for (int i = -(int)extra_bkwd; i < (int)(data_len+extra_fwd); i++) { tdata[i] = 0; } // should be removed after debugging etc

        for (int i = 0; i < (int)data_len; i++)
            tdata[i] = data[i*nch + c];

        apply_window(tdata, data_len);
        compute_autocorr(tdata, data_len, autoc, lpc_order);
        max_order = compute_lpc(autoc, lpc, lpc_order);

        // restore after apply_window
        for (int i = 0; i < (int)data_len; i++)
            tdata[i] = data[i*nch + c];

        if (extra_fwd)
        {
            lpc_extrapolate_data(tdata, data_len, extra_fwd, lpc, max_order, false);
            for (size_t i = data_len; i < (data_len+extra_fwd); i++)
                data[i*nch + c] = tdata[i];
        }
        if (extra_bkwd)
        {
            lpc_extrapolate_data(tdata, data_len, extra_bkwd, lpc, max_order, true);
            for (int i = -(int)extra_bkwd; i < 0; i++)
                data[i*nch + c] = tdata[i];
        }
    }
}

static void apply_window(float * const data, const size_t data_len)
{
#if 0
    if (0) // subtract the mean
    {
        double mean = 0;
        for(int i = 0; i < (int)data_len; i++)
            mean += data[i];
        mean /= data_len;

        for(int i = 0; i < (int)data_len; i++)
            data[i] -= (float)mean;
    }
#endif

    if (1)    // Welch window
    {
        const float n2 = (data_len+1)/2.0f;
        for(int i = 0; i < (int)data_len; i++)
        {
            float k = (i+1-n2)/n2;
            data[i] *= 1.0f - k*k;
        }
    }
}

static void compute_autocorr(const float * const data, const size_t data_len, double * const autoc, const int m)
{
    int i, j;

    j = m + 1;
    // for(j = 0; j <= m; j++)
    while(j--)
    {
        double d = 0;
        for(i = j; i < (int)data_len; i++)
            d += (double)data[i] * data[i-j];

        autoc[j] = d;
    }
}

static int compute_lpc(const double * const autoc, double * const lpc, const int lpc_order)
{
    int i, j;
    double error, epsilon;
    int max_order = lpc_order;

    error = autoc[0] * (1.+1e-10);
    epsilon = 1e-9*autoc[0] + 1e-10;

    for(i = 0; i < lpc_order; i++)
    {
        if (error < epsilon)
        {
            memset(&lpc[i], 0, (lpc_order-i)*sizeof(lpc[0]));
            max_order = i; break;
        }

        double r = -autoc[i+1];
        for(j = 0; j < i; j++)
            r -= lpc[j] * autoc[i-j];
        r /= error;

        lpc[i] = r;
        for(j = 0; j < i/2; j++)
        {
            double tmp = lpc[j];
            lpc[j    ] += r * lpc[i-1-j];
            lpc[i-1-j] += r * tmp;
        }
        if (i&1)
            lpc[j] += lpc[j]*r;

        error *= 1.0 - r*r;
    }

    if (1) /* slightly damp the filter */
    {
        const double g = 0.999;
        double damp = g;
        for(j = 0; j < max_order; j++)
        {
            lpc[j] *= damp;
            damp *= g;
        }
    }

    if (max_order == 0) /* in case the signal is constant AND we subtract the mean in apply_window() */
    {
        max_order = 1;
        lpc[0] = -1;
    }

    return max_order;
}

static void lpc_extrapolate_data(float * const data0, const size_t data_len, const size_t extra, const double * const lpc, const int order, const bool invdir)
{
    int i, j;
    if (invdir == false)
    {
        float* data = data0 + data_len - order;
        for(i = 0; i < (int)extra; i++)
        {
            float sum = 0;
            for(j = 0; j < order; j++)
                sum -= data[i+j] * (float)lpc[order-1-j];

            if (sum > 10.f) sum = 10.f; else if (sum < -10.f) sum = -10.f; // should be removed after debugging etc
            data[order+i] = sum;
        }
    }
    else
    {
        float* data = data0 - 1 + order;
        for(i = 0; i < (int)extra; i++)
        {
            float sum = 0;
            for(j = 0; j < order; j++)
                sum -= data[-i-j] * (float)lpc[order-1-j];

            if (sum > 10.f) sum = 10.f; else if (sum < -10.f) sum = -10.f; // should be removed after debugging etc
            data[-order-i] = sum;
        }
    }
}
