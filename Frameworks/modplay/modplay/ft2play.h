#ifndef _FT2PLAY_H_
#define _FT2PLAY_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
    
enum
{
    FT2_RAMP_NONE = 0,
    FT2_RAMP_ONOFF_ONLY = 1,
    FT2_RAMP_FULL = 2
};
    
enum
{
    FT2_INTERPOLATE_ZOH = 0,
    FT2_INTERPOLATE_LINEAR = 1,
    FT2_INTERPOLATE_CUBIC = 2,
    FT2_INTERPOLATE_SINC = 3
};
    
void * ft2play_Alloc(uint32_t _samplingFrequency, int8_t interpolation, int8_t ramp_style);
void ft2play_Free(void *);

int8_t ft2play_LoadModule(void *, const uint8_t *buffer, size_t size);

void ft2play_PlaySong(void *, int32_t startOrder);

/* Calling this function with a NULL buffer skips mixing altogether */
void ft2play_RenderFloat(void *, float *buffer, int32_t count);
    
/* These two absolutely require a real buffer */
void ft2play_RenderFixed32(void *, int32_t *buffer, int32_t count, int8_t depth);
void ft2play_RenderFixed16(void *, int16_t *buffer, int32_t count, int8_t depth);

void ft2play_Mute(void *, int8_t channel, int8_t mute);
    
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
