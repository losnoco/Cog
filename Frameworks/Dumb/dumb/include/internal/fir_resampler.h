#ifndef _FIR_RESAMPLER_H_
#define _FIR_RESAMPLER_H_

void fir_init();

void * fir_resampler_create();
void fir_resampler_delete(void *);
void * fir_resampler_dup(void *);

int fir_resampler_get_free_count(void *);
void fir_resampler_write_sample(void *, short sample);
void fir_resampler_set_rate( void *, double new_factor );
int fir_resampler_ready(void *);
void fir_resampler_clear(void *);
int fir_resampler_get_sample(void *);
void fir_resampler_remove_sample(void *);

#endif
