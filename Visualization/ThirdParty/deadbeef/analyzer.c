/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Alexey Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/
#include "analyzer.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma mark - Forward declarations

static float
_get_bar_height(ddb_analyzer_t *analyzer, float normalized_height, int view_height);

static void
_generate_octave_note_bars(ddb_analyzer_t *analyzer);

static float
_interpolate_bin_with_ratio(float *fft_data, int bin, float ratio, int fft_size);

#pragma mark - Public

ddb_analyzer_t *
ddb_analyzer_alloc(void) {
	return calloc(1, sizeof(ddb_analyzer_t));
}

ddb_analyzer_t *
ddb_analyzer_init(ddb_analyzer_t *analyzer) {
	analyzer->view_width = 1000;
	analyzer->peak_hold = 10;
	analyzer->peak_speed_scale = 1000.f;
	analyzer->db_lower_bound = -80;
	analyzer->octave_bars_step = 1;
	analyzer->bar_gap_denominator = 3;
	return analyzer;
}

void ddb_analyzer_dealloc(ddb_analyzer_t *analyzer) {
	free(analyzer->fft_data);
	memset(analyzer, 0, sizeof(ddb_analyzer_t));
}

void ddb_analyzer_free(ddb_analyzer_t *analyzer) {
	free(analyzer);
}

void ddb_analyzer_process(ddb_analyzer_t *analyzer, int samplerate, int channels, const float *fft_data, int fft_size) {
	int need_regenerate = 0;

	if(channels > 2) {
		channels = 2;
	}

	if(channels != analyzer->channels || fft_size != analyzer->fft_size || samplerate != analyzer->samplerate) {
		analyzer->channels = channels;
		analyzer->fft_size = fft_size;
		analyzer->samplerate = samplerate;
		free(analyzer->fft_data);
		analyzer->fft_data = malloc(fft_size * channels * sizeof(float));
		need_regenerate = 1;
		analyzer->mode_did_change = 0;
	}

	memcpy(analyzer->fft_data, fft_data, fft_size * channels * sizeof(float));

	if(need_regenerate) {
		_generate_octave_note_bars(analyzer);
	}
}

/// Update bars and peaks for the next frame
void ddb_analyzer_tick(ddb_analyzer_t *analyzer) {
	if(analyzer->mode_did_change) {
		return; // avoid ticks until the next data update
	}
	// frequency lines
	for(int ch = 0; ch < analyzer->channels; ch++) {
		float *fft_data = analyzer->fft_data + ch * analyzer->fft_size;
		ddb_analyzer_bar_t *bar = analyzer->bars;
		for(int i = 0; i < analyzer->bar_count; i++, bar++) {
			float norm_h = fft_data[bar->bin];

			float bound = -analyzer->db_lower_bound;
			float height = (20 * log10(norm_h) + bound) / bound;

			if(ch == 0) {
				bar->height = height;
			} else if(height > bar->height) {
				bar->height = height;
			}
		}
	}

	// peaks
	ddb_analyzer_bar_t *bar = analyzer->bars;
	for(int i = 0; i < analyzer->bar_count; i++, bar++) {
		if(bar->peak < bar->height) {
			bar->peak = bar->height;
			bar->peak_speed = analyzer->peak_hold;
		}

		if(bar->peak_speed-- < 0) {
			bar->peak += bar->peak_speed / analyzer->peak_speed_scale;
			if(bar->peak < bar->height) {
				bar->peak = bar->height;
			}
		}
	}
}

void ddb_analyzer_get_draw_data(ddb_analyzer_t *analyzer, int view_width, int view_height, ddb_analyzer_draw_data_t *draw_data) {
	if(draw_data->bar_count != analyzer->bar_count) {
		free(draw_data->bars);
		draw_data->bars = calloc(analyzer->bar_count, sizeof(ddb_analyzer_draw_bar_t));
		draw_data->bar_count = analyzer->bar_count;
	}

	{
		if(analyzer->fractional_bars) {
			float width = (float)view_width / analyzer->bar_count;
			float gap = analyzer->bar_gap_denominator > 0 ? width / analyzer->bar_gap_denominator : 0;
			draw_data->bar_width = width - gap;
		} else {
			int width = view_width / analyzer->bar_count;
			int gap = analyzer->bar_gap_denominator > 0 ? width / analyzer->bar_gap_denominator : 0;
			if(gap < 1) {
				gap = 1;
			}
			if(width <= 1) {
				width = 1;
				gap = 0;
			}
			draw_data->bar_width = width - gap;
		}
	}

	ddb_analyzer_bar_t *bar = analyzer->bars;
	ddb_analyzer_draw_bar_t *draw_bar = draw_data->bars;
	for(int i = 0; i < analyzer->bar_count; i++, bar++, draw_bar++) {
		float height = bar->height;

		draw_bar->bar_height = _get_bar_height(analyzer, height, view_height);
		draw_bar->xpos = bar->xpos * view_width;
		draw_bar->peak_ypos = _get_bar_height(analyzer, bar->peak, view_height);
	}
}

void ddb_analyzer_draw_data_dealloc(ddb_analyzer_draw_data_t *draw_data) {
	free(draw_data->bars);
	memset(draw_data, 0, sizeof(ddb_analyzer_draw_data_t));
}

#pragma mark - Private

static float
_get_bar_height(ddb_analyzer_t *analyzer, float normalized_height, int view_height) {
	float height = normalized_height;
	if(height < 0) {
		height = 0;
	} else if(height > 1) {
		height = 1;
	}
	height *= view_height;
	return height;
}

static void
_generate_octave_note_bars(ddb_analyzer_t *analyzer) {
	analyzer->bar_count = 0;

	if(analyzer->bar_count_max != 88) {
		free(analyzer->bars);
		analyzer->bars = calloc(88, sizeof(ddb_analyzer_bar_t));
		analyzer->bar_count_max = 88;
	}

	int minBand = -1;
	int maxBand = -1;

	for(int i = 0; i < 88; i += analyzer->octave_bars_step) {
		if(minBand == -1) {
			minBand = i;
		}

		maxBand = i;

		ddb_analyzer_bar_t *bar = analyzer->bars + analyzer->bar_count;

		int bin = i;

		bar->bin = bin;

		analyzer->bar_count += 1;
	}

	for(int i = 0; i < analyzer->bar_count; i++) {
		analyzer->bars[i].xpos = (float)i / analyzer->bar_count;
	}
}

static float
_interpolate_bin_with_ratio(float *fft_data, int bin, float ratio, int fft_size) {
	return bin < fft_size ? (bin + 1 < fft_size ? (fft_data[bin] + (fft_data[bin + 1] - fft_data[bin]) * ratio) : fft_data[bin]) : 0.0;
}
