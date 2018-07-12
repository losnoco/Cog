#include <string.h>

#include "usf.h"
#include "usf_internal.h"

#include "os.h"
#include "cpu_hle.h"

#include "audio.h"
#include "interpreter_cpu.h"
#include "main.h"
#include "memory.h"


#define N64WORD(x)		(*(uint32_t*)PageVRAM((x)))
#define N64HALF(x)		(*(uint16_t*)PageVRAM((x)))
#define N64BYTE(x)		(*(uint8_t*)PageVRAM((x^3)))

#define N64DWORD(addr) (((long long)N64WORD((addr))) << 32) + N64WORD((addr)+4);



int __osRestoreInt(usf_state_t * state, int n)
{
	STATUS_REGISTER |= state->GPR[0x4].UW[0];
	return 1;
}

int __osDisableInt(usf_state_t * state, int n)
{
	state->GPR[0x2].UW[0] = STATUS_REGISTER & 1;
	STATUS_REGISTER &= 0xFFFFFFFE;
	return 1;
}


void osEnqueueThread(usf_state_t * state, uint32_t osThreadQueueAddr, uint32_t threadVAddr)
{

	OSThread *thread = (OSThread*) PageVRAM(threadVAddr);
	OSThread *oldThread = (OSThread*) PageVRAM(osThreadQueueAddr);
	OSThread *curThread = (OSThread*) PageVRAM(oldThread->next);

	while((int32_t)curThread->priority >= (int32_t)thread->priority) {
		oldThread = curThread;
		curThread = (OSThread*) PageVRAM(curThread->next);
	}

	thread->next = oldThread->next;
	oldThread->next = threadVAddr;
	thread->queue = osThreadQueueAddr;
}

int __osEnqueueThread(usf_state_t * state, int n) {
	osEnqueueThread(state, state->GPR[4].UW[0],state->GPR[5].UW[0]);
	return 1;
}

int osStartThread(usf_state_t * state, int n)
{
	OSMesgQueue *osThreadQueue = NULL;
	uint32_t osThreadQueueAddr = 0;
	uint32_t oldStatus = STATUS_REGISTER & 1;
	uint32_t osActiveThreadAddr = 0;
	uint32_t osActiveThread = 0;

	OSThread *thread = (OSThread*)PageVRAM(state->GPR[4].UW[0]);

	STATUS_REGISTER &= 0xFFFFFFFE;

	osThreadQueueAddr = ((*(uint32_t*)PageRAM2(n + 0x40)) & 0xFFFF) << 16;
	osThreadQueueAddr += *(int16_t*)PageRAM2(n + 0x50);

	osThreadQueue = (OSMesgQueue*) PageVRAM(osThreadQueueAddr);

	if(thread->state != 8 ) {
		DisplayError(state, "OMG, thread state is not OS_STATE_WAITING!\n");
        return 0;
	}

	thread->state = OS_STATE_RUNNABLE;
	osEnqueueThread(state,osThreadQueueAddr,state->GPR[4].UW[0]);

	osActiveThreadAddr = ((*(uint32_t*)PageRAM2(n + 0xDC)) & 0xFFFF) << 16;
	osActiveThreadAddr += *(int16_t*)PageRAM2(n + 0xE0);

	osActiveThread = *(uint32_t*)PageVRAM(osActiveThreadAddr);

	if(osActiveThread==0) {
		DisplayError(state,"OMG, active thread is NULL!\n");
        return 0;
	}

	STATUS_REGISTER |= oldStatus;
//	CheckInterrupts();

	return 1;
}


int osRecvMesg(usf_state_t * state, int n)
{
	//unsigned long devAddr = state->GPR[7].UW[0];
	//unsigned long vAddr = state->GPR[0x11].UW[0];
	//unsigned long nbytes = state->GPR[0x10].UW[0];

	//unsigned long oldStatus = STATUS_REGISTER & 1;

	RunFunction(state, n | (state->PROGRAM_COUNTER & 0xF0000000));

	//DisplayError("%08x\n%08x\n%08x",devAddr, vAddr, nbytes);

	return 1;
}

// doesnt even use?
int osSetIntMask(usf_state_t * state, int paddr) {
#if 0
	uint32_t globalIntMask = 0;
	uint32_t mask = STATUS_REGISTER & 0xFF01;
	uint32_t interrupts = 0;
	uint32_t intAddress = 0, newMask = 0, workMask = 0;

	uint32_t baseAddress = 0;

	globalIntMask = ((*(uint16_t*)PageRAM2(paddr + 0x8)) & 0xFFFF) << 16;
	globalIntMask += *(uint16_t*)PageRAM2(paddr + 0xc);
	globalIntMask = *(uint32_t*)PageVRAM(globalIntMask);

	interrupts = (globalIntMask ^ 0xffffffff) & 0xff00;
	mask |= interrupts;
	newMask = MI_INTR_MASK_REG;

	if(!newMask)
		newMask = ((globalIntMask >> 16) ^ 0xFFFFFFFF) & 0x3F;

	mask |= (newMask << 16);

	baseAddress = ((*(uint16_t*)PageRAM2(paddr + 0x5C)) & 0xFFFF) << 16;
	baseAddress += *(int16_t*)PageRAM2(paddr + 0x64);
	baseAddress += ((state->GPR[4].UW[0] & 0x3F0000) & globalIntMask) >> 15;;

	MI_INTR_MASK_REG = *(uint16_t*)PageVRAM(baseAddress);

	state->STATUS_REGISTER = ((state->GPR[4].UW[0] & 0xff01) & (globalIntMask & 0xff00)) | (STATUS_REGISTER & 0xFFFF00FF);

#endif
	return 1;
}


int osVirtualToPhysical(usf_state_t * state, int paddr) {
	uintptr_t address = 0;
	uintptr_t vaddr = state->GPR[4].UW[0];

	address = (state->TLB_Map[vaddr >> 12] + vaddr) - (uintptr_t)state->N64MEM;

	if(address < 0x800000) {
		state->GPR[2].UW[0] = (uint32_t)address;
	} else
		state->GPR[2].UW[0] = 0xFFFFFFFF;

	return 1;
}

int osAiSetNextBuffer(usf_state_t * state, int paddr) {
	uint32_t var = 0, var2 = 0;
	var = ((*(int16_t*)PageRAM2(paddr + 0x4)) & 0xFFFF) << 16;
	var += *(int16_t*)PageRAM2(paddr + 0x8);

	var2 = N64WORD(var);
	if(AI_CONTROL_REG & 0x80000000)
		state->GPR[2].UW[0] = -1;

	AI_DRAM_ADDR_REG = state->GPR[4].UW[0];
	AI_LEN_REG = state->GPR[5].UW[0]&0x3FFF;
	AiLenChanged(state);
	state->GPR[2].UW[0] = 0;
	return 1;
}

int saveThreadContext(usf_state_t * state, int paddr) {
#if 0
	uint32_t OSThreadContextAddr = 0;

	OSThreadContextAddr = ((*(int16_t*)PageRAM2(paddr)) & 0xFFFF) << 16;
	OSThreadContextAddr += *(int16_t*)PageRAM2(paddr + 0x4);

	OSThreadContextAddr = N64WORD(OSThreadContextAddr);

	if((PageVRAM2(OSThreadContextAddr) & 0xffff) > 0xFF00) {
		DisplayError(state,"OMG! Too high!");
		return 0;
	}
#endif
	return 0;
}

int loadThreadContext(usf_state_t * state, int paddr) {
#if 0
    uint32_t i = 0, OSThreadContextAddr = 0, T9 = 0, osOSThread = 0, Addr2 = 0, GlobalBitMask = 0, Tmp = 0;
	uint32_t K0 = 0, K1 = 0, T0 = 0, R1 = 0, RCP = 0, intrList = 0;
	OSThread t;
	OSThreadContextAddr = ((*(int16_t*)PageRAM2(paddr)) & 0xFFFF) << 16;
	OSThreadContextAddr += *(int16_t*)PageRAM2(paddr + 0x8);

	Addr2 = ((*(int16_t*)PageRAM2(paddr + 0xC)) & 0xFFFF) << 16;
	Addr2 += *(int16_t*)PageRAM2(paddr + 0x10);

	GlobalBitMask = ((*(int16_t*)PageRAM2(paddr + 0x20)) & 0xFFFF) << 16;
	GlobalBitMask += *(int16_t*)PageRAM2(paddr + 0x28);
	GlobalBitMask = N64WORD(GlobalBitMask);

	intrList = ((*(int16_t*)PageRAM2(paddr + 0x14C + 0x0)) & 0xFFFF) << 16;
	intrList += *(int16_t*)PageRAM2(paddr + 0x150 + 0x0);

	return 0;

	if((PageVRAM2(OSThreadContextAddr) & 0xffff) > 0xFE80) {
		DisplayError(state, "OMG this number is too high!!!!\n");
	}

	osOSThread = N64WORD(OSThreadContextAddr);
	T9 = N64WORD(osOSThread);

	N64WORD(OSThreadContextAddr) = T9;
	N64WORD(Addr2) = osOSThread;

	N64WORD(osOSThread + 0x10) = OS_STATE_RUNNING; //T0 is globalbitmask

	K1 = N64WORD(osOSThread + 0x118); //osOSThread.context.k0

	STATUS_REGISTER = (K1 & 0xFFFF00FF) | (GlobalBitMask & 0xFF00);

	for(i = 1; i <= 0x19; i++) {
		state->GPR[i].DW = N64DWORD(osOSThread + 0x18 + (i * 8));
	}

	for(i = 0x1C; i <= 0x1F; i++) {
		state->GPR[i].DW = N64DWORD(osOSThread + 0x8 + (i * 8));
	}

	state->LO.DW = N64DWORD(osOSThread + 0x108);
	state->HI.DW = N64DWORD(osOSThread + 0x110);

	EPC_REGISTER = (uint32_t) N64WORD(osOSThread + 0x11C);

	state->FPCR[31] = (uint32_t) N64WORD(osOSThread + 0x12C);

	if(N64WORD(osOSThread + 0x18)) {
		for(i = 0; i <= 30; i+=2) {
			(*(uint64_t *)state->FPRDoubleLocation[i]) = N64DWORD(osOSThread + 0x130 + (i * 4));
		}
	} else {
	}

	state->GPR[0x1A].UW[0] = (uint32_t) osOSThread;

	RCP = N64WORD(osOSThread + 0x128);

	K0 = (intrList + ((RCP & (GlobalBitMask >> 16)) << 1));
	K1 = 0;

    r4300i_LH_VAddr(state, K0, &K1);

    // cheap hack?
    //K1 = 0xAAA;

    SW_Register(0x0430000C, K1);

	NextInstruction = JUMP;

	if ((STATUS_REGISTER & STATUS_ERL) != 0) {
		JumpToLocation = ERROREPC_REGISTER;
		STATUS_REGISTER &= ~STATUS_ERL;
	} else {
		JumpToLocation = EPC_REGISTER;
		STATUS_REGISTER &= ~STATUS_EXL;
	}

	LLBit = 0;
	CheckInterrupts();
#endif
	return 0;
}
