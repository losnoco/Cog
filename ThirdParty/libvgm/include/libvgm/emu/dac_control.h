#ifndef __DAC_CONTROL_H__
#define __DAC_CONTROL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "../stdtype.h"
#include "EmuStructs.h"

void daccontrol_update(void* info, UINT32 samples, DEV_SMPL** dummy);
UINT8 device_start_daccontrol(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf);
void device_stop_daccontrol(void* info);
void device_reset_daccontrol(void* info);

void daccontrol_setup_chip(void* info, DEV_INFO* devInf, UINT8 ChType, UINT16 Command);
void daccontrol_set_data(void* info, UINT8* Data, UINT32 DataLen, UINT8 StepSize, UINT8 StepBase);
void daccontrol_refresh_data(void* info, UINT8* Data, UINT32 DataLen);
void daccontrol_set_frequency(void* info, UINT32 Frequency);
void daccontrol_start(void* info, UINT32 DataPos, UINT8 LenMode, UINT32 Length);
void daccontrol_stop(void* info);

#define DCTRL_LMODE_IGNORE	0x00
#define DCTRL_LMODE_CMDS	0x01
#define DCTRL_LMODE_MSEC	0x02
#define DCTRL_LMODE_TOEND	0x03
#define DCTRL_LMODE_BYTES	0x0F

#ifdef __cplusplus
}
#endif

#endif	// __DAC_CONTROL_H__
