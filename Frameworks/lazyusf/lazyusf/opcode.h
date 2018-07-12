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
#ifndef __OpCode
#define __OpCode

#include "types.h"

typedef struct {
	union {

		uint32_t Hex;
		uint8_t Ascii[4];

		struct {
			unsigned offset : 16;
			unsigned rt : 5;
			unsigned rs : 5;
			unsigned op : 6;
		} b;

		struct {
			unsigned immediate : 16;
			unsigned : 5;
			unsigned base : 5;
			unsigned : 6;
		} c;

		struct {
			unsigned target : 26;
			unsigned : 6;
		} d;

		struct {
			unsigned funct : 6;
			unsigned sa : 5;
			unsigned rd : 5;
			unsigned : 5;
			unsigned : 5;
			unsigned : 6;
		} e;

		struct {
			unsigned : 6;
			unsigned fd : 5;
			unsigned fs : 5;
			unsigned ft : 5;
			unsigned fmt : 5;
			unsigned : 6;
		} f;
	} u;

} OPCODE;

//R4300i OpCodes
#define	R4300i_SPECIAL				 0
#define	R4300i_REGIMM				 1
#define R4300i_J					 2
#define R4300i_JAL					 3
#define R4300i_BEQ					 4
#define R4300i_BNE					 5
#define R4300i_BLEZ					 6
#define R4300i_BGTZ					 7
#define R4300i_ADDI					 8
#define R4300i_ADDIU				 9
#define R4300i_SLTI					10
#define R4300i_SLTIU				11
#define R4300i_ANDI					12
#define R4300i_ORI					13
#define R4300i_XORI					14
#define R4300i_LUI					15
#define	R4300i_CP0					16
#define	R4300i_CP1					17
#define R4300i_BEQL					20
#define R4300i_BNEL					21
#define R4300i_BLEZL				22
#define R4300i_BGTZL				23
#define R4300i_DADDI				24
#define R4300i_DADDIU				25
#define R4300i_LDL					26
#define R4300i_LDR					27
#define R4300i_LB					32
#define R4300i_LH					33
#define R4300i_LWL					34
#define R4300i_LW					35
#define R4300i_LBU					36
#define R4300i_LHU					37
#define R4300i_LWR					38
#define R4300i_LWU					39
#define R4300i_SB					40
#define R4300i_SH					41
#define R4300i_SWL					42
#define R4300i_SW					43
#define R4300i_SDL					44
#define R4300i_SDR					45
#define R4300i_SWR					46
#define R4300i_CACHE				47
#define R4300i_LL					48
#define R4300i_LWC1					49
#define R4300i_LWC2					0x32
#define R4300i_LLD					0x34
#define R4300i_LDC1					53
#define R4300i_LDC2					0x36
#define R4300i_LD					55
#define R4300i_SC					0x38
#define R4300i_SWC1					57
#define R4300i_SWC2					0x3A
#define R4300i_SCD					0x3C
#define R4300i_SDC1					61
#define R4300i_SDC2					62
#define R4300i_SD					63

/* R4300i Special opcodes */
#define R4300i_SPECIAL_SLL			 0
#define R4300i_SPECIAL_SRL			 2
#define R4300i_SPECIAL_SRA			 3
#define R4300i_SPECIAL_SLLV			 4
#define R4300i_SPECIAL_SRLV			 6
#define R4300i_SPECIAL_SRAV			 7
#define R4300i_SPECIAL_JR			 8
#define R4300i_SPECIAL_JALR			 9
#define R4300i_SPECIAL_SYSCALL		12
#define R4300i_SPECIAL_BREAK		13
#define R4300i_SPECIAL_SYNC			15
#define R4300i_SPECIAL_MFHI			16
#define R4300i_SPECIAL_MTHI			17
#define R4300i_SPECIAL_MFLO			18
#define R4300i_SPECIAL_MTLO			19
#define R4300i_SPECIAL_DSLLV		20
#define R4300i_SPECIAL_DSRLV		22
#define R4300i_SPECIAL_DSRAV		23
#define R4300i_SPECIAL_MULT			24
#define R4300i_SPECIAL_MULTU		25
#define R4300i_SPECIAL_DIV			26
#define R4300i_SPECIAL_DIVU			27
#define R4300i_SPECIAL_DMULT		28
#define R4300i_SPECIAL_DMULTU		29
#define R4300i_SPECIAL_DDIV			30
#define R4300i_SPECIAL_DDIVU		31
#define R4300i_SPECIAL_ADD			32
#define R4300i_SPECIAL_ADDU			33
#define R4300i_SPECIAL_SUB			34
#define R4300i_SPECIAL_SUBU			35
#define R4300i_SPECIAL_AND			36
#define R4300i_SPECIAL_OR			37
#define R4300i_SPECIAL_XOR			38
#define R4300i_SPECIAL_NOR			39
#define R4300i_SPECIAL_SLT			42
#define R4300i_SPECIAL_SLTU			43
#define R4300i_SPECIAL_DADD			44
#define R4300i_SPECIAL_DADDU		45
#define R4300i_SPECIAL_DSUB			46
#define R4300i_SPECIAL_DSUBU		47
#define R4300i_SPECIAL_TGE			48
#define R4300i_SPECIAL_TGEU			49
#define R4300i_SPECIAL_TLT			50
#define R4300i_SPECIAL_TLTU			51
#define R4300i_SPECIAL_TEQ			52
#define R4300i_SPECIAL_TNE			54
#define R4300i_SPECIAL_DSLL			56
#define R4300i_SPECIAL_DSRL			58
#define R4300i_SPECIAL_DSRA			59
#define R4300i_SPECIAL_DSLL32		60
#define R4300i_SPECIAL_DSRL32		62
#define R4300i_SPECIAL_DSRA32		63

/* R4300i RegImm opcodes */
#define R4300i_REGIMM_BLTZ			0
#define R4300i_REGIMM_BGEZ			1
#define R4300i_REGIMM_BLTZL			2
#define R4300i_REGIMM_BGEZL			3
#define R4300i_REGIMM_TGEI			0x08
#define R4300i_REGIMM_TGEIU			0x09
#define R4300i_REGIMM_TLTI			0x0A
#define R4300i_REGIMM_TLTIU			0x0B
#define R4300i_REGIMM_TEQI			0x0C
#define R4300i_REGIMM_TNEI			0x0E
#define R4300i_REGIMM_BLTZAL		0x10
#define R4300i_REGIMM_BGEZAL		17
#define R4300i_REGIMM_BLTZALL		0x12
#define R4300i_REGIMM_BGEZALL		0x13

/* R4300i COP0 opcodes */
#define	R4300i_COP0_MF				 0
#define	R4300i_COP0_MT				 4

/* R4300i COP0 CO opcodes */
#define R4300i_COP0_CO_TLBR			1
#define R4300i_COP0_CO_TLBWI		2
#define R4300i_COP0_CO_TLBWR		6
#define R4300i_COP0_CO_TLBP			8
#define R4300i_COP0_CO_ERET			24

/* R4300i COP1 opcodes */
#define	R4300i_COP1_MF				0
#define	R4300i_COP1_DMF				1
#define	R4300i_COP1_CF				2
#define	R4300i_COP1_MT				4
#define	R4300i_COP1_DMT				5
#define	R4300i_COP1_CT				6
#define	R4300i_COP1_BC				8
#define R4300i_COP1_S				16
#define R4300i_COP1_D				17
#define R4300i_COP1_W				20
#define R4300i_COP1_L				21

/* R4300i COP1 BC opcodes */
#define	R4300i_COP1_BC_BCF			0
#define	R4300i_COP1_BC_BCT			1
#define	R4300i_COP1_BC_BCFL			2
#define	R4300i_COP1_BC_BCTL			3

#define R4300i_COP1_FUNCT_ADD		 0
#define R4300i_COP1_FUNCT_SUB		 1
#define R4300i_COP1_FUNCT_MUL		 2
#define R4300i_COP1_FUNCT_DIV		 3
#define R4300i_COP1_FUNCT_SQRT		 4
#define R4300i_COP1_FUNCT_ABS		 5
#define R4300i_COP1_FUNCT_MOV		 6
#define R4300i_COP1_FUNCT_NEG		 7
#define R4300i_COP1_FUNCT_ROUND_L	 8
#define R4300i_COP1_FUNCT_TRUNC_L	 9
#define R4300i_COP1_FUNCT_CEIL_L	10
#define R4300i_COP1_FUNCT_FLOOR_L	11
#define R4300i_COP1_FUNCT_ROUND_W	12
#define R4300i_COP1_FUNCT_TRUNC_W	13
#define R4300i_COP1_FUNCT_CEIL_W	14
#define R4300i_COP1_FUNCT_FLOOR_W	15
#define R4300i_COP1_FUNCT_CVT_S		32
#define R4300i_COP1_FUNCT_CVT_D		33
#define R4300i_COP1_FUNCT_CVT_W		36
#define R4300i_COP1_FUNCT_CVT_L		37
#define R4300i_COP1_FUNCT_C_F		48
#define R4300i_COP1_FUNCT_C_UN		49
#define R4300i_COP1_FUNCT_C_EQ		50
#define R4300i_COP1_FUNCT_C_UEQ		51
#define R4300i_COP1_FUNCT_C_OLT		52
#define R4300i_COP1_FUNCT_C_ULT		53
#define R4300i_COP1_FUNCT_C_OLE		54
#define R4300i_COP1_FUNCT_C_ULE		55
#define R4300i_COP1_FUNCT_C_SF		56
#define R4300i_COP1_FUNCT_C_NGLE	57
#define R4300i_COP1_FUNCT_C_SEQ		58
#define R4300i_COP1_FUNCT_C_NGL		59
#define R4300i_COP1_FUNCT_C_LT		60
#define R4300i_COP1_FUNCT_C_NGE		61
#define R4300i_COP1_FUNCT_C_LE		62
#define R4300i_COP1_FUNCT_C_NGT		63

#endif


