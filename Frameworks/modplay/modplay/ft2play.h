#ifndef _FT2PLAY_H_
#define _FT2PLAY_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
    
void * ft2play_Alloc(uint32_t _samplingFrequency, int8_t interpolation);
void ft2play_Free(void *);

int8_t ft2play_LoadModule(void *, const int8_t *buffer, size_t size);

void ft2play_PlaySong(void *, int32_t startOrder);

/* Calling this function with a NULL buffer skips mixing altogether */
void ft2play_RenderFloat(void *, float *buffer, int32_t count);
    
/* These two absolutely require a real buffer */
void ft2play_RenderFixed32(void *, int32_t *buffer, int32_t count, int8_t depth);
void ft2play_RenderFixed16(void *, int16_t *buffer, int32_t count, int8_t depth);
    
uint32_t ft2play_GetLoopCount(void *);

typedef struct
{
    uint16_t order;
    uint16_t pattern;
    uint16_t row;
    uint16_t speed;
    uint16_t tempo;
    uint8_t channels_playing;
} ft2_info;
    
void ft2play_GetInfo(void *, ft2_info *);
    
#ifdef __cplusplus
}
#endif

#endif
