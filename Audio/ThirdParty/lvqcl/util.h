/* Copyright (c) 2018 lvqcl. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef UTIL_H_
#define UTIL_H_

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif

static inline unsigned local_gcd(unsigned a, unsigned b)
{
    if (a == 0 || b == 0) return 0;
    unsigned c = a % b;
    while (c != 0) { a = b; b = c; c = a % b; }
    return b;
}

/*
    In:  *r1 and *r2: samplerates;
    Out: *r1 and *r2: numbers of samples;

    multiply r1 and r2 by n so that durations is 1/N th of second;
    limit n so that r1 and r2 aren't bigger than M
*/
static void samples_len(unsigned* r1, unsigned* r2, unsigned N, unsigned M)        // example: r1 = 44100, r2 = 48000, N = 20, M = 8192
{
    if (r1 == 0 || r2 == 0) return;
    unsigned v = local_gcd(*r1, *r2);        // v = 300
    *r1 /= v; *r2 /= v;                        // r1 = 147; r2 = 160  == 1/300th of second
    unsigned n = (v + N-1) / N;                // n = 300/20 = 15 times
    unsigned z = max(*r1, *r2);        // z = 160
    if (z*n > M) n = M / z;                    // 160*15 = 2400 < 8192;; if M == 1024: n = 1024/160 = 6; 160*6 = 960
    if (n < 1) n = 1;
    *r1 *= n; *r2 *= n;                        // r1 = 147*15 = 2205 samples, r2 = 160*15 = 2400 samples
}

#endif
