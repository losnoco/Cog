/*
Author: John Tsiombikas <nuclear@member.fsf.org>

I place this piece of code in the public domain. Feel free to use as you see
fit.  I'd appreciate it if you keep my name at the top of the code somehwere,
but whatever.

Main project site: https://github.com/jtsiomb/c11threads
*/

/* TODO: port to MacOSX: no timed mutexes under macosx...
 * just delete that bit if you don't care about timed mutexes
 */

#ifndef C11THREADS_H_
#define C11THREADS_H_

#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>	/* for sched_yield */
#include <sys/time.h>

#define ONCE_FLAG_INIT	PTHREAD_ONCE_INIT

/* types */
typedef pthread_t thrd_t;
typedef pthread_mutex_t mtx_t;
typedef pthread_cond_t cnd_t;
typedef pthread_key_t tss_t;
typedef pthread_once_t once_flag;

typedef int (*thrd_start_t)(void*);
typedef void (*tss_dtor_t)(void*);


typedef struct {
	time_t sec;
	long nsec;
} xtime;

enum {
	mtx_plain		= 0,
	mtx_recursive	= 1,
	mtx_timed		= 2,
	mtx_try			= 4
};

enum {
	thrd_success,
	thrd_busy,
	thrd_error,
	thrd_nomem
};


/* ---- thread management ---- */

static inline int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
	/* XXX there's a third possible value returned according to the standard:
	 * thrd_nomem. but it doesn't seem to correspond to any pthread_create errors.
	 */
	return pthread_create(thr, 0, (void*(*)(void*))func, arg) == 0 ? thrd_success : thrd_error;
}

static inline void thrd_exit(int res)
{
	pthread_exit((void*)(long)res);
}

static inline int thrd_join(thrd_t thr, int *res)
{
	void *retval;

	if(pthread_join(thr, &retval) != 0) {
		return thrd_error;
	}
	if(res) {
		*res = (long)retval;
	}
	return thrd_success;
}

static inline int thrd_detach(thrd_t thr)
{
	return pthread_detach(thr) == 0 ? thrd_success : thrd_error;
}

static inline thrd_t thrd_current(void)
{
	return pthread_self();
}

static inline int thrd_equal(thrd_t a, thrd_t b)
{
	return pthread_equal(a, b);
}

static inline void thrd_sleep(const xtime *xt)
{
	int res;
	struct timespec ts;
	ts.tv_sec = (long)xt->sec;
	ts.tv_nsec = xt->nsec;

	do {
		struct timespec rem;
		res = nanosleep(&ts, &rem);
		ts = rem;
	} while(res == -1 && errno == EINTR);
}

static inline void thrd_yield(void)
{
	sched_yield();
}

/* ---- mutexes ---- */

static inline int mtx_init(mtx_t *mtx, int type)
{
	int res;
	pthread_mutexattr_t attr;

	pthread_mutexattr_init(&attr);

	/* XXX I don't think these are exactly correct semantics */
	if(type & mtx_try) {
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
	}
#if 0
	if(type & mtx_timed) {
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_TIMED_NP);
	}
#endif
	if(type & mtx_recursive) {
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	}

	res = pthread_mutex_init(mtx, &attr) == 0 ? thrd_success : thrd_error;
	pthread_mutexattr_destroy(&attr);
	return res;
}

static inline void mtx_destroy(mtx_t *mtx)
{
	pthread_mutex_destroy(mtx);
}

static inline int mtx_lock(mtx_t *mtx)
{
	int res = pthread_mutex_lock(mtx);
	if(res == EDEADLK) {
		return thrd_busy;
	}
	return res == 0 ? thrd_success : thrd_error;
}

static inline int mtx_trylock(mtx_t *mtx)
{
	int res = pthread_mutex_trylock(mtx);
	if(res == EBUSY) {
		return thrd_busy;
	}
	return res == 0 ? thrd_success : thrd_error;
}
#if 0
static inline int mtx_timedlock(mtx_t *mtx, const xtime *xt)
{
	int res;
	struct timespec ts;

	ts.tv_sec = (long)xt->sec;
	ts.tv_nsec = xt->nsec;

	if((res = pthread_mutex_timedlock(mtx, &ts)) == EBUSY) {
		return thrd_busy;
	}
	return res == 0 ? thrd_success : thrd_error;
}
#endif
static inline int mtx_unlock(mtx_t *mtx)
{
	return pthread_mutex_unlock(mtx) == 0 ? thrd_success : thrd_error;
}

/* ---- condition variables ---- */

static inline int cnd_init(cnd_t *cond)
{
	return pthread_cond_init(cond, 0) == 0 ? thrd_success : thrd_error;
}

static inline void cnd_destroy(cnd_t *cond)
{
	pthread_cond_destroy(cond);
}

static inline int cnd_signal(cnd_t *cond)
{
	return pthread_cond_signal(cond) == 0 ? thrd_success : thrd_error;
}

static inline int cnd_broadcast(cnd_t *cond)
{
	return pthread_cond_broadcast(cond) == 0 ? thrd_success : thrd_error;
}

static inline int cnd_wait(cnd_t *cond, mtx_t *mtx)
{
	return pthread_cond_wait(cond, mtx) == 0 ? thrd_success : thrd_error;
}

static inline int cnd_timedwait(cnd_t *cond, mtx_t *mtx, const xtime *xt)
{
	int res;
	struct timespec ts;

	ts.tv_sec = (long)xt->sec;
	ts.tv_nsec = xt->nsec;

	if((res = pthread_cond_timedwait(cond, mtx, &ts)) != 0) {
		return res == ETIMEDOUT ? thrd_busy : thrd_error;
	}
	return thrd_success;
}

/* ---- thread-specific data ---- */

static inline int tss_create(tss_t *key, tss_dtor_t dtor)
{
	return pthread_key_create(key, dtor) == 0 ? thrd_success : thrd_error;
}

static inline void tss_delete(tss_t key)
{
	pthread_key_delete(key);
}

static inline int tss_set(tss_t key, void *val)
{
	return pthread_setspecific(key, val) == 0 ? thrd_success : thrd_error;
}

static inline void *tss_get(tss_t key)
{
	return pthread_getspecific(key);
}

/* ---- misc ---- */

static inline void call_once(once_flag *flag, void (*func)(void))
{
	pthread_once(flag, func);
}

/* TODO take base into account */
static inline int xtime_get(xtime *xt, int base)
{
	struct timeval tv;

	gettimeofday(&tv, 0);

	xt->sec = tv.tv_sec;
	xt->nsec = tv.tv_usec * 1000;
	return base;
}

#endif	/* C11THREADS_H_ */
