#ifndef __QSOUND_CTR_H__
#define __QSOUND_CTR_H__

#include <stdint.h>

typedef int8_t INT8;
typedef int16_t INT16;
typedef int32_t INT32;
typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;

void qsoundc_update(void* param, UINT32 samples, INT16* output);
UINT32 device_get_qsound_ctr_state_size();
UINT32 device_start_qsound_ctr(UINT32 clock, void* retDevInf);
void device_stop_qsound_ctr(void* info);
void device_reset_qsound_ctr(void* info);

UINT8 qsoundc_r(void* info, UINT8 offset);
void qsoundc_w(void* info, UINT8 offset, UINT8 data);
void qsoundc_write_data(void* info, UINT8 address, UINT16 data);

void qsoundc_alloc_rom(void* info, UINT32 memsize);
void qsoundc_write_rom(void* info, UINT32 offset, UINT32 length, const UINT8* data);
void qsoundc_set_mute_mask(void* info, UINT32 MuteMask);

#endif	// __QSOUND_CTR_H__
