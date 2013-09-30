/////////////////////////////////////////////////////////////////////////////
//
// r3000dis - R3000 disassembler
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EMU_COMPILE
#error "Hi I forgot to set EMU_COMPILE"
#endif

#include "r3000dis.h"

/////////////////////////////////////////////////////////////////////////////

#define INS_S (uint32)((ins>>21)&0x1F)
#define INS_T (uint32)((ins>>16)&0x1F)
#define INS_D (uint32)((ins>>11)&0x1F)
#define INS_H (uint32)((ins>>6)&0x1F)
#define INS_I (uint32)(ins&0xFFFF)

#define SIGNED16(x) ((sint32)(((sint16)(x))))

#define REL_I (pc+4+(((sint32)(SIGNED16(INS_I)))<<2))
#define ABS_I ((pc&0xF0000000)|((ins<<2)&0x0FFFFFFC))

#define RICH_CODE_REGISTER ('R'-64)
#define RICH_CODE_ADDRESS  ('A'-64)

/////////////////////////////////////////////////////////////////////////////
//
// Returns negative on unknown instuction
// dest must have 256 bytes available
//
sint32 EMU_CALL r3000dis(char *dest, uint32 rich, uint32 pc, uint32 ins) {
  sint32 delayslot = 0;
  static const char *regname[32] = {
    "$0" ,"$at","$v0","$v1","$a0","$a1","$a2","$a3",
    "$t0","$t1","$t2","$t3","$t4","$t5","$t6","$t7",
    "$s0","$s1","$s2","$s3","$s4","$s5","$s6","$s7",
    "$t8","$t9","$k0","$k1","$gp","$sp","$fp","$ra"
  };
  static const char *C0regname[32] = {
    "C0_0","C0_1","C0_2","C0_3","C0_4","C0_5","C0_6","C0_7",
    "C0_8","C0_9","C0_10","C0_11","C0_status","C0_cause","C0_epc","C0_prid",
    "C0_16","C0_17","C0_18","C0_19","C0_20","C0_21","C0_22","C0_23",
    "C0_24","C0_25","C0_26","C0_27","C0_28","C0_29","C0_30","C0_31"
  };
  const char *fmt;
  int j, col = 0;
  if(ins < 0x04000000) {
    switch(ins & 0x3F) {
    case 0x00:
      if(!ins)           fmt = "nop"; goto ok;
      if(INS_D==INS_T)   fmt = "sll D,H"; goto ok;
      if(!INS_H)         fmt = "move D,T"; goto ok;
                         fmt = "sll D,T,H"; goto ok;
    case 0x02:
      if(INS_D==INS_T)   fmt = "srl D,H"; goto ok;
      if(!INS_H)         fmt = "move D,T"; goto ok;
                         fmt = "srl D,T,H"; goto ok;
    case 0x03:
      if(INS_D==INS_T)   fmt = "sra D,H"; goto ok;
      if(!INS_H)         fmt = "move D,T"; goto ok;
                         fmt = "sra D,T,H"; goto ok;
    case 0x04:
      if(INS_D==INS_T)   fmt = "sllv D,S"; goto ok;
      if(!INS_S)         fmt = "move D,T"; goto ok;
                         fmt = "sllv D,T,S"; goto ok;
    case 0x06:
      if(INS_D==INS_T)   fmt = "srlv D,S"; goto ok;
      if(!INS_S)         fmt = "move D,T"; goto ok;
                         fmt = "srlv D,T,S"; goto ok;
    case 0x07:
      if(INS_D==INS_T)   fmt = "srav D,S"; goto ok;
      if(!INS_S)         fmt = "move D,T"; goto ok;
                         fmt = "srav D,T,S"; goto ok;
    case 0x08:           fmt = "jr S"; goto okdelayslot;
    case 0x09:
      if(INS_D==31)      fmt = "jalr S"; goto okdelayslot;
                         fmt = "jalr D,S"; goto okdelayslot;
    case 0x0C:           fmt = "syscall"; goto ok;
    case 0x10:           fmt = "mfhi D"; goto ok;
    case 0x11:           fmt = "mthi S"; goto ok;
    case 0x12:           fmt = "mflo D"; goto ok;
    case 0x13:           fmt = "mtlo S"; goto ok;
    case 0x18:           fmt = "mult S,T"; goto ok;
    case 0x19:           fmt = "multu S,T"; goto ok;
    case 0x1A:           fmt = "div S,T"; goto ok;
    case 0x1B:           fmt = "divu S,T"; goto ok;
    case 0x20:
      if(INS_D==INS_S)   fmt = "add D,T"; goto ok;
      if(!INS_T)         fmt = "move D,S"; goto ok;
      if(!INS_S)         fmt = "move D,T"; goto ok;
                         fmt = "add D,S,T"; goto ok;
    case 0x21:
      if(INS_D==INS_S)   fmt = "addu D,T"; goto ok;
      if(!INS_T)         fmt = "move D,S"; goto ok;
      if(!INS_S)         fmt = "move D,T"; goto ok;
                         fmt = "addu D,S,T"; goto ok;
    case 0x22:
      if(INS_D==INS_S)   fmt = "sub D,T"; goto ok;
      if(!INS_T)         fmt = "move D,S"; goto ok;
      if(!INS_S) {
        if(INS_D==INS_T) fmt = "neg D"; goto ok;
                         fmt = "neg D,T"; goto ok;
      }
                         fmt = "sub D,S,T"; goto ok;
    case 0x23:
      if(INS_D==INS_S)   fmt = "subu D,T"; goto ok;
      if(!INS_T)         fmt = "move D,S"; goto ok;
      if(!INS_S) {
        if(INS_D==INS_T) fmt = "negu D"; goto ok;
                         fmt = "negu D,T"; goto ok;
      }
                         fmt = "subu D,S,T"; goto ok;
    case 0x24:
      if(INS_D==INS_S)   fmt = "and D,T"; goto ok;
                         fmt = "and D,S,T"; goto ok;
    case 0x25:
      if(INS_D==INS_S)   fmt = "or D,T"; goto ok;
      if(!INS_T)         fmt = "move D,S"; goto ok;
                         fmt = "or D,S,T"; goto ok;
    case 0x26:
      if(INS_D==INS_S)   fmt = "xor D,T"; goto ok;
      if(!INS_T)         fmt = "move D,S"; goto ok;
                         fmt = "xor D,S,T"; goto ok;
    case 0x27:
      if(!INS_T) {
        if(INS_D==INS_S) fmt = "not D"; goto ok;
                         fmt = "not D,S"; goto ok;
      } else if(!INS_S) {
        if(INS_D==INS_T) fmt = "not D"; goto ok;
                         fmt = "not D,T"; goto ok;
      } else {
        if(INS_D==INS_S) fmt = "nor D,T"; goto ok;
                         fmt = "nor D,S,T"; goto ok;
      }
    case 0x2A: fmt = "slt D,S,T"; goto ok;
    case 0x2B: fmt = "sltu D,S,T"; goto ok;
    default: goto invalid;
    }
  } else {
    switch(ins >> 26) {
    case 0x01:
      switch(INS_T) {
      case 0x00: fmt = "bltz S,B"; goto okdelayslot;
      case 0x01: fmt = "bgez S,B"; goto okdelayslot;
      case 0x10: fmt = "bltzal S,B"; goto okdelayslot;
      case 0x11: fmt = "bgezal S,B"; goto okdelayslot;
      default: goto invalid;
      }
      break;
    case 0x02: fmt = "j J"; goto okdelayslot;
    case 0x03: fmt = "jal J"; goto okdelayslot;
    case 0x04:
      if(INS_S==INS_T) fmt = "b B"; goto okdelayslot;
                       fmt = "beq S,T,B"; goto okdelayslot;
    case 0x05: fmt = "bne S,T,B"; goto okdelayslot;
    case 0x06:
      if(!INS_S)       fmt = "b B"; goto okdelayslot;
                       fmt = "blez S,B"; goto okdelayslot;
    case 0x07: fmt = "bgtz S,B"; goto okdelayslot;
    case 0x08:
      if(INS_T==INS_S) fmt = "addi T,I"; goto ok;
      if(!INS_S)       fmt = "li T,I"; goto ok;
      if(!INS_I)       fmt = "move T,S"; goto ok;
                       fmt = "addi T,S,I"; goto ok;
    case 0x09:
      if(INS_T==INS_S) fmt = "addiu T,I"; goto ok;
      if(!INS_S)       fmt = "li T,I"; goto ok;
      if(!INS_I)       fmt = "move T,S"; goto ok;
                       fmt = "addiu T,S,I"; goto ok;
    case 0x0A: fmt = "slti T,S,I"; goto ok;
    case 0x0B: fmt = "sltiu T,S,I"; goto ok;
    case 0x0C:
      if(INS_T==INS_S) fmt = "andi T,I"; goto ok;
                       fmt = "andi T,S,I"; goto ok;
    case 0x0D:
      if(INS_T==INS_S) fmt = "ori T,I"; goto ok;
                       fmt = "ori T,S,I"; goto ok;
    case 0x0E:
      if(INS_T==INS_S) fmt = "xori T,I"; goto ok;
                       fmt = "xori T,S,I"; goto ok;
    case 0x0F: fmt = "lui T,I"; goto ok;
    case 0x10:
      switch(INS_S) {
      case 0x00: fmt = "mfc0 T,CD"; goto ok;
      case 0x04: fmt = "mtc0 T,CD"; goto ok;
      case 0x10: fmt = "rfe"; goto ok;
      default: goto invalid;
      }
      break;
    case 0x20:
      if(!INS_I) fmt = "lb T,(S)"; goto ok;
                 fmt = "lb T,I(S)"; goto ok;
    case 0x21:
      if(!INS_I) fmt = "lh T,(S)"; goto ok;
                 fmt = "lh T,I(S)"; goto ok;
    case 0x22:
      if(!INS_I) fmt = "lwl T,(S)"; goto ok;
                 fmt = "lwl T,I(S)"; goto ok;
    case 0x23:
      if(!INS_I) fmt = "lw T,(S)"; goto ok;
                 fmt = "lw T,I(S)"; goto ok;
    case 0x24:
      if(!INS_I) fmt = "lbu T,(S)"; goto ok;
                 fmt = "lbu T,I(S)"; goto ok;
    case 0x25:
      if(!INS_I) fmt = "lhu T,(S)"; goto ok;
                 fmt = "lhu T,I(S)"; goto ok;
    case 0x26:
      if(!INS_I) fmt = "lwr T,(S)"; goto ok;
                 fmt = "lwr T,I(S)"; goto ok;
    case 0x28:
      if(!INS_I) fmt = "sb T,(S)"; goto ok;
                 fmt = "sb T,I(S)"; goto ok;
    case 0x29:
      if(!INS_I) fmt = "sh T,(S)"; goto ok;
                 fmt = "sh T,I(S)"; goto ok;
    case 0x2A:
      if(!INS_I) fmt = "swl T,(S)"; goto ok;
                 fmt = "swl T,I(S)"; goto ok;
    case 0x2B:
      if(!INS_I) fmt = "sw T,(S)"; goto ok;
                 fmt = "sw T,I(S)"; goto ok;
    case 0x2E:
      if(!INS_I) fmt = "swr T,(S)"; goto ok;
                 fmt = "swr T,I(S)"; goto ok;
    default: goto invalid;
    }
  }
invalid:
  *dest = 0;
  return -1;
okdelayslot:
  delayslot = 1;
ok:
  for(; *fmt; fmt++) {
    switch(*fmt) {
    case 'S': if(rich) dest[col++] = RICH_CODE_REGISTER; strcpy(dest+col, regname[INS_S]); while(dest[col]) col++; break;
    case 'T': if(rich) dest[col++] = RICH_CODE_REGISTER; strcpy(dest+col, regname[INS_T]); while(dest[col]) col++; break;
    case 'D': if(rich) dest[col++] = RICH_CODE_REGISTER; strcpy(dest+col, regname[INS_D]); while(dest[col]) col++; break;
    case 'H': sprintf(dest+col, "%d", INS_H); while(dest[col]) col++; break;
    case 'I': sprintf(dest+col, "0x%04X", INS_I); while(dest[col]) col++; break;
    case 'B': if(rich) dest[col++] = RICH_CODE_ADDRESS; sprintf(dest+col, "0x%08X", REL_I); while(dest[col]) col++; break;
    case 'J': if(rich) dest[col++] = RICH_CODE_ADDRESS; sprintf(dest+col, "0x%08X", ABS_I); while(dest[col]) col++; break;
    case 'C':
      fmt++;
      if(rich) dest[col++] = RICH_CODE_REGISTER;
      j = 0;
      switch(*fmt) {
      case 'D': j = INS_D; break;
      }
      strcpy(dest+col, C0regname[j]); while(dest[col]) col++;
      break;
    case ' ': while(col < 6) dest[col++] = ' '; break;
    case ',': dest[col++] = ','; dest[col++] = ' '; break;
    default: dest[col++] = *fmt; break;
    }
  }
  dest[col] = 0;
  return delayslot ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
