/* LazyUSF Public Interface */

#ifndef _USF_H_
#define _USF_H_
#define _CRT_SECURE_NO_WARNINGS


#include <stdint.h>
#include <stdlib.h>

typedef struct usf_state usf_state_t;

typedef struct usf_state_helper usf_state_helper_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the size of the base emulator state. */
size_t usf_get_state_size();

/* Clears and prepares an allocated state.
   Do not call this on a state which has already been rendering
   without calling usf_shutdown first, or else you will leak memory. */
void usf_clear(void * state);

/* These are both required functions before calling usf_render.
   Their values are assumed to be zero, unless the respective
   _enablecompare or _enablefifofull tags are present in the file. */
void usf_set_compare(void * state, int enable);
void usf_set_fifo_full(void * state, int enable);
    
/* This option should speed up decoding significantly, at the expense
   of accuracy, and potentially emulation bugs. */
void usf_set_hle_audio(void * state, int enable);

/* This processes and uploads the ROM and/or Project 64 save state data
   present in the reserved section of each USF file. They should be
   uploaded in the order in which psf_load processes them, or in priority
   of deepest and first nested _lib first, top level files, then numbered
   _lib# files.
   Returns -1 on invalid data error, or 0 on success. */
int usf_upload_section(void * state, const uint8_t * data, size_t size);

/* Renders at least enough sample DMA blocks to fill the count passed in.
   A null pointer is acceptable, in which case samples will be discarded.
   Requesting zero samples with a null pointer is an acceptable way to
   force at least one block of samples to render and return the current
   sample rate in the variable passed in.
   Requesting a non-zero number of samples with a null buffer pointer will
   result in exactly count samples being rendered and discarded.
   Emulation runs in whole blocks until there have been exactly enough
   Audio Interface DMA transfers to at least fill count samples, at which
   point the remainder is buffered in the emulator state until the next
   usf_render() call.
   Returns 0 on success, or a pointer to the last error message on failure. */
const char * usf_render(void * state, int16_t * buffer, size_t count, int32_t * sample_rate);

/* Reloads the ROM and save state, effectively restarting emulation. Also
   discards any buffered sample data. */
void usf_restart(void * state);

/* Frees all allocated memory associated with the emulator state. Necessary
   after at least one call to usf_render, or else the memory will be leaked. */
void usf_shutdown(void * state);

#ifdef __cplusplus
}
#endif

#endif
