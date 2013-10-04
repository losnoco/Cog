/* Threads.c -- multithreading library
2012-12-20 : Chris Moeller : Public domain */

#include "Threads.h"

WRes Thread_Create(CThread *p, THREAD_FUNC_TYPE func, LPVOID param)
{
    int ret;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );
    ret = pthread_create( &p->thread, &attr, func, param );
    if ( ret == 0 )
    {
        p->created = TRUE;
        return 0;
    }
    return -1;
}

WRes Event_Create(CEvent *p, BOOL manualReset, int signaled)
{
    pthread_mutex_init( &p->mutex, NULL );
    pthread_cond_init( &p->cond, NULL );
    p->created = TRUE;
    p->autoreset = !manualReset;
    p->triggered = !!signaled;
    return 0;
}

WRes Event_Wait(CEvent *p)
{
    pthread_mutex_lock(&p->mutex);
    while (!p->triggered)
        pthread_cond_wait(&p->cond, &p->mutex);
    if ( p->autoreset )
        p->triggered = FALSE;
    pthread_mutex_unlock(&p->mutex);
    return 0;
}

WRes Event_Set(CEvent *p)
{
    pthread_mutex_lock(&p->mutex);
    p->triggered = TRUE;
    pthread_cond_signal(&p->cond);
    pthread_mutex_unlock(&p->mutex);
    return 0;
}

WRes Event_Reset(CEvent *p)
{
    pthread_mutex_lock(&p->mutex);
    p->triggered = FALSE;
    pthread_mutex_unlock(&p->mutex);
    return 0;
}

WRes ManualResetEvent_Create(CManualResetEvent *p, int signaled) { return Event_Create(p, TRUE, signaled); }
WRes AutoResetEvent_Create(CAutoResetEvent *p, int signaled) { return Event_Create(p, FALSE, signaled); }
WRes ManualResetEvent_CreateNotSignaled(CManualResetEvent *p) { return ManualResetEvent_Create(p, 0); }
WRes AutoResetEvent_CreateNotSignaled(CAutoResetEvent *p) { return AutoResetEvent_Create(p, 0); }


WRes Semaphore_Create(CSemaphore *p, UInt32 initCount, UInt32 maxCount)
{
    sem_init( &p->sem, 0, initCount );
    p->maxCount = maxCount;
    return 0;
}

static WRes Semaphore_Release(CSemaphore *p, LONG releaseCount, LONG *previousCount)
{
    if (previousCount)
    {
        int sval = 0;
        sem_getvalue( &p->sem, &sval );
        *previousCount = sval;
    }
    while (releaseCount--)
        sem_post( &p->sem );
    return 0;
}
WRes Semaphore_ReleaseN(CSemaphore *p, UInt32 num)
  { return Semaphore_Release(p, (LONG)num, NULL); }
WRes Semaphore_Release1(CSemaphore *p) { return Semaphore_ReleaseN(p, 1); }

WRes CriticalSection_Init(CCriticalSection *p)
{
    pthread_mutex_init( p, NULL );
    return 0;
}
