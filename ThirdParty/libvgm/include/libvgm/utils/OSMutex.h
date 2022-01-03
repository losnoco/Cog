#ifndef __OSMUTEX_H__
#define __OSMUTEX_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "../stdtype.h"

typedef struct _os_mutex OS_MUTEX;

UINT8 OSMutex_Init(OS_MUTEX** retMutex, UINT8 initLocked);
void OSMutex_Deinit(OS_MUTEX* mtx);
UINT8 OSMutex_Lock(OS_MUTEX* mtx);
UINT8 OSMutex_TryLock(OS_MUTEX* mtx);
UINT8 OSMutex_Unlock(OS_MUTEX* mtx);

#ifdef __cplusplus
}
#endif

#endif	// __OSMUTEX_H__
