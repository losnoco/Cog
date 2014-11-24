#ifndef _CPU_HLE_OS_
#define _CPU_HLE_OS_

#include "cpu_hle.h"


#pragma pack(1)


#define OS_STATE_STOPPED	1
#define OS_STATE_RUNNABLE	2
#define OS_STATE_RUNNING	4
#define OS_STATE_WAITING	8

typedef uint32_t	OSPri;
typedef uint32_t	OSId;
typedef
	union
	{
		struct
		{
			float f_odd;
			float f_even;
		} f;
		double d;
	}
__OSfp;

typedef struct {
	uint64_t	at, v0, v1, a0, a1, a2, a3;
	uint64_t	t0, t1, t2, t3, t4, t5, t6, t7;
	uint64_t	s0, s1, s2, s3, s4, s5, s6, s7;
	uint64_t	t8, t9,         gp, sp, s8, ra;
	uint64_t	lo, hi;
	uint32_t	sr, pc, cause, badvaddr, rcp;
	uint32_t	fpcsr;
	__OSfp	 fp0,  fp2,  fp4,  fp6,  fp8, fp10, fp12, fp14;
	__OSfp	fp16, fp18, fp20, fp22, fp24, fp26, fp28, fp30;
} __OSThreadContext;

typedef struct OSThread_s
{
	uint32_t                next;		// run/mesg queue link
	OSPri                   priority;	// run/mesg queue priority
	uint32_t                queue;      // queue thread is on
	uint32_t                tlnext;     // all threads queue link
#if 0
	uint16_t			state;					// OS_STATE_*
	uint16_t			flags;					// flags for rmon
#endif
	//swap these because of byteswapping
	uint16_t			flags;					// flags for rmon
	uint16_t			state;					// OS_STATE_*
	OSId		id;						// id for debugging
	int			fp;						// thread has used fp unit
	__OSThreadContext	context;		// register/interrupt mask
} OSThread;

typedef void *	OSMesg;

//
// Structure for message queue
//
typedef struct OSMesgQueue_s
{
	OSThread	*mtqueue;		// Queue to store threads blocked
								//   on empty mailboxes (receive)
	OSThread	*fullqueue;		// Queue to store threads blocked
								//   on full mailboxes (send)
	int32_t		validCount;		// Contains number of valid message
	int32_t		first;			// Points to first valid message
	int32_t		msgCount;		// Contains total # of messages
	OSMesg		*msg;			// Points to message buffer array
} OSMesgQueue;


int __osRestoreInt(usf_state_t *, int n);
int __osDisableInt(usf_state_t *, int n);
int __osEnqueueThread(usf_state_t *, int n) ;

int osStartThread(usf_state_t *, int n);
int osRecvMesg(usf_state_t *, int n);
int osSetIntMask(usf_state_t *, int paddr) ;
int osVirtualToPhysical(usf_state_t *, int paddr);
int osAiSetNextBuffer(usf_state_t *, int paddr);

int saveThreadContext(usf_state_t *, int paddr);
int loadThreadContext(usf_state_t *, int paddr);
#endif
