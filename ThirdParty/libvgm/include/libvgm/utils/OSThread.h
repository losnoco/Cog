#ifndef __OSTHREAD_H__
#define __OSTHREAD_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "../stdtype.h"

typedef struct _os_thread OS_THREAD;
typedef void (*OS_THR_FUNC)(void* args);

UINT8 OSThread_Init(OS_THREAD** retThread, OS_THR_FUNC threadFunc, void* args);
void OSThread_Deinit(OS_THREAD* thr);
void OSThread_Join(OS_THREAD* thr);
void OSThread_Cancel(OS_THREAD* thr);
UINT64 OSThread_GetID(const OS_THREAD* thr);
void* OSThread_GetHandle(OS_THREAD* thr);	// return a reference to the actual handle

#ifdef __cplusplus
}
#endif

#endif	// __OSTHREAD_H__
