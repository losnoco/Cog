/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - ops.h                                                   *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef M64P_R4300_OPS_H
#define M64P_R4300_OPS_H

#include "osal/preproc.h"

typedef struct _cpu_instruction_table
{
	/* All jump/branch instructions (except JR and JALR) have three versions:
	 * - JUMPNAME() which for jumps inside the current block.
	 * - JUMPNAME_OUT() which jumps outside the current block.
	 * - JUMPNAME_IDLE() which does busy wait optimization.
	 *
	 * Busy wait optimization is used when a jump jumps to itself,
	 * and the instruction on the delay slot is a NOP.
	 * The program is waiting for the next interrupt, so we can just
	 * increase Count until the point where the next interrupt happens. */

	// Load and store instructions
	void (osal_fastcall *LB)(usf_state_t *);
	void (osal_fastcall *LBU)(usf_state_t *);
	void (osal_fastcall *LH)(usf_state_t *);
	void (osal_fastcall *LHU)(usf_state_t *);
	void (osal_fastcall *LW)(usf_state_t *);
	void (osal_fastcall *LWL)(usf_state_t *);
	void (osal_fastcall *LWR)(usf_state_t *);
	void (osal_fastcall *SB)(usf_state_t *);
	void (osal_fastcall *SH)(usf_state_t *);
	void (osal_fastcall *SW)(usf_state_t *);
	void (osal_fastcall *SWL)(usf_state_t *);
	void (osal_fastcall *SWR)(usf_state_t *);

	void (osal_fastcall *LD)(usf_state_t *);
	void (osal_fastcall *LDL)(usf_state_t *);
	void (osal_fastcall *LDR)(usf_state_t *);
	void (osal_fastcall *LL)(usf_state_t *);
	void (osal_fastcall *LWU)(usf_state_t *);
	void (osal_fastcall *SC)(usf_state_t *);
	void (osal_fastcall *SD)(usf_state_t *);
	void (osal_fastcall *SDL)(usf_state_t *);
	void (osal_fastcall *SDR)(usf_state_t *);
	void (osal_fastcall *SYNC)(usf_state_t *);

	// Arithmetic instructions (ALU immediate)
	void (osal_fastcall *ADDI)(usf_state_t *);
	void (osal_fastcall *ADDIU)(usf_state_t *);
	void (osal_fastcall *SLTI)(usf_state_t *);
	void (osal_fastcall *SLTIU)(usf_state_t *);
	void (osal_fastcall *ANDI)(usf_state_t *);
	void (osal_fastcall *ORI)(usf_state_t *);
	void (osal_fastcall *XORI)(usf_state_t *);
	void (osal_fastcall *LUI)(usf_state_t *);

	void (osal_fastcall *DADDI)(usf_state_t *);
	void (osal_fastcall *DADDIU)(usf_state_t *);

	// Arithmetic instructions (3-operand)
	void (osal_fastcall *ADD)(usf_state_t *);
	void (osal_fastcall *ADDU)(usf_state_t *);
	void (osal_fastcall *SUB)(usf_state_t *);
	void (osal_fastcall *SUBU)(usf_state_t *);
	void (osal_fastcall *SLT)(usf_state_t *);
	void (osal_fastcall *SLTU)(usf_state_t *);
	void (osal_fastcall *AND)(usf_state_t *);
	void (osal_fastcall *OR)(usf_state_t *);
	void (osal_fastcall *XOR)(usf_state_t *);
	void (osal_fastcall *NOR)(usf_state_t *);

	void (osal_fastcall *DADD)(usf_state_t *);
	void (osal_fastcall *DADDU)(usf_state_t *);
	void (osal_fastcall *DSUB)(usf_state_t *);
	void (osal_fastcall *DSUBU)(usf_state_t *);

	// Multiply and divide instructions
	void (osal_fastcall *MULT)(usf_state_t *);
	void (osal_fastcall *MULTU)(usf_state_t *);
	void (osal_fastcall *DIV)(usf_state_t *);
	void (osal_fastcall *DIVU)(usf_state_t *);
	void (osal_fastcall *MFHI)(usf_state_t *);
	void (osal_fastcall *MTHI)(usf_state_t *);
	void (osal_fastcall *MFLO)(usf_state_t *);
	void (osal_fastcall *MTLO)(usf_state_t *);

	void (osal_fastcall *DMULT)(usf_state_t *);
	void (osal_fastcall *DMULTU)(usf_state_t *);
	void (osal_fastcall *DDIV)(usf_state_t *);
	void (osal_fastcall *DDIVU)(usf_state_t *);

	// Jump and branch instructions
	void (osal_fastcall *J)(usf_state_t *);
	void (osal_fastcall *J_OUT)(usf_state_t *);
	void (osal_fastcall *J_IDLE)(usf_state_t *);
	void (osal_fastcall *JAL)(usf_state_t *);
	void (osal_fastcall *JAL_OUT)(usf_state_t *);
	void (osal_fastcall *JAL_IDLE)(usf_state_t *);
	void (osal_fastcall *JR)(usf_state_t *);
	void (osal_fastcall *JALR)(usf_state_t *);
	void (osal_fastcall *BEQ)(usf_state_t *);
	void (osal_fastcall *BEQ_OUT)(usf_state_t *);
	void (osal_fastcall *BEQ_IDLE)(usf_state_t *);
	void (osal_fastcall *BNE)(usf_state_t *);
	void (osal_fastcall *BNE_OUT)(usf_state_t *);
	void (osal_fastcall *BNE_IDLE)(usf_state_t *);
	void (osal_fastcall *BLEZ)(usf_state_t *);
	void (osal_fastcall *BLEZ_OUT)(usf_state_t *);
	void (osal_fastcall *BLEZ_IDLE)(usf_state_t *);
	void (osal_fastcall *BGTZ)(usf_state_t *);
	void (osal_fastcall *BGTZ_OUT)(usf_state_t *);
	void (osal_fastcall *BGTZ_IDLE)(usf_state_t *);
	void (osal_fastcall *BLTZ)(usf_state_t *);
	void (osal_fastcall *BLTZ_OUT)(usf_state_t *);
	void (osal_fastcall *BLTZ_IDLE)(usf_state_t *);
	void (osal_fastcall *BGEZ)(usf_state_t *);
	void (osal_fastcall *BGEZ_OUT)(usf_state_t *);
	void (osal_fastcall *BGEZ_IDLE)(usf_state_t *);
	void (osal_fastcall *BLTZAL)(usf_state_t *);
	void (osal_fastcall *BLTZAL_OUT)(usf_state_t *);
	void (osal_fastcall *BLTZAL_IDLE)(usf_state_t *);
	void (osal_fastcall *BGEZAL)(usf_state_t *);
	void (osal_fastcall *BGEZAL_OUT)(usf_state_t *);
	void (osal_fastcall *BGEZAL_IDLE)(usf_state_t *);

	void (osal_fastcall *BEQL)(usf_state_t *);
	void (osal_fastcall *BEQL_OUT)(usf_state_t *);
	void (osal_fastcall *BEQL_IDLE)(usf_state_t *);
	void (osal_fastcall *BNEL)(usf_state_t *);
	void (osal_fastcall *BNEL_OUT)(usf_state_t *);
	void (osal_fastcall *BNEL_IDLE)(usf_state_t *);
	void (osal_fastcall *BLEZL)(usf_state_t *);
	void (osal_fastcall *BLEZL_OUT)(usf_state_t *);
	void (osal_fastcall *BLEZL_IDLE)(usf_state_t *);
	void (osal_fastcall *BGTZL)(usf_state_t *);
	void (osal_fastcall *BGTZL_OUT)(usf_state_t *);
	void (osal_fastcall *BGTZL_IDLE)(usf_state_t *);
	void (osal_fastcall *BLTZL)(usf_state_t *);
	void (osal_fastcall *BLTZL_OUT)(usf_state_t *);
	void (osal_fastcall *BLTZL_IDLE)(usf_state_t *);
	void (osal_fastcall *BGEZL)(usf_state_t *);
	void (osal_fastcall *BGEZL_OUT)(usf_state_t *);
	void (osal_fastcall *BGEZL_IDLE)(usf_state_t *);
	void (osal_fastcall *BLTZALL)(usf_state_t *);
	void (osal_fastcall *BLTZALL_OUT)(usf_state_t *);
	void (osal_fastcall *BLTZALL_IDLE)(usf_state_t *);
	void (osal_fastcall *BGEZALL)(usf_state_t *);
	void (osal_fastcall *BGEZALL_OUT)(usf_state_t *);
	void (osal_fastcall *BGEZALL_IDLE)(usf_state_t *);
	void (osal_fastcall *BC1TL)(usf_state_t *);
	void (osal_fastcall *BC1TL_OUT)(usf_state_t *);
	void (osal_fastcall *BC1TL_IDLE)(usf_state_t *);
	void (osal_fastcall *BC1FL)(usf_state_t *);
	void (osal_fastcall *BC1FL_OUT)(usf_state_t *);
	void (osal_fastcall *BC1FL_IDLE)(usf_state_t *);

	// Shift instructions
	void (osal_fastcall *SLL)(usf_state_t *);
	void (osal_fastcall *SRL)(usf_state_t *);
	void (osal_fastcall *SRA)(usf_state_t *);
	void (osal_fastcall *SLLV)(usf_state_t *);
	void (osal_fastcall *SRLV)(usf_state_t *);
	void (osal_fastcall *SRAV)(usf_state_t *);

	void (osal_fastcall *DSLL)(usf_state_t *);
	void (osal_fastcall *DSRL)(usf_state_t *);
	void (osal_fastcall *DSRA)(usf_state_t *);
	void (osal_fastcall *DSLLV)(usf_state_t *);
	void (osal_fastcall *DSRLV)(usf_state_t *);
	void (osal_fastcall *DSRAV)(usf_state_t *);
	void (osal_fastcall *DSLL32)(usf_state_t *);
	void (osal_fastcall *DSRL32)(usf_state_t *);
	void (osal_fastcall *DSRA32)(usf_state_t *);

	// COP0 instructions
	void (osal_fastcall *MTC0)(usf_state_t *);
	void (osal_fastcall *MFC0)(usf_state_t *);

	void (osal_fastcall *TLBR)(usf_state_t *);
	void (osal_fastcall *TLBWI)(usf_state_t *);
	void (osal_fastcall *TLBWR)(usf_state_t *);
	void (osal_fastcall *TLBP)(usf_state_t *);
	void (osal_fastcall *CACHE)(usf_state_t *);
	void (osal_fastcall *ERET)(usf_state_t *);

	// COP1 instructions
	void (osal_fastcall *LWC1)(usf_state_t *);
	void (osal_fastcall *SWC1)(usf_state_t *);
	void (osal_fastcall *MTC1)(usf_state_t *);
	void (osal_fastcall *MFC1)(usf_state_t *);
	void (osal_fastcall *CTC1)(usf_state_t *);
	void (osal_fastcall *CFC1)(usf_state_t *);
	void (osal_fastcall *BC1T)(usf_state_t *);
	void (osal_fastcall *BC1T_OUT)(usf_state_t *);
	void (osal_fastcall *BC1T_IDLE)(usf_state_t *);
	void (osal_fastcall *BC1F)(usf_state_t *);
	void (osal_fastcall *BC1F_OUT)(usf_state_t *);
	void (osal_fastcall *BC1F_IDLE)(usf_state_t *);

	void (osal_fastcall *DMFC1)(usf_state_t *);
	void (osal_fastcall *DMTC1)(usf_state_t *);
	void (osal_fastcall *LDC1)(usf_state_t *);
	void (osal_fastcall *SDC1)(usf_state_t *);

	void (osal_fastcall *CVT_S_D)(usf_state_t *);
	void (osal_fastcall *CVT_S_W)(usf_state_t *);
	void (osal_fastcall *CVT_S_L)(usf_state_t *);
	void (osal_fastcall *CVT_D_S)(usf_state_t *);
	void (osal_fastcall *CVT_D_W)(usf_state_t *);
	void (osal_fastcall *CVT_D_L)(usf_state_t *);
	void (osal_fastcall *CVT_W_S)(usf_state_t *);
	void (osal_fastcall *CVT_W_D)(usf_state_t *);
	void (osal_fastcall *CVT_L_S)(usf_state_t *);
	void (osal_fastcall *CVT_L_D)(usf_state_t *);

	void (osal_fastcall *ROUND_W_S)(usf_state_t *);
	void (osal_fastcall *ROUND_W_D)(usf_state_t *);
	void (osal_fastcall *ROUND_L_S)(usf_state_t *);
	void (osal_fastcall *ROUND_L_D)(usf_state_t *);

	void (osal_fastcall *TRUNC_W_S)(usf_state_t *);
	void (osal_fastcall *TRUNC_W_D)(usf_state_t *);
	void (osal_fastcall *TRUNC_L_S)(usf_state_t *);
	void (osal_fastcall *TRUNC_L_D)(usf_state_t *);

	void (osal_fastcall *CEIL_W_S)(usf_state_t *);
	void (osal_fastcall *CEIL_W_D)(usf_state_t *);
	void (osal_fastcall *CEIL_L_S)(usf_state_t *);
	void (osal_fastcall *CEIL_L_D)(usf_state_t *);

	void (osal_fastcall *FLOOR_W_S)(usf_state_t *);
	void (osal_fastcall *FLOOR_W_D)(usf_state_t *);
	void (osal_fastcall *FLOOR_L_S)(usf_state_t *);
	void (osal_fastcall *FLOOR_L_D)(usf_state_t *);

	void (osal_fastcall *ADD_S)(usf_state_t *);
	void (osal_fastcall *ADD_D)(usf_state_t *);

	void (osal_fastcall *SUB_S)(usf_state_t *);
	void (osal_fastcall *SUB_D)(usf_state_t *);

	void (osal_fastcall *MUL_S)(usf_state_t *);
	void (osal_fastcall *MUL_D)(usf_state_t *);

	void (osal_fastcall *DIV_S)(usf_state_t *);
	void (osal_fastcall *DIV_D)(usf_state_t *);
	
	void (osal_fastcall *ABS_S)(usf_state_t *);
	void (osal_fastcall *ABS_D)(usf_state_t *);

	void (osal_fastcall *MOV_S)(usf_state_t *);
	void (osal_fastcall *MOV_D)(usf_state_t *);

	void (osal_fastcall *NEG_S)(usf_state_t *);
	void (osal_fastcall *NEG_D)(usf_state_t *);

	void (osal_fastcall *SQRT_S)(usf_state_t *);
	void (osal_fastcall *SQRT_D)(usf_state_t *);

	void (osal_fastcall *C_F_S)(usf_state_t *);
	void (osal_fastcall *C_F_D)(usf_state_t *);
	void (osal_fastcall *C_UN_S)(usf_state_t *);
	void (osal_fastcall *C_UN_D)(usf_state_t *);
	void (osal_fastcall *C_EQ_S)(usf_state_t *);
	void (osal_fastcall *C_EQ_D)(usf_state_t *);
	void (osal_fastcall *C_UEQ_S)(usf_state_t *);
	void (osal_fastcall *C_UEQ_D)(usf_state_t *);
	void (osal_fastcall *C_OLT_S)(usf_state_t *);
	void (osal_fastcall *C_OLT_D)(usf_state_t *);
	void (osal_fastcall *C_ULT_S)(usf_state_t *);
	void (osal_fastcall *C_ULT_D)(usf_state_t *);
	void (osal_fastcall *C_OLE_S)(usf_state_t *);
	void (osal_fastcall *C_OLE_D)(usf_state_t *);
	void (osal_fastcall *C_ULE_S)(usf_state_t *);
	void (osal_fastcall *C_ULE_D)(usf_state_t *);
	void (osal_fastcall *C_SF_S)(usf_state_t *);
	void (osal_fastcall *C_SF_D)(usf_state_t *);
	void (osal_fastcall *C_NGLE_S)(usf_state_t *);
	void (osal_fastcall *C_NGLE_D)(usf_state_t *);
	void (osal_fastcall *C_SEQ_S)(usf_state_t *);
	void (osal_fastcall *C_SEQ_D)(usf_state_t *);
	void (osal_fastcall *C_NGL_S)(usf_state_t *);
	void (osal_fastcall *C_NGL_D)(usf_state_t *);
	void (osal_fastcall *C_LT_S)(usf_state_t *);
	void (osal_fastcall *C_LT_D)(usf_state_t *);
	void (osal_fastcall *C_NGE_S)(usf_state_t *);
	void (osal_fastcall *C_NGE_D)(usf_state_t *);
	void (osal_fastcall *C_LE_S)(usf_state_t *);
	void (osal_fastcall *C_LE_D)(usf_state_t *);
	void (osal_fastcall *C_NGT_S)(usf_state_t *);
	void (osal_fastcall *C_NGT_D)(usf_state_t *);

	// Special instructions
	void (osal_fastcall *SYSCALL)(usf_state_t *);
	void (osal_fastcall *BREAK)(usf_state_t *);

	// Exception instructions
	void (osal_fastcall *TEQ)(usf_state_t *);

	// Emulator helper functions
	void (osal_fastcall *NOP)(usf_state_t *);          // No operation (used to nullify R0 writes)
	void (osal_fastcall *RESERVED)(usf_state_t *);     // Reserved instruction handler
	void (osal_fastcall *NI)(usf_state_t *);	        // Not implemented instruction handler

	void (osal_fastcall *FIN_BLOCK)(usf_state_t *);    // Handler for the end of a block
	void (osal_fastcall *NOTCOMPILED)(usf_state_t *);  // Handler for not yet compiled code
	void (osal_fastcall *NOTCOMPILED2)(usf_state_t *); // TODOXXX
} cpu_instruction_table;

#endif /* M64P_R4300_OPS_H_*/
