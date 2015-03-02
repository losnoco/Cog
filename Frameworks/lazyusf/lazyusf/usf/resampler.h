#ifndef _RESAMPLER_H_
#define _RESAMPLER_H_

#ifdef RESAMPLER_DECORATE
#define PASTE(a,b) a ## b
#define EVALUATE(a,b) PASTE(a,b)
#define resampler_create EVALUATE(RESAMPLER_DECORATE,_resampler_create)
#define resampler_delete EVALUATE(RESAMPLER_DECORATE,_resampler_delete)
#define resampler_dup EVALUATE(RESAMPLER_DECORATE,_resampler_dup)
#define resampler_dup_inplace EVALUATE(RESAMPLER_DECORATE,_resampler_dup_inplace)
#define resampler_set_quality EVALUATE(RESAMPLER_DECORATE,_resampler_set_quality)
#define resampler_get_free_count EVALUATE(RESAMPLER_DECORATE,_resampler_get_free_count)
#define resampler_write_sample EVALUATE(RESAMPLER_DECORATE,_resampler_write_sample)
#define resampler_write_sample_fixed EVALUATE(RESAMPLER_DECORATE,_resampler_write_sample_fixed)
#define resampler_set_rate EVALUATE(RESAMPLER_DECORATE,_resampler_set_rate)
#define resampler_ready EVALUATE(RESAMPLER_DECORATE,_resampler_ready)
#define resampler_clear EVALUATE(RESAMPLER_DECORATE,_resampler_clear)
#define resampler_get_sample_count EVALUATE(RESAMPLER_DECORATE,_resampler_get_sample_count)
#define resampler_get_sample EVALUATE(RESAMPLER_DECORATE,_resampler_get_sample)
#define resampler_get_sample_float EVALUATE(RESAMPLER_DECORATE,_resampler_get_sample_float)
#define resampler_remove_sample EVALUATE(RESAMPLER_DECORATE,_resampler_remove_sample)
#endif

void * resampler_create(void);
void resampler_delete(void *);
void * resampler_dup(const void *);
void resampler_dup_inplace(void *, const void *);

int resampler_get_free_count(void *);
void resampler_write_sample(void *, short sample_l, short sample_r);
void resampler_set_rate( void *, double new_factor );
int resampler_ready(void *);
void resampler_clear(void *);
int resampler_get_sample_count(void *);
void resampler_get_sample(void *, short * sample_l, short * sample_r);
void resampler_remove_sample(void *);

#endif
