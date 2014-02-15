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
/************************* OpCode functions *************************/
void  r4300i_J              ( usf_state_t * );
void  r4300i_JAL            ( usf_state_t * );
void  r4300i_BNE            ( usf_state_t * );
void  r4300i_BEQ            ( usf_state_t * );
void  r4300i_BLEZ           ( usf_state_t * );
void  r4300i_BGTZ           ( usf_state_t * );
void  r4300i_ADDI           ( usf_state_t * );
void  r4300i_ADDIU          ( usf_state_t * );
void  r4300i_SLTI           ( usf_state_t * );
void  r4300i_SLTIU          ( usf_state_t * );
void  r4300i_ANDI           ( usf_state_t * );
void  r4300i_ORI            ( usf_state_t * );
void  r4300i_XORI           ( usf_state_t * );
void  r4300i_LUI            ( usf_state_t * );
void  r4300i_BEQL           ( usf_state_t * );
void  r4300i_BNEL           ( usf_state_t * );
void  r4300i_BLEZL          ( usf_state_t * );
void  r4300i_BGTZL          ( usf_state_t * );
void  r4300i_DADDIU         ( usf_state_t * );
void  r4300i_LDL            ( usf_state_t * );
void  r4300i_LDR            ( usf_state_t * );
void  r4300i_LB             ( usf_state_t * );
void  r4300i_LH             ( usf_state_t * );
void  r4300i_LWL            ( usf_state_t * );
void  r4300i_LW             ( usf_state_t * );
void  r4300i_LBU            ( usf_state_t * );
void  r4300i_LHU            ( usf_state_t * );
void  r4300i_LWR            ( usf_state_t * );
void  r4300i_LWU            ( usf_state_t * );
void  r4300i_SB             ( usf_state_t * );
void  r4300i_SH             ( usf_state_t * );
void  r4300i_SWL            ( usf_state_t * );
void  r4300i_SW             ( usf_state_t * );
void  r4300i_SDL            ( usf_state_t * );
void  r4300i_SDR            ( usf_state_t * );
void  r4300i_SWR            ( usf_state_t * );
void  r4300i_CACHE          ( usf_state_t * );
void  r4300i_LL             ( usf_state_t * );
void  r4300i_LWC1           ( usf_state_t * );
void  r4300i_LDC1           ( usf_state_t * );
void  r4300i_LD             ( usf_state_t * );
void  r4300i_SC             ( usf_state_t * );
void  r4300i_SWC1           ( usf_state_t * );
void  r4300i_SDC1           ( usf_state_t * );
void  r4300i_SD             ( usf_state_t * );

/********************** R4300i OpCodes: Special **********************/
void  r4300i_SPECIAL_SLL    ( usf_state_t * );
void  r4300i_SPECIAL_SRL    ( usf_state_t * );
void  r4300i_SPECIAL_SRA    ( usf_state_t * );
void  r4300i_SPECIAL_SLLV   ( usf_state_t * );
void  r4300i_SPECIAL_SRLV   ( usf_state_t * );
void  r4300i_SPECIAL_SRAV   ( usf_state_t * );
void  r4300i_SPECIAL_JR     ( usf_state_t * );
void  r4300i_SPECIAL_JALR   ( usf_state_t * );
void  r4300i_SPECIAL_SYSCALL ( usf_state_t * );
void  r4300i_SPECIAL_BREAK   ( usf_state_t * );
void  r4300i_SPECIAL_SYNC    ( usf_state_t * );
void  r4300i_SPECIAL_MFHI    ( usf_state_t * );
void  r4300i_SPECIAL_MTHI    ( usf_state_t * );
void  r4300i_SPECIAL_MFLO   ( usf_state_t * );
void  r4300i_SPECIAL_MTLO   ( usf_state_t * );
void  r4300i_SPECIAL_DSLLV  ( usf_state_t * );
void  r4300i_SPECIAL_DSRLV  ( usf_state_t * );
void  r4300i_SPECIAL_DSRAV  ( usf_state_t * );
void  r4300i_SPECIAL_MULT   ( usf_state_t * );
void  r4300i_SPECIAL_MULTU  ( usf_state_t * );
void  r4300i_SPECIAL_DIV    ( usf_state_t * );
void  r4300i_SPECIAL_DIVU   ( usf_state_t * );
void  r4300i_SPECIAL_DMULT  ( usf_state_t * );
void  r4300i_SPECIAL_DMULTU ( usf_state_t * );
void  r4300i_SPECIAL_DDIV   ( usf_state_t * );
void  r4300i_SPECIAL_DDIVU  ( usf_state_t * );
void  r4300i_SPECIAL_ADD    ( usf_state_t * );
void  r4300i_SPECIAL_ADDU   ( usf_state_t * );
void  r4300i_SPECIAL_SUB    ( usf_state_t * );
void  r4300i_SPECIAL_SUBU   ( usf_state_t * );
void  r4300i_SPECIAL_AND    ( usf_state_t * );
void  r4300i_SPECIAL_OR     ( usf_state_t * );
void  r4300i_SPECIAL_XOR    ( usf_state_t * );
void  r4300i_SPECIAL_NOR    ( usf_state_t * );
void  r4300i_SPECIAL_SLT    ( usf_state_t * );
void  r4300i_SPECIAL_SLTU   ( usf_state_t * );
void  r4300i_SPECIAL_DADD   ( usf_state_t * );
void  r4300i_SPECIAL_DADDU  ( usf_state_t * );
void  r4300i_SPECIAL_DSUB   ( usf_state_t * );
void  r4300i_SPECIAL_DSUBU  ( usf_state_t * );
void  r4300i_SPECIAL_TEQ    ( usf_state_t * );
void  r4300i_SPECIAL_DSLL   ( usf_state_t * );
void  r4300i_SPECIAL_DSRL   ( usf_state_t * );
void  r4300i_SPECIAL_DSRA   ( usf_state_t * );
void  r4300i_SPECIAL_DSLL32 ( usf_state_t * );
void  r4300i_SPECIAL_DSRL32 ( usf_state_t * );
void  r4300i_SPECIAL_DSRA32 ( usf_state_t * );

/********************** R4300i OpCodes: RegImm **********************/
void  r4300i_REGIMM_BLTZ    ( usf_state_t * );
void  r4300i_REGIMM_BGEZ    ( usf_state_t * );
void  r4300i_REGIMM_BLTZL   ( usf_state_t * );
void  r4300i_REGIMM_BGEZL   ( usf_state_t * );
void  r4300i_REGIMM_BLTZAL  ( usf_state_t * );
void  r4300i_REGIMM_BGEZAL  ( usf_state_t * );

/************************** COP0 functions **************************/
void  r4300i_COP0_MF        ( usf_state_t * );
void  r4300i_COP0_MT        ( usf_state_t * );

/************************** COP0 CO functions ***********************/
void  r4300i_COP0_CO_TLBR   ( usf_state_t * );
void  r4300i_COP0_CO_TLBWI  ( usf_state_t * );
void  r4300i_COP0_CO_TLBWR  ( usf_state_t * );
void  r4300i_COP0_CO_TLBP   ( usf_state_t * );
void  r4300i_COP0_CO_ERET   ( usf_state_t * );

/************************** COP1 functions **************************/
void  r4300i_COP1_MF        ( usf_state_t * );
void  r4300i_COP1_DMF       ( usf_state_t * );
void  r4300i_COP1_CF        ( usf_state_t * );
void  r4300i_COP1_MT        ( usf_state_t * );
void  r4300i_COP1_DMT       ( usf_state_t * );
void  r4300i_COP1_CT        ( usf_state_t * );

/************************* COP1: BC1 functions ***********************/
void  r4300i_COP1_BCF       ( usf_state_t * );
void  r4300i_COP1_BCT       ( usf_state_t * );
void  r4300i_COP1_BCFL      ( usf_state_t * );
void  r4300i_COP1_BCTL      ( usf_state_t * );

/************************** COP1: S functions ************************/
void  r4300i_COP1_S_ADD     ( usf_state_t * );
void  r4300i_COP1_S_SUB     ( usf_state_t * );
void  r4300i_COP1_S_MUL     ( usf_state_t * );
void  r4300i_COP1_S_DIV     ( usf_state_t * );
void  r4300i_COP1_S_SQRT    ( usf_state_t * );
void  r4300i_COP1_S_ABS     ( usf_state_t * );
void  r4300i_COP1_S_MOV     ( usf_state_t * );
void  r4300i_COP1_S_NEG     ( usf_state_t * );
void  r4300i_COP1_S_TRUNC_L ( usf_state_t * );
void  r4300i_COP1_S_CEIL_L  ( usf_state_t * );	//added by Witten
void  r4300i_COP1_S_FLOOR_L ( usf_state_t * );	//added by Witten
void  r4300i_COP1_S_ROUND_W ( usf_state_t * );
void  r4300i_COP1_S_TRUNC_W ( usf_state_t * );
void  r4300i_COP1_S_CEIL_W  ( usf_state_t * );	//added by Witten
void  r4300i_COP1_S_FLOOR_W ( usf_state_t * );
void  r4300i_COP1_S_CVT_D   ( usf_state_t * );
void  r4300i_COP1_S_CVT_W   ( usf_state_t * );
void  r4300i_COP1_S_CVT_L   ( usf_state_t * );
void  r4300i_COP1_S_CMP     ( usf_state_t * );

/************************** COP1: D functions ************************/
void  r4300i_COP1_D_ADD     ( usf_state_t * );
void  r4300i_COP1_D_SUB     ( usf_state_t * );
void  r4300i_COP1_D_MUL     ( usf_state_t * );
void  r4300i_COP1_D_DIV     ( usf_state_t * );
void  r4300i_COP1_D_SQRT    ( usf_state_t * );
void  r4300i_COP1_D_ABS     ( usf_state_t * );
void  r4300i_COP1_D_MOV     ( usf_state_t * );
void  r4300i_COP1_D_NEG     ( usf_state_t * );
void  r4300i_COP1_D_TRUNC_L ( usf_state_t * );	//added by Witten
void  r4300i_COP1_D_CEIL_L  ( usf_state_t * );	//added by Witten
void  r4300i_COP1_D_FLOOR_L ( usf_state_t * );	//added by Witten
void  r4300i_COP1_D_ROUND_W ( usf_state_t * );
void  r4300i_COP1_D_TRUNC_W ( usf_state_t * );
void  r4300i_COP1_D_CEIL_W  ( usf_state_t * );	//added by Witten
void  r4300i_COP1_D_FLOOR_W ( usf_state_t * );	//added by Witten
void  r4300i_COP1_D_CVT_S   ( usf_state_t * );
void  r4300i_COP1_D_CVT_W   ( usf_state_t * );
void  r4300i_COP1_D_CVT_L   ( usf_state_t * );
void  r4300i_COP1_D_CMP     ( usf_state_t * );

/************************** COP1: W functions ************************/
void  r4300i_COP1_W_CVT_S   ( usf_state_t * );
void  r4300i_COP1_W_CVT_D   ( usf_state_t * );

/************************** COP1: L functions ************************/
void  r4300i_COP1_L_CVT_S   ( usf_state_t * );
void  r4300i_COP1_L_CVT_D   ( usf_state_t * );

/************************** Other functions **************************/
void   R4300i_UnknownOpcode ( usf_state_t * );
