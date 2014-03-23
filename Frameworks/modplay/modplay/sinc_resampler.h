#ifndef _SINC_RESAMPLER_H_
#define _SINC_RESAMPLER_H_

// Ugglay
#ifdef SINC_DECORATE
#define PASTE(a,b) a ## b
#define EVALUATE(a,b) PASTE(a,b)
#define sinc_init EVALUATE(SINC_DECORATE,_sinc_init)
#define sinc_resampler_create EVALUATE(SINC_DECORATE,_sinc_resampler_create)
#define sinc_resampler_delete EVALUATE(SINC_DECORATE,_sinc_resampler_delete)
#define sinc_resampler_dup EVALUATE(SINC_DECORATE,_sinc_resampler_dup)
#define sinc_resampler_dup_inplace EVALUATE(SINC_DECORATE,_sinc_resampler_dup_inplace)
#define sinc_resampler_get_free_count EVALUATE(SINC_DECORATE,_sinc_resampler_get_free_count)
#define sinc_resampler_write_sample EVALUATE(SINC_DECORATE,_sinc_resampler_write_sample)
#define sinc_resampler_set_rate EVALUATE(SINC_DECORATE,_sinc_resampler_set_rate)
#define sinc_resampler_ready EVALUATE(SINC_DECORATE,_sinc_resampler_ready)
#define sinc_resampler_clear EVALUATE(SINC_DECORATE,_sinc_resampler_clear)
#define sinc_resampler_get_sample_count EVALUATE(SINC_DECORATE,_sinc_resampler_get_sample_count)
#define sinc_resampler_get_sample EVALUATE(SINC_DECORATE,_sinc_resampler_get_sample)
#define sinc_resampler_remove_sample EVALUATE(SINC_DECORATE,_sinc_resampler_remove_sample)
#endif

void sinc_init(void);

void * sinc_resampler_create(void);
void sinc_resampler_delete(void *);
void * sinc_resampler_dup(const void *);
void sinc_resampler_dup_inplace(void *, const void *);

int sinc_resampler_get_free_count(void *);
void sinc_resampler_write_sample(void *, short sample);
void sinc_resampler_set_rate( void *, double new_factor );
int sinc_resampler_ready(void *);
void sinc_resampler_clear(void *);
int sinc_resampler_get_sample_count(void *);
float sinc_resampler_get_sample(void *);
void sinc_resampler_remove_sample(void *);

#endif
