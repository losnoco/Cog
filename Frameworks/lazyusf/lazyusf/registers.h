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

#ifndef REGISTERS_H
#define REGISTERS_H

#include "types.h"

#define INDEX_REGISTER			state->CP0[0]
#define RANDOM_REGISTER			state->CP0[1]
#define ENTRYLO0_REGISTER		state->CP0[2]
#define ENTRYLO1_REGISTER		state->CP0[3]
#define CONTEXT_REGISTER		state->CP0[4]
#define PAGE_MASK_REGISTER		state->CP0[5]
#define WIRED_REGISTER			state->CP0[6]
#define BAD_VADDR_REGISTER		state->CP0[8]
#define COUNT_REGISTER			state->CP0[9]
#define ENTRYHI_REGISTER		state->CP0[10]
#define COMPARE_REGISTER		state->CP0[11]
#define STATUS_REGISTER			state->CP0[12]
#define CAUSE_REGISTER			state->CP0[13]
#define EPC_REGISTER			state->CP0[14]
#define CONFIG_REGISTER			state->CP0[16]
#define TAGLO_REGISTER			state->CP0[28]
#define TAGHI_REGISTER			state->CP0[29]
#define ERROREPC_REGISTER		state->CP0[30]
#define FAKE_CAUSE_REGISTER		state->CP0[32]

#define COMPARE_REGISTER_NO		11
#define STATUS_REGISTER_NO		12
#define CAUSE_REGISTER_NO		13

#define REVISION_REGISTER		state->FPCR[0]
#define FSTATUS_REGISTER		state->FPCR[31]

#define GPR_S0					state->GPR[16]
#define GPR_S1					state->GPR[17]
#define GPR_S2					state->GPR[18]
#define GPR_S3					state->GPR[19]
#define GPR_S4					state->GPR[20]
#define GPR_S5					state->GPR[21]
#define GPR_S6					state->GPR[22]
#define GPR_S7					state->GPR[23]
#define GPR_SP					state->GPR[29]
#define GPR_RA					state->GPR[31]

#define RDRAM_CONFIG_REG		state->RegRDRAM[0]
#define RDRAM_DEVICE_TYPE_REG	state->RegRDRAM[0]
#define RDRAM_DEVICE_ID_REG		state->RegRDRAM[1]
#define RDRAM_DELAY_REG			state->RegRDRAM[2]
#define RDRAM_MODE_REG			state->RegRDRAM[3]
#define RDRAM_REF_INTERVAL_REG	state->RegRDRAM[4]
#define RDRAM_REF_ROW_REG		state->RegRDRAM[5]
#define RDRAM_RAS_INTERVAL_REG	state->RegRDRAM[6]
#define RDRAM_MIN_INTERVAL_REG	state->RegRDRAM[7]
#define RDRAM_ADDR_SELECT_REG	state->RegRDRAM[8]
#define RDRAM_DEVICE_MANUF_REG	state->RegRDRAM[9]

#define SP_MEM_ADDR_REG			state->RegSP[0]
#define SP_DRAM_ADDR_REG		state->RegSP[1]
#define SP_RD_LEN_REG			state->RegSP[2]
#define SP_WR_LEN_REG			state->RegSP[3]
#define SP_STATUS_REG			state->RegSP[4]
#define SP_DMA_FULL_REG			state->RegSP[5]
#define SP_DMA_BUSY_REG			state->RegSP[6]
#define SP_SEMAPHORE_REG		state->RegSP[7]
#define SP_PC_REG				state->RegSP[8]
#define SP_IBIST_REG			state->RegSP[9]

#define DPC_START_REG			state->RegDPC[0]
#define DPC_END_REG				state->RegDPC[1]
#define DPC_CURRENT_REG			state->RegDPC[2]
#define DPC_STATUS_REG			state->RegDPC[3]
#define DPC_CLOCK_REG			state->RegDPC[4]
#define DPC_BUFBUSY_REG			state->RegDPC[5]
#define DPC_PIPEBUSY_REG		state->RegDPC[6]
#define DPC_TMEM_REG			state->RegDPC[7]

#define MI_INIT_MODE_REG		state->RegMI[0]
#define MI_MODE_REG				state->RegMI[0]
#define MI_VERSION_REG			state->RegMI[1]
#define MI_NOOP_REG				state->RegMI[1]
#define MI_INTR_REG				state->RegMI[2]
#define MI_INTR_MASK_REG		state->RegMI[3]

#define VI_STATUS_REG			state->RegVI[0]
#define VI_CONTROL_REG			state->RegVI[0]
#define VI_ORIGIN_REG 			state->RegVI[1]
#define VI_DRAM_ADDR_REG		state->RegVI[1]
#define VI_WIDTH_REG 			state->RegVI[2]
#define VI_H_WIDTH_REG 			state->RegVI[2]
#define VI_INTR_REG  			state->RegVI[3]
#define VI_V_INTR_REG 			state->RegVI[3]
#define VI_CURRENT_REG 			state->RegVI[4]
#define VI_V_CURRENT_LINE_REG	state->RegVI[4]
#define VI_BURST_REG  			state->RegVI[5]
#define VI_TIMING_REG 			state->RegVI[5]
#define VI_V_SYNC_REG 			state->RegVI[6]
#define VI_H_SYNC_REG 			state->RegVI[7]
#define VI_LEAP_REG  			state->RegVI[8]
#define VI_H_SYNC_LEAP_REG		state->RegVI[8]
#define VI_H_START_REG 			state->RegVI[9]
#define VI_H_VIDEO_REG			state->RegVI[9]
#define VI_V_START_REG 			state->RegVI[10]
#define VI_V_VIDEO_REG			state->RegVI[10]
#define VI_V_BURST_REG			state->RegVI[11]
#define VI_X_SCALE_REG			state->RegVI[12]
#define VI_Y_SCALE_REG			state->RegVI[13]

#define AI_DRAM_ADDR_REG		state->RegAI[0]
#define AI_LEN_REG				state->RegAI[1]
#define AI_CONTROL_REG			state->RegAI[2]
#define AI_STATUS_REG			state->RegAI[3]
#define AI_DACRATE_REG			state->RegAI[4]
#define AI_BITRATE_REG			state->RegAI[5]

#define PI_DRAM_ADDR_REG		state->RegPI[0]
#define PI_CART_ADDR_REG		state->RegPI[1]
#define PI_RD_LEN_REG			state->RegPI[2]
#define PI_WR_LEN_REG			state->RegPI[3]
#define PI_STATUS_REG			state->RegPI[4]
#define PI_BSD_DOM1_LAT_REG 	state->RegPI[5]
#define PI_DOMAIN1_REG		 	state->RegPI[5]
#define PI_BSD_DOM1_PWD_REG	 	state->RegPI[6]
#define PI_BSD_DOM1_PGS_REG	 	state->RegPI[7]
#define PI_BSD_DOM1_RLS_REG	 	state->RegPI[8]
#define PI_BSD_DOM2_LAT_REG	 	state->RegPI[9]
#define PI_DOMAIN2_REG		 	state->RegPI[9]
#define PI_BSD_DOM2_PWD_REG	 	state->RegPI[10]
#define PI_BSD_DOM2_PGS_REG	 	state->RegPI[11]
#define PI_BSD_DOM2_RLS_REG	 	state->RegPI[12]

#define RI_MODE_REG				state->RegRI[0]
#define RI_CONFIG_REG			state->RegRI[1]
#define RI_CURRENT_LOAD_REG		state->RegRI[2]
#define RI_SELECT_REG			state->RegRI[3]
#define RI_COUNT_REG			state->RegRI[4]
#define RI_REFRESH_REG			state->RegRI[4]
#define RI_LATENCY_REG			state->RegRI[5]
#define RI_RERROR_REG			state->RegRI[6]
#define RI_WERROR_REG			state->RegRI[7]

#define SI_DRAM_ADDR_REG		state->RegSI[0]
#define SI_PIF_ADDR_RD64B_REG	state->RegSI[1]
#define SI_PIF_ADDR_WR64B_REG	state->RegSI[2]
#define SI_STATUS_REG			state->RegSI[3]

#define STATUS_IE				0x00000001
#define STATUS_EXL				0x00000002
#define STATUS_ERL				0x00000004
#define STATUS_IP0				0x00000100
#define STATUS_IP1				0x00000200
#define STATUS_IP2				0x00000400
#define STATUS_IP3				0x00000800
#define STATUS_IP4				0x00001000
#define STATUS_IP5				0x00002000
#define STATUS_IP6				0x00004000
#define STATUS_IP7				0x00008000
#define STATUS_BEV				0x00400000
#define STATUS_FR				0x04000000
#define STATUS_CU0				0x10000000
#define STATUS_CU1				0x20000000

#define CAUSE_EXC_CODE			0xFF
#define CAUSE_IP0				0x100
#define CAUSE_IP1				0x200
#define CAUSE_IP2				0x400
#define CAUSE_IP3				0x800
#define CAUSE_IP4				0x1000
#define CAUSE_IP5				0x2000
#define CAUSE_IP6				0x4000
#define CAUSE_IP7				0x8000
#define CAUSE_BD				0x80000000

#define SP_CLR_HALT				0x00001	    /* Bit  0: clear halt */
#define SP_SET_HALT				0x00002	    /* Bit  1: set halt */
#define SP_CLR_BROKE			0x00004	    /* Bit  2: clear broke */
#define SP_CLR_INTR				0x00008	    /* Bit  3: clear intr */
#define SP_SET_INTR				0x00010	    /* Bit  4: set intr */
#define SP_CLR_SSTEP			0x00020	    /* Bit  5: clear sstep */
#define SP_SET_SSTEP			0x00040	    /* Bit  6: set sstep */
#define SP_CLR_INTR_BREAK		0x00080	    /* Bit  7: clear intr on break */
#define SP_SET_INTR_BREAK		0x00100	    /* Bit  8: set intr on break */
#define SP_CLR_SIG0				0x00200	    /* Bit  9: clear signal 0 */
#define SP_SET_SIG0				0x00400	    /* Bit 10: set signal 0 */
#define SP_CLR_SIG1				0x00800	    /* Bit 11: clear signal 1 */
#define SP_SET_SIG1				0x01000	    /* Bit 12: set signal 1 */
#define SP_CLR_SIG2				0x02000	    /* Bit 13: clear signal 2 */
#define SP_SET_SIG2				0x04000	    /* Bit 14: set signal 2 */
#define SP_CLR_SIG3				0x08000	    /* Bit 15: clear signal 3 */
#define SP_SET_SIG3				0x10000	    /* Bit 16: set signal 3 */
#define SP_CLR_SIG4				0x20000	    /* Bit 17: clear signal 4 */
#define SP_SET_SIG4				0x40000	    /* Bit 18: set signal 4 */
#define SP_CLR_SIG5				0x80000	    /* Bit 19: clear signal 5 */
#define SP_SET_SIG5				0x100000	/* Bit 20: set signal 5 */
#define SP_CLR_SIG6				0x200000	/* Bit 21: clear signal 6 */
#define SP_SET_SIG6				0x400000	/* Bit 22: set signal 6 */
#define SP_CLR_SIG7				0x800000	/* Bit 23: clear signal 7 */
#define SP_SET_SIG7				0x1000000   /* Bit 24: set signal 7 */

#define SP_STATUS_HALT			0x001		/* Bit  0: halt */
#define SP_STATUS_BROKE			0x002		/* Bit  1: broke */
#define SP_STATUS_DMA_BUSY		0x004		/* Bit  2: dma busy */
#define SP_STATUS_DMA_FULL		0x008		/* Bit  3: dma full */
#define SP_STATUS_IO_FULL		0x010		/* Bit  4: io full */
#define SP_STATUS_SSTEP			0x020		/* Bit  5: single step */
#define SP_STATUS_INTR_BREAK	0x040		/* Bit  6: interrupt on break */
#define SP_STATUS_SIG0			0x080		/* Bit  7: signal 0 set */
#define SP_STATUS_SIG1			0x100		/* Bit  8: signal 1 set */
#define SP_STATUS_SIG2			0x200		/* Bit  9: signal 2 set */
#define SP_STATUS_SIG3			0x400		/* Bit 10: signal 3 set */
#define SP_STATUS_SIG4			0x800		/* Bit 11: signal 4 set */
#define SP_STATUS_SIG5	       0x1000		/* Bit 12: signal 5 set */
#define SP_STATUS_SIG6	       0x2000		/* Bit 13: signal 6 set */
#define SP_STATUS_SIG7	       0x4000		/* Bit 14: signal 7 set */

#define DPC_CLR_XBUS_DMEM_DMA	0x0001		/* Bit 0: clear xbus_dmem_dma */
#define DPC_SET_XBUS_DMEM_DMA	0x0002		/* Bit 1: set xbus_dmem_dma */
#define DPC_CLR_FREEZE			0x0004		/* Bit 2: clear freeze */
#define DPC_SET_FREEZE			0x0008		/* Bit 3: set freeze */
#define DPC_CLR_FLUSH			0x0010		/* Bit 4: clear flush */
#define DPC_SET_FLUSH			0x0020		/* Bit 5: set flush */
#define DPC_CLR_TMEM_CTR		0x0040		/* Bit 6: clear tmem ctr */
#define DPC_CLR_PIPE_CTR		0x0080		/* Bit 7: clear pipe ctr */
#define DPC_CLR_CMD_CTR			0x0100		/* Bit 8: clear cmd ctr */
#define DPC_CLR_CLOCK_CTR		0x0200		/* Bit 9: clear clock ctr */

#define DPC_STATUS_XBUS_DMEM_DMA	0x001	/* Bit  0: xbus_dmem_dma */
#define DPC_STATUS_FREEZE			0x002	/* Bit  1: freeze */
#define DPC_STATUS_FLUSH			0x004	/* Bit  2: flush */
#define DPC_STATUS_START_GCLK		0x008	/* Bit  3: start gclk */
#define DPC_STATUS_TMEM_BUSY		0x010	/* Bit  4: tmem busy */
#define DPC_STATUS_PIPE_BUSY		0x020	/* Bit  5: pipe busy */
#define DPC_STATUS_CMD_BUSY			0x040	/* Bit  6: cmd busy */
#define DPC_STATUS_CBUF_READY		0x080	/* Bit  7: cbuf ready */
#define DPC_STATUS_DMA_BUSY			0x100	/* Bit  8: dma busy */
#define DPC_STATUS_END_VALID		0x200	/* Bit  9: end valid */
#define DPC_STATUS_START_VALID		0x400	/* Bit 10: start valid */

#define MI_CLR_INIT				0x0080		/* Bit  7: clear init mode */
#define MI_SET_INIT				0x0100		/* Bit  8: set init mode */
#define MI_CLR_EBUS				0x0200		/* Bit  9: clear ebus test */
#define MI_SET_EBUS				0x0400		/* Bit 10: set ebus test mode */
#define MI_CLR_DP_INTR			0x0800		/* Bit 11: clear dp interrupt */
#define MI_CLR_RDRAM			0x1000		/* Bit 12: clear RDRAM reg */
#define MI_SET_RDRAM			0x2000		/* Bit 13: set RDRAM reg mode */

#define MI_MODE_INIT			0x0080		/* Bit  7: init mode */
#define MI_MODE_EBUS			0x0100		/* Bit  8: ebus test mode */
#define MI_MODE_RDRAM			0x0200		/* Bit  9: RDRAM reg mode */

#define MI_INTR_MASK_CLR_SP		0x0001		/* Bit  0: clear SP mask */
#define MI_INTR_MASK_SET_SP		0x0002		/* Bit  1: set SP mask */
#define MI_INTR_MASK_CLR_SI		0x0004		/* Bit  2: clear SI mask */
#define MI_INTR_MASK_SET_SI		0x0008		/* Bit  3: set SI mask */
#define MI_INTR_MASK_CLR_AI		0x0010		/* Bit  4: clear AI mask */
#define MI_INTR_MASK_SET_AI		0x0020		/* Bit  5: set AI mask */
#define MI_INTR_MASK_CLR_VI		0x0040		/* Bit  6: clear VI mask */
#define MI_INTR_MASK_SET_VI		0x0080		/* Bit  7: set VI mask */
#define MI_INTR_MASK_CLR_PI		0x0100		/* Bit  8: clear PI mask */
#define MI_INTR_MASK_SET_PI		0x0200		/* Bit  9: set PI mask */
#define MI_INTR_MASK_CLR_DP		0x0400		/* Bit 10: clear DP mask */
#define MI_INTR_MASK_SET_DP		0x0800		/* Bit 11: set DP mask */

#define MI_INTR_MASK_SP			0x01		/* Bit 0: SP intr mask */
#define MI_INTR_MASK_SI			0x02		/* Bit 1: SI intr mask */
#define MI_INTR_MASK_AI			0x04		/* Bit 2: AI intr mask */
#define MI_INTR_MASK_VI			0x08		/* Bit 3: VI intr mask */
#define MI_INTR_MASK_PI			0x10		/* Bit 4: PI intr mask */
#define MI_INTR_MASK_DP			0x20		/* Bit 5: DP intr mask */

#define MI_INTR_SP				0x01		/* Bit 0: SP intr */
#define MI_INTR_SI				0x02		/* Bit 1: SI intr */
#define MI_INTR_AI				0x04		/* Bit 2: AI intr */
#define MI_INTR_VI				0x08		/* Bit 3: VI intr */
#define MI_INTR_PI				0x10		/* Bit 4: PI intr */
#define MI_INTR_DP				0x20		/* Bit 5: DP intr */

#define	PI_STATUS_DMA_BUSY		0x01
#define	PI_STATUS_IO_BUSY		0x02
#define	PI_STATUS_ERROR			0x04

#define	PI_SET_RESET			0x01
#define	PI_CLR_INTR				0x02

#define	SI_STATUS_DMA_BUSY		0x0001
#define	SI_STATUS_RD_BUSY		0x0002
#define	SI_STATUS_DMA_ERROR		0x0008
#define	SI_STATUS_INTERRUPT		0x1000

#define FPCSR_FS				0x01000000	/* flush denorm to zero */
#define	FPCSR_C					0x00800000	/* condition bit */
#define	FPCSR_CE				0x00020000	/* cause: unimplemented operation */
#define	FPCSR_CV				0x00010000	/* cause: invalid operation */
#define	FPCSR_CZ				0x00008000	/* cause: division by zero */
#define	FPCSR_CO				0x00004000	/* cause: overflow */
#define	FPCSR_CU				0x00002000	/* cause: underflow */
#define	FPCSR_CI				0x00001000	/* cause: inexact operation */
#define	FPCSR_EV				0x00000800	/* enable: invalid operation */
#define	FPCSR_EZ				0x00000400	/* enable: division by zero */
#define	FPCSR_EO				0x00000200	/* enable: overflow */
#define	FPCSR_EU				0x00000100	/* enable: underflow */
#define	FPCSR_EI				0x00000080	/* enable: inexact operation */
#define	FPCSR_FV				0x00000040	/* flag: invalid operation */
#define	FPCSR_FZ				0x00000020	/* flag: division by zero */
#define	FPCSR_FO				0x00000010	/* flag: overflow */
#define	FPCSR_FU				0x00000008	/* flag: underflow */
#define	FPCSR_FI				0x00000004	/* flag: inexact operation */
#define	FPCSR_RM_MASK			0x00000003	/* rounding mode mask */
#define	FPCSR_RM_RN				0x00000000	/* round to nearest */
#define	FPCSR_RM_RZ				0x00000001	/* round to zero */
#define	FPCSR_RM_RP				0x00000002	/* round to positive infinity */
#define	FPCSR_RM_RM				0x00000003	/* round to negative infinity */

#define FPR_Type(Reg)	(Reg) == R4300i_COP1_S ? "S" : (Reg) == R4300i_COP1_D ? "D" :\
						(Reg) == R4300i_COP1_W ? "W" : "L"

typedef struct {
	uint32_t      PROGRAM_COUNTER;
    MIPS_DWORD    GPR[32];
	MIPS_DWORD    FPR[32];
	uint32_t      CP0[33];
	uint32_t      FPCR[32];
	MIPS_DWORD	  HI;
	MIPS_DWORD 	  LO;
	uint32_t      RDRAM[10];
	uint32_t      SP[10];
	uint32_t      DPC[10];
	uint32_t      MI[4];
	uint32_t      VI[14];
	uint32_t      AI[6];
	uint32_t      PI[13];
	uint32_t      RI[8];
	uint32_t      SI[4];
	int8_t       PIF_Ram[0x40];
} N64_REGISTERS;

enum FPU_Format {
	FPU_Unkown,FPU_Dword, FPU_Qword, FPU_Float, FPU_Double
};

enum FPU_RoundingModel {
	RoundUnknown, RoundDefault, RoundTruncate, RoundNearest, RoundDown, RoundUp
};

void ChangeMiIntrMask         ( usf_state_t * );
void ChangeMiModeReg          ( usf_state_t * );
void ChangeSpStatus           ( usf_state_t * );
void InitalizeR4300iRegisters ( usf_state_t * );
void UpdateCurrentHalfLine    ( usf_state_t * );
void SetFpuLocations          ( usf_state_t * );

void SetupRegisters(usf_state_t *, N64_REGISTERS * n64_Registers);

#endif
