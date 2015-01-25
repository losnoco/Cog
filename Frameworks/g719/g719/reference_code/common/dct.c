/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "cnst.h"
#include "complxop.h"
#include "rom.h"

/*--------------------------------------------------------------------------*/
/*  Function  wfft120                                                       */
/*  ~~~~~~~~~~~~~~~~~                                                       */
/*                                                                          */
/*  Winograd 120 point FFT                                                  */
/*--------------------------------------------------------------------------*/
/*  complex      *xi      (i/o)    input and output of the FFT              */
/*--------------------------------------------------------------------------*/
void wfft120(complex *xi)
{
    short ind0, n3, n5;
    float t0r, t1r, t2r, t3r, t4r, t5r, t6r, t7r, q1r, x1r, x5r;
    float t0i, t1i, t2i, t3i, t4i, t5i, t6i, t7i, q1i, x1i, x5i;
    float *ps;
    complex x[144];
    complex *p0, *p1, *p2, *p3, *p4, *p5, *p6, *p7;
    complex *pi0, *pi1, *pi2, *pi3, *pi4, *pi5, *pi6, *pi7;

    p0 = x;
    for (ind0 = 0; ind0 < 120; ind0 += 8)
    {                
        pi0 = xi + indxPre[ind0+0];
        pi1 = xi + indxPre[ind0+1];
        pi2 = xi + indxPre[ind0+2];
        pi3 = xi + indxPre[ind0+3];
        pi4 = xi + indxPre[ind0+4];
        pi5 = xi + indxPre[ind0+5];
        pi6 = xi + indxPre[ind0+6];
        pi7 = xi + indxPre[ind0+7];

        t0r = pi0->r + pi4->r;
        t0i = pi0->i + pi4->i;
        t1r = pi1->r + pi5->r;
        t1i = pi1->i + pi5->i;
        t2r = pi2->r + pi6->r;
        t2i = pi2->i + pi6->i;
        t3r = pi3->r + pi7->r;
        t3i = pi3->i + pi7->i;
        t5r = pi1->r - pi5->r;
        t5i = pi1->i - pi5->i;
        t7r = pi3->r - pi7->r;
        t7i = pi3->i - pi7->i;

        p0->r = t0r + t2r;
        p0->i = t0i + t2i;
        p0++;
        p0->r = t1r + t3r;
        p0->i = t1i + t3i;
        p0++;
        p0->r = t0r - t2r;
        p0->i = t0i - t2i;
        p0++;
        p0->r = t1r - t3r;
        p0->i = t1i - t3i;
        p0++;
        p0->r = pi0->r - pi4->r;
        p0->i = pi0->i - pi4->i;
        p0++;
        p0->r = t5r + t7r;
        p0->i = t5i + t7i;
        p0++;
        p0->r = pi2->r - pi6->r;
        p0->i = pi2->i - pi6->i;
        p0++;
        p0->r = t5r - t7r;
        p0->i = t5i - t7i;
        p0++;
    }

    for (n5 = 0; n5 < 120; n5 += 24)
    {
        for (ind0 = n5; ind0 < (n5+8); ind0++)
        {
            p0 = x + ind0;
            p1 = x + ind0 + 8;
            p2 = x + ind0 + 16;

            x1r = p1->r;
            x1i = p1->i;
            p1->r = x1r + p2->r;
            p1->i = x1i + p2->i;
            p2->r = x1r - p2->r;
            p2->i = x1i - p2->i;
            cadd(*p0, *p1, p0);
        }
    }

    for (n3 = 0; n3 < 24; n3 += 8)
    {
        for (ind0 = n3; ind0 < (n3+8); ind0++)
        {
            p0 = x + ind0;
            p1 = x + ind0 + 24;
            p2 = x + ind0 + 48;
            p3 = x + ind0 + 72;
            p4 = x + ind0 + 96;
            p5 = x + ind0 + 120;
            ps = fft120cnst + ind0;

            t3r = p3->r + p2->r;
            t3i = p3->i + p2->i;
            t2r = p3->r - p2->r;
            t2i = p3->i - p2->i;
            t1r = p1->r + p4->r;
            t1i = p1->i + p4->i;
            csub(*p1, *p4, p2);
            
            p1->r = t1r + t3r;
            p1->i = t1i + t3i;
            p3->r = t1r - t3r;
            p3->i = t1i - t3i;

            x5r = p2->r + t2r;
            x5i = p2->i + t2i;
            cadd(*p0, *p1, p0);

            cmpys(*p0, *ps, p0);
            ps += 24;
            cmpys(*p1, *ps, p1);
            ps += 24;
            cmpys(*p2, *ps, p2);
            ps += 24;
            cmpys(*p3, *ps, p3);
            ps += 24;
            p4->r = t2r * *ps;
            p4->i = t2i * *ps;
            ps += 24;
            p5->r = x5r * *ps;
            p5->i = x5i * *ps;
        }
    }

    for (n3 = 0; n3 < 24; n3 += 8)
    {
        for (ind0 = n3; ind0 < (n3+8); ind0++)
        {       
            p0 = x + ind0;
            p1 = x + ind0 + 24;
            p2 = x + ind0 + 48;
            p3 = x + ind0 + 72;
            p4 = x + ind0 + 96;
            p5 = x + ind0 + 120;

            cmpyj(p2);
            cmpyj(p4);
            cmpyj(p5);

            q1r = p0->r + p1->r;
            q1i = p0->i + p1->i;

            t1r = q1r + p3->r;
            t1i = q1i + p3->i;
            t4r = p2->r + p5->r;
            t4i = p2->i + p5->i;
            t3r = q1r - p3->r;
            t3i = q1i - p3->i;
            t2r = p4->r + p5->r;
            t2i = p4->i + p5->i;

            p1->r = t1r + t4r;
            p1->i = t1i + t4i;
            p4->r = t1r - t4r;
            p4->i = t1i - t4i;
            p3->r = t3r + t2r;
            p3->i = t3i + t2i;
            p2->r = t3r - t2r;
            p2->i = t3i - t2i;
        }
    }

    for (n5 = 0; n5 < 120; n5 += 24)
    {
        for (ind0 = n5; ind0 < (n5+8); ind0++)
        {
            p0 = x + ind0;
            p1 = x + ind0 + 8;
            p2 = x + ind0 + 16;

            t1r = p0->r + p1->r;
            t1i = p0->i + p1->i;
            cmpyj(p2);

            p1->r = t1r + p2->r;
            p1->i = t1i + p2->i;
            p2->r = t1r - p2->r;          
            p2->i = t1i - p2->i;          
       }

    }

    for (ind0 = 0; ind0 < 120; ind0 += 8)
    {       
        p0 = x + ind0;
        p1 = x + ind0 + 1;
        p2 = x + ind0 + 2;
        p3 = x + ind0 + 3;
        p4 = x + ind0 + 4;
        p5 = x + ind0 + 5;
        p6 = x + ind0 + 6;
        p7 = x + ind0 + 7;

        pi0 = xi + indxPost[ind0+0];
        pi1 = xi + indxPost[ind0+1];
        pi2 = xi + indxPost[ind0+2];
        pi3 = xi + indxPost[ind0+3];
        pi4 = xi + indxPost[ind0+4];
        pi5 = xi + indxPost[ind0+5];
        pi6 = xi + indxPost[ind0+6];
        pi7 = xi + indxPost[ind0+7];

        cmpyj(p3); 
        cmpyj(p5); 
        cmpyj(p6); 

        t5r = p4->r + p5->r;
        t5i = p4->i + p5->i;
        t4r = p4->r - p5->r;
        t4i = p4->i - p5->i;
        t7r = p6->r + p7->r;
        t7i = p6->i + p7->i;
        t6r = p6->r - p7->r;
        t6i = p6->i - p7->i;

        cadd(*p0, *p1, pi0);
        csub(*p0, *p1, pi4);
        cadd(*p2, *p3, pi2);
        csub(*p2, *p3, pi6);
        pi1->r = t5r + t7r;
        pi1->i = t5i + t7i;
        pi5->r = t4r + t6r;
        pi5->i = t4i + t6i;
        pi3->r = t5r - t7r;
        pi3->i = t5i - t7i;
        pi7->r = t4r - t6r;
        pi7->i = t4i - t6i;
    }

    return;
}

/*--------------------------------------------------------------------------*/
/*  Function  wfft480                                                       */
/*  ~~~~~~~~~~~~~~~~~                                                       */
/*                                                                          */
/*  Winograd 480 points FFT                                                 */
/*--------------------------------------------------------------------------*/
/*  complex      *pI      (i)    input to the FFT                           */
/*  complex      *pO      (o)    output to the FFT                          */
/*--------------------------------------------------------------------------*/
static void wfft480(complex *pI, complex *pO)
{
    short i, j;
    float temp;
    float x0r, x1r, x2r, x3r, t0r, t1r, t2r, t3r;
    float x0i, x1i, x2i, x3i, t0i, t1i, t2i, t3i;
    complex *pw1, *pw2, *pw3;
    complex *pt0, *pt1, *pt2, *pt3;

    pt0 = pI;
    pt1 = pI + 120;
    pt2 = pI + 240;
    pt3 = pI + 360;
    pw1 = ptwdf + 1;
    pw2 = ptwdf + 2;
    pw3 = ptwdf + 3;

    for(i = 0; i < 120; i++)
    {
        t0r = pt0->r + pt2->r;
        t0i = pt0->i + pt2->i;
        t2r = pt0->r - pt2->r;
        t2i = pt0->i - pt2->i;
        t1r = pt1->r + pt3->r;
        t1i = pt1->i + pt3->i;
        t3r = pt3->r - pt1->r;
        t3i = pt3->i - pt1->i;

        temp = t3r;
        t3r = -t3i;
        t3i = temp;

        x0r = t0r + t1r;
        x0i = t0i + t1i;
        x1r = t2r + t3r;
        x1i = t2i + t3i;
        x2r = t0r - t1r;
        x2i = t0i - t1i;
        x3r = t2r - t3r;
        x3i = t2i - t3i;

        // twiddle factors
        pt0->r = x0r;
        pt0->i = x0i;
        pt1->r = x1r * pw1->r - x1i * pw1->i;
        pt1->i = x1r * pw1->i + x1i * pw1->r;
        pt2->r = x2r * pw2->r - x2i * pw2->i;
        pt2->i = x2r * pw2->i + x2i * pw2->r;
        pt3->r = x3r * pw3->r - x3i * pw3->i;
        pt3->i = x3r * pw3->i + x3i * pw3->r;

        pt0++;
        pt1++;
        pt2++;
        pt3++;
        pw1 += 4;
        pw2 += 4;
        pw3 += 4;
    }
    for(i = 0; i < 480; i+=120)
    {
       wfft120(pI+i);
    }

    pt0 = pO;
    for(j = 0; j < 120; j++)
    {
        pt1 = pI + j;
        for(i = 0; i < 4; i++)
        {
            *pt0++ = *pt1;
            pt1 += 120;
        }
    }
    return;
}

/*--------------------------------------------------------------------------*/
/*  Function  dct4_960                                                      */
/*  ~~~~~~~~~~~~~~~~~~                                                      */
/*                                                                          */
/*  DCT4 960 points                                                         */
/*--------------------------------------------------------------------------*/
/*  float      v[]        (i)    input of the DCT4                          */
/*  float      coefs32[]  (o)    coefficient of the DCT4                    */
/*--------------------------------------------------------------------------*/
void dct4_960(float v[MLT960_LENGTH], float coefs32[MLT960_LENGTH])
{
    short n;
    float f_int[MLT960_LENGTH];

    for(n = 0; n < MLT960_LENGTH; n += 2)  
    {
        f_int[n]   = ((v[n] * dct480_table_1[n>>1].r  - 
                           v[(MLT960_LENGTH_MINUS_1-n)] * dct480_table_1[n>>1].i)); 
        f_int[n+1] = ((v[n] * dct480_table_1[n>>1].i + 
                           v[(MLT960_LENGTH_MINUS_1-n)] * dct480_table_1[n>>1].r));
    }

    wfft480((complex *)f_int, (complex *)v);

    for(n = 0; n < MLT960_LENGTH; n += 2)
    {
        coefs32[n] = ((v[n] * dct480_table_2[n>>1].r - 
                           v[n+1] * dct480_table_2[n>>1].i))/4.0f;
        coefs32[MLT960_LENGTH_MINUS_1-n] = -(v[n] * dct480_table_2[n>>1].i + 
                           v[n+1] * dct480_table_2[n>>1].r)/4.0f;
    }
}


/*--------------------------------------------------------------------------*/
/*  Function  dct4_240                                                      */
/*  ~~~~~~~~~~~~~~~~~~                                                      */
/*                                                                          */
/*  DCT4 240 points                                                         */
/*--------------------------------------------------------------------------*/
/*  float      v[]        (i)    input of the DCT4                          */
/*  float      coefs32[]  (o)    coefficient of the DCT4                    */
/*--------------------------------------------------------------------------*/
void dct4_240(float v[MLT240_LENGTH], float coefs32[MLT240_LENGTH])
{
    short n;
    float f_int[MLT240_LENGTH];

    for (n=0; n < MLT240_LENGTH; n+=2)  
    {
        f_int[n]   = (v[n]*dct120_table_1[n>>1].r - 
                  v[(MLT240_LENGTH_MINUS_1-n)]*dct120_table_1[n>>1].i); 
        f_int[n+1] = (v[n]*dct120_table_1[n>>1].i + 
                  v[(MLT240_LENGTH_MINUS_1-n)]*dct120_table_1[n>>1].r);
    }

    wfft120((complex *)f_int);

    for (n=0; n < MLT240_LENGTH; n+=2)
    {
        coefs32[n] = (f_int[n]*dct120_table_2[n>>1].r - 
                  f_int[n+1]*dct120_table_2[n>>1].i)/2.0f;
        coefs32[MLT240_LENGTH_MINUS_1-n] = -(f_int[n]*dct120_table_2[n>>1].i + 
                  f_int[n+1]*dct120_table_2[n>>1].r)/2.0f;
    }
}

