#ifndef __OSSIGNAL_H__
#define __OSSIGNAL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "../stdtype.h"

typedef struct _os_signal OS_SIGNAL;

UINT8 OSSignal_Init(OS_SIGNAL** retSignal, UINT8 initState);
void OSSignal_Deinit(OS_SIGNAL* sig);
UINT8 OSSignal_Signal(OS_SIGNAL* sig);
UINT8 OSSignal_Reset(OS_SIGNAL* sig);
UINT8 OSSignal_Wait(OS_SIGNAL* sig);

#ifdef __cplusplus
}
#endif

#endif	// __OSSIGNAL_H__
