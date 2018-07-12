#ifndef _CPU_HLE_
#define _CPU_HLE_

#include "usf.h"
#include "cpu.h"
#include "interpreter_ops.h"
#include "memory.h"

typedef struct {
	char *name;
	int num;
	int length;
	long bytes[80];
	int used;
	int phys;
	int (*location)(usf_state_t *, int);
} _HLE_Entry;

int CPUHLE_Scan(usf_state_t *);
int DoCPUHLE(usf_state_t *, unsigned long loc);
////////////////////////////////////////////////////////////////////
// OS Thread Stuff
// found this stuff in daedalus



#endif
