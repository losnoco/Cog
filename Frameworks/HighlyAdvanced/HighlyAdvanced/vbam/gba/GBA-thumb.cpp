#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdarg.h>
#include <string.h>

#include "GBA.h"
#include "GBAcpu.h"
#include "GBAinline.h"
#include "Globals.h"

#include "Sound.h"
#include "bios.h"
#include "../common/Types.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

///////////////////////////////////////////////////////////////////////////

static INSN_REGPARM void thumbUnknownInsn(GBASystem *gba, u32 opcode)
{
  CPUUndefinedException(gba);
}

// Common macros //////////////////////////////////////////////////////////

#define THUMB_CONSOLE_OUTPUT(a,b)
#define UPDATE_OLDREG

#define NEG(i) ((i) >> 31)
#define POS(i) ((~(i)) >> 31)

// C core
#ifndef ADDCARRY
 #define ADDCARRY(a, b, c) \
  gba->C_FLAG = ((NEG(a) & NEG(b)) |\
            (NEG(a) & POS(c)) |\
            (NEG(b) & POS(c))) ? true : false;
#endif
#ifndef ADDOVERFLOW
 #define ADDOVERFLOW(a, b, c) \
  gba->V_FLAG = ((NEG(a) & NEG(b) & POS(c)) |\
            (POS(a) & POS(b) & NEG(c))) ? true : false;
#endif
#ifndef SUBCARRY
 #define SUBCARRY(a, b, c) \
  gba->C_FLAG = ((NEG(a) & POS(b)) |\
            (NEG(a) & POS(c)) |\
            (POS(b) & POS(c))) ? true : false;
#endif
#ifndef SUBOVERFLOW
 #define SUBOVERFLOW(a, b, c)\
  gba->V_FLAG = ((NEG(a) & POS(b) & POS(c)) |\
            (POS(a) & NEG(b) & NEG(c))) ? true : false;
#endif
#ifndef ADD_RD_RS_RN
 #define ADD_RD_RS_RN(N) \
   {\
     u32 lhs = gba->reg[source].I;\
     u32 rhs = gba->reg[N].I;\
     u32 res = lhs + rhs;\
     gba->reg[dest].I = res;\
     gba->Z_FLAG = (res == 0) ? true : false;\
     gba->N_FLAG = NEG(res) ? true : false;\
     ADDCARRY(lhs, rhs, res);\
     ADDOVERFLOW(lhs, rhs, res);\
   }
#endif
#ifndef ADD_RD_RS_O3
 #define ADD_RD_RS_O3(N) \
   {\
     u32 lhs = gba->reg[source].I;\
     u32 rhs = N;\
     u32 res = lhs + rhs;\
     gba->reg[dest].I = res;\
     gba->Z_FLAG = (res == 0) ? true : false;\
     gba->N_FLAG = NEG(res) ? true : false;\
     ADDCARRY(lhs, rhs, res);\
     ADDOVERFLOW(lhs, rhs, res);\
   }
#endif
#ifndef ADD_RD_RS_O3_0
# define ADD_RD_RS_O3_0 ADD_RD_RS_O3
#endif
#ifndef ADD_RN_O8
 #define ADD_RN_O8(d) \
   {\
     u32 lhs = gba->reg[(d)].I;\
     u32 rhs = (opcode & 255);\
     u32 res = lhs + rhs;\
     gba->reg[(d)].I = res;\
     gba->Z_FLAG = (res == 0) ? true : false;\
     gba->N_FLAG = NEG(res) ? true : false;\
     ADDCARRY(lhs, rhs, res);\
     ADDOVERFLOW(lhs, rhs, res);\
   }
#endif
#ifndef CMN_RD_RS
 #define CMN_RD_RS \
   {\
     u32 lhs = gba->reg[dest].I;\
     u32 rhs = value;\
     u32 res = lhs + rhs;\
     gba->Z_FLAG = (res == 0) ? true : false;\
     gba->N_FLAG = NEG(res) ? true : false;\
     ADDCARRY(lhs, rhs, res);\
     ADDOVERFLOW(lhs, rhs, res);\
   }
#endif
#ifndef ADC_RD_RS
 #define ADC_RD_RS \
   {\
     u32 lhs = gba->reg[dest].I;\
     u32 rhs = value;\
     u32 res = lhs + rhs + (u32)gba->C_FLAG;\
     gba->reg[dest].I = res;\
     gba->Z_FLAG = (res == 0) ? true : false;\
     gba->N_FLAG = NEG(res) ? true : false;\
     ADDCARRY(lhs, rhs, res);\
     ADDOVERFLOW(lhs, rhs, res);\
   }
#endif
#ifndef SUB_RD_RS_RN
 #define SUB_RD_RS_RN(N) \
   {\
     u32 lhs = gba->reg[source].I;\
     u32 rhs = gba->reg[N].I;\
     u32 res = lhs - rhs;\
     gba->reg[dest].I = res;\
     gba->Z_FLAG = (res == 0) ? true : false;\
     gba->N_FLAG = NEG(res) ? true : false;\
     SUBCARRY(lhs, rhs, res);\
     SUBOVERFLOW(lhs, rhs, res);\
   }
#endif
#ifndef SUB_RD_RS_O3
 #define SUB_RD_RS_O3(N) \
   {\
     u32 lhs = gba->reg[source].I;\
     u32 rhs = N;\
     u32 res = lhs - rhs;\
     gba->reg[dest].I = res;\
     gba->Z_FLAG = (res == 0) ? true : false;\
     gba->N_FLAG = NEG(res) ? true : false;\
     SUBCARRY(lhs, rhs, res);\
     SUBOVERFLOW(lhs, rhs, res);\
   }
#endif
#ifndef SUB_RD_RS_O3_0
# define SUB_RD_RS_O3_0 SUB_RD_RS_O3
#endif
#ifndef SUB_RN_O8
 #define SUB_RN_O8(d) \
   {\
     u32 lhs = gba->reg[(d)].I;\
     u32 rhs = (opcode & 255);\
     u32 res = lhs - rhs;\
     gba->reg[(d)].I = res;\
     gba->Z_FLAG = (res == 0) ? true : false;\
     gba->N_FLAG = NEG(res) ? true : false;\
     SUBCARRY(lhs, rhs, res);\
     SUBOVERFLOW(lhs, rhs, res);\
   }
#endif
#ifndef MOV_RN_O8
 #define MOV_RN_O8(d) \
   {\
     gba->reg[d].I = opcode & 255;\
     gba->N_FLAG = false;\
     gba->Z_FLAG = (gba->reg[d].I ? false : true);\
   }
#endif
#ifndef CMP_RN_O8
 #define CMP_RN_O8(d) \
   {\
     u32 lhs = gba->reg[(d)].I;\
     u32 rhs = (opcode & 255);\
     u32 res = lhs - rhs;\
     gba->Z_FLAG = (res == 0) ? true : false;\
     gba->N_FLAG = NEG(res) ? true : false;\
     SUBCARRY(lhs, rhs, res);\
     SUBOVERFLOW(lhs, rhs, res);\
   }
#endif
#ifndef SBC_RD_RS
 #define SBC_RD_RS \
   {\
     u32 lhs = gba->reg[dest].I;\
     u32 rhs = value;\
     u32 res = lhs - rhs - !((u32)gba->C_FLAG);\
     gba->reg[dest].I = res;\
     gba->Z_FLAG = (res == 0) ? true : false;\
     gba->N_FLAG = NEG(res) ? true : false;\
     SUBCARRY(lhs, rhs, res);\
     SUBOVERFLOW(lhs, rhs, res);\
   }
#endif
#ifndef LSL_RD_RM_I5
 #define LSL_RD_RM_I5 \
   {\
     gba->C_FLAG = (gba->reg[source].I >> (32 - shift)) & 1 ? true : false;\
     value = gba->reg[source].I << shift;\
   }
#endif
#ifndef LSL_RD_RS
 #define LSL_RD_RS \
   {\
     gba->C_FLAG = (gba->reg[dest].I >> (32 - value)) & 1 ? true : false;\
     value = gba->reg[dest].I << value;\
   }
#endif
#ifndef LSR_RD_RM_I5
 #define LSR_RD_RM_I5 \
   {\
     gba->C_FLAG = (gba->reg[source].I >> (shift - 1)) & 1 ? true : false;\
     value = gba->reg[source].I >> shift;\
   }
#endif
#ifndef LSR_RD_RS
 #define LSR_RD_RS \
   {\
     gba->C_FLAG = (gba->reg[dest].I >> (value - 1)) & 1 ? true : false;\
     value = gba->reg[dest].I >> value;\
   }
#endif
#ifndef ASR_RD_RM_I5
 #define ASR_RD_RM_I5 \
   {\
     gba->C_FLAG = ((s32)gba->reg[source].I >> (int)(shift - 1)) & 1 ? true : false;\
     value = (s32)gba->reg[source].I >> (int)shift;\
   }
#endif
#ifndef ASR_RD_RS
 #define ASR_RD_RS \
   {\
     gba->C_FLAG = ((s32)gba->reg[dest].I >> (int)(value - 1)) & 1 ? true : false;\
     value = (s32)gba->reg[dest].I >> (int)value;\
   }
#endif
#ifndef ROR_RD_RS
 #define ROR_RD_RS \
   {\
     gba->C_FLAG = (gba->reg[dest].I >> (value - 1)) & 1 ? true : false;\
     value = ((gba->reg[dest].I << (32 - value)) |\
              (gba->reg[dest].I >> value));\
   }
#endif
#ifndef NEG_RD_RS
 #define NEG_RD_RS \
   {\
     u32 lhs = gba->reg[source].I;\
     u32 rhs = 0;\
     u32 res = rhs - lhs;\
     gba->reg[dest].I = res;\
     gba->Z_FLAG = (res == 0) ? true : false;\
     gba->N_FLAG = NEG(res) ? true : false;\
     SUBCARRY(rhs, lhs, res);\
     SUBOVERFLOW(rhs, lhs, res);\
   }
#endif
#ifndef CMP_RD_RS
 #define CMP_RD_RS \
   {\
     u32 lhs = gba->reg[dest].I;\
     u32 rhs = value;\
     u32 res = lhs - rhs;\
     gba->Z_FLAG = (res == 0) ? true : false;\
     gba->N_FLAG = NEG(res) ? true : false;\
     SUBCARRY(lhs, rhs, res);\
     SUBOVERFLOW(lhs, rhs, res);\
   }
#endif
#ifndef IMM5_INSN
 #define IMM5_INSN(OP,N) \
  int dest = opcode & 0x07;\
  int source = (opcode >> 3) & 0x07;\
  u32 value;\
  OP(N);\
  gba->reg[dest].I = value;\
  gba->N_FLAG = (value & 0x80000000 ? true : false);\
  gba->Z_FLAG = (value ? false : true);
 #define IMM5_INSN_0(OP) \
  int dest = opcode & 0x07;\
  int source = (opcode >> 3) & 0x07;\
  u32 value;\
  OP;\
  gba->reg[dest].I = value;\
  gba->N_FLAG = (value & 0x80000000 ? true : false);\
  gba->Z_FLAG = (value ? false : true);
 #define IMM5_LSL(N) \
  int shift = N;\
  LSL_RD_RM_I5;
 #define IMM5_LSL_0 \
  value = gba->reg[source].I;
 #define IMM5_LSR(N) \
  int shift = N;\
  LSR_RD_RM_I5;
 #define IMM5_LSR_0 \
  gba->C_FLAG = gba->reg[source].I & 0x80000000 ? true : false;\
  value = 0;
 #define IMM5_ASR(N) \
  int shift = N;\
  ASR_RD_RM_I5;
 #define IMM5_ASR_0 \
  if(gba->reg[source].I & 0x80000000) {\
    value = 0xFFFFFFFF;\
    gba->C_FLAG = true;\
  } else {\
    value = 0;\
    gba->C_FLAG = false;\
  }
#endif
#ifndef THREEARG_INSN
 #define THREEARG_INSN(OP,N) \
  int dest = opcode & 0x07;          \
  int source = (opcode >> 3) & 0x07; \
  OP(N);
#endif

// Shift instructions /////////////////////////////////////////////////////

#define DEFINE_IMM5_INSN(OP,BASE) \
  static INSN_REGPARM void thumb##BASE##_00(GBASystem *gba, u32 opcode) { IMM5_INSN_0(OP##_0); } \
  static INSN_REGPARM void thumb##BASE##_01(GBASystem *gba, u32 opcode) { IMM5_INSN(OP, 1); } \
  static INSN_REGPARM void thumb##BASE##_02(GBASystem *gba, u32 opcode) { IMM5_INSN(OP, 2); } \
  static INSN_REGPARM void thumb##BASE##_03(GBASystem *gba, u32 opcode) { IMM5_INSN(OP, 3); } \
  static INSN_REGPARM void thumb##BASE##_04(GBASystem *gba, u32 opcode) { IMM5_INSN(OP, 4); } \
  static INSN_REGPARM void thumb##BASE##_05(GBASystem *gba, u32 opcode) { IMM5_INSN(OP, 5); } \
  static INSN_REGPARM void thumb##BASE##_06(GBASystem *gba, u32 opcode) { IMM5_INSN(OP, 6); } \
  static INSN_REGPARM void thumb##BASE##_07(GBASystem *gba, u32 opcode) { IMM5_INSN(OP, 7); } \
  static INSN_REGPARM void thumb##BASE##_08(GBASystem *gba, u32 opcode) { IMM5_INSN(OP, 8); } \
  static INSN_REGPARM void thumb##BASE##_09(GBASystem *gba, u32 opcode) { IMM5_INSN(OP, 9); } \
  static INSN_REGPARM void thumb##BASE##_0A(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,10); } \
  static INSN_REGPARM void thumb##BASE##_0B(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,11); } \
  static INSN_REGPARM void thumb##BASE##_0C(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,12); } \
  static INSN_REGPARM void thumb##BASE##_0D(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,13); } \
  static INSN_REGPARM void thumb##BASE##_0E(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,14); } \
  static INSN_REGPARM void thumb##BASE##_0F(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,15); } \
  static INSN_REGPARM void thumb##BASE##_10(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,16); } \
  static INSN_REGPARM void thumb##BASE##_11(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,17); } \
  static INSN_REGPARM void thumb##BASE##_12(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,18); } \
  static INSN_REGPARM void thumb##BASE##_13(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,19); } \
  static INSN_REGPARM void thumb##BASE##_14(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,20); } \
  static INSN_REGPARM void thumb##BASE##_15(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,21); } \
  static INSN_REGPARM void thumb##BASE##_16(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,22); } \
  static INSN_REGPARM void thumb##BASE##_17(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,23); } \
  static INSN_REGPARM void thumb##BASE##_18(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,24); } \
  static INSN_REGPARM void thumb##BASE##_19(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,25); } \
  static INSN_REGPARM void thumb##BASE##_1A(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,26); } \
  static INSN_REGPARM void thumb##BASE##_1B(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,27); } \
  static INSN_REGPARM void thumb##BASE##_1C(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,28); } \
  static INSN_REGPARM void thumb##BASE##_1D(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,29); } \
  static INSN_REGPARM void thumb##BASE##_1E(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,30); } \
  static INSN_REGPARM void thumb##BASE##_1F(GBASystem *gba, u32 opcode) { IMM5_INSN(OP,31); }

// LSL Rd, Rm, #Imm 5
DEFINE_IMM5_INSN(IMM5_LSL,00)
// LSR Rd, Rm, #Imm 5
DEFINE_IMM5_INSN(IMM5_LSR,08)
// ASR Rd, Rm, #Imm 5
DEFINE_IMM5_INSN(IMM5_ASR,10)

// 3-argument ADD/SUB /////////////////////////////////////////////////////

#define DEFINE_REG3_INSN(OP,BASE) \
  static INSN_REGPARM void thumb##BASE##_0(GBASystem *gba, u32 opcode) { THREEARG_INSN(OP,0); } \
  static INSN_REGPARM void thumb##BASE##_1(GBASystem *gba, u32 opcode) { THREEARG_INSN(OP,1); } \
  static INSN_REGPARM void thumb##BASE##_2(GBASystem *gba, u32 opcode) { THREEARG_INSN(OP,2); } \
  static INSN_REGPARM void thumb##BASE##_3(GBASystem *gba, u32 opcode) { THREEARG_INSN(OP,3); } \
  static INSN_REGPARM void thumb##BASE##_4(GBASystem *gba, u32 opcode) { THREEARG_INSN(OP,4); } \
  static INSN_REGPARM void thumb##BASE##_5(GBASystem *gba, u32 opcode) { THREEARG_INSN(OP,5); } \
  static INSN_REGPARM void thumb##BASE##_6(GBASystem *gba, u32 opcode) { THREEARG_INSN(OP,6); } \
  static INSN_REGPARM void thumb##BASE##_7(GBASystem *gba, u32 opcode) { THREEARG_INSN(OP,7); }

#define DEFINE_IMM3_INSN(OP,BASE) \
  static INSN_REGPARM void thumb##BASE##_0(GBASystem *gba, u32 opcode) { THREEARG_INSN(OP##_0,0); } \
  static INSN_REGPARM void thumb##BASE##_1(GBASystem *gba, u32 opcode) { THREEARG_INSN(OP,1); } \
  static INSN_REGPARM void thumb##BASE##_2(GBASystem *gba, u32 opcode) { THREEARG_INSN(OP,2); } \
  static INSN_REGPARM void thumb##BASE##_3(GBASystem *gba, u32 opcode) { THREEARG_INSN(OP,3); } \
  static INSN_REGPARM void thumb##BASE##_4(GBASystem *gba, u32 opcode) { THREEARG_INSN(OP,4); } \
  static INSN_REGPARM void thumb##BASE##_5(GBASystem *gba, u32 opcode) { THREEARG_INSN(OP,5); } \
  static INSN_REGPARM void thumb##BASE##_6(GBASystem *gba, u32 opcode) { THREEARG_INSN(OP,6); } \
  static INSN_REGPARM void thumb##BASE##_7(GBASystem *gba, u32 opcode) { THREEARG_INSN(OP,7); }

// ADD Rd, Rs, Rn
DEFINE_REG3_INSN(ADD_RD_RS_RN,18)
// SUB Rd, Rs, Rn
DEFINE_REG3_INSN(SUB_RD_RS_RN,1A)
// ADD Rd, Rs, #Offset3
DEFINE_IMM3_INSN(ADD_RD_RS_O3,1C)
// SUB Rd, Rs, #Offset3
DEFINE_IMM3_INSN(SUB_RD_RS_O3,1E)

// MOV/CMP/ADD/SUB immediate //////////////////////////////////////////////

// MOV R0, #Offset8
static INSN_REGPARM void thumb20(GBASystem *gba, u32 opcode) { MOV_RN_O8(0); }
// MOV R1, #Offset8
static INSN_REGPARM void thumb21(GBASystem *gba, u32 opcode) { MOV_RN_O8(1); }
// MOV R2, #Offset8
static INSN_REGPARM void thumb22(GBASystem *gba, u32 opcode) { MOV_RN_O8(2); }
// MOV R3, #Offset8
static INSN_REGPARM void thumb23(GBASystem *gba, u32 opcode) { MOV_RN_O8(3); }
// MOV R4, #Offset8
static INSN_REGPARM void thumb24(GBASystem *gba, u32 opcode) { MOV_RN_O8(4); }
// MOV R5, #Offset8
static INSN_REGPARM void thumb25(GBASystem *gba, u32 opcode) { MOV_RN_O8(5); }
// MOV R6, #Offset8
static INSN_REGPARM void thumb26(GBASystem *gba, u32 opcode) { MOV_RN_O8(6); }
// MOV R7, #Offset8
static INSN_REGPARM void thumb27(GBASystem *gba, u32 opcode) { MOV_RN_O8(7); }

// CMP R0, #Offset8
static INSN_REGPARM void thumb28(GBASystem *gba, u32 opcode) { CMP_RN_O8(0); }
// CMP R1, #Offset8
static INSN_REGPARM void thumb29(GBASystem *gba, u32 opcode) { CMP_RN_O8(1); }
// CMP R2, #Offset8
static INSN_REGPARM void thumb2A(GBASystem *gba, u32 opcode) { CMP_RN_O8(2); }
// CMP R3, #Offset8
static INSN_REGPARM void thumb2B(GBASystem *gba, u32 opcode) { CMP_RN_O8(3); }
// CMP R4, #Offset8
static INSN_REGPARM void thumb2C(GBASystem *gba, u32 opcode) { CMP_RN_O8(4); }
// CMP R5, #Offset8
static INSN_REGPARM void thumb2D(GBASystem *gba, u32 opcode) { CMP_RN_O8(5); }
// CMP R6, #Offset8
static INSN_REGPARM void thumb2E(GBASystem *gba, u32 opcode) { CMP_RN_O8(6); }
// CMP R7, #Offset8
static INSN_REGPARM void thumb2F(GBASystem *gba, u32 opcode) { CMP_RN_O8(7); }

// ADD R0,#Offset8
static INSN_REGPARM void thumb30(GBASystem *gba, u32 opcode) { ADD_RN_O8(0); }
// ADD R1,#Offset8
static INSN_REGPARM void thumb31(GBASystem *gba, u32 opcode) { ADD_RN_O8(1); }
// ADD R2,#Offset8
static INSN_REGPARM void thumb32(GBASystem *gba, u32 opcode) { ADD_RN_O8(2); }
// ADD R3,#Offset8
static INSN_REGPARM void thumb33(GBASystem *gba, u32 opcode) { ADD_RN_O8(3); }
// ADD R4,#Offset8
static INSN_REGPARM void thumb34(GBASystem *gba, u32 opcode) { ADD_RN_O8(4); }
// ADD R5,#Offset8
static INSN_REGPARM void thumb35(GBASystem *gba, u32 opcode) { ADD_RN_O8(5); }
// ADD R6,#Offset8
static INSN_REGPARM void thumb36(GBASystem *gba, u32 opcode) { ADD_RN_O8(6); }
// ADD R7,#Offset8
static INSN_REGPARM void thumb37(GBASystem *gba, u32 opcode) { ADD_RN_O8(7); }

// SUB R0,#Offset8
static INSN_REGPARM void thumb38(GBASystem *gba, u32 opcode) { SUB_RN_O8(0); }
// SUB R1,#Offset8
static INSN_REGPARM void thumb39(GBASystem *gba, u32 opcode) { SUB_RN_O8(1); }
// SUB R2,#Offset8
static INSN_REGPARM void thumb3A(GBASystem *gba, u32 opcode) { SUB_RN_O8(2); }
// SUB R3,#Offset8
static INSN_REGPARM void thumb3B(GBASystem *gba, u32 opcode) { SUB_RN_O8(3); }
// SUB R4,#Offset8
static INSN_REGPARM void thumb3C(GBASystem *gba, u32 opcode) { SUB_RN_O8(4); }
// SUB R5,#Offset8
static INSN_REGPARM void thumb3D(GBASystem *gba, u32 opcode) { SUB_RN_O8(5); }
// SUB R6,#Offset8
static INSN_REGPARM void thumb3E(GBASystem *gba, u32 opcode) { SUB_RN_O8(6); }
// SUB R7,#Offset8
static INSN_REGPARM void thumb3F(GBASystem *gba, u32 opcode) { SUB_RN_O8(7); }

// ALU operations /////////////////////////////////////////////////////////

// AND Rd, Rs
static INSN_REGPARM void thumb40_0(GBASystem *gba, u32 opcode)
{
  int dest = opcode & 7;
  gba->reg[dest].I &= gba->reg[(opcode >> 3)&7].I;
  gba->N_FLAG = gba->reg[dest].I & 0x80000000 ? true : false;
  gba->Z_FLAG = gba->reg[dest].I ? false : true;
  THUMB_CONSOLE_OUTPUT(NULL, reg[2].I);
}

// EOR Rd, Rs
static INSN_REGPARM void thumb40_1(GBASystem *gba, u32 opcode)
{
  int dest = opcode & 7;
  gba->reg[dest].I ^= gba->reg[(opcode >> 3)&7].I;
  gba->N_FLAG = gba->reg[dest].I & 0x80000000 ? true : false;
  gba->Z_FLAG = gba->reg[dest].I ? false : true;
}

// LSL Rd, Rs
static INSN_REGPARM void thumb40_2(GBASystem *gba, u32 opcode)
{
  int dest = opcode & 7;
  u32 value = gba->reg[(opcode >> 3)&7].B.B0;
  if(value) {
    if(value == 32) {
      value = 0;
      gba->C_FLAG = (gba->reg[dest].I & 1 ? true : false);
    } else if(value < 32) {
      LSL_RD_RS;
    } else {
      value = 0;
      gba->C_FLAG = false;
    }
    gba->reg[dest].I = value;
  }
  gba->N_FLAG = gba->reg[dest].I & 0x80000000 ? true : false;
  gba->Z_FLAG = gba->reg[dest].I ? false : true;
  gba->clockTicks = codeTicksAccess16(gba, gba->armNextPC)+2;
}

// LSR Rd, Rs
static INSN_REGPARM void thumb40_3(GBASystem *gba, u32 opcode)
{
  int dest = opcode & 7;
  u32 value = gba->reg[(opcode >> 3)&7].B.B0;
  if(value) {
    if(value == 32) {
      value = 0;
      gba->C_FLAG = (gba->reg[dest].I & 0x80000000 ? true : false);
    } else if(value < 32) {
      LSR_RD_RS;
    } else {
      value = 0;
      gba->C_FLAG = false;
    }
    gba->reg[dest].I = value;
  }
  gba->N_FLAG = gba->reg[dest].I & 0x80000000 ? true : false;
  gba->Z_FLAG = gba->reg[dest].I ? false : true;
  gba->clockTicks = codeTicksAccess16(gba, gba->armNextPC)+2;
}

// ASR Rd, Rs
static INSN_REGPARM void thumb41_0(GBASystem *gba, u32 opcode)
{
  int dest = opcode & 7;
  u32 value = gba->reg[(opcode >> 3)&7].B.B0;
  if(value) {
    if(value < 32) {
      ASR_RD_RS;
      gba->reg[dest].I = value;
    } else {
      if(gba->reg[dest].I & 0x80000000){
        gba->reg[dest].I = 0xFFFFFFFF;
        gba->C_FLAG = true;
      } else {
        gba->reg[dest].I = 0x00000000;
        gba->C_FLAG = false;
      }
    }
  }
  gba->N_FLAG = gba->reg[dest].I & 0x80000000 ? true : false;
  gba->Z_FLAG = gba->reg[dest].I ? false : true;
  gba->clockTicks = codeTicksAccess16(gba, gba->armNextPC)+2;
}

// ADC Rd, Rs
static INSN_REGPARM void thumb41_1(GBASystem *gba, u32 opcode)
{
  int dest = opcode & 0x07;
  u32 value = gba->reg[(opcode >> 3)&7].I;
  ADC_RD_RS;
}

// SBC Rd, Rs
static INSN_REGPARM void thumb41_2(GBASystem *gba, u32 opcode)
{
  int dest = opcode & 0x07;
  u32 value = gba->reg[(opcode >> 3)&7].I;
  SBC_RD_RS;
}

// ROR Rd, Rs
static INSN_REGPARM void thumb41_3(GBASystem *gba, u32 opcode)
{
  int dest = opcode & 7;
  u32 value = gba->reg[(opcode >> 3)&7].B.B0;

  if(value) {
    value = value & 0x1f;
    if(value == 0) {
      gba->C_FLAG = (gba->reg[dest].I & 0x80000000 ? true : false);
    } else {
      ROR_RD_RS;
      gba->reg[dest].I = value;
    }
  }
  gba->clockTicks = codeTicksAccess16(gba, gba->armNextPC)+2;
  gba->N_FLAG = gba->reg[dest].I & 0x80000000 ? true : false;
  gba->Z_FLAG = gba->reg[dest].I ? false : true;
}

// TST Rd, Rs
static INSN_REGPARM void thumb42_0(GBASystem *gba, u32 opcode)
{
  u32 value = gba->reg[opcode & 7].I & gba->reg[(opcode >> 3) & 7].I;
  gba->N_FLAG = value & 0x80000000 ? true : false;
  gba->Z_FLAG = value ? false : true;
}

// NEG Rd, Rs
static INSN_REGPARM void thumb42_1(GBASystem *gba, u32 opcode)
{
  int dest = opcode & 7;
  int source = (opcode >> 3) & 7;
  NEG_RD_RS;
}

// CMP Rd, Rs
static INSN_REGPARM void thumb42_2(GBASystem *gba, u32 opcode)
{
  int dest = opcode & 7;
  u32 value = gba->reg[(opcode >> 3)&7].I;
  CMP_RD_RS;
}

// CMN Rd, Rs
static INSN_REGPARM void thumb42_3(GBASystem *gba, u32 opcode)
{
  int dest = opcode & 7;
  u32 value = gba->reg[(opcode >> 3)&7].I;
  CMN_RD_RS;
}

// ORR Rd, Rs
static INSN_REGPARM void thumb43_0(GBASystem *gba, u32 opcode)
{
  int dest = opcode & 7;
  gba->reg[dest].I |= gba->reg[(opcode >> 3) & 7].I;
  gba->Z_FLAG = gba->reg[dest].I ? false : true;
  gba->N_FLAG = gba->reg[dest].I & 0x80000000 ? true : false;
}

// MUL Rd, Rs
static INSN_REGPARM void thumb43_1(GBASystem *gba, u32 opcode)
{
  gba->clockTicks = 1;
  int dest = opcode & 7;
  u32 rm = gba->reg[dest].I;
  gba->reg[dest].I = gba->reg[(opcode >> 3) & 7].I * rm;
  if (((s32)rm) < 0)
    rm = ~rm;
  if ((rm & 0xFFFFFF00) == 0)
    gba->clockTicks += 0;
  else if ((rm & 0xFFFF0000) == 0)
    gba->clockTicks += 1;
  else if ((rm & 0xFF000000) == 0)
    gba->clockTicks += 2;
  else
    gba->clockTicks += 3;
  gba->busPrefetchCount = (gba->busPrefetchCount<<gba->clockTicks) | (0xFF>>(8-gba->clockTicks));
  gba->clockTicks += codeTicksAccess16(gba, gba->armNextPC) + 1;
  gba->Z_FLAG = gba->reg[dest].I ? false : true;
  gba->N_FLAG = gba->reg[dest].I & 0x80000000 ? true : false;
}

// BIC Rd, Rs
static INSN_REGPARM void thumb43_2(GBASystem *gba, u32 opcode)
{
  int dest = opcode & 7;
  gba->reg[dest].I &= (~gba->reg[(opcode >> 3) & 7].I);
  gba->Z_FLAG = gba->reg[dest].I ? false : true;
  gba->N_FLAG = gba->reg[dest].I & 0x80000000 ? true : false;
}

// MVN Rd, Rs
static INSN_REGPARM void thumb43_3(GBASystem *gba, u32 opcode)
{
  int dest = opcode & 7;
  gba->reg[dest].I = ~gba->reg[(opcode >> 3) & 7].I;
  gba->Z_FLAG = gba->reg[dest].I ? false : true;
  gba->N_FLAG = gba->reg[dest].I & 0x80000000 ? true : false;
}

// High-register instructions and BX //////////////////////////////////////

// ADD Rd, Hs
static INSN_REGPARM void thumb44_1(GBASystem *gba, u32 opcode)
{
  gba->reg[opcode&7].I += gba->reg[((opcode>>3)&7)+8].I;
}

// ADD Hd, Rs
static INSN_REGPARM void thumb44_2(GBASystem *gba, u32 opcode)
{
  gba->reg[(opcode&7)+8].I += gba->reg[(opcode>>3)&7].I;
  if((opcode&7) == 7) {
    gba->reg[15].I &= 0xFFFFFFFE;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC)*2
        + codeTicksAccess16(gba, gba->armNextPC) + 3;
  }
}

// ADD Hd, Hs
static INSN_REGPARM void thumb44_3(GBASystem *gba, u32 opcode)
{
  gba->reg[(opcode&7)+8].I += gba->reg[((opcode>>3)&7)+8].I;
  if((opcode&7) == 7) {
    gba->reg[15].I &= 0xFFFFFFFE;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC)*2
        + codeTicksAccess16(gba, gba->armNextPC) + 3;
  }
}

// CMP Rd, Hs
static INSN_REGPARM void thumb45_1(GBASystem *gba, u32 opcode)
{
  int dest = opcode & 7;
  u32 value = gba->reg[((opcode>>3)&7)+8].I;
  CMP_RD_RS;
}

// CMP Hd, Rs
static INSN_REGPARM void thumb45_2(GBASystem *gba, u32 opcode)
{
  int dest = (opcode & 7) + 8;
  u32 value = gba->reg[(opcode>>3)&7].I;
  CMP_RD_RS;
}

// CMP Hd, Hs
static INSN_REGPARM void thumb45_3(GBASystem *gba, u32 opcode)
{
  int dest = (opcode & 7) + 8;
  u32 value = gba->reg[((opcode>>3)&7)+8].I;
  CMP_RD_RS;
}

// MOV Rd, Hs
static INSN_REGPARM void thumb46_1(GBASystem *gba, u32 opcode)
{
  gba->reg[opcode&7].I = gba->reg[((opcode>>3)&7)+8].I;
}

// MOV Hd, Rs
static INSN_REGPARM void thumb46_2(GBASystem *gba, u32 opcode)
{
  gba->reg[(opcode&7)+8].I = gba->reg[(opcode>>3)&7].I;
  if((opcode&7) == 7) {
    UPDATE_OLDREG;
    gba->reg[15].I &= 0xFFFFFFFE;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC)*2
        + codeTicksAccess16(gba, gba->armNextPC) + 3;
  }
}

// MOV Hd, Hs
static INSN_REGPARM void thumb46_3(GBASystem *gba, u32 opcode)
{
  gba->reg[(opcode&7)+8].I = gba->reg[((opcode>>3)&7)+8].I;
  if((opcode&7) == 7) {
    UPDATE_OLDREG;
    gba->reg[15].I &= 0xFFFFFFFE;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC)*2
        + codeTicksAccess16(gba, gba->armNextPC) + 3;
  }
}


// BX Rs
static INSN_REGPARM void thumb47(GBASystem *gba, u32 opcode)
{
  int base = (opcode >> 3) & 15;
  gba->busPrefetchCount=0;
  UPDATE_OLDREG;
  gba->reg[15].I = gba->reg[base].I;
  if(gba->reg[base].I & 1) {
    gba->armState = false;
    gba->reg[15].I &= 0xFFFFFFFE;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC)
        + codeTicksAccessSeq16(gba, gba->armNextPC) + codeTicksAccess16(gba, gba->armNextPC) + 3;
  } else {
    gba->armState = true;
    gba->reg[15].I &= 0xFFFFFFFC;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 4;
    ARM_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq32(gba, gba->armNextPC)
        + codeTicksAccessSeq32(gba, gba->armNextPC) + codeTicksAccess32(gba, gba->armNextPC) + 3;
  }
}

// Load/store instructions ////////////////////////////////////////////////

// LDR R0~R7,[PC, #Imm]
static INSN_REGPARM void thumb48(GBASystem *gba, u32 opcode)
{
  u8 regist = (opcode >> 8) & 7;
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = (gba->reg[15].I & 0xFFFFFFFC) + ((opcode & 0xFF) << 2);
  gba->reg[regist].I = CPUReadMemoryQuick(gba, address);
  gba->busPrefetchCount=0;
  gba->clockTicks = 3 + dataTicksAccess32(gba, address) + codeTicksAccess16(gba, gba->armNextPC);
}

// STR Rd, [Rs, Rn]
static INSN_REGPARM void thumb50(GBASystem *gba, u32 opcode)
{
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = gba->reg[(opcode>>3)&7].I + gba->reg[(opcode>>6)&7].I;
  CPUWriteMemory(gba, address, gba->reg[opcode & 7].I);
  gba->clockTicks = dataTicksAccess32(gba, address) + codeTicksAccess16(gba, gba->armNextPC) + 2;
}

// STRH Rd, [Rs, Rn]
static INSN_REGPARM void thumb52(GBASystem *gba, u32 opcode)
{
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = gba->reg[(opcode>>3)&7].I + gba->reg[(opcode>>6)&7].I;
  CPUWriteHalfWord(gba, address, gba->reg[opcode&7].W.W0);
  gba->clockTicks = dataTicksAccess16(gba, address) + codeTicksAccess16(gba, gba->armNextPC) + 2;
}

// STRB Rd, [Rs, Rn]
static INSN_REGPARM void thumb54(GBASystem *gba, u32 opcode)
{
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = gba->reg[(opcode>>3)&7].I + gba->reg[(opcode >>6)&7].I;
  CPUWriteByte(gba, address, gba->reg[opcode & 7].B.B0);
  gba->clockTicks = dataTicksAccess16(gba, address) + codeTicksAccess16(gba, gba->armNextPC) + 2;
}

// LDSB Rd, [Rs, Rn]
static INSN_REGPARM void thumb56(GBASystem *gba, u32 opcode)
{
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = gba->reg[(opcode>>3)&7].I + gba->reg[(opcode>>6)&7].I;
  gba->reg[opcode&7].I = (s8)CPUReadByte(gba, address);
  gba->clockTicks = 3 + dataTicksAccess16(gba, address) + codeTicksAccess16(gba, gba->armNextPC);
}

// LDR Rd, [Rs, Rn]
static INSN_REGPARM void thumb58(GBASystem *gba, u32 opcode)
{
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = gba->reg[(opcode>>3)&7].I + gba->reg[(opcode>>6)&7].I;
  gba->reg[opcode&7].I = CPUReadMemory(gba, address);
  gba->clockTicks = 3 + dataTicksAccess32(gba, address) + codeTicksAccess16(gba, gba->armNextPC);
}

// LDRH Rd, [Rs, Rn]
static INSN_REGPARM void thumb5A(GBASystem *gba, u32 opcode)
{
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = gba->reg[(opcode>>3)&7].I + gba->reg[(opcode>>6)&7].I;
  gba->reg[opcode&7].I = CPUReadHalfWord(gba, address);
  gba->clockTicks = 3 + dataTicksAccess32(gba, address) + codeTicksAccess16(gba, gba->armNextPC);
}

// LDRB Rd, [Rs, Rn]
static INSN_REGPARM void thumb5C(GBASystem *gba, u32 opcode)
{
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = gba->reg[(opcode>>3)&7].I + gba->reg[(opcode>>6)&7].I;
  gba->reg[opcode&7].I = CPUReadByte(gba, address);
  gba->clockTicks = 3 + dataTicksAccess16(gba, address) + codeTicksAccess16(gba, gba->armNextPC);
}

// LDSH Rd, [Rs, Rn]
static INSN_REGPARM void thumb5E(GBASystem *gba, u32 opcode)
{
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = gba->reg[(opcode>>3)&7].I + gba->reg[(opcode>>6)&7].I;
  gba->reg[opcode&7].I = (s16)CPUReadHalfWordSigned(gba, address);
  gba->clockTicks = 3 + dataTicksAccess16(gba, address) + codeTicksAccess16(gba, gba->armNextPC);
}

// STR Rd, [Rs, #Imm]
static INSN_REGPARM void thumb60(GBASystem *gba, u32 opcode)
{
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = gba->reg[(opcode>>3)&7].I + (((opcode>>6)&31)<<2);
  CPUWriteMemory(gba, address, gba->reg[opcode&7].I);
  gba->clockTicks = dataTicksAccess32(gba, address) + codeTicksAccess16(gba, gba->armNextPC) + 2;
}

// LDR Rd, [Rs, #Imm]
static INSN_REGPARM void thumb68(GBASystem *gba, u32 opcode)
{
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = gba->reg[(opcode>>3)&7].I + (((opcode>>6)&31)<<2);
  gba->reg[opcode&7].I = CPUReadMemory(gba, address);
  gba->clockTicks = 3 + dataTicksAccess32(gba, address) + codeTicksAccess16(gba, gba->armNextPC);
}

// STRB Rd, [Rs, #Imm]
static INSN_REGPARM void thumb70(GBASystem *gba, u32 opcode)
{
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = gba->reg[(opcode>>3)&7].I + (((opcode>>6)&31));
  CPUWriteByte(gba, address, gba->reg[opcode&7].B.B0);
  gba->clockTicks = dataTicksAccess16(gba, address) + codeTicksAccess16(gba, gba->armNextPC) + 2;
}

// LDRB Rd, [Rs, #Imm]
static INSN_REGPARM void thumb78(GBASystem *gba, u32 opcode)
{
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = gba->reg[(opcode>>3)&7].I + (((opcode>>6)&31));
  gba->reg[opcode&7].I = CPUReadByte(gba, address);
  gba->clockTicks = 3 + dataTicksAccess16(gba, address) + codeTicksAccess16(gba, gba->armNextPC);
}

// STRH Rd, [Rs, #Imm]
static INSN_REGPARM void thumb80(GBASystem *gba, u32 opcode)
{
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = gba->reg[(opcode>>3)&7].I + (((opcode>>6)&31)<<1);
  CPUWriteHalfWord(gba, address, gba->reg[opcode&7].W.W0);
  gba->clockTicks = dataTicksAccess16(gba, address) + codeTicksAccess16(gba, gba->armNextPC) + 2;
}

// LDRH Rd, [Rs, #Imm]
static INSN_REGPARM void thumb88(GBASystem *gba, u32 opcode)
{
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = gba->reg[(opcode>>3)&7].I + (((opcode>>6)&31)<<1);
  gba->reg[opcode&7].I = CPUReadHalfWord(gba, address);
  gba->clockTicks = 3 + dataTicksAccess16(gba, address) + codeTicksAccess16(gba, gba->armNextPC);
}

// STR R0~R7, [SP, #Imm]
static INSN_REGPARM void thumb90(GBASystem *gba, u32 opcode)
{
  u8 regist = (opcode >> 8) & 7;
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = gba->reg[13].I + ((opcode&255)<<2);
  CPUWriteMemory(gba, address, gba->reg[regist].I);
  gba->clockTicks = dataTicksAccess32(gba, address) + codeTicksAccess16(gba, gba->armNextPC) + 2;
}

// LDR R0~R7, [SP, #Imm]
static INSN_REGPARM void thumb98(GBASystem *gba, u32 opcode)
{
  u8 regist = (opcode >> 8) & 7;
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = gba->reg[13].I + ((opcode&255)<<2);
  gba->reg[regist].I = CPUReadMemoryQuick(gba, address);
  gba->clockTicks = 3 + dataTicksAccess32(gba, address) + codeTicksAccess16(gba, gba->armNextPC);
}

// PC/stack-related ///////////////////////////////////////////////////////

// ADD R0~R7, PC, Imm
static INSN_REGPARM void thumbA0(GBASystem *gba, u32 opcode)
{
  u8 regist = (opcode >> 8) & 7;
  gba->reg[regist].I = (gba->reg[15].I & 0xFFFFFFFC) + ((opcode&255)<<2);
}

// ADD R0~R7, SP, Imm
static INSN_REGPARM void thumbA8(GBASystem *gba, u32 opcode)
{
  u8 regist = (opcode >> 8) & 7;
  gba->reg[regist].I = gba->reg[13].I + ((opcode&255)<<2);
}

// ADD SP, Imm
static INSN_REGPARM void thumbB0(GBASystem *gba, u32 opcode)
{
  int offset = (opcode & 127) << 2;
  if(opcode & 0x80)
    offset = -offset;
  gba->reg[13].I += offset;
}

// Push and pop ///////////////////////////////////////////////////////////

#define PUSH_REG(val, r)                                    \
  if (opcode & (val)) {                                     \
    CPUWriteMemory(gba, address, gba->reg[(r)].I);                    \
    if (!count) {                                           \
        gba->clockTicks += 1 + dataTicksAccess32(gba, address);       \
    } else {                                                \
        gba->clockTicks += 1 + dataTicksAccessSeq32(gba, address);    \
    }                                                       \
    count++;                                                \
    address += 4;                                           \
  }

#define POP_REG(val, r)                                     \
  if (opcode & (val)) {                                     \
    gba->reg[(r)].I = CPUReadMemory(gba, address);                    \
    if (!count) {                                           \
        gba->clockTicks += 1 + dataTicksAccess32(gba, address);       \
    } else {                                                \
        gba->clockTicks += 1 + dataTicksAccessSeq32(gba, address);    \
    }                                                       \
    count++;                                                \
    address += 4;                                           \
  }

// PUSH {Rlist}
static INSN_REGPARM void thumbB4(GBASystem *gba, u32 opcode)
{
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  int count = 0;
  u32 temp = gba->reg[13].I - 4 * gba->cpuBitsSet[opcode & 0xff];
  u32 address = temp & 0xFFFFFFFC;
  PUSH_REG(1, 0);
  PUSH_REG(2, 1);
  PUSH_REG(4, 2);
  PUSH_REG(8, 3);
  PUSH_REG(16, 4);
  PUSH_REG(32, 5);
  PUSH_REG(64, 6);
  PUSH_REG(128, 7);
  gba->clockTicks += 1 + codeTicksAccess16(gba, gba->armNextPC);
  gba->reg[13].I = temp;
}

// PUSH {Rlist, LR}
static INSN_REGPARM void thumbB5(GBASystem *gba, u32 opcode)
{
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  int count = 0;
  u32 temp = gba->reg[13].I - 4 - 4 * gba->cpuBitsSet[opcode & 0xff];
  u32 address = temp & 0xFFFFFFFC;
  PUSH_REG(1, 0);
  PUSH_REG(2, 1);
  PUSH_REG(4, 2);
  PUSH_REG(8, 3);
  PUSH_REG(16, 4);
  PUSH_REG(32, 5);
  PUSH_REG(64, 6);
  PUSH_REG(128, 7);
  PUSH_REG(256, 14);
  gba->clockTicks += 1 + codeTicksAccess16(gba, gba->armNextPC);
  gba->reg[13].I = temp;
}

// POP {Rlist}
static INSN_REGPARM void thumbBC(GBASystem *gba, u32 opcode)
{
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  int count = 0;
  u32 address = gba->reg[13].I & 0xFFFFFFFC;
  u32 temp = gba->reg[13].I + 4*gba->cpuBitsSet[opcode & 0xFF];
  POP_REG(1, 0);
  POP_REG(2, 1);
  POP_REG(4, 2);
  POP_REG(8, 3);
  POP_REG(16, 4);
  POP_REG(32, 5);
  POP_REG(64, 6);
  POP_REG(128, 7);
  gba->reg[13].I = temp;
  gba->clockTicks = 2 + codeTicksAccess16(gba, gba->armNextPC);
}

// POP {Rlist, PC}
static INSN_REGPARM void thumbBD(GBASystem *gba, u32 opcode)
{
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  int count = 0;
  u32 address = gba->reg[13].I & 0xFFFFFFFC;
  u32 temp = gba->reg[13].I + 4 + 4*gba->cpuBitsSet[opcode & 0xFF];
  POP_REG(1, 0);
  POP_REG(2, 1);
  POP_REG(4, 2);
  POP_REG(8, 3);
  POP_REG(16, 4);
  POP_REG(32, 5);
  POP_REG(64, 6);
  POP_REG(128, 7);
  gba->reg[15].I = (CPUReadMemory(gba, address) & 0xFFFFFFFE);
  if (!count) {
    gba->clockTicks += 1 + dataTicksAccess32(gba, address);
  } else {
    gba->clockTicks += 1 + dataTicksAccessSeq32(gba, address);
  }
  count++;
  gba->armNextPC = gba->reg[15].I;
  gba->reg[15].I += 2;
  gba->reg[13].I = temp;
  THUMB_PREFETCH;
  gba->busPrefetchCount = 0;
  gba->clockTicks += 3 + codeTicksAccess16(gba, gba->armNextPC) + codeTicksAccess16(gba, gba->armNextPC);
}

// Load/store multiple ////////////////////////////////////////////////////

#define THUMB_STM_REG(val,r,b)                              \
  if(opcode & (val)) {                                      \
    CPUWriteMemory(gba, address, gba->reg[(r)].I);                    \
    gba->reg[(b)].I = temp;                                      \
    if (!count) {                                           \
        gba->clockTicks += 1 + dataTicksAccess32(gba, address);       \
    } else {                                                \
        gba->clockTicks += 1 + dataTicksAccessSeq32(gba, address);    \
    }                                                       \
    count++;                                                \
    address += 4;                                           \
  }

#define THUMB_LDM_REG(val,r)                                \
  if(opcode & (val)) {                                      \
    gba->reg[(r)].I = CPUReadMemory(gba, address);                    \
    if (!count) {                                           \
        gba->clockTicks += 1 + dataTicksAccess32(gba, address);       \
    } else {                                                \
        gba->clockTicks += 1 + dataTicksAccessSeq32(gba, address);    \
    }                                                       \
    count++;                                                \
    address += 4;                                           \
  }

// STM R0~7!, {Rlist}
static INSN_REGPARM void thumbC0(GBASystem *gba, u32 opcode)
{
  u8 regist = (opcode >> 8) & 7;
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = gba->reg[regist].I & 0xFFFFFFFC;
  u32 temp = gba->reg[regist].I + 4*gba->cpuBitsSet[opcode & 0xff];
  int count = 0;
  // store
  THUMB_STM_REG(1, 0, regist);
  THUMB_STM_REG(2, 1, regist);
  THUMB_STM_REG(4, 2, regist);
  THUMB_STM_REG(8, 3, regist);
  THUMB_STM_REG(16, 4, regist);
  THUMB_STM_REG(32, 5, regist);
  THUMB_STM_REG(64, 6, regist);
  THUMB_STM_REG(128, 7, regist);
  gba->clockTicks = 1 + codeTicksAccess16(gba, gba->armNextPC);
}

// LDM R0~R7!, {Rlist}
static INSN_REGPARM void thumbC8(GBASystem *gba, u32 opcode)
{
  u8 regist = (opcode >> 8) & 7;
  if (gba->busPrefetchCount == 0)
    gba->busPrefetch = gba->busPrefetchEnable;
  u32 address = gba->reg[regist].I & 0xFFFFFFFC;
  u32 temp = gba->reg[regist].I + 4*gba->cpuBitsSet[opcode & 0xFF];
  int count = 0;
  // load
  THUMB_LDM_REG(1, 0);
  THUMB_LDM_REG(2, 1);
  THUMB_LDM_REG(4, 2);
  THUMB_LDM_REG(8, 3);
  THUMB_LDM_REG(16, 4);
  THUMB_LDM_REG(32, 5);
  THUMB_LDM_REG(64, 6);
  THUMB_LDM_REG(128, 7);
  gba->clockTicks = 2 + codeTicksAccess16(gba, gba->armNextPC);
  if(!(opcode & (1<<regist)))
    gba->reg[regist].I = temp;
}

// Conditional branches ///////////////////////////////////////////////////

// BEQ offset
static INSN_REGPARM void thumbD0(GBASystem *gba, u32 opcode)
{
  UPDATE_OLDREG;
  if(gba->Z_FLAG) {
    gba->reg[15].I += ((s8)(opcode & 0xFF)) << 1;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC) + codeTicksAccessSeq16(gba, gba->armNextPC) +
        codeTicksAccess16(gba, gba->armNextPC)+3;
    gba->busPrefetchCount=0;
  }
}

// BNE offset
static INSN_REGPARM void thumbD1(GBASystem *gba, u32 opcode)
{
  UPDATE_OLDREG;
  if(!gba->Z_FLAG) {
    gba->reg[15].I += ((s8)(opcode & 0xFF)) << 1;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC) + codeTicksAccessSeq16(gba, gba->armNextPC) +
        codeTicksAccess16(gba, gba->armNextPC)+3;
    gba->busPrefetchCount=0;
  }
}

// BCS offset
static INSN_REGPARM void thumbD2(GBASystem *gba, u32 opcode)
{
  UPDATE_OLDREG;
  if(gba->C_FLAG) {
    gba->reg[15].I += ((s8)(opcode & 0xFF)) << 1;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC) + codeTicksAccessSeq16(gba, gba->armNextPC) +
        codeTicksAccess16(gba, gba->armNextPC)+3;
    gba->busPrefetchCount=0;
  }
}

// BCC offset
static INSN_REGPARM void thumbD3(GBASystem *gba, u32 opcode)
{
  UPDATE_OLDREG;
  if(!gba->C_FLAG) {
    gba->reg[15].I += ((s8)(opcode & 0xFF)) << 1;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC) + codeTicksAccessSeq16(gba, gba->armNextPC) +
        codeTicksAccess16(gba, gba->armNextPC)+3;
    gba->busPrefetchCount=0;
  }
}

// BMI offset
static INSN_REGPARM void thumbD4(GBASystem *gba, u32 opcode)
{
  UPDATE_OLDREG;
  if(gba->N_FLAG) {
    gba->reg[15].I += ((s8)(opcode & 0xFF)) << 1;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC) + codeTicksAccessSeq16(gba, gba->armNextPC) +
        codeTicksAccess16(gba, gba->armNextPC)+3;
    gba->busPrefetchCount=0;
  }
}

// BPL offset
static INSN_REGPARM void thumbD5(GBASystem *gba, u32 opcode)
{
  UPDATE_OLDREG;
  if(!gba->N_FLAG) {
    gba->reg[15].I += ((s8)(opcode & 0xFF)) << 1;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC) + codeTicksAccessSeq16(gba, gba->armNextPC) +
        codeTicksAccess16(gba, gba->armNextPC)+3;
    gba->busPrefetchCount=0;
  }
}

// BVS offset
static INSN_REGPARM void thumbD6(GBASystem *gba, u32 opcode)
{
  UPDATE_OLDREG;
  if(gba->V_FLAG) {
    gba->reg[15].I += ((s8)(opcode & 0xFF)) << 1;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC) + codeTicksAccessSeq16(gba, gba->armNextPC) +
        codeTicksAccess16(gba, gba->armNextPC)+3;
    gba->busPrefetchCount=0;
  }
}

// BVC offset
static INSN_REGPARM void thumbD7(GBASystem *gba, u32 opcode)
{
  UPDATE_OLDREG;
  if(!gba->V_FLAG) {
    gba->reg[15].I += ((s8)(opcode & 0xFF)) << 1;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC) + codeTicksAccessSeq16(gba, gba->armNextPC) +
        codeTicksAccess16(gba, gba->armNextPC)+3;
    gba->busPrefetchCount=0;
  }
}

// BHI offset
static INSN_REGPARM void thumbD8(GBASystem *gba, u32 opcode)
{
  UPDATE_OLDREG;
  if(gba->C_FLAG && !gba->Z_FLAG) {
    gba->reg[15].I += ((s8)(opcode & 0xFF)) << 1;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC) + codeTicksAccessSeq16(gba, gba->armNextPC) +
        codeTicksAccess16(gba, gba->armNextPC)+3;
    gba->busPrefetchCount=0;
  }
}

// BLS offset
static INSN_REGPARM void thumbD9(GBASystem *gba, u32 opcode)
{
  UPDATE_OLDREG;
  if(!gba->C_FLAG || gba->Z_FLAG) {
    gba->reg[15].I += ((s8)(opcode & 0xFF)) << 1;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC) + codeTicksAccessSeq16(gba, gba->armNextPC) +
        codeTicksAccess16(gba, gba->armNextPC)+3;
    gba->busPrefetchCount=0;
  }
}

// BGE offset
static INSN_REGPARM void thumbDA(GBASystem *gba, u32 opcode)
{
  UPDATE_OLDREG;
  if(gba->N_FLAG == gba->V_FLAG) {
    gba->reg[15].I += ((s8)(opcode & 0xFF)) << 1;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC) + codeTicksAccessSeq16(gba, gba->armNextPC) +
        codeTicksAccess16(gba, gba->armNextPC)+3;
    gba->busPrefetchCount=0;
  }
}

// BLT offset
static INSN_REGPARM void thumbDB(GBASystem *gba, u32 opcode)
{
  UPDATE_OLDREG;
  if(gba->N_FLAG != gba->V_FLAG) {
    gba->reg[15].I += ((s8)(opcode & 0xFF)) << 1;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC) + codeTicksAccessSeq16(gba, gba->armNextPC) +
        codeTicksAccess16(gba, gba->armNextPC)+3;
    gba->busPrefetchCount=0;
  }
}

// BGT offset
static INSN_REGPARM void thumbDC(GBASystem *gba, u32 opcode)
{
  UPDATE_OLDREG;
  if(!gba->Z_FLAG && (gba->N_FLAG == gba->V_FLAG)) {
    gba->reg[15].I += ((s8)(opcode & 0xFF)) << 1;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC) + codeTicksAccessSeq16(gba, gba->armNextPC) +
        codeTicksAccess16(gba, gba->armNextPC)+3;
    gba->busPrefetchCount=0;
  }
}

// BLE offset
static INSN_REGPARM void thumbDD(GBASystem *gba, u32 opcode)
{
  UPDATE_OLDREG;
  if(gba->Z_FLAG || (gba->N_FLAG != gba->V_FLAG)) {
    gba->reg[15].I += ((s8)(opcode & 0xFF)) << 1;
    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH;
    gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC) + codeTicksAccessSeq16(gba, gba->armNextPC) +
        codeTicksAccess16(gba, gba->armNextPC)+3;
    gba->busPrefetchCount=0;
  }
}

// SWI, B, BL /////////////////////////////////////////////////////////////

// SWI #comment
static INSN_REGPARM void thumbDF(GBASystem *gba, u32 opcode)
{
  u32 address = 0;
  gba->clockTicks = codeTicksAccessSeq16(gba, address) + codeTicksAccessSeq16(gba, address) +
      codeTicksAccess16(gba, address)+3;
  gba->busPrefetchCount=0;
  CPUSoftwareInterrupt(gba, opcode & 0xFF);
}

// B offset
static INSN_REGPARM void thumbE0(GBASystem *gba, u32 opcode)
{
  int offset = (opcode & 0x3FF) << 1;
  if(opcode & 0x0400)
    offset |= 0xFFFFF800;
  gba->reg[15].I += offset;
  gba->armNextPC = gba->reg[15].I;
  gba->reg[15].I += 2;
  THUMB_PREFETCH;
  gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC) + codeTicksAccessSeq16(gba, gba->armNextPC) +
      codeTicksAccess16(gba, gba->armNextPC) + 3;
  gba->busPrefetchCount=0;
}

// BLL #offset (forward)
static INSN_REGPARM void thumbF0(GBASystem *gba, u32 opcode)
{
  int offset = (opcode & 0x7FF);
  gba->reg[14].I = gba->reg[15].I + (offset << 12);
  gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC) + 1;
}

// BLL #offset (backward)
static INSN_REGPARM void thumbF4(GBASystem *gba, u32 opcode)
{
  int offset = (opcode & 0x7FF);
  gba->reg[14].I = gba->reg[15].I + ((offset << 12) | 0xFF800000);
  gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC) + 1;
}

// BLH #offset
static INSN_REGPARM void thumbF8(GBASystem *gba, u32 opcode)
{
  int offset = (opcode & 0x7FF);
  u32 temp = gba->reg[15].I-2;
  gba->reg[15].I = (gba->reg[14].I + (offset<<1))&0xFFFFFFFE;
  gba->armNextPC = gba->reg[15].I;
  gba->reg[15].I += 2;
  gba->reg[14].I = temp|1;
  THUMB_PREFETCH;
  gba->clockTicks = codeTicksAccessSeq16(gba, gba->armNextPC) +
      codeTicksAccess16(gba, gba->armNextPC) + codeTicksAccessSeq16(gba, gba->armNextPC) + 3;
  gba->busPrefetchCount = 0;
}

// Instruction table //////////////////////////////////////////////////////

typedef INSN_REGPARM void (*insnfunc_t)(GBASystem *gba, u32 opcode);
#define thumbUI thumbUnknownInsn
#define thumbBP thumbUnknownInsn
static insnfunc_t thumbInsnTable[1024] = {
  thumb00_00,thumb00_01,thumb00_02,thumb00_03,thumb00_04,thumb00_05,thumb00_06,thumb00_07,  // 00
  thumb00_08,thumb00_09,thumb00_0A,thumb00_0B,thumb00_0C,thumb00_0D,thumb00_0E,thumb00_0F,
  thumb00_10,thumb00_11,thumb00_12,thumb00_13,thumb00_14,thumb00_15,thumb00_16,thumb00_17,
  thumb00_18,thumb00_19,thumb00_1A,thumb00_1B,thumb00_1C,thumb00_1D,thumb00_1E,thumb00_1F,
  thumb08_00,thumb08_01,thumb08_02,thumb08_03,thumb08_04,thumb08_05,thumb08_06,thumb08_07,  // 08
  thumb08_08,thumb08_09,thumb08_0A,thumb08_0B,thumb08_0C,thumb08_0D,thumb08_0E,thumb08_0F,
  thumb08_10,thumb08_11,thumb08_12,thumb08_13,thumb08_14,thumb08_15,thumb08_16,thumb08_17,
  thumb08_18,thumb08_19,thumb08_1A,thumb08_1B,thumb08_1C,thumb08_1D,thumb08_1E,thumb08_1F,
  thumb10_00,thumb10_01,thumb10_02,thumb10_03,thumb10_04,thumb10_05,thumb10_06,thumb10_07,  // 10
  thumb10_08,thumb10_09,thumb10_0A,thumb10_0B,thumb10_0C,thumb10_0D,thumb10_0E,thumb10_0F,
  thumb10_10,thumb10_11,thumb10_12,thumb10_13,thumb10_14,thumb10_15,thumb10_16,thumb10_17,
  thumb10_18,thumb10_19,thumb10_1A,thumb10_1B,thumb10_1C,thumb10_1D,thumb10_1E,thumb10_1F,
  thumb18_0,thumb18_1,thumb18_2,thumb18_3,thumb18_4,thumb18_5,thumb18_6,thumb18_7,          // 18
  thumb1A_0,thumb1A_1,thumb1A_2,thumb1A_3,thumb1A_4,thumb1A_5,thumb1A_6,thumb1A_7,
  thumb1C_0,thumb1C_1,thumb1C_2,thumb1C_3,thumb1C_4,thumb1C_5,thumb1C_6,thumb1C_7,
  thumb1E_0,thumb1E_1,thumb1E_2,thumb1E_3,thumb1E_4,thumb1E_5,thumb1E_6,thumb1E_7,
  thumb20,thumb20,thumb20,thumb20,thumb21,thumb21,thumb21,thumb21,  // 20
  thumb22,thumb22,thumb22,thumb22,thumb23,thumb23,thumb23,thumb23,
  thumb24,thumb24,thumb24,thumb24,thumb25,thumb25,thumb25,thumb25,
  thumb26,thumb26,thumb26,thumb26,thumb27,thumb27,thumb27,thumb27,
  thumb28,thumb28,thumb28,thumb28,thumb29,thumb29,thumb29,thumb29,  // 28
  thumb2A,thumb2A,thumb2A,thumb2A,thumb2B,thumb2B,thumb2B,thumb2B,
  thumb2C,thumb2C,thumb2C,thumb2C,thumb2D,thumb2D,thumb2D,thumb2D,
  thumb2E,thumb2E,thumb2E,thumb2E,thumb2F,thumb2F,thumb2F,thumb2F,
  thumb30,thumb30,thumb30,thumb30,thumb31,thumb31,thumb31,thumb31,  // 30
  thumb32,thumb32,thumb32,thumb32,thumb33,thumb33,thumb33,thumb33,
  thumb34,thumb34,thumb34,thumb34,thumb35,thumb35,thumb35,thumb35,
  thumb36,thumb36,thumb36,thumb36,thumb37,thumb37,thumb37,thumb37,
  thumb38,thumb38,thumb38,thumb38,thumb39,thumb39,thumb39,thumb39,  // 38
  thumb3A,thumb3A,thumb3A,thumb3A,thumb3B,thumb3B,thumb3B,thumb3B,
  thumb3C,thumb3C,thumb3C,thumb3C,thumb3D,thumb3D,thumb3D,thumb3D,
  thumb3E,thumb3E,thumb3E,thumb3E,thumb3F,thumb3F,thumb3F,thumb3F,
  thumb40_0,thumb40_1,thumb40_2,thumb40_3,thumb41_0,thumb41_1,thumb41_2,thumb41_3,  // 40
  thumb42_0,thumb42_1,thumb42_2,thumb42_3,thumb43_0,thumb43_1,thumb43_2,thumb43_3,
  thumbUI,thumb44_1,thumb44_2,thumb44_3,thumbUI,thumb45_1,thumb45_2,thumb45_3,
  thumbUI,thumb46_1,thumb46_2,thumb46_3,thumb47,thumb47,thumbUI,thumbUI,
  thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,  // 48
  thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,
  thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,
  thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,
  thumb50,thumb50,thumb50,thumb50,thumb50,thumb50,thumb50,thumb50,  // 50
  thumb52,thumb52,thumb52,thumb52,thumb52,thumb52,thumb52,thumb52,
  thumb54,thumb54,thumb54,thumb54,thumb54,thumb54,thumb54,thumb54,
  thumb56,thumb56,thumb56,thumb56,thumb56,thumb56,thumb56,thumb56,
  thumb58,thumb58,thumb58,thumb58,thumb58,thumb58,thumb58,thumb58,  // 58
  thumb5A,thumb5A,thumb5A,thumb5A,thumb5A,thumb5A,thumb5A,thumb5A,
  thumb5C,thumb5C,thumb5C,thumb5C,thumb5C,thumb5C,thumb5C,thumb5C,
  thumb5E,thumb5E,thumb5E,thumb5E,thumb5E,thumb5E,thumb5E,thumb5E,
  thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,  // 60
  thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,
  thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,
  thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,
  thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,  // 68
  thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,
  thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,
  thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,
  thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,  // 70
  thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,
  thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,
  thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,
  thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,  // 78
  thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,
  thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,
  thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,
  thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,  // 80
  thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,
  thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,
  thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,
  thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,  // 88
  thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,
  thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,
  thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,
  thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,  // 90
  thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,
  thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,
  thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,
  thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,  // 98
  thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,
  thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,
  thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,
  thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,  // A0
  thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,
  thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,
  thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,
  thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,  // A8
  thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,
  thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,
  thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,
  thumbB0,thumbB0,thumbB0,thumbB0,thumbUI,thumbUI,thumbUI,thumbUI,  // B0
  thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,
  thumbB4,thumbB4,thumbB4,thumbB4,thumbB5,thumbB5,thumbB5,thumbB5,
  thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,
  thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,  // B8
  thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,
  thumbBC,thumbBC,thumbBC,thumbBC,thumbBD,thumbBD,thumbBD,thumbBD,
  thumbBP,thumbBP,thumbBP,thumbBP,thumbUI,thumbUI,thumbUI,thumbUI,
  thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,  // C0
  thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,
  thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,
  thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,
  thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,  // C8
  thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,
  thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,
  thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,
  thumbD0,thumbD0,thumbD0,thumbD0,thumbD1,thumbD1,thumbD1,thumbD1,  // D0
  thumbD2,thumbD2,thumbD2,thumbD2,thumbD3,thumbD3,thumbD3,thumbD3,
  thumbD4,thumbD4,thumbD4,thumbD4,thumbD5,thumbD5,thumbD5,thumbD5,
  thumbD6,thumbD6,thumbD6,thumbD6,thumbD7,thumbD7,thumbD7,thumbD7,
  thumbD8,thumbD8,thumbD8,thumbD8,thumbD9,thumbD9,thumbD9,thumbD9,  // D8
  thumbDA,thumbDA,thumbDA,thumbDA,thumbDB,thumbDB,thumbDB,thumbDB,
  thumbDC,thumbDC,thumbDC,thumbDC,thumbDD,thumbDD,thumbDD,thumbDD,
  thumbUI,thumbUI,thumbUI,thumbUI,thumbDF,thumbDF,thumbDF,thumbDF,
  thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,  // E0
  thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,
  thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,
  thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,
  thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,  // E8
  thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,
  thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,
  thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,
  thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,  // F0
  thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,
  thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,
  thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,
  thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,  // F8
  thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,
  thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,
  thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,
};

// Wrapper routine (execution loop) ///////////////////////////////////////

int thumbExecute(GBASystem *gba)
{
  do {
    //if ((armNextPC & 0x0803FFFF) == 0x08020000)
    //    gba->busPrefetchCount=0x100;

    u32 opcode = gba->cpuPrefetch[0];
    gba->cpuPrefetch[0] = gba->cpuPrefetch[1];

    gba->busPrefetch = false;
    if (gba->busPrefetchCount & 0xFFFFFF00)
      gba->busPrefetchCount = 0x100 | (gba->busPrefetchCount & 0xFF);
    gba->clockTicks = 0;
    u32 oldArmNextPC = gba->armNextPC;

    gba->armNextPC = gba->reg[15].I;
    gba->reg[15].I += 2;
    THUMB_PREFETCH_NEXT;

    (*thumbInsnTable[opcode>>6])(gba, opcode);

    if (gba->clockTicks < 0)
      return 0;
    if (gba->clockTicks==0)
      gba->clockTicks = codeTicksAccessSeq16(gba, oldArmNextPC) + 1;
    gba->cpuTotalTicks += gba->clockTicks;

  } while (gba->cpuTotalTicks < gba->cpuNextEvent && !gba->armState && !gba->holdState && !gba->SWITicks);
  return 1;
}
