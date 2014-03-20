#ifndef _LANCZOS_RESAMPLER_H_
#define _LANCZOS_RESAMPLER_H_

// Ugglay
#ifdef LANCZOS_DECORATE
#define PASTE(a,b) a ## b
#define EVALUATE(a,b) PASTE(a,b)
#define lanczos_init EVALUATE(LANCZOS_DECORATE,_lanczos_init)
#define lanczos_resampler_create EVALUATE(LANCZOS_DECORATE,_lanczos_resampler_create)
#define lanczos_resampler_delete EVALUATE(LANCZOS_DECORATE,_lanczos_resampler_delete)
#define lanczos_resampler_dup EVALUATE(LANCZOS_DECORATE,_lanczos_resampler_dup)
#define lanczos_resampler_dup_inplace EVALUATE(LANCZOS_DECORATE,_lanczos_resampler_dup_inplace)
#define lanczos_resampler_get_free_count EVALUATE(LANCZOS_DECORATE,_lanczos_resampler_get_free_count)
#define lanczos_resampler_write_sample EVALUATE(LANCZOS_DECORATE,_lanczos_resampler_write_sample)
#define lanczos_resampler_set_rate EVALUATE(LANCZOS_DECORATE,_lanczos_resampler_set_rate)
#define lanczos_resampler_ready EVALUATE(LANCZOS_DECORATE,_lanczos_resampler_ready)
#define lanczos_resampler_clear EVALUATE(LANCZOS_DECORATE,_lanczos_resampler_clear)
#define lanczos_resampler_get_sample_count EVALUATE(LANCZOS_DECORATE,_lanczos_resampler_get_sample_count)
#define lanczos_resampler_get_sample EVALUATE(LANCZOS_DECORATE,_lanczos_resampler_get_sample)
#define lanczos_resampler_remove_sample EVALUATE(LANCZOS_DECORATE,_lanczos_resampler_remove_sample)
#endif

void lanczos_init(void);

void * lanczos_resampler_create(void);
void lanczos_resampler_delete(void *);
void * lanczos_resampler_dup(const void *);
void lanczos_resampler_dup_inplace(void *, const void *);

int lanczos_resampler_get_free_count(void *);
void lanczos_resampler_write_sample(void *, short sample);
void lanczos_resampler_set_rate( void *, double new_factor );
int lanczos_resampler_ready(void *);
void lanczos_resampler_clear(void *);
int lanczos_resampler_get_sample_count(void *);
float lanczos_resampler_get_sample(void *);
void lanczos_resampler_remove_sample(void *);

#endif
