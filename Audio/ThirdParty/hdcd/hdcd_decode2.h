/*
   Copyright (C) 2010-2017, Christopher Snowhill,
   All rights reserved.
   Optimizations by Gumboot
   Additional work by Burt P.
   Original code reverse engineered from HDCD decoder library by Christopher Key,
   which was likely reverse engineered from Windows Media Player.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote
        products derived from this software without specific prior written
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
  HDCD is High Definition Compatible Digital
  http://wiki.hydrogenaud.io/index.php?title=High_Definition_Compatible_Digital

  More information about HDCD-encoded audio CDs:
  http://www.audiomisc.co.uk/HFN/HDCD/Enigma.html
  http://www.audiomisc.co.uk/HFN/HDCD/Examined.html
 */

#ifndef _HDCD_DECODE2_H_
#define _HDCD_DECODE2_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t window;
    unsigned char readahead;

    /* arg is set when a packet prefix is found.
     * control is the active control code, where
     * bit 0-3: target_gain, 4-bit (3.1) fixed-point value
     * bit 4  : peak_extend
     * bit 5  : transient_filter
     * bit 6,7: always zero */
    unsigned char arg, control;
    unsigned sustain, sustain_reset; /* code detect timer */

    int running_gain; /* 11-bit (3.8) fixed point, extended from target_gain */

    /* counters */
    int code_counterA;            /* 8-bit format packet */
    int code_counterA_almost;     /* looks like an A code, but a bit expected to be 0 is 1 */
    int code_counterB;            /* 16-bit format packet, 8-bit code, 8-bit XOR of code */
    int code_counterB_checkfails; /* looks like a B code, but doesn't pass the XOR check */
    int code_counterC;            /* packet prefix was found, expect a code */
    int code_counterC_unmatched;  /* told to look for a code, but didn't find one */
    int count_peak_extend;        /* valid packets where peak_extend was enabled */
    int count_transient_filter;   /* valid packets where filter was detected */
    /* target_gain is a 4-bit (3.1) fixed-point value, always
     * negative, but stored positive.
     * The 16 possible values range from -7.5 to 0.0 dB in
     * steps of 0.5, but no value below -6.0 dB should appear. */
    int gain_counts[16]; /* for cursiosity, mostly */
    int max_gain;
    /* occurences of code detect timer expiring without detecting
     * a code. -1 for timer never set. */
    int count_sustain_expired;
} hdcd_state_t;

typedef struct {
    hdcd_state_t channel[2];
    int val_target_gain;
    int count_tg_mismatch;
} hdcd_state_stereo_t;

typedef enum {
    HDCD_PE_NEVER        = 0,
    HDCD_PE_INTERMITTENT = 1,
    HDCD_PE_PERMANENT    = 2,
} hdcd_pe_t;

typedef struct {
    int hdcd_detected;
    int errors;                /* detectable errors */
    hdcd_pe_t peak_extend;
    int uses_transient_filter;
    float max_gain_adjustment; /* in dB, expected in the range -7.5 to 0.0 */
} hdcd_detection_data_t;

void hdcd_reset(hdcd_state_t *state, unsigned rate);
void hdcd_process(hdcd_state_t *state, int *samples, int count, int stride);

void hdcd_reset_stereo(hdcd_state_stereo_t *state, unsigned rate);
void hdcd_process_stereo(hdcd_state_stereo_t *state, int *samples, int count);

void hdcd_detect_reset(hdcd_detection_data_t *detect);
void hdcd_detect_str(hdcd_detection_data_t *detect, char *str); /* char str[256] should be enough */
/* there isn't a non-stereo version */
void hdcd_detect_stereo(hdcd_state_stereo_t *state, hdcd_detection_data_t *detect);

#ifdef __cplusplus
}
#endif

#endif
