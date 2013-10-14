#ifndef _LANCZOS_RESAMPLER_H_
#define _LANCZOS_RESAMPLER_H_

#ifdef __cplusplus
extern "C" {
#endif

void lanczos_init();

void * lanczos_resampler_create();
void lanczos_resampler_delete(void *);

int lanczos_resampler_get_free_count(void *);
void lanczos_resampler_write_sample(void *, int sample);
void lanczos_resampler_set_rate( void *, double new_factor );
int lanczos_resampler_ready(void *);
void lanczos_resampler_clear(void *);
int lanczos_resampler_get_sample_count(void *);
int lanczos_resampler_get_sample(void *);
void lanczos_resampler_remove_sample(void *);

#ifdef __cplusplus
};
#endif

#endif
