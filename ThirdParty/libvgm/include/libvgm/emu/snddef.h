#ifndef __SNDDEF_H__
#define __SNDDEF_H__

#include "../stdtype.h"

// DEV_SMPL is used to represent a single sample in a sound stream
typedef INT32 DEV_SMPL;

// generic device data structure
// MUST be the first variable included in all device-specifc structures
// (i.e. all sound cores inherit this structure)
typedef struct _device_data
{
	void* chipInf;	// pointer to CHIP_INF (depends on specific chip)
} DEV_DATA;

#endif	// __SNDDEF_H__
