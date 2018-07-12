/*
 * Project 64 - A Nintendo 64 emulator.
 *
 * (c) Copyright 2001 zilmar (zilmar@emulation64.com) and
 * Jabo (jabo@emulation64.com).
 *
 * pj64 homepage: www.pj64.net
 *
 * Permission to use, copy, modify and distribute Project64 in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Project64 is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Project64 or software derived from Project64.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so if they want them.
 *
 */

#include <stdint.h>
#include <string.h>

#include "main.h"
#include "cpu.h"
#include "usf.h"
#include "audio.h"
#include "registers.h"
#include "rsp.h"
#include "cpu_hle.h"

#include "usf_internal.h"

#include <stdlib.h>

void ChangeCompareTimer(usf_state_t * state) {
	uint32_t NextCompare = COMPARE_REGISTER - COUNT_REGISTER;
	if ((NextCompare & 0x80000000) != 0) {  NextCompare = 0x7FFFFFFF; }
	if (NextCompare == 0) { NextCompare = 0x1; }
	ChangeTimer(state,CompareTimer,NextCompare);
}

void ChangeTimer(usf_state_t * state, int32_t Type, int32_t Value) {
	if (Value == 0) {
		state->Timers->NextTimer[Type] = 0;
		state->Timers->Active[Type] = 0;
		return;
	}
	state->Timers->NextTimer[Type] = Value - state->Timers->Timer;
	state->Timers->Active[Type] = 1;
	CheckTimer(state);
}

void CheckTimer (usf_state_t * state) {
	int32_t count;

	for (count = 0; count < MaxTimers; count++) {
		if (!state->Timers->Active[count]) { continue; }
		if (!(count == CompareTimer && state->Timers->NextTimer[count] == 0x7FFFFFFF)) {
			state->Timers->NextTimer[count] += state->Timers->Timer;
		}
	}
	state->Timers->CurrentTimerType = -1;
	state->Timers->Timer = 0x7FFFFFFF;
	for (count = 0; count < MaxTimers; count++) {
		if (!state->Timers->Active[count]) { continue; }
		if (state->Timers->NextTimer[count] >= state->Timers->Timer) { continue; }
		state->Timers->Timer = state->Timers->NextTimer[count];
		state->Timers->CurrentTimerType = count;
	}
	if (state->Timers->CurrentTimerType == -1) {
		DisplayError(state, "No active timers ???\nEmulation Stopped");
		StopEmulation(state);
	}
	for (count = 0; count < MaxTimers; count++) {
		if (!state->Timers->Active[count]) { continue; }
		if (!(count == CompareTimer && state->Timers->NextTimer[count] == 0x7FFFFFFF)) {
			state->Timers->NextTimer[count] -= state->Timers->Timer;
		}
	}

	if (state->Timers->NextTimer[CompareTimer] == 0x7FFFFFFF) {
		uint32_t NextCompare = COMPARE_REGISTER - COUNT_REGISTER;
		if ((NextCompare & 0x80000000) == 0 && NextCompare != 0x7FFFFFFF) {
			ChangeCompareTimer(state);
		}
	}
}



void CloseCpu (usf_state_t * state) {
	uint32_t count = 0;

	if(!state->MemChunk) return;
	if (!state->cpu_running) { return; }

	state->cpu_running = 0;

	for (count = 0; count < 3; count ++ ) {
		state->CPU_Action->CloseCPU = 1;
		state->CPU_Action->DoSomething = 1;
	}

	state->CPURunning = 0;
}

int32_t DelaySlotEffectsCompare (usf_state_t * state, uint32_t PC, uint32_t Reg1, uint32_t Reg2) {
	OPCODE Command;

	if (!r4300i_LW_VAddr(state, PC + 4, (uint32_t*)&Command.u.Hex)) {
		return 1;
	}

	switch (Command.u.b.op) {
	case R4300i_SPECIAL:
		switch (Command.u.e.funct) {
		case R4300i_SPECIAL_SLL:
		case R4300i_SPECIAL_SRL:
		case R4300i_SPECIAL_SRA:
		case R4300i_SPECIAL_SLLV:
		case R4300i_SPECIAL_SRLV:
		case R4300i_SPECIAL_SRAV:
		case R4300i_SPECIAL_MFHI:
		case R4300i_SPECIAL_MTHI:
		case R4300i_SPECIAL_MFLO:
		case R4300i_SPECIAL_MTLO:
		case R4300i_SPECIAL_DSLLV:
		case R4300i_SPECIAL_DSRLV:
		case R4300i_SPECIAL_DSRAV:
		case R4300i_SPECIAL_ADD:
		case R4300i_SPECIAL_ADDU:
		case R4300i_SPECIAL_SUB:
		case R4300i_SPECIAL_SUBU:
		case R4300i_SPECIAL_AND:
		case R4300i_SPECIAL_OR:
		case R4300i_SPECIAL_XOR:
		case R4300i_SPECIAL_NOR:
		case R4300i_SPECIAL_SLT:
		case R4300i_SPECIAL_SLTU:
		case R4300i_SPECIAL_DADD:
		case R4300i_SPECIAL_DADDU:
		case R4300i_SPECIAL_DSUB:
		case R4300i_SPECIAL_DSUBU:
		case R4300i_SPECIAL_DSLL:
		case R4300i_SPECIAL_DSRL:
		case R4300i_SPECIAL_DSRA:
		case R4300i_SPECIAL_DSLL32:
		case R4300i_SPECIAL_DSRL32:
		case R4300i_SPECIAL_DSRA32:
			if (Command.u.e.rd == 0) { return 0; }
			if (Command.u.e.rd == Reg1) { return 1; }
			if (Command.u.e.rd == Reg2) { return 1; }
			break;
		case R4300i_SPECIAL_MULT:
		case R4300i_SPECIAL_MULTU:
		case R4300i_SPECIAL_DIV:
		case R4300i_SPECIAL_DIVU:
		case R4300i_SPECIAL_DMULT:
		case R4300i_SPECIAL_DMULTU:
		case R4300i_SPECIAL_DDIV:
		case R4300i_SPECIAL_DDIVU:
			break;
		default:
			return 1;
		}
		break;
	case R4300i_CP0:
		switch (Command.u.b.rs) {
		case R4300i_COP0_MT: break;
		case R4300i_COP0_MF:
			if (Command.u.b.rt == 0) { return 0; }
			if (Command.u.b.rt == Reg1) { return 1; }
			if (Command.u.b.rt == Reg2) { return 1; }
			break;
		default:
			if ( (Command.u.b.rs & 0x10 ) != 0 ) {
				switch( state->Opcode.u.e.funct ) {
				case R4300i_COP0_CO_TLBR: break;
				case R4300i_COP0_CO_TLBWI: break;
				case R4300i_COP0_CO_TLBWR: break;
				case R4300i_COP0_CO_TLBP: break;
				default:
					return 1;
				}
				return 1;
			}
		}
		break;
	case R4300i_CP1:
		switch (Command.u.f.fmt) {
		case R4300i_COP1_MF:
			if (Command.u.b.rt == 0) { return 0; }
			if (Command.u.b.rt == Reg1) { return 1; }
			if (Command.u.b.rt == Reg2) { return 1; }
			break;
		case R4300i_COP1_CF: break;
		case R4300i_COP1_MT: break;
		case R4300i_COP1_CT: break;
		case R4300i_COP1_S: break;
		case R4300i_COP1_D: break;
		case R4300i_COP1_W: break;
		case R4300i_COP1_L: break;
			return 1;
		}
		break;
	case R4300i_ANDI:
	case R4300i_ORI:
	case R4300i_XORI:
	case R4300i_LUI:
	case R4300i_ADDI:
	case R4300i_ADDIU:
	case R4300i_SLTI:
	case R4300i_SLTIU:
	case R4300i_DADDI:
	case R4300i_DADDIU:
	case R4300i_LB:
	case R4300i_LH:
	case R4300i_LW:
	case R4300i_LWL:
	case R4300i_LWR:
	case R4300i_LDL:
	case R4300i_LDR:
	case R4300i_LBU:
	case R4300i_LHU:
	case R4300i_LD:
	case R4300i_LWC1:
	case R4300i_LDC1:
		if (Command.u.b.rt == 0) { return 0; }
		if (Command.u.b.rt == Reg1) { return 1; }
		if (Command.u.b.rt == Reg2) { return 1; }
		break;
	case R4300i_CACHE: break;
	case R4300i_SB: break;
	case R4300i_SH: break;
	case R4300i_SW: break;
	case R4300i_SWR: break;
	case R4300i_SWL: break;
	case R4300i_SWC1: break;
	case R4300i_SDC1: break;
	case R4300i_SD: break;
	default:

		return 1;
	}
	return 0;
}

int32_t DelaySlotEffectsJump (usf_state_t * state, uint32_t JumpPC) {
	OPCODE Command;

	if (!r4300i_LW_VAddr(state, JumpPC, &Command.u.Hex)) { return 1; }

	switch (Command.u.b.op) {
	case R4300i_SPECIAL:
		switch (Command.u.e.funct) {
		case R4300i_SPECIAL_JR:	return DelaySlotEffectsCompare(state,JumpPC,Command.u.b.rs,0);
		case R4300i_SPECIAL_JALR: return DelaySlotEffectsCompare(state,JumpPC,Command.u.b.rs,31);
		}
		break;
	case R4300i_REGIMM:
		switch (Command.u.b.rt) {
		case R4300i_REGIMM_BLTZ:
		case R4300i_REGIMM_BGEZ:
		case R4300i_REGIMM_BLTZL:
		case R4300i_REGIMM_BGEZL:
		case R4300i_REGIMM_BLTZAL:
		case R4300i_REGIMM_BGEZAL:
			return DelaySlotEffectsCompare(state,JumpPC,Command.u.b.rs,0);
		}
		break;
	case R4300i_JAL:
	case R4300i_SPECIAL_JALR: return DelaySlotEffectsCompare(state,JumpPC,31,0); break;
	case R4300i_J: return 0;
	case R4300i_BEQ:
	case R4300i_BNE:
	case R4300i_BLEZ:
	case R4300i_BGTZ:
		return DelaySlotEffectsCompare(state,JumpPC,Command.u.b.rs,Command.u.b.rt);
	case R4300i_CP1:
		switch (Command.u.f.fmt) {
		case R4300i_COP1_BC:
			switch (Command.u.f.ft) {
			case R4300i_COP1_BC_BCF:
			case R4300i_COP1_BC_BCT:
			case R4300i_COP1_BC_BCFL:
			case R4300i_COP1_BC_BCTL:
				{
					int32_t EffectDelaySlot;
					OPCODE NewCommand;

					if (!r4300i_LW_VAddr(state, JumpPC + 4, &NewCommand.u.Hex)) { return 1; }

					EffectDelaySlot = 0;
					if (NewCommand.u.b.op == R4300i_CP1) {
						if (NewCommand.u.f.fmt == R4300i_COP1_S && (NewCommand.u.e.funct & 0x30) == 0x30 ) {
							EffectDelaySlot = 1;
						}
						if (NewCommand.u.f.fmt == R4300i_COP1_D && (NewCommand.u.e.funct & 0x30) == 0x30 ) {
							EffectDelaySlot = 1;
						}
					}
					return EffectDelaySlot;
				}
				break;
			}
			break;
		}
		break;
	case R4300i_BEQL:
	case R4300i_BNEL:
	case R4300i_BLEZL:
	case R4300i_BGTZL:
		return DelaySlotEffectsCompare(state,JumpPC,Command.u.b.rs,Command.u.b.rt);
	}
	return 1;
}

void DoSomething ( usf_state_t * state ) {
	if (state->CPU_Action->CloseCPU) {
		//StopEmulation();
		state->cpu_running = 0;
		//printf("Stopping?\n");
	}
	if (state->CPU_Action->CheckInterrupts) {
		state->CPU_Action->CheckInterrupts = 0;
		CheckInterrupts(state);
	}
	if (state->CPU_Action->DoInterrupt) {
		state->CPU_Action->DoInterrupt = 0;
		DoIntrException(state, 0);
	}


	state->CPU_Action->DoSomething = 0;

	if (state->CPU_Action->DoInterrupt) { state->CPU_Action->DoSomething = 1; }
}

void InPermLoop ( usf_state_t * state ) {
	// *** Changed ***/
	if (state->CPU_Action->DoInterrupt) { return; }

	/* Interrupts enabled */
	if (( STATUS_REGISTER & STATUS_IE  ) == 0 ) { goto InterruptsDisabled; }
	if (( STATUS_REGISTER & STATUS_EXL ) != 0 ) { goto InterruptsDisabled; }
	if (( STATUS_REGISTER & STATUS_ERL ) != 0 ) { goto InterruptsDisabled; }
	if (( STATUS_REGISTER & 0xFF00) == 0) { goto InterruptsDisabled; }

	/* check sound playing */

	/* check RSP running */
	/* check RDP running */
	if (state->Timers->Timer >= 0) {
		COUNT_REGISTER += state->Timers->Timer + 1;
		state->Timers->Timer = -1;
	}
	return;

InterruptsDisabled:
	DisplayError(state, "Stuck in Permanent Loop");
	StopEmulation(state);
}

void ReadFromMem(const void * source, void * target, uint32_t length, uint32_t *offset) {
	memcpy((uint8_t*)target,((uint8_t*)source)+*offset,length);
	*offset+=length;
}


uint32_t Machine_LoadStateFromRAM(usf_state_t * state, void * savestatespace) {
	uint8_t LoadHeader[0x40];
	uint32_t Value, count, SaveRDRAMSize, offset=0;

	ReadFromMem( savestatespace,&Value,sizeof(Value),&offset);
	if (Value != 0x23D8A6C8) { return 0; }
	ReadFromMem( savestatespace,&SaveRDRAMSize,sizeof(SaveRDRAMSize),&offset);
	ReadFromMem( savestatespace,&LoadHeader,0x40,&offset);

	state->Timers->CurrentTimerType = -1;
	state->Timers->Timer = 0;
	for (count = 0; count < MaxTimers; count ++) { state->Timers->Active[count] = 0; }

	//fix rdram size
	if (SaveRDRAMSize != state->RdramSize) {
		// dothis :)
	}		

	state->RdramSize = SaveRDRAMSize;

	ReadFromMem( savestatespace,&Value,sizeof(Value),&offset);
	ChangeTimer(state,ViTimer,Value);
	ReadFromMem( savestatespace,&state->PROGRAM_COUNTER,sizeof(state->PROGRAM_COUNTER),&offset);
	ReadFromMem( savestatespace,state->GPR,sizeof(int64_t)*32,&offset);
	ReadFromMem( savestatespace,state->FPR,sizeof(int64_t)*32,&offset);
	ReadFromMem( savestatespace,state->CP0,sizeof(uint32_t)*32,&offset);
	ReadFromMem( savestatespace,state->FPCR,sizeof(uint32_t)*32,&offset);
	ReadFromMem( savestatespace,&state->HI,sizeof(int64_t),&offset);
	ReadFromMem( savestatespace,&state->LO,sizeof(int64_t),&offset);
	ReadFromMem( savestatespace,state->RegRDRAM,sizeof(uint32_t)*10,&offset);
	ReadFromMem( savestatespace,state->RegSP,sizeof(uint32_t)*10,&offset);
	ReadFromMem( savestatespace,state->RegDPC,sizeof(uint32_t)*10,&offset);
	ReadFromMem( savestatespace,state->RegMI,sizeof(uint32_t)*4,&offset);
	ReadFromMem( savestatespace,state->RegVI,sizeof(uint32_t)*14,&offset);
	ReadFromMem( savestatespace,state->RegAI,sizeof(uint32_t)*6,&offset);
	ReadFromMem( savestatespace,state->RegPI,sizeof(uint32_t)*13,&offset);
	ReadFromMem( savestatespace,state->RegRI,sizeof(uint32_t)*8,&offset);
	ReadFromMem( savestatespace,state->RegSI,sizeof(uint32_t)*4,&offset);
	ReadFromMem( savestatespace,state->tlb,sizeof(TLB)*32,&offset);
	ReadFromMem( savestatespace,(uint8_t*)state->PIF_Ram,0x40,&offset);
	ReadFromMem( savestatespace,state->RDRAM,SaveRDRAMSize,&offset);
	ReadFromMem( savestatespace,state->DMEM,0x1000,&offset);
	ReadFromMem( savestatespace,state->IMEM,0x1000,&offset);

	state->CP0[32] = 0;

	SetupTLB(state);
	ChangeCompareTimer(state);
	AI_STATUS_REG = 0;
	AiDacrateChanged(state, AI_DACRATE_REG);

//	StartAiInterrupt(state);

	SetFpuLocations(state); // important if FR=1

	return 1;
}

void StartEmulationFromSave ( usf_state_t * state, void * savestate ) {
	uint32_t count = 0;
	
	//printf("Starting generic Cpu\n");

	//CloseCpu();
	memset(state->N64MEM, 0, state->RdramSize);
	
	memset(state->DMEM, 0, 0x1000);
	memset(state->IMEM, 0, 0x1000);
	memset(state->TLB_Map, 0, 0x100000 * sizeof(uintptr_t) + 0x10000);

	memset(state->CPU_Action,0,sizeof(*state->CPU_Action));
	state->WrittenToRom = 0;

	InitilizeTLB(state);

	SetupRegisters(state, state->Registers);

	BuildInterpreter(state);

	state->Timers->CurrentTimerType = -1;
	state->Timers->Timer = 0;

	for (count = 0; count < MaxTimers; count ++) { state->Timers->Active[count] = 0; }
	ChangeTimer(state,ViTimer,5000);
	ChangeCompareTimer(state);
	state->ViFieldNumber = 0;
	state->CPURunning = 1;
	*state->WaitMode = 0;

	init_rsp(state);

	Machine_LoadStateFromRAM(state, savestate);

	state->SampleRate = 48681812 / (AI_DACRATE_REG + 1);

	if(state->enableFIFOfull) {
		const float VSyncTiming = 789000.0f;
		double BytesPerSecond = 48681812.0 / (AI_DACRATE_REG + 1) * 4;
		double CountsPerSecond = (double)(((double)VSyncTiming) * (double)60.0);
		double CountsPerByte = (double)CountsPerSecond / (double)BytesPerSecond;
		uint32_t IntScheduled = (uint32_t)((double)AI_LEN_REG * CountsPerByte);

		ChangeTimer(state,AiTimer,IntScheduled);
		AI_STATUS_REG|=0x40000000;
	}
    
    state->OLD_VI_V_SYNC_REG = ~VI_V_SYNC_REG;
    
    CPUHLE_Scan(state);
}


void RefreshScreen (usf_state_t * state){
    if (state->OLD_VI_V_SYNC_REG != VI_V_SYNC_REG)
    {
        state->OLD_VI_V_SYNC_REG = VI_V_SYNC_REG;
        if (VI_V_SYNC_REG == 0)
        {
            state->VI_INTR_TIME = 500000;
        }
        else
        {
            state->VI_INTR_TIME = (VI_V_SYNC_REG + 1) * 1500;
            if ((VI_V_SYNC_REG & 1) != 0)
            {
                state->VI_INTR_TIME -= 38;
            }
        }
    }
    
    ChangeTimer(state,ViTimer,state->Timers->Timer + state->Timers->NextTimer[ViTimer] + state->VI_INTR_TIME);
    
    if ((VI_STATUS_REG & 0x10) != 0)
    {
        if (state->ViFieldNumber == 0)
        {
            state->ViFieldNumber = 1;
        }
        else
        {
            state->ViFieldNumber = 0;
        }
    }
    else
    {
        state->ViFieldNumber = 0;
    }
}

void RunRsp (usf_state_t * state) {
#ifdef DEBUG_INFO
    fprintf(state->debug_log, "RSP Task:");
#endif
	if ( ( SP_STATUS_REG & SP_STATUS_HALT ) == 0) {
		if ( ( SP_STATUS_REG & SP_STATUS_BROKE ) == 0 ) {

			uint32_t Task = *( uint32_t *)(state->DMEM + 0xFC0);

			switch (Task) {
			case 1:	{
					MI_INTR_REG |= 0x20;

					SP_STATUS_REG |= (0x0203 );
					if ((SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0 )
						MI_INTR_REG |= 1;
                
#ifdef DEBUG_INFO
                    fprintf(state->debug_log, " DList - interrupts %d\n", MI_INTR_REG);
#endif

					CheckInterrupts(state);

					DPC_STATUS_REG &= ~0x0002;
					return;

				}
				break;
			case 2: {
#ifdef DEBUG_INFO
                fprintf(state->debug_log, " AList");
#endif
					break;
				}
				break;
			default:

				break;
			}

			real_run_rsp(state, 100);
			SP_STATUS_REG |= (0x0203 );
			if ((SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0 ) {
#ifdef DEBUG_INFO
                fprintf(state->debug_log, " - interrupt");
#endif
				MI_INTR_REG |= 1;
				CheckInterrupts(state);
			}
		}
	}
#ifdef DEBUG_INFO
    fprintf(state->debug_log, "\n");
#endif
}

void TimerDone (usf_state_t * state) {
	switch (state->Timers->CurrentTimerType) {
	case CompareTimer:
		if(state->enablecompare)
			FAKE_CAUSE_REGISTER |= CAUSE_IP7;
		CheckInterrupts(state);
		ChangeCompareTimer(state);
		break;
	case ViTimer:
		RefreshScreen(state);
		MI_INTR_REG |= MI_INTR_VI;
		CheckInterrupts(state);
		*state->WaitMode=0;
		break;
	case AiTimer:
		ChangeTimer(state,AiTimer,0);
		AI_STATUS_REG=0;
        state->AudioIntrReg|=4;
		//CheckInterrupts(state);
		break;
	}
	CheckTimer(state);
}
