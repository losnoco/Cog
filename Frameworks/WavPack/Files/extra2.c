////////////////////////////////////////////////////////////////////////////
//			     **** WAVPACK ****				  //
//		    Hybrid Lossless Wavefile Compressor			  //
//		Copyright (c) 1998 - 2005 Conifer Software.		  //
//			    All Rights Reserved.			  //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// extra2.c

// This module handles the "extra" mode for stereo files.

#include "wavpack.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define LOG_LIMIT 6912
//#define EXTRA_DUMP

#ifdef DEBUG_ALLOC
#define malloc malloc_db
#define realloc realloc_db
#define free free_db
void *malloc_db (uint32_t size);
void *realloc_db (void *ptr, uint32_t size);
void free_db (void *ptr);
int32_t dump_alloc (void);
#endif

//////////////////////////////// local tables ///////////////////////////////

typedef struct {
    int32_t *sampleptrs [MAX_NTERMS+2];
    struct decorr_pass dps [MAX_NTERMS];
    int nterms, log_limit;
    uint32_t best_bits;
} WavpackExtraInfo;

extern const signed char default_terms [], high_terms [], fast_terms [];

static void decorr_stereo_pass (int32_t *in_samples, int32_t *out_samples, int32_t num_samples, struct decorr_pass *dpp, int dir)
{
    int m = 0, i;

    dpp->sum_A = dpp->sum_B = 0;

    if (dir < 0) {
	out_samples += (num_samples - 1) * 2;
	in_samples += (num_samples - 1) * 2;
	dir = -2;
    }
    else
	dir = 2;

    dpp->weight_A = restore_weight (store_weight (dpp->weight_A));
    dpp->weight_B = restore_weight (store_weight (dpp->weight_B));

    for (i = 0; i < 8; ++i) {
	dpp->samples_A [i] = exp2s (log2s (dpp->samples_A [i]));
	dpp->samples_B [i] = exp2s (log2s (dpp->samples_B [i]));
    }

    if (dpp->term == 17) {
	while (num_samples--) {
	    int32_t left, right;
	    int32_t sam_A, sam_B;

	    sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
	    dpp->samples_A [1] = dpp->samples_A [0];
	    dpp->samples_A [0] = left = in_samples [0];
	    left -= apply_weight (dpp->weight_A, sam_A);
	    update_weight (dpp->weight_A, dpp->delta, sam_A, left);
	    dpp->sum_A += dpp->weight_A;
	    out_samples [0] = left;

	    sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];
	    dpp->samples_B [1] = dpp->samples_B [0];
	    dpp->samples_B [0] = right = in_samples [1];
	    right -= apply_weight (dpp->weight_B, sam_B);
	    update_weight (dpp->weight_B, dpp->delta, sam_B, right);
	    dpp->sum_B += dpp->weight_B;
	    out_samples [1] = right;
	    in_samples += dir;
	    out_samples += dir;
	}
    }
    else if (dpp->term == 18) {
	while (num_samples--) {
	    int32_t left, right;
	    int32_t sam_A, sam_B;

	    sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
	    dpp->samples_A [1] = dpp->samples_A [0];
	    dpp->samples_A [0] = left = in_samples [0];
	    left -= apply_weight (dpp->weight_A, sam_A);
	    update_weight (dpp->weight_A, dpp->delta, sam_A, left);
	    dpp->sum_A += dpp->weight_A;
	    out_samples [0] = left;

	    sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;
	    dpp->samples_B [1] = dpp->samples_B [0];
	    dpp->samples_B [0] = right = in_samples [1];
	    right -= apply_weight (dpp->weight_B, sam_B);
	    update_weight (dpp->weight_B, dpp->delta, sam_B, right);
	    dpp->sum_B += dpp->weight_B;
	    out_samples [1] = right;
	    in_samples += dir;
	    out_samples += dir;
	}
    }
    else if (dpp->term > 0) {
	while (num_samples--) {
	    int k = (m + dpp->term) & (MAX_TERM - 1);
	    int32_t left, right;
	    int32_t sam_A, sam_B;

	    sam_A = dpp->samples_A [m];
	    dpp->samples_A [k] = left = in_samples [0];
	    left -= apply_weight (dpp->weight_A, sam_A);
	    update_weight (dpp->weight_A, dpp->delta, sam_A, left);
	    dpp->sum_A += dpp->weight_A;
	    out_samples [0] = left;

	    sam_B = dpp->samples_B [m];
	    dpp->samples_B [k] = right = in_samples [1];
	    right -= apply_weight (dpp->weight_B, sam_B);
	    update_weight (dpp->weight_B, dpp->delta, sam_B, right);
	    dpp->sum_B += dpp->weight_B;
	    out_samples [1] = right;
	    in_samples += dir;
	    out_samples += dir;
	    m = (m + 1) & (MAX_TERM - 1);
	}
    }
    else if (dpp->term == -1) {
	while (num_samples--) {
	    int32_t left = in_samples [0];
	    int32_t right = in_samples [1];
	    int32_t sam_A = dpp->samples_A [0], sam_B = left;

	    dpp->samples_A [0] = right;
	    right -= apply_weight (dpp->weight_B, sam_B);
	    update_weight_clip (dpp->weight_B, dpp->delta, sam_B, right);
	    left -= apply_weight (dpp->weight_A, sam_A);
	    update_weight_clip (dpp->weight_A, dpp->delta, sam_A, left);
	    dpp->sum_A += dpp->weight_A;
	    dpp->sum_B += dpp->weight_B;
	    out_samples [0] = left;
	    out_samples [1] = right;
	    in_samples += dir;
	    out_samples += dir;
	}
    }
    else if (dpp->term == -2) {
	while (num_samples--) {
	    int32_t left = in_samples [0];
	    int32_t right = in_samples [1];
	    int32_t sam_B = dpp->samples_B [0], sam_A = right;

	    dpp->samples_B [0] = left;
	    left -= apply_weight (dpp->weight_A, sam_A);
	    update_weight_clip (dpp->weight_A, dpp->delta, sam_A, left);
	    right -= apply_weight (dpp->weight_B, sam_B);
	    update_weight_clip (dpp->weight_B, dpp->delta, sam_B, right);
	    dpp->sum_A += dpp->weight_A;
	    dpp->sum_B += dpp->weight_B;
	    out_samples [0] = left;
	    out_samples [1] = right;
	    in_samples += dir;
	    out_samples += dir;
	}
    }
    else if (dpp->term == -3) {
	while (num_samples--) {
	    int32_t left = in_samples [0];
	    int32_t right = in_samples [1];
	    int32_t sam_A = dpp->samples_A [0], sam_B = dpp->samples_B [0];

	    dpp->samples_A [0] = right;
	    dpp->samples_B [0] = left;
	    left -= apply_weight (dpp->weight_A, sam_A);
	    update_weight_clip (dpp->weight_A, dpp->delta, sam_A, left);
	    right -= apply_weight (dpp->weight_B, sam_B);
	    update_weight_clip (dpp->weight_B, dpp->delta, sam_B, right);
	    dpp->sum_A += dpp->weight_A;
	    dpp->sum_B += dpp->weight_B;
	    out_samples [0] = left;
	    out_samples [1] = right;
	    in_samples += dir;
	    out_samples += dir;
	}
    }

    if (m && dpp->term > 0 && dpp->term <= MAX_TERM) {
	int32_t temp_A [MAX_TERM], temp_B [MAX_TERM];
	int k;

	memcpy (temp_A, dpp->samples_A, sizeof (dpp->samples_A));
	memcpy (temp_B, dpp->samples_B, sizeof (dpp->samples_B));

	for (k = 0; k < MAX_TERM; k++) {
	    dpp->samples_A [k] = temp_A [m];
	    dpp->samples_B [k] = temp_B [m];
	    m = (m + 1) & (MAX_TERM - 1);
	}
    }
}

static void reverse_decorr (struct decorr_pass *dpp)
{
    if (dpp->term > MAX_TERM) {
	int32_t sam_A, sam_B;

	if (dpp->term & 1) {
	    sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
	    sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];
	}
	else {
	    sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
	    sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;
	}

	dpp->samples_A [1] = dpp->samples_A [0];
	dpp->samples_B [1] = dpp->samples_B [0];
	dpp->samples_A [0] = sam_A;
	dpp->samples_B [0] = sam_B;

	if (dpp->term & 1) {
	    sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
	    sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];
	}
	else {
	    sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
	    sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;
	}

	dpp->samples_A [1] = sam_A;
	dpp->samples_B [1] = sam_B;
    }
    else if (dpp->term > 1) {
	int i = 0, j = dpp->term - 1, cnt = dpp->term / 2;

	while (cnt--) {
	    i &= (MAX_TERM - 1);
	    j &= (MAX_TERM - 1);
	    dpp->samples_A [i] ^= dpp->samples_A [j];
	    dpp->samples_A [j] ^= dpp->samples_A [i];
	    dpp->samples_A [i] ^= dpp->samples_A [j];
	    dpp->samples_B [i] ^= dpp->samples_B [j];
	    dpp->samples_B [j] ^= dpp->samples_B [i];
	    dpp->samples_B [i++] ^= dpp->samples_B [j--];
	}
    }
    else if (dpp->term == -1) {
    }
    else if (dpp->term == -2) {
    }
    else if (dpp->term == -3) {
    }
}

static void decorr_stereo_buffer (int32_t *samples, int32_t *outsamples, int32_t num_samples, struct decorr_pass *dpp, int tindex)
{
    struct decorr_pass dp, *dppi = dpp + tindex;
    int delta = dppi->delta, pre_delta;
    int term = dppi->term;

    if (delta == 7)
	pre_delta = 7;
    else if (delta < 2)
	pre_delta = 3;
    else
	pre_delta = delta + 1;

    CLEAR (dp);
    dp.term = term;
    dp.delta = pre_delta;
    decorr_stereo_pass (samples, outsamples, num_samples > 2048 ? 2048 : num_samples, &dp, -1);
    dp.delta = delta;

    if (tindex == 0)
        reverse_decorr (&dp);
    else {
	CLEAR (dp.samples_A);
	CLEAR (dp.samples_B);
    }

    memcpy (dppi->samples_A, dp.samples_A, sizeof (dp.samples_A));
    memcpy (dppi->samples_B, dp.samples_B, sizeof (dp.samples_B));
    dppi->weight_A = dp.weight_A;
    dppi->weight_B = dp.weight_B;

    if (delta == 0) {
	dp.delta = 1;
	decorr_stereo_pass (samples, outsamples, num_samples, &dp, 1);
	dp.delta = 0;
	memcpy (dp.samples_A, dppi->samples_A, sizeof (dp.samples_A));
	memcpy (dp.samples_B, dppi->samples_B, sizeof (dp.samples_B));
	dppi->weight_A = dp.weight_A = dp.sum_A / num_samples;
	dppi->weight_B = dp.weight_B = dp.sum_B / num_samples;
    }

//    if (memcmp (dppi, &dp, sizeof (dp)))
//	error_line ("decorr_passes don't match, delta = %d", delta);

    decorr_stereo_pass (samples, outsamples, num_samples, &dp, 1);
}

static void recurse_stereo (WavpackContext *wpc, WavpackExtraInfo *info, int depth, int delta, uint32_t input_bits)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
    int term, branches = ((wpc->config.extra_flags & EXTRA_BRANCHES) >> 6) - depth;
    int32_t *samples, *outsamples;
    uint32_t term_bits [22], bits;

    if (branches < 1 || depth + 1 == info->nterms)
	branches = 1;

    CLEAR (term_bits);
    samples = info->sampleptrs [depth];
    outsamples = info->sampleptrs [depth + 1];

    for (term = -3; term <= 18; ++term) {
	if (!term)
	    continue;

	if (term == 17 && branches == 1 && depth + 1 < info->nterms)
	    continue;

	if (term == -1 || term == -2)
	    if (!(wps->wphdr.flags & CROSS_DECORR))
		continue;

	if (term >= 9 && term <= 16)
	    if (term > MAX_TERM || !(wpc->config.flags & CONFIG_HIGH_FLAG) || (wpc->config.extra_flags & EXTRA_SKIP_8TO16))
		continue;

	if ((wpc->config.flags & CONFIG_FAST_FLAG) && (term >= 5 && term <= 16))
	    continue;

	info->dps [depth].term = term;
	info->dps [depth].delta = delta;
	decorr_stereo_buffer (samples, outsamples, wps->wphdr.block_samples, info->dps, depth);
	bits = log2buffer (outsamples, wps->wphdr.block_samples * 2, info->log_limit);

	if (bits < info->best_bits) {
	    info->best_bits = bits;
	    CLEAR (wps->decorr_passes);
	    memcpy (wps->decorr_passes, info->dps, sizeof (info->dps [0]) * (depth + 1));
	    memcpy (info->sampleptrs [info->nterms + 1], info->sampleptrs [depth + 1], wps->wphdr.block_samples * 8);
	}

	term_bits [term + 3] = bits;
    }

    while (depth + 1 < info->nterms && branches--) {
	uint32_t local_best_bits = input_bits;
	int best_term = 0, i;

	for (i = 0; i < 22; ++i)
	    if (term_bits [i] && term_bits [i] < local_best_bits) {
		local_best_bits = term_bits [i];
		term_bits [i] = 0;
		best_term = i - 3;
	    }

	if (!best_term)
	    break;

	info->dps [depth].term = best_term;
	info->dps [depth].delta = delta;
	decorr_stereo_buffer (samples, outsamples, wps->wphdr.block_samples, info->dps, depth);

//	if (log2buffer (outsamples, wps->wphdr.block_samples * 2, 0) != local_best_bits)
//	    error_line ("data doesn't match!");

	recurse_stereo (wpc, info, depth + 1, delta, local_best_bits);
    }
}

static void delta_stereo (WavpackContext *wpc, WavpackExtraInfo *info)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
    int lower = FALSE;
    int delta, d;
    uint32_t bits;

    if (wps->decorr_passes [0].term)
	delta = wps->decorr_passes [0].delta;
    else
	return;

    for (d = delta - 1; d >= 0; --d) {
	int i;

	if (!d && (wps->wphdr.flags & HYBRID_FLAG))
	    break;

	for (i = 0; i < info->nterms && wps->decorr_passes [i].term; ++i) {
	    info->dps [i].term = wps->decorr_passes [i].term;
	    info->dps [i].delta = d;
	    decorr_stereo_buffer (info->sampleptrs [i], info->sampleptrs [i+1], wps->wphdr.block_samples, info->dps, i);
	}

	bits = log2buffer (info->sampleptrs [i], wps->wphdr.block_samples * 2, info->log_limit);

	if (bits < info->best_bits) {
	    lower = TRUE;
	    info->best_bits = bits;
	    CLEAR (wps->decorr_passes);
	    memcpy (wps->decorr_passes, info->dps, sizeof (info->dps [0]) * i);
	    memcpy (info->sampleptrs [info->nterms + 1], info->sampleptrs [i], wps->wphdr.block_samples * 8);
	}
	else
	    break;
    }

    for (d = delta + 1; !lower && d <= 7; ++d) {
	int i;

	for (i = 0; i < info->nterms && wps->decorr_passes [i].term; ++i) {
	    info->dps [i].term = wps->decorr_passes [i].term;
	    info->dps [i].delta = d;
	    decorr_stereo_buffer (info->sampleptrs [i], info->sampleptrs [i+1], wps->wphdr.block_samples, info->dps, i);
	}

	bits = log2buffer (info->sampleptrs [i], wps->wphdr.block_samples * 2, info->log_limit);

	if (bits < info->best_bits) {
	    info->best_bits = bits;
	    CLEAR (wps->decorr_passes);
	    memcpy (wps->decorr_passes, info->dps, sizeof (info->dps [0]) * i);
	    memcpy (info->sampleptrs [info->nterms + 1], info->sampleptrs [i], wps->wphdr.block_samples * 8);
	}
	else
	    break;
    }
}

static void sort_stereo (WavpackContext *wpc, WavpackExtraInfo *info)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
    int reversed = TRUE;
    uint32_t bits;

    while (reversed) {
	int ri, i;

	memcpy (info->dps, wps->decorr_passes, sizeof (wps->decorr_passes));
	reversed = FALSE;

	for (ri = 0; ri < info->nterms && wps->decorr_passes [ri].term; ++ri) {

	    if (ri + 1 >= info->nterms || !wps->decorr_passes [ri+1].term)
		break;

	    if (wps->decorr_passes [ri].term == wps->decorr_passes [ri+1].term) {
		decorr_stereo_buffer (info->sampleptrs [ri], info->sampleptrs [ri+1], wps->wphdr.block_samples, info->dps, ri);
		continue;
	    }

	    info->dps [ri] = wps->decorr_passes [ri+1];
	    info->dps [ri+1] = wps->decorr_passes [ri];

	    for (i = ri; i < info->nterms && wps->decorr_passes [i].term; ++i)
		decorr_stereo_buffer (info->sampleptrs [i], info->sampleptrs [i+1], wps->wphdr.block_samples, info->dps, i);

	    bits = log2buffer (info->sampleptrs [i], wps->wphdr.block_samples * 2, info->log_limit);

	    if (bits < info->best_bits) {
		reversed = TRUE;
		info->best_bits = bits;
		CLEAR (wps->decorr_passes);
		memcpy (wps->decorr_passes, info->dps, sizeof (info->dps [0]) * i);
		memcpy (info->sampleptrs [info->nterms + 1], info->sampleptrs [i], wps->wphdr.block_samples * 8);
	    }
	    else {
		info->dps [ri] = wps->decorr_passes [ri];
		info->dps [ri+1] = wps->decorr_passes [ri+1];
		decorr_stereo_buffer (info->sampleptrs [ri], info->sampleptrs [ri+1], wps->wphdr.block_samples, info->dps, ri);
	    }
	}
    }
}

#define EXTRA_ADVANCED (EXTRA_BRANCHES | EXTRA_SORT_FIRST | EXTRA_SORT_LAST | EXTRA_TRY_DELTAS)

void analyze_stereo (WavpackContext *wpc, int32_t *samples)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
#ifdef EXTRA_DUMP
    uint32_t bits, default_bits, cnt;
#else
    uint32_t bits, cnt;
#endif
    const signed char *decorr_terms = default_terms, *tp;
    WavpackExtraInfo info;
    int32_t *lptr;
    int i;

#ifdef LOG_LIMIT
    info.log_limit = (((wps->wphdr.flags & MAG_MASK) >> MAG_LSB) + 4) * 256;

    if (info.log_limit > LOG_LIMIT)
	info.log_limit = LOG_LIMIT;
#else
    info.log_limit = 0;
#endif

    CLEAR (wps->decorr_passes);
    cnt = wps->wphdr.block_samples * 2;
    lptr = samples;

    while (cnt--)
	if (*lptr++)
	    break;

    if (cnt == (uint32_t) -1) {
	scan_word (wps, samples, wps->wphdr.block_samples, -1);
	wps->num_terms = 0;
	return;
    }

    if (wpc->config.flags & CONFIG_HIGH_FLAG)
	decorr_terms = high_terms;
    else if (wpc->config.flags & CONFIG_FAST_FLAG)
	decorr_terms = fast_terms;

    info.nterms = strlen (decorr_terms);

    if (wpc->config.extra_flags & EXTRA_TERMS)
	if ((info.nterms += (wpc->config.extra_flags & EXTRA_TERMS) >> 10) > MAX_NTERMS)
	    info.nterms = MAX_NTERMS;

    for (i = 0; i < info.nterms + 2; ++i)
	info.sampleptrs [i] = malloc (wps->wphdr.block_samples * 8);

    memcpy (info.sampleptrs [info.nterms + 1], samples, wps->wphdr.block_samples * 8);
    info.best_bits = log2buffer (info.sampleptrs [info.nterms + 1], wps->wphdr.block_samples * 2, 0);

    if ((wpc->config.extra_flags & EXTRA_STEREO_MODES) || !(wps->wphdr.flags & JOINT_STEREO)) {
	memcpy (info.sampleptrs [0], samples, wps->wphdr.block_samples * 8);

	CLEAR (info.dps);

	for (tp = decorr_terms, i = 0; *tp;) {
	    if (*tp > 0 || (wps->wphdr.flags & CROSS_DECORR))
		info.dps [i].term = *tp++;
	    else {
		info.dps [i].term = -3;
		tp++;
	    }

	    info.dps [i].delta = 2;
	    decorr_stereo_buffer (info.sampleptrs [i], info.sampleptrs [i+1], wps->wphdr.block_samples, info.dps, i);
	    ++i;
	}

#ifdef EXTRA_DUMP
	default_bits = bits = log2buffer (info.sampleptrs [i], wps->wphdr.block_samples * 2, 0);
#else
	bits = log2buffer (info.sampleptrs [i], wps->wphdr.block_samples * 2, 0);
#endif

	wps->wphdr.flags &= ~JOINT_STEREO;

	if (bits < info.best_bits) {
	    info.best_bits = bits;
	    CLEAR (wps->decorr_passes);
	    memcpy (wps->decorr_passes, info.dps, sizeof (info.dps [0]) * i);
	    memcpy (info.sampleptrs [info.nterms + 1], info.sampleptrs [i], wps->wphdr.block_samples * 8);
	}
    }

    if ((wpc->config.extra_flags & EXTRA_STEREO_MODES) || (wps->wphdr.flags & JOINT_STEREO)) {
	memcpy (info.sampleptrs [0], samples, wps->wphdr.block_samples * 8);
	cnt = wps->wphdr.block_samples;
	lptr = info.sampleptrs [0];

	while (cnt--) {
	    lptr [1] += ((lptr [0] -= lptr [1]) >> 1);
	    lptr += 2;
	}

	CLEAR (info.dps);

	for (tp = decorr_terms, i = 0; *tp;) {
	    if (*tp > 0 || (wps->wphdr.flags & CROSS_DECORR))
		info.dps [i].term = *tp++;
	    else {
		info.dps [i].term = -3;
		tp++;
	    }

	    info.dps [i].delta = 2;
	    decorr_stereo_buffer (info.sampleptrs [i], info.sampleptrs [i+1], wps->wphdr.block_samples, info.dps, i);
	    ++i;
	}

#ifdef EXTRA_DUMP
	default_bits = bits = log2buffer (info.sampleptrs [i], wps->wphdr.block_samples * 2, 0);
#else
	bits = log2buffer (info.sampleptrs [i], wps->wphdr.block_samples * 2, 0);
#endif

	wps->wphdr.flags |= JOINT_STEREO;

	if (bits < info.best_bits) {
	    info.best_bits = bits;
	    CLEAR (wps->decorr_passes);
	    memcpy (wps->decorr_passes, info.dps, sizeof (info.dps [0]) * i);
	    memcpy (info.sampleptrs [info.nterms + 1], info.sampleptrs [i], wps->wphdr.block_samples * 8);
	}
	else {
	    memcpy (info.sampleptrs [0], samples, wps->wphdr.block_samples * 8);
	    wps->wphdr.flags &= ~JOINT_STEREO;
	}
    }

    if ((wps->wphdr.flags & HYBRID_FLAG) && (wpc->config.extra_flags & EXTRA_ADVANCED)) {
	int shaping_weight, new = wps->wphdr.flags & NEW_SHAPING;
	int32_t *rptr = info.sampleptrs [info.nterms + 1], error [2], temp;

	scan_word (wps, rptr, wps->wphdr.block_samples, -1);
	cnt = wps->wphdr.block_samples;
	lptr = info.sampleptrs [0];
	CLEAR (error);

	if (wps->wphdr.flags & HYBRID_SHAPE) {
	    while (cnt--) {
		shaping_weight = (wps->dc.shaping_acc [0] += wps->dc.shaping_delta [0]) >> 16;
		temp = -apply_weight (shaping_weight, error [0]);

		if (new && shaping_weight < 0 && temp) {
		    if (temp == error [0])
			temp = (temp < 0) ? temp + 1 : temp - 1;

		    lptr [0] += (error [0] = nosend_word (wps, rptr [0], 0) - rptr [0] + temp);
		}
		else
		    lptr [0] += (error [0] = nosend_word (wps, rptr [0], 0) - rptr [0]) + temp;

		shaping_weight = (wps->dc.shaping_acc [1] += wps->dc.shaping_delta [1]) >> 16;
		temp = -apply_weight (shaping_weight, error [1]);

		if (new && shaping_weight < 0 && temp) {
		    if (temp == error [1])
			temp = (temp < 0) ? temp + 1 : temp - 1;

		    lptr [1] += (error [1] = nosend_word (wps, rptr [1], 1) - rptr [1] + temp);
		}
		else
		    lptr [1] += (error [1] = nosend_word (wps, rptr [1], 1) - rptr [1]) + temp;

		lptr += 2;
		rptr += 2;
	    }

	    wps->dc.shaping_acc [0] -= wps->dc.shaping_delta [0] * wps->wphdr.block_samples;
	    wps->dc.shaping_acc [1] -= wps->dc.shaping_delta [1] * wps->wphdr.block_samples;
	}
	else
	    while (cnt--) {
		lptr [0] += nosend_word (wps, rptr [0], 0) - rptr [0];
		lptr [1] += nosend_word (wps, rptr [1], 1) - rptr [1];
		lptr += 2;
		rptr += 2;
	    }

	memcpy (info.dps, wps->decorr_passes, sizeof (info.dps));

	for (i = 0; i < info.nterms && info.dps [i].term; ++i)
	    decorr_stereo_buffer (info.sampleptrs [i], info.sampleptrs [i + 1], wps->wphdr.block_samples, info.dps, i);

#ifdef EXTRA_DUMP
	info.best_bits = default_bits = log2buffer (info.sampleptrs [i], wps->wphdr.block_samples * 2, 0);
#else
	info.best_bits = log2buffer (info.sampleptrs [i], wps->wphdr.block_samples * 2, 0);
#endif

	CLEAR (wps->decorr_passes);
	memcpy (wps->decorr_passes, info.dps, sizeof (info.dps [0]) * i);
	memcpy (info.sampleptrs [info.nterms + 1], info.sampleptrs [i], wps->wphdr.block_samples * 8);
    }

    if (wpc->config.extra_flags & EXTRA_BRANCHES)
	recurse_stereo (wpc, &info, 0, (int) floor (wps->delta_decay + 0.5),
	    log2buffer (info.sampleptrs [0], wps->wphdr.block_samples * 2, 0));

    if (wpc->config.extra_flags & EXTRA_SORT_FIRST)
	sort_stereo (wpc, &info);

    if (wpc->config.extra_flags & EXTRA_TRY_DELTAS) {
	delta_stereo (wpc, &info);

	if ((wpc->config.extra_flags & EXTRA_ADJUST_DELTAS) && wps->decorr_passes [0].term)
	    wps->delta_decay = (wps->delta_decay * 2.0 + wps->decorr_passes [0].delta) / 3.0;
	else
	    wps->delta_decay = 2.0;
    }

    if (wpc->config.extra_flags & EXTRA_SORT_LAST)
	sort_stereo (wpc, &info);

#if 0
    memcpy (info.dps, wps->decorr_passes, sizeof (info.dps));

    for (i = 0; i < info.nterms && info.dps [i].term; ++i)
	decorr_stereo_pass (info.sampleptrs [i], info.sampleptrs [i + 1], wps->wphdr.block_samples, info.dps + i, 1);

    if (log2buffer (info.sampleptrs [i], wps->wphdr.block_samples * 2, 0) != info.best_bits)
	error_line ("(1) samples do not match!");

    if (log2buffer (info.sampleptrs [info.nterms + 1], wps->wphdr.block_samples * 2, 0) != info.best_bits)
	error_line ("(2) samples do not match!");
#endif

    scan_word (wps, info.sampleptrs [info.nterms + 1], wps->wphdr.block_samples, -1);

#ifdef EXTRA_DUMP
    if (1) {
	char string [256], substring [20];
	int i;

	sprintf (string, "%s: delta = %.4f%%, terms =",
	    (wps->wphdr.flags & JOINT_STEREO) ? "JS" : "TS",
		((double) info.best_bits - default_bits) / 256.0 / wps->wphdr.block_samples / 32.0 * 100.0);

	for (i = 0; i < info.nterms; ++i) {
	    if (wps->decorr_passes [i].term) {
		if (i && wps->decorr_passes [i-1].delta == wps->decorr_passes [i].delta)
		    sprintf (substring, " %d", wps->decorr_passes [i].term);
		else
		    sprintf (substring, " %d->%d", wps->decorr_passes [i].term,
			wps->decorr_passes [i].delta);
	    }
	    else
		sprintf (substring, " *");

	    strcat (string, substring);
	}

	error_line (string);
    }
#endif

    for (i = 0; i < info.nterms; ++i)
	if (!wps->decorr_passes [i].term)
	    break;

    wps->num_terms = i;

    for (i = 0; i < info.nterms + 2; ++i)
	free (info.sampleptrs [i]);
}
