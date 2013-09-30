/*
** Starscream 680x0 emulation library
** Copyright 1997, 1998, 1999 Neill Corlett
**
** Refer to STARDOC.TXT for terms of use, API reference, and directions on
** how to compile.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "starcpu.h"
#include "cpudebug.h"

static void *starcontext;

static void cpudebug_gets(char *s, int n) {
//  if(cpudebug_get) cpudebug_get(s, n);
//  else
  fgets(s, n, stdin);
}

static void cpudebug_putc(char c) {
  static char buffer[100];
  static unsigned l = 0;
  buffer[l++] = c;
  if((c == '\n') || (l == (sizeof(buffer) - 1))) {
    buffer[l] = 0;
//    if(cpudebug_put) cpudebug_put(buffer);
//    else
    fputs(buffer, stdout);
    l = 0;
  }
}

static void cpudebug_printf(const char *fmt, ...) {
  static char buffer[400];
  char *s = buffer;
  va_list ap;
  va_start(ap, fmt);
  vsprintf(s, fmt, ap);
  va_end(ap);
  while(*s) cpudebug_putc(*s++);
}

#define byte unsigned char
#define word unsigned short int
#define dword unsigned int

#define int08 signed char
#define int16 signed short int
#define int32 signed int

#define ea eacalc[inst&0x3F]()

/******************************
** SNAGS / EA CONSIDERATIONS **
*******************************

- ADDX is encoded the same way as ADD->ea
- SUBX is encoded the same way as SUB->ea
- ABCD is encoded the same way as AND->ea
- EXG is encoded the same way as AND->ea
- SBCD is encoded the same way as OR->ea
- EOR is encoded the same way as CMPM
- ASR is encoded the same way as LSR
  (so are LSL and ASL, but they do the same thing)
- Bcc does NOT support 32-bit offsets on the 68000.
  (this is a reminder, don't bother implementing 32-bit offsets!)
- Look on p. 3-19 for how to calculate branch conditions (GE, LT, GT, etc.)
- Bit operations are 32-bit for registers, 8-bit for memory locations.
- MOVEM->memory is encoded the same way as EXT
  If the EA is just a register, then it's EXT, otherwise it's MOVEM.
- MOVEP done the same way as the bit operations
- Scc done the same way as DBcc.
  Assume it's Scc, unless the EA is a direct An mode (then it's a DBcc).
- SWAP done the same way as PEA
- TAS done the same way as ILLEGAL

- LINK, NOP, RTR, RTS, TRAP, TRAPV, UNLK are encoded the same way.

******************************/

#define hex08 "%02X"
#define hex16 "%04X"
#define hex32 "%08X"
#define hexlong "%06X"

#define isregister ((inst&0x0030)==0x0000)
#define isaddressr ((inst&0x0038)==0x0008)

static char eabuffer[20],sdebug[80];
static dword debugpc,hexaddr;
static int isize;
static word inst;

static word fetch(void){
  debugpc+=2;
  isize+=2;
  return(s68000fetch(starcontext,debugpc-2)&0xFFFF);
}

static dword fetchl(void){
  dword t;
  t=(s68000fetch(starcontext,debugpc)&0xFFFF);
  t<<=16;
  t|=(s68000fetch(starcontext,debugpc+2)&0xFFFF);
  debugpc+=4;
  isize+=4;
  return t;
}

static int08 opsize[1024]={
1,2,4,0,0,0,0,0,1,2,4,0,0,0,0,0,1,2,4,0,0,0,0,0,1,2,4,0,0,0,0,0,
0,0,0,0,0,0,0,0,1,2,4,0,0,0,0,0,1,2,4,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
1,2,4,2,0,0,2,4,1,2,4,1,0,0,2,4,1,2,4,1,0,0,2,4,1,2,4,2,0,0,2,4,
1,4,2,4,0,0,2,4,1,2,4,1,0,0,2,4,0,0,2,4,0,0,2,4,0,2,0,0,0,0,2,4,
1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,
1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
4,4,4,4,0,0,0,0,4,4,4,4,0,0,0,0,4,4,4,4,0,0,0,0,4,4,4,4,0,0,0,0,
4,4,4,4,0,0,0,0,4,4,4,4,0,0,0,0,4,4,4,4,0,0,0,0,4,4,4,4,0,0,0,0,
1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,
1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,
1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,
1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,
1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,
1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,
1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,
1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,
1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,
1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,
1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/******************** EA GENERATION ********************/

/* These are the functions to generate effective addresses.
   Here is the syntax:

   eacalc[mode]();

   mode   is the EA mode << 3 + register number

   The function sets addr so you can use m68read and m68write.

   These routines screw around with the PC, and might call fetch() a couple
   times to get extension words.  Place the eacalc() calls strategically to
   get the extension words in the right order.
*/

/* These are the jump tables for EA calculation.
   drd     = data reg direct                         Dn
   ard     = address reg direct                      An
   ari     = address reg indirect                   (An)
   ari_inc = address reg indirect, postincrement    (An)+
   ari_dec = address reg indirect, predecrement    -(An)
   ari_dis = address reg indirect, displacement     (d16,An)
   ari_ind = address reg indirect, index            (d8,An,Xn)
   pci_dis = prog. counter indirect, displacement   (d16,PC)
   pci_ind = prog. counter indirect, index          (d8,PC,Xn)
*/

#define eacalc_drd(n,r) static void n(void){sprintf(eabuffer,"d%d",r);}
#define eacalc_ard(n,r) static void n(void){sprintf(eabuffer,"a%d",r);}
#define eacalc_ari(n,r) static void n(void){sprintf(eabuffer,"(a%d)",r);}
#define eacalc_ari_inc(n,r) static void n(void){sprintf(eabuffer,"(a%d)+",r);}
#define eacalc_ari_dec(n,r) static void n(void){sprintf(eabuffer,"-(a%d)",r);}
#define eacalc_ari_dis(n,r) static void n(void){\
  int16 briefext=fetch();\
  sprintf(eabuffer,"%c$" hex16 "(a%d)",(briefext<0)?'-':'+',(briefext<0)?-briefext:briefext,r);\
}
#define eacalc_ari_ind(n,r) static void n(void){\
  int16 briefext=fetch();\
  if(briefext<0)sprintf(eabuffer,"-$" hex08 "(a%d,%c%d.%c)",\
    (int08)-briefext,r,\
    briefext&0x8000?'a':'d',(briefext>>12)&7,\
    briefext&0x800?'l':'w');\
  else sprintf(eabuffer,"+$" hex08 "(a%d,%c%d.%c)",\
    (int08)briefext,r,\
    briefext&0x8000?'a':'d',(briefext>>12)&7,\
    briefext&0x800?'l':'w');\
}
eacalc_drd(ea_0_0,0)
eacalc_drd(ea_0_1,1)
eacalc_drd(ea_0_2,2)
eacalc_drd(ea_0_3,3)
eacalc_drd(ea_0_4,4)
eacalc_drd(ea_0_5,5)
eacalc_drd(ea_0_6,6)
eacalc_drd(ea_0_7,7)
eacalc_ard(ea_1_0,0)
eacalc_ard(ea_1_1,1)
eacalc_ard(ea_1_2,2)
eacalc_ard(ea_1_3,3)
eacalc_ard(ea_1_4,4)
eacalc_ard(ea_1_5,5)
eacalc_ard(ea_1_6,6)
eacalc_ard(ea_1_7,7)
eacalc_ari(ea_2_0,0)
eacalc_ari(ea_2_1,1)
eacalc_ari(ea_2_2,2)
eacalc_ari(ea_2_3,3)
eacalc_ari(ea_2_4,4)
eacalc_ari(ea_2_5,5)
eacalc_ari(ea_2_6,6)
eacalc_ari(ea_2_7,7)
eacalc_ari_inc(ea_3_0,0)
eacalc_ari_inc(ea_3_1,1)
eacalc_ari_inc(ea_3_2,2)
eacalc_ari_inc(ea_3_3,3)
eacalc_ari_inc(ea_3_4,4)
eacalc_ari_inc(ea_3_5,5)
eacalc_ari_inc(ea_3_6,6)
eacalc_ari_inc(ea_3_7,7)
eacalc_ari_dec(ea_4_0,0)
eacalc_ari_dec(ea_4_1,1)
eacalc_ari_dec(ea_4_2,2)
eacalc_ari_dec(ea_4_3,3)
eacalc_ari_dec(ea_4_4,4)
eacalc_ari_dec(ea_4_5,5)
eacalc_ari_dec(ea_4_6,6)
eacalc_ari_dec(ea_4_7,7)
eacalc_ari_dis(ea_5_0,0)
eacalc_ari_dis(ea_5_1,1)
eacalc_ari_dis(ea_5_2,2)
eacalc_ari_dis(ea_5_3,3)
eacalc_ari_dis(ea_5_4,4)
eacalc_ari_dis(ea_5_5,5)
eacalc_ari_dis(ea_5_6,6)
eacalc_ari_dis(ea_5_7,7)
eacalc_ari_ind(ea_6_0,0)
eacalc_ari_ind(ea_6_1,1)
eacalc_ari_ind(ea_6_2,2)
eacalc_ari_ind(ea_6_3,3)
eacalc_ari_ind(ea_6_4,4)
eacalc_ari_ind(ea_6_5,5)
eacalc_ari_ind(ea_6_6,6)
eacalc_ari_ind(ea_6_7,7)

/* These are the "special" addressing modes:
   abshort    absolute short address
   abslong    absolute long address
   immdata    immediate data
*/

static void eacalcspecial_abshort(void){
  word briefext;
  briefext=fetch();
  sprintf(eabuffer,"($" hex16 ")",briefext);
}

static void eacalcspecial_abslong(void){
  dword briefext;
  briefext=fetch();
  briefext<<=16;
  briefext|=fetch();
  sprintf(eabuffer,"($"hexlong")",briefext);
}

static void eacalcspecial_immdata(void){
  dword briefext;
  switch(opsize[inst>>6]){
    case 1:
      briefext=fetch()&0xFF;
      sprintf(eabuffer,"#$" hex08,briefext);
      break;
    case 2:
      briefext=fetch();
      sprintf(eabuffer,"#$" hex16,briefext);
      break;
    default:
      briefext=fetch();
      briefext<<=16;
      briefext|=fetch();
      sprintf(eabuffer,"#$" hex32,briefext);
      break;
  }
}

static void eacalcspecial_pci_dis(void){
  dword dpc = debugpc;
  word briefext = fetch();
  sprintf(eabuffer,"$" hexlong "(pc)",((int16)(briefext))+dpc);
}

static void eacalcspecial_pci_ind(void){
  dword dpc = debugpc;
  word briefext = fetch();
  sprintf(eabuffer,"$" hexlong "(pc,%c%d)",
    ((int08)(briefext))+dpc,
    briefext&0x8000?'a':'d',(briefext>>12)&7);
}

static void eacalcspecial_unknown(void){
  sprintf(eabuffer,"*** UNKNOWN EA MODE ***");
}

static void (*(eacalc[64]))(void)={
ea_0_0,ea_0_1,ea_0_2,ea_0_3,ea_0_4,ea_0_5,ea_0_6,ea_0_7,
ea_1_0,ea_1_1,ea_1_2,ea_1_3,ea_1_4,ea_1_5,ea_1_6,ea_1_7,
ea_2_0,ea_2_1,ea_2_2,ea_2_3,ea_2_4,ea_2_5,ea_2_6,ea_2_7,
ea_3_0,ea_3_1,ea_3_2,ea_3_3,ea_3_4,ea_3_5,ea_3_6,ea_3_7,
ea_4_0,ea_4_1,ea_4_2,ea_4_3,ea_4_4,ea_4_5,ea_4_6,ea_4_7,
ea_5_0,ea_5_1,ea_5_2,ea_5_3,ea_5_4,ea_5_5,ea_5_6,ea_5_7,
ea_6_0,ea_6_1,ea_6_2,ea_6_3,ea_6_4,ea_6_5,ea_6_6,ea_6_7,
eacalcspecial_abshort,eacalcspecial_abslong,eacalcspecial_pci_dis,eacalcspecial_pci_ind,
eacalcspecial_immdata,eacalcspecial_unknown,eacalcspecial_unknown,eacalcspecial_unknown};

static void m68unsupported(void){
  sprintf(sdebug,"*** NOT RECOGNIZED ***");
}

static void m68_unrecog_x(void){
  sprintf(sdebug,"unrecognized");
}

/******************** BIT TEST-AND-____ ********************/

static void m68_bitopdn_x(void){
  int16 d16;
  if((inst&0x38)==0x08){
    d16=fetch();
    sprintf(eabuffer,"%c$" hex16 "(a%d)",(d16<0)?'-':'+',(d16<0)?-d16:d16,inst&7);
    if(!(inst&0x80)){
      sprintf(sdebug,"movep%s %s,d%d",
        ((inst&0x40)==0x40)?".l":"  ",
        eabuffer,inst>>9
      );
    }else{
      sprintf(sdebug,"movep%s d%d,%s",
        ((inst&0x40)==0x40)?".l":"  ",
        inst>>9,eabuffer
      );
    }
  }else{
    ea;
    switch((inst>>6)&3){
    case 0:sprintf(sdebug,"btst    d%d,%s",inst>>9,eabuffer);break;
    case 1:sprintf(sdebug,"bchg    d%d,%s",inst>>9,eabuffer);break;
    case 2:sprintf(sdebug,"bclr    d%d,%s",inst>>9,eabuffer);break;
    case 3:sprintf(sdebug,"bset    d%d,%s",inst>>9,eabuffer);break;
    default:break;
    }
  }
}

#define bittest_st(name,dump) static void name(void){\
  byte shiftby=(fetch()&0xFF);\
  ea;sprintf(sdebug,"%s    #$"hex08",%s",dump,shiftby,eabuffer);\
}

bittest_st(m68_btst_st_x,"btst")
bittest_st(m68_bclr_st_x,"bclr")
bittest_st(m68_bset_st_x,"bset")
bittest_st(m68_bchg_st_x,"bchg")

/******************** Bcc ********************/

#define conditional_branch(name,dump) static void name(void){\
  int16 disp;\
  int32 currentpc=debugpc;\
  disp=(int08)(inst&0xFF);\
  if(!disp)disp=fetch();\
  sprintf(sdebug,"%s     ($"hexlong")",dump,currentpc+disp);\
}

conditional_branch(m68_bra_____x,"bra")
conditional_branch(m68_bhi_____x,"bhi")
conditional_branch(m68_bls_____x,"bls")
conditional_branch(m68_bcc_____x,"bcc")
conditional_branch(m68_bcs_____x,"bcs")
conditional_branch(m68_bne_____x,"bne")
conditional_branch(m68_beq_____x,"beq")
conditional_branch(m68_bvc_____x,"bvc")
conditional_branch(m68_bvs_____x,"bvs")
conditional_branch(m68_bpl_____x,"bpl")
conditional_branch(m68_bmi_____x,"bmi")
conditional_branch(m68_bge_____x,"bge")
conditional_branch(m68_blt_____x,"blt")
conditional_branch(m68_bgt_____x,"bgt")
conditional_branch(m68_ble_____x,"ble")

/******************** Scc, DBcc ********************/

#define scc_dbcc(name,dump1,dump2)\
static void name(void){\
  ea;if(isaddressr){\
    int16 disp;\
    disp=fetch();\
    sprintf(sdebug,"%s    d%d,($"hexlong")",dump2,inst&7,debugpc+disp-2);\
  }else sprintf(sdebug,"%s     %s",dump1,eabuffer);\
}

scc_dbcc(m68_st______x,"st ","dbt ")
scc_dbcc(m68_sf______x,"sf ","dbra")
scc_dbcc(m68_shi_____x,"shi","dbhi")
scc_dbcc(m68_sls_____x,"sls","dbls")
scc_dbcc(m68_scc_____x,"scc","dbcc")
scc_dbcc(m68_scs_____x,"scs","dbcs")
scc_dbcc(m68_sne_____x,"sne","dbne")
scc_dbcc(m68_seq_____x,"seq","dbeq")
scc_dbcc(m68_svc_____x,"svc","dbvc")
scc_dbcc(m68_svs_____x,"svs","dbvs")
scc_dbcc(m68_spl_____x,"spl","dbpl")
scc_dbcc(m68_smi_____x,"smi","dbmi")
scc_dbcc(m68_sge_____x,"sge","dbge")
scc_dbcc(m68_slt_____x,"slt","dblt")
scc_dbcc(m68_sgt_____x,"sgt","dbgt")
scc_dbcc(m68_sle_____x,"sle","dble")

/******************** JMP ********************/

static void m68_jmp_____x(void){ea;sprintf(sdebug,"jmp     %s",eabuffer);}

/******************** JSR/BSR ********************/

static void m68_jsr_____x(void){ea;sprintf(sdebug,"jsr     %s",eabuffer);}

static void m68_bsr_____x(void){
  int16 disp;
  int32 currentpc=debugpc;
  disp=(int08)(inst&0xFF);
  if(!disp)disp=fetch();
  sprintf(sdebug,"%s     ($"hexlong")","bsr",disp+currentpc);
}

/******************** TAS ********************/

/* Test-and-set / illegal */
static void m68_tas_____b(void){
  if(inst==0x4AFC){
    sprintf(sdebug,"illegal");
  }else{
    ea;
    sprintf(sdebug,"tas     %s",eabuffer);
  }
}

/******************** LEA ********************/

static void m68_lea___n_l(void){
  ea;sprintf(sdebug,"lea     %s,a%d",eabuffer,(inst>>9)&7);
}

/******************** PEA ********************/

static void m68_pea_____l(void){
  ea;if(isregister){/* SWAP Dn */
    sprintf(sdebug,"swap    d%d",inst&7);
  }else{
    sprintf(sdebug,"pea     %s",eabuffer);
  }
}

/******************** CLR, NEG, NEGX, NOT, TST ********************/

#define negate_ea(name,dump) static void name(void){\
  ea;sprintf(sdebug,"%s  %s",dump,eabuffer);\
}

negate_ea(m68_neg_____b,"neg.b ")
negate_ea(m68_neg_____w,"neg   ")
negate_ea(m68_neg_____l,"neg.l ")
negate_ea(m68_negx____b,"negx.b")
negate_ea(m68_negx____w,"negx  ")
negate_ea(m68_negx____l,"negx.l")
negate_ea(m68_not_____b,"not.b ")
negate_ea(m68_not_____w,"not   ")
negate_ea(m68_not_____l,"not.l ")
negate_ea(m68_clr_____b,"clr.b ")
negate_ea(m68_clr_____w,"clr   ")
negate_ea(m68_clr_____l,"clr.l ")
negate_ea(m68_tst_____b,"tst.b ")
negate_ea(m68_tst_____w,"tst   ")
negate_ea(m68_tst_____l,"tst.l ")

/********************      SOURCE: IMMEDIATE DATA
********************* DESTINATION: EFFECTIVE ADDRESS
********************/

#define im_to_ea(name,type,hextype,fetchtype,dump,r) static void name(void){\
  type src=(type)fetchtype();\
  if((inst&0x3F)==0x3C){\
  sprintf(sdebug,"%s   #$"hextype",%s",dump,src,r);\
  }else{\
  ea;sprintf(sdebug,"%s   #$"hextype",%s",dump,src,eabuffer);\
  }\
}

im_to_ea(m68_ori_____b,byte ,hex08,fetch ,"or.b ","ccr")
im_to_ea(m68_ori_____w,word ,hex16,fetch ,"or   ","sr" )
im_to_ea(m68_ori_____l,dword,hex32,fetchl,"or.l ",""   )
im_to_ea(m68_andi____b,byte ,hex08,fetch ,"and.b","ccr")
im_to_ea(m68_andi____w,word ,hex16,fetch ,"and  ","sr" )
im_to_ea(m68_andi____l,dword,hex32,fetchl,"and.l",""   )
im_to_ea(m68_eori____b,byte ,hex08,fetch ,"eor.b","ccr")
im_to_ea(m68_eori____w,word ,hex16,fetch ,"eor  ","sr" )
im_to_ea(m68_eori____l,dword,hex32,fetchl,"eor.l",""   )
im_to_ea(m68_addi____b,byte ,hex08,fetch ,"add.b",""   )
im_to_ea(m68_addi____w,word ,hex16,fetch ,"add  ",""   )
im_to_ea(m68_addi____l,dword,hex32,fetchl,"add.l",""   )
im_to_ea(m68_subi____b,byte ,hex08,fetch ,"sub.b",""   )
im_to_ea(m68_subi____w,word ,hex16,fetch ,"sub  ",""   )
im_to_ea(m68_subi____l,dword,hex32,fetchl,"sub.l",""   )
im_to_ea(m68_cmpi____b,byte ,hex08,fetch ,"cmp.b",""   )
im_to_ea(m68_cmpi____w,word ,hex16,fetch ,"cmp  ",""   )
im_to_ea(m68_cmpi____l,dword,hex32,fetchl,"cmp.l",""   )

/********************      SOURCE: EFFECTIVE ADDRESS
********************* DESTINATION: DATA REGISTER
********************/

#define ea_to_dn(name,dump) static void name(void){\
  ea;sprintf(sdebug,"%s   %s,d%d",dump,eabuffer,(inst>>9)&7);\
}

ea_to_dn(m68_or__d_n_b,"or.b ")
ea_to_dn(m68_or__d_n_w,"or   ")
ea_to_dn(m68_or__d_n_l,"or.l ")
ea_to_dn(m68_and_d_n_b,"and.b")
ea_to_dn(m68_and_d_n_w,"and  ")
ea_to_dn(m68_and_d_n_l,"and.l")
ea_to_dn(m68_add_d_n_b,"add.b")
ea_to_dn(m68_add_d_n_w,"add  ")
ea_to_dn(m68_add_d_n_l,"add.l")
ea_to_dn(m68_sub_d_n_b,"sub.b")
ea_to_dn(m68_sub_d_n_w,"sub  ")
ea_to_dn(m68_sub_d_n_l,"sub.l")
ea_to_dn(m68_cmp_d_n_b,"cmp.b")
ea_to_dn(m68_cmp_d_n_w,"cmp  ")
ea_to_dn(m68_cmp_d_n_l,"cmp.l")

/********************      SOURCE: EFFECTIVE ADDRESS
********************* DESTINATION: ADDRESS REGISTER
********************/

#define ea_to_an(name,dump) static void name(void){\
  ea;sprintf(sdebug,"%s   %s,a%d",dump,eabuffer,((inst>>9)&7));\
}

ea_to_an(m68_adda__n_w,"add  ")
ea_to_an(m68_adda__n_l,"add.l")
ea_to_an(m68_suba__n_w,"sub  ")
ea_to_an(m68_suba__n_l,"sub.l")
ea_to_an(m68_cmpa__n_w,"cmp  ")
ea_to_an(m68_cmpa__n_l,"cmp.l")

/******************** SUPPORT ROUTINE:  ADDX AND SUBX
********************/

#define support_addsubx(name,dump)\
static void name(void){\
  word nregx=(inst>>9)&7,nregy=inst&7;\
  if(inst&0x0008)sprintf(sdebug,"%s  -(a%d),-(a%d)",dump,nregy,nregx);\
  else sprintf(sdebug,"%s  d%d,d%d",dump,nregy,nregx);\
}

support_addsubx(m68support_addx_b,"addx.b")
support_addsubx(m68support_addx_w,"addx  ")
support_addsubx(m68support_addx_l,"addx.l")
support_addsubx(m68support_subx_b,"subx.b")
support_addsubx(m68support_subx_w,"subx  ")
support_addsubx(m68support_subx_l,"subx.l")

#define support_bcd(name,dump)\
static void name(void){\
  word nregx=(inst>>9)&7,nregy=inst&7;\
  if(inst&0x0008)sprintf(sdebug,"%s    -(a%d),-(a%d)",dump,nregy,nregx);\
  else sprintf(sdebug,"%s  d%d,d%d",dump,nregy,nregx);\
}

support_bcd(m68support_abcd,"abcd")
support_bcd(m68support_sbcd,"sbcd")

/******************** SUPPORT ROUTINE:  CMPM
********************/

#define support_cmpm(name,dump) static void name(void){\
  sprintf(sdebug,"%s   (a%d)+,(a%d)+",dump,inst&7,(inst>>9)&7);\
}

support_cmpm(m68support_cmpm_b,"cmp.b")
support_cmpm(m68support_cmpm_w,"cmp  ")
support_cmpm(m68support_cmpm_l,"cmp.l")

/******************** SUPPORT ROUTINE:  EXG
********************/

static void m68support_exg_same(void){
  dword rx;
  rx=(inst&8)|((inst>>9)&7);
  sprintf(sdebug,"exg     %c%d,%c%d",
    rx&8?'a':'d',rx&7,inst&8?'a':'d',inst&7);
}

static void m68support_exg_diff(void){
  sprintf(sdebug,"exg     d%d,a%d",(inst>>9)&7,inst&7);
}

/********************      SOURCE: DATA REGISTER
********************* DESTINATION: EFFECTIVE ADDRESS
*********************
********************* calls a support routine if EA is a register
********************/

#define dn_to_ea(name,dump,s_cond,s_routine)\
static void name(void){\
  ea;if(s_cond)s_routine();\
  else sprintf(sdebug,"%s   d%d,%s",dump,(inst>>9)&7,eabuffer);\
}

dn_to_ea(m68_add_e_n_b,"add.b",isregister,m68support_addx_b)
dn_to_ea(m68_add_e_n_w,"add  ",isregister,m68support_addx_w)
dn_to_ea(m68_add_e_n_l,"add.l",isregister,m68support_addx_l)
dn_to_ea(m68_sub_e_n_b,"sub.b",isregister,m68support_subx_b)
dn_to_ea(m68_sub_e_n_w,"sub  ",isregister,m68support_subx_w)
dn_to_ea(m68_sub_e_n_l,"sub.l",isregister,m68support_subx_l)
dn_to_ea(m68_eor_e_n_b,"eor.b",isaddressr,m68support_cmpm_b)
dn_to_ea(m68_eor_e_n_w,"eor  ",isaddressr,m68support_cmpm_w)
dn_to_ea(m68_eor_e_n_l,"eor.l",isaddressr,m68support_cmpm_l)
dn_to_ea(m68_or__e_n_b,"or.b ",isregister,m68support_sbcd)
dn_to_ea(m68_or__e_n_w,"or   ",isregister,m68unsupported)
dn_to_ea(m68_or__e_n_l,"or.l ",isregister,m68unsupported)
dn_to_ea(m68_and_e_n_b,"and.b",isregister,m68support_abcd)
dn_to_ea(m68_and_e_n_w,"and  ",isregister,m68support_exg_same)
dn_to_ea(m68_and_e_n_l,"and.l",isaddressr,m68support_exg_diff)

/********************      SOURCE: QUICK DATA
********************* DESTINATION: EFFECTIVE ADDRESS
********************/

#define qn_to_ea(name,dump1) static void name(void){\
  ea;sprintf(sdebug,"%s   #%d,%s",dump1,(((inst>>9)&7)==0)?8:(inst>>9)&7,eabuffer);\
}

qn_to_ea(m68_addq__n_b,"add.b")
qn_to_ea(m68_addq__n_w,"add  ")
qn_to_ea(m68_addq__n_l,"add.l")
qn_to_ea(m68_subq__n_b,"sub.b")
qn_to_ea(m68_subq__n_w,"sub  ")
qn_to_ea(m68_subq__n_l,"sub.l")

/********************      SOURCE: QUICK DATA
********************* DESTINATION: DATA REGISTER
********************/
/* MOVEQ is the only instruction that uses this form */

static void m68_moveq_n_l(void){
  sprintf(sdebug,"moveq   #$"hex08",d%d",inst&0xFF,(inst>>9)&7);
}

/********************      SOURCE: EFFECTIVE ADDRESS
********************* DESTINATION: EFFECTIVE ADDRESS
********************/
/* MOVE is the only instruction that uses this form */

#define ea_to_ea(name,dump) static void name(void){\
  char tmpbuf[40];\
  ea;strcpy(tmpbuf,eabuffer);\
  eacalc[((((inst>>3)&(7<<3)))|((inst>>9)&7))]();\
  sprintf(sdebug,"%s  %s,%s",dump,tmpbuf,eabuffer);\
}

ea_to_ea(m68_move____b,"move.b")
ea_to_ea(m68_move____w,"move  ")
ea_to_ea(m68_move____l,"move.l")

/******************** MOVEM, EXT
********************/

static void getreglistf(word mask,char*rl){
  if(mask&0x0001){*(rl++)='d';*(rl++)='0';*(rl++)='/';}
  if(mask&0x0002){*(rl++)='d';*(rl++)='1';*(rl++)='/';}
  if(mask&0x0004){*(rl++)='d';*(rl++)='2';*(rl++)='/';}
  if(mask&0x0008){*(rl++)='d';*(rl++)='3';*(rl++)='/';}
  if(mask&0x0010){*(rl++)='d';*(rl++)='4';*(rl++)='/';}
  if(mask&0x0020){*(rl++)='d';*(rl++)='5';*(rl++)='/';}
  if(mask&0x0040){*(rl++)='d';*(rl++)='6';*(rl++)='/';}
  if(mask&0x0080){*(rl++)='d';*(rl++)='7';*(rl++)='/';}
  if(mask&0x0100){*(rl++)='a';*(rl++)='0';*(rl++)='/';}
  if(mask&0x0200){*(rl++)='a';*(rl++)='1';*(rl++)='/';}
  if(mask&0x0400){*(rl++)='a';*(rl++)='2';*(rl++)='/';}
  if(mask&0x0800){*(rl++)='a';*(rl++)='3';*(rl++)='/';}
  if(mask&0x1000){*(rl++)='a';*(rl++)='4';*(rl++)='/';}
  if(mask&0x2000){*(rl++)='a';*(rl++)='5';*(rl++)='/';}
  if(mask&0x4000){*(rl++)='a';*(rl++)='6';*(rl++)='/';}
  if(mask&0x8000){*(rl++)='a';*(rl++)='7';*(rl++)='/';}
  *(--rl)=0;
}

static void getreglistb(word mask,char*rl){
  if(mask&0x0001){*(rl++)='a';*(rl++)='7';*(rl++)='/';}
  if(mask&0x0002){*(rl++)='a';*(rl++)='6';*(rl++)='/';}
  if(mask&0x0004){*(rl++)='a';*(rl++)='5';*(rl++)='/';}
  if(mask&0x0008){*(rl++)='a';*(rl++)='4';*(rl++)='/';}
  if(mask&0x0010){*(rl++)='a';*(rl++)='3';*(rl++)='/';}
  if(mask&0x0020){*(rl++)='a';*(rl++)='2';*(rl++)='/';}
  if(mask&0x0040){*(rl++)='a';*(rl++)='1';*(rl++)='/';}
  if(mask&0x0080){*(rl++)='a';*(rl++)='0';*(rl++)='/';}
  if(mask&0x0100){*(rl++)='d';*(rl++)='7';*(rl++)='/';}
  if(mask&0x0200){*(rl++)='d';*(rl++)='6';*(rl++)='/';}
  if(mask&0x0400){*(rl++)='d';*(rl++)='5';*(rl++)='/';}
  if(mask&0x0800){*(rl++)='d';*(rl++)='4';*(rl++)='/';}
  if(mask&0x1000){*(rl++)='d';*(rl++)='3';*(rl++)='/';}
  if(mask&0x2000){*(rl++)='d';*(rl++)='2';*(rl++)='/';}
  if(mask&0x4000){*(rl++)='d';*(rl++)='1';*(rl++)='/';}
  if(mask&0x8000){*(rl++)='d';*(rl++)='0';*(rl++)='/';}
  *(--rl)=0;
}

#define movem_mem(name,dumpm,dumpx)\
static void name(void){\
  word regmask;\
  char reglist[50];\
  if((inst&0x38)==0x0000){ /* ext */\
    sprintf(sdebug,dumpx"d%d",inst&7);\
  }else if((inst&0x38)==0x0020){ /* predecrement addressing mode */\
    regmask=fetch();\
    getreglistb(regmask,reglist);\
    sprintf(sdebug,dumpm"%s,-(a%d)",reglist,inst&7);\
  }else{\
    regmask=fetch();\
    ea;getreglistf(regmask,reglist);\
    sprintf(sdebug,dumpm "%s,%s",reglist,eabuffer);\
  }\
}

movem_mem(m68_movem___w,"movem   ","ext     ")
movem_mem(m68_movem___l,"movem.l ","ext.l   ")

#define movem_reg(name,dump) static void name(void){\
  word regmask;\
  char reglist[50];\
  regmask=fetch();\
  ea;getreglistf(regmask,reglist);\
  sprintf(sdebug,dump "%s,%s",eabuffer,reglist);\
}

movem_reg(m68_movem_r_w,"movem   ")
movem_reg(m68_movem_r_l,"movem.l ")

/******************** INSTRUCTIONS THAT INVOLVE SR/CCR ********************/

static void m68_move2sr_w(void){ea;sprintf(sdebug,"move    %s,sr",eabuffer);}
static void m68_movefsr_w(void){ea;sprintf(sdebug,"move    sr,%s",eabuffer);}
static void m68_move2cc_w(void){ea;sprintf(sdebug,"move.b  %s,ccr",eabuffer);}
static void m68_movefcc_w(void){ea;sprintf(sdebug,"move.b  ccr,%s",eabuffer);}

static void m68_rts_____x(void){sprintf(sdebug,"rts");}

/******************** SHIFTS AND ROTATES ********************/

#define regshift(name,sizedump) \
static void name(void){\
  char tmpbuf[10];\
  if((inst&0x20)==0)sprintf(tmpbuf,"#$"hex08",d%d",(((inst>>9)&7)==0)?8:(inst>>9)&7,inst&7);\
  else sprintf(tmpbuf,"d%d,d%d",(inst>>9)&7,inst&7);\
  switch(inst&0x18){\
    case 0x00:sprintf(sdebug,"as%s   %s",sizedump,tmpbuf);break;\
    case 0x08:sprintf(sdebug,"ls%s   %s",sizedump,tmpbuf);break;\
    case 0x10:sprintf(sdebug,"rox%s  %s",sizedump,tmpbuf);break;\
    case 0x18:sprintf(sdebug,"ro%s   %s",sizedump,tmpbuf);break;\
  }\
}

regshift(m68_shl_r_n_b,"l.b")
regshift(m68_shl_r_n_w,"l  ")
regshift(m68_shl_r_n_l,"l.l")
regshift(m68_shr_r_n_b,"r.b")
regshift(m68_shr_r_n_w,"r  ")
regshift(m68_shr_r_n_l,"r.l")

/******************** NOP ********************/

static void m68_nop_____x(void){sprintf(sdebug,"nop");}

/******************** LINK / UNLINK ********************/

static void m68_link_an_w(void){
  int16 briefext=fetch();
  sprintf(sdebug,"link    a%d,%c#$"hex16,inst&7,briefext<0?'-':'+',
    briefext<0?-briefext:briefext
  );
}

static void m68_unlk_an_x(void){
  sprintf(sdebug,"unlk    a%d",inst&7);
}

static void m68_stop____x(void){sprintf(sdebug,"stop    #$%04X",fetch());}
static void m68_rte_____x(void){sprintf(sdebug,"rte");}
static void m68_rtr_____x(void){sprintf(sdebug,"rtr");}
static void m68_reset___x(void){sprintf(sdebug,"reset");}
static void m68_rtd_____x(void){
  int16 briefext=fetch();
  sprintf(sdebug,"rtd     %c#$"hex16,briefext<0?'-':'+',
    briefext<0?-briefext:briefext);
}
static void m68_divu__n_w(void){ea;sprintf(sdebug,"divu    %s,d%d",eabuffer,(inst>>9)&7);}
static void m68_divs__n_w(void){ea;sprintf(sdebug,"divs    %s,d%d",eabuffer,(inst>>9)&7);}
static void m68_mulu__n_w(void){ea;sprintf(sdebug,"mulu    %s,d%d",eabuffer,(inst>>9)&7);}
static void m68_muls__n_w(void){ea;sprintf(sdebug,"muls    %s,d%d",eabuffer,(inst>>9)&7);}

static void m68_asr_m___w(void){ea;sprintf(sdebug,"asr     %s",eabuffer);}
static void m68_asl_m___w(void){ea;sprintf(sdebug,"asl     %s",eabuffer);}
static void m68_lsr_m___w(void){ea;sprintf(sdebug,"lsr     %s",eabuffer);}
static void m68_lsl_m___w(void){ea;sprintf(sdebug,"lsl     %s",eabuffer);}
static void m68_roxr_m__w(void){ea;sprintf(sdebug,"roxr    %s",eabuffer);}
static void m68_roxl_m__w(void){ea;sprintf(sdebug,"roxl    %s",eabuffer);}
static void m68_ror_m___w(void){ea;sprintf(sdebug,"ror     %s",eabuffer);}
static void m68_rol_m___w(void){ea;sprintf(sdebug,"rol     %s",eabuffer);}

static void m68_nbcd____b(void){ea;sprintf(sdebug,"nbcd.b  %s",eabuffer);}
static void m68_chk___n_w(void){ea;sprintf(sdebug,"chk     %s,d%d",eabuffer,(inst>>9)&7);}

static void m68_trap_nn_x(void){sprintf(sdebug,"trap    #%d",inst&0xF);}
static void m68_move_2u_l(void){sprintf(sdebug,"move.l  a%d,usp",inst&7);}
static void m68_move_fu_l(void){sprintf(sdebug,"move.l  usp,a%d",inst&7);}

static void m68_trapv___x(void){sprintf(sdebug,"trapv");}

static char*specialregister(unsigned short int code){
  switch(code&0xFFF){
  case 0x000:return("sfc");
  case 0x001:return("dfc");
  case 0x800:return("usp");
  case 0x801:return("vbr");
  }
  return("???");
}

static void m68_movec_r_x(void){
  unsigned short int f=fetch();
  sprintf(sdebug,"movec   %s,%c%d",
    specialregister(f),
    (f&0x8000)?'a':'d',(f>>12)&7
  );
}

static void m68_movec_c_x(void){
  unsigned short int f=fetch();
  sprintf(sdebug,"movec   %c%d,%s",
    (f&0x8000)?'a':'d',(f>>12)&7,
    specialregister(f)
  );
}

/******************** SPECIAL INSTRUCTION TABLE ********************/

/* This table is used for 0100111001xxxxxx instructions (4E4x-4E7x) */
static void(*(debugspecialmap[64]))(void)={
/* 0000xx */ m68_trap_nn_x,m68_trap_nn_x,m68_trap_nn_x,m68_trap_nn_x,
/* 0001xx */ m68_trap_nn_x,m68_trap_nn_x,m68_trap_nn_x,m68_trap_nn_x,
/* 0010xx */ m68_trap_nn_x,m68_trap_nn_x,m68_trap_nn_x,m68_trap_nn_x,
/* 0011xx */ m68_trap_nn_x,m68_trap_nn_x,m68_trap_nn_x,m68_trap_nn_x,
/* 0100xx */ m68_link_an_w,m68_link_an_w,m68_link_an_w,m68_link_an_w,
/* 0101xx */ m68_link_an_w,m68_link_an_w,m68_link_an_w,m68_link_an_w,
/* 0110xx */ m68_unlk_an_x,m68_unlk_an_x,m68_unlk_an_x,m68_unlk_an_x,
/* 0111xx */ m68_unlk_an_x,m68_unlk_an_x,m68_unlk_an_x,m68_unlk_an_x,
/* 1000xx */ m68_move_2u_l,m68_move_2u_l,m68_move_2u_l,m68_move_2u_l,
/* 1001xx */ m68_move_2u_l,m68_move_2u_l,m68_move_2u_l,m68_move_2u_l,
/* 1010xx */ m68_move_fu_l,m68_move_fu_l,m68_move_fu_l,m68_move_fu_l,
/* 1011xx */ m68_move_fu_l,m68_move_fu_l,m68_move_fu_l,m68_move_fu_l,
/* 1100xx */ m68_reset___x,m68_nop_____x,m68_stop____x,m68_rte_____x,
/* 1101xx */ m68_rtd_____x,m68_rts_____x,m68_trapv___x,m68_rtr_____x,
/* 1110xx */ m68_unrecog_x,m68_unrecog_x,m68_movec_r_x,m68_movec_c_x,
/* 1111xx */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x};

static void m68_special_x(void){
  debugspecialmap[inst&0x3F]();
}

/******************** INSTRUCTION TABLE ********************/

/* The big 1024-element jump table that handles all possible opcodes
   is in the following header file.
   Use this syntax to execute an instruction:

   cpumap[i>>6]();

   ("i" is the instruction word)
*/

static void(*(debugmap[1024]))(void)={
/* 00000000ss */ m68_ori_____b,m68_ori_____w,m68_ori_____l,m68_unrecog_x,
/* 00000001ss */ m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,
/* 00000010ss */ m68_andi____b,m68_andi____w,m68_andi____l,m68_unrecog_x,
/* 00000011ss */ m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,
/* 00000100ss */ m68_subi____b,m68_subi____w,m68_subi____l,m68_unrecog_x,
/* 00000101ss */ m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,
/* 00000110ss */ m68_addi____b,m68_addi____w,m68_addi____l,m68_unrecog_x,
/* 00000111ss */ m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,
/* 00001000ss */ m68_btst_st_x,m68_bchg_st_x,m68_bclr_st_x,m68_bset_st_x,
/* 00001001ss */ m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,
/* 00001010ss */ m68_eori____b,m68_eori____w,m68_eori____l,m68_unrecog_x,
/* 00001011ss */ m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,
/* 00001100ss */ m68_cmpi____b,m68_cmpi____w,m68_cmpi____l,m68_unrecog_x,
/* 00001101ss */ m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,
/* 00001110ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 00001111ss */ m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,
/* 00010000ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00010001ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00010010ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00010011ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00010100ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00010101ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00010110ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00010111ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00011000ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00011001ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00011010ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00011011ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00011100ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00011101ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00011110ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00011111ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00100000ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00100001ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00100010ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00100011ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00100100ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00100101ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00100110ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00100111ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00101000ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00101001ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00101010ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00101011ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00101100ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00101101ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00101110ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00101111ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00110000ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00110001ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00110010ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00110011ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00110100ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00110101ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00110110ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00110111ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00111000ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00111001ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00111010ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00111011ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00111100ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00111101ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00111110ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00111111ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 01000000ss */ m68_negx____b,m68_negx____w,m68_negx____l,m68_movefsr_w,
/* 01000001ss */ m68_unrecog_x,m68_unrecog_x,m68_chk___n_w,m68_lea___n_l,
/* 01000010ss */ m68_clr_____b,m68_clr_____w,m68_clr_____l,m68_movefcc_w,
/* 01000011ss */ m68_unrecog_x,m68_unrecog_x,m68_chk___n_w,m68_lea___n_l,
/* 01000100ss */ m68_neg_____b,m68_neg_____w,m68_neg_____l,m68_move2cc_w,
/* 01000101ss */ m68_unrecog_x,m68_unrecog_x,m68_chk___n_w,m68_lea___n_l,
/* 01000110ss */ m68_not_____b,m68_not_____w,m68_not_____l,m68_move2sr_w,
/* 01000111ss */ m68_unrecog_x,m68_unrecog_x,m68_chk___n_w,m68_lea___n_l,
/* 01001000ss */ m68_nbcd____b,m68_pea_____l,m68_movem___w,m68_movem___l,
/* 01001001ss */ m68_unrecog_x,m68_unrecog_x,m68_chk___n_w,m68_lea___n_l,
/* 01001010ss */ m68_tst_____b,m68_tst_____w,m68_tst_____l,m68_tas_____b,
/* 01001011ss */ m68_unrecog_x,m68_unrecog_x,m68_chk___n_w,m68_lea___n_l,
/* 01001100ss */ m68_unrecog_x,m68_unrecog_x,m68_movem_r_w,m68_movem_r_l,
/* 01001101ss */ m68_unrecog_x,m68_unrecog_x,m68_chk___n_w,m68_lea___n_l,
/* 01001110ss */ m68_unrecog_x,m68_special_x,m68_jsr_____x,m68_jmp_____x,
/* 01001111ss */ m68_unrecog_x,m68_unrecog_x,m68_chk___n_w,m68_lea___n_l,
/* 01010000ss */ m68_addq__n_b,m68_addq__n_w,m68_addq__n_l,m68_st______x,
/* 01010001ss */ m68_subq__n_b,m68_subq__n_w,m68_subq__n_l,m68_sf______x,
/* 01010010ss */ m68_addq__n_b,m68_addq__n_w,m68_addq__n_l,m68_shi_____x,
/* 01010011ss */ m68_subq__n_b,m68_subq__n_w,m68_subq__n_l,m68_sls_____x,
/* 01010100ss */ m68_addq__n_b,m68_addq__n_w,m68_addq__n_l,m68_scc_____x,
/* 01010101ss */ m68_subq__n_b,m68_subq__n_w,m68_subq__n_l,m68_scs_____x,
/* 01010110ss */ m68_addq__n_b,m68_addq__n_w,m68_addq__n_l,m68_sne_____x,
/* 01010111ss */ m68_subq__n_b,m68_subq__n_w,m68_subq__n_l,m68_seq_____x,
/* 01011000ss */ m68_addq__n_b,m68_addq__n_w,m68_addq__n_l,m68_svc_____x,
/* 01011001ss */ m68_subq__n_b,m68_subq__n_w,m68_subq__n_l,m68_svs_____x,
/* 01011010ss */ m68_addq__n_b,m68_addq__n_w,m68_addq__n_l,m68_spl_____x,
/* 01011011ss */ m68_subq__n_b,m68_subq__n_w,m68_subq__n_l,m68_smi_____x,
/* 01011100ss */ m68_addq__n_b,m68_addq__n_w,m68_addq__n_l,m68_sge_____x,
/* 01011101ss */ m68_subq__n_b,m68_subq__n_w,m68_subq__n_l,m68_slt_____x,
/* 01011110ss */ m68_addq__n_b,m68_addq__n_w,m68_addq__n_l,m68_sgt_____x,
/* 01011111ss */ m68_subq__n_b,m68_subq__n_w,m68_subq__n_l,m68_sle_____x,
/* 01100000ss */ m68_bra_____x,m68_bra_____x,m68_bra_____x,m68_bra_____x,
/* 01100001ss */ m68_bsr_____x,m68_bsr_____x,m68_bsr_____x,m68_bsr_____x,
/* 01100010ss */ m68_bhi_____x,m68_bhi_____x,m68_bhi_____x,m68_bhi_____x,
/* 01100011ss */ m68_bls_____x,m68_bls_____x,m68_bls_____x,m68_bls_____x,
/* 01100100ss */ m68_bcc_____x,m68_bcc_____x,m68_bcc_____x,m68_bcc_____x,
/* 01100101ss */ m68_bcs_____x,m68_bcs_____x,m68_bcs_____x,m68_bcs_____x,
/* 01100110ss */ m68_bne_____x,m68_bne_____x,m68_bne_____x,m68_bne_____x,
/* 01100111ss */ m68_beq_____x,m68_beq_____x,m68_beq_____x,m68_beq_____x,
/* 01101000ss */ m68_bvc_____x,m68_bvc_____x,m68_bvc_____x,m68_bvc_____x,
/* 01101001ss */ m68_bvs_____x,m68_bvs_____x,m68_bvs_____x,m68_bvs_____x,
/* 01101010ss */ m68_bpl_____x,m68_bpl_____x,m68_bpl_____x,m68_bpl_____x,
/* 01101011ss */ m68_bmi_____x,m68_bmi_____x,m68_bmi_____x,m68_bmi_____x,
/* 01101100ss */ m68_bge_____x,m68_bge_____x,m68_bge_____x,m68_bge_____x,
/* 01101101ss */ m68_blt_____x,m68_blt_____x,m68_blt_____x,m68_blt_____x,
/* 01101110ss */ m68_bgt_____x,m68_bgt_____x,m68_bgt_____x,m68_bgt_____x,
/* 01101111ss */ m68_ble_____x,m68_ble_____x,m68_ble_____x,m68_ble_____x,
/* 01110000ss */ m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,
/* 01110001ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 01110010ss */ m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,
/* 01110011ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 01110100ss */ m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,
/* 01110101ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 01110110ss */ m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,
/* 01110111ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 01111000ss */ m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,
/* 01111001ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 01111010ss */ m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,
/* 01111011ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 01111100ss */ m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,
/* 01111101ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 01111110ss */ m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,
/* 01111111ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10000000ss */ m68_or__d_n_b,m68_or__d_n_w,m68_or__d_n_l,m68_divu__n_w,
/* 10000001ss */ m68_or__e_n_b,m68_or__e_n_w,m68_or__e_n_l,m68_divs__n_w,
/* 10000010ss */ m68_or__d_n_b,m68_or__d_n_w,m68_or__d_n_l,m68_divu__n_w,
/* 10000011ss */ m68_or__e_n_b,m68_or__e_n_w,m68_or__e_n_l,m68_divs__n_w,
/* 10000100ss */ m68_or__d_n_b,m68_or__d_n_w,m68_or__d_n_l,m68_divu__n_w,
/* 10000101ss */ m68_or__e_n_b,m68_or__e_n_w,m68_or__e_n_l,m68_divs__n_w,
/* 10000110ss */ m68_or__d_n_b,m68_or__d_n_w,m68_or__d_n_l,m68_divu__n_w,
/* 10000111ss */ m68_or__e_n_b,m68_or__e_n_w,m68_or__e_n_l,m68_divs__n_w,
/* 10001000ss */ m68_or__d_n_b,m68_or__d_n_w,m68_or__d_n_l,m68_divu__n_w,
/* 10001001ss */ m68_or__e_n_b,m68_or__e_n_w,m68_or__e_n_l,m68_divs__n_w,
/* 10001010ss */ m68_or__d_n_b,m68_or__d_n_w,m68_or__d_n_l,m68_divu__n_w,
/* 10001011ss */ m68_or__e_n_b,m68_or__e_n_w,m68_or__e_n_l,m68_divs__n_w,
/* 10001100ss */ m68_or__d_n_b,m68_or__d_n_w,m68_or__d_n_l,m68_divu__n_w,
/* 10001101ss */ m68_or__e_n_b,m68_or__e_n_w,m68_or__e_n_l,m68_divs__n_w,
/* 10001110ss */ m68_or__d_n_b,m68_or__d_n_w,m68_or__d_n_l,m68_divu__n_w,
/* 10001111ss */ m68_or__e_n_b,m68_or__e_n_w,m68_or__e_n_l,m68_divs__n_w,
/* 10010000ss */ m68_sub_d_n_b,m68_sub_d_n_w,m68_sub_d_n_l,m68_suba__n_w,
/* 10010001ss */ m68_sub_e_n_b,m68_sub_e_n_w,m68_sub_e_n_l,m68_suba__n_l,
/* 10010010ss */ m68_sub_d_n_b,m68_sub_d_n_w,m68_sub_d_n_l,m68_suba__n_w,
/* 10010011ss */ m68_sub_e_n_b,m68_sub_e_n_w,m68_sub_e_n_l,m68_suba__n_l,
/* 10010100ss */ m68_sub_d_n_b,m68_sub_d_n_w,m68_sub_d_n_l,m68_suba__n_w,
/* 10010101ss */ m68_sub_e_n_b,m68_sub_e_n_w,m68_sub_e_n_l,m68_suba__n_l,
/* 10010110ss */ m68_sub_d_n_b,m68_sub_d_n_w,m68_sub_d_n_l,m68_suba__n_w,
/* 10010111ss */ m68_sub_e_n_b,m68_sub_e_n_w,m68_sub_e_n_l,m68_suba__n_l,
/* 10011000ss */ m68_sub_d_n_b,m68_sub_d_n_w,m68_sub_d_n_l,m68_suba__n_w,
/* 10011001ss */ m68_sub_e_n_b,m68_sub_e_n_w,m68_sub_e_n_l,m68_suba__n_l,
/* 10011010ss */ m68_sub_d_n_b,m68_sub_d_n_w,m68_sub_d_n_l,m68_suba__n_w,
/* 10011011ss */ m68_sub_e_n_b,m68_sub_e_n_w,m68_sub_e_n_l,m68_suba__n_l,
/* 10011100ss */ m68_sub_d_n_b,m68_sub_d_n_w,m68_sub_d_n_l,m68_suba__n_w,
/* 10011101ss */ m68_sub_e_n_b,m68_sub_e_n_w,m68_sub_e_n_l,m68_suba__n_l,
/* 10011110ss */ m68_sub_d_n_b,m68_sub_d_n_w,m68_sub_d_n_l,m68_suba__n_w,
/* 10011111ss */ m68_sub_e_n_b,m68_sub_e_n_w,m68_sub_e_n_l,m68_suba__n_l,
/* 10100000ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10100001ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10100010ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10100011ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10100100ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10100101ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10100110ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10100111ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10101000ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10101001ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10101010ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10101011ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10101100ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10101101ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10101110ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10101111ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10110000ss */ m68_cmp_d_n_b,m68_cmp_d_n_w,m68_cmp_d_n_l,m68_cmpa__n_w,
/* 10110001ss */ m68_eor_e_n_b,m68_eor_e_n_w,m68_eor_e_n_l,m68_cmpa__n_l,
/* 10110010ss */ m68_cmp_d_n_b,m68_cmp_d_n_w,m68_cmp_d_n_l,m68_cmpa__n_w,
/* 10110011ss */ m68_eor_e_n_b,m68_eor_e_n_w,m68_eor_e_n_l,m68_cmpa__n_l,
/* 10110100ss */ m68_cmp_d_n_b,m68_cmp_d_n_w,m68_cmp_d_n_l,m68_cmpa__n_w,
/* 10110101ss */ m68_eor_e_n_b,m68_eor_e_n_w,m68_eor_e_n_l,m68_cmpa__n_l,
/* 10110110ss */ m68_cmp_d_n_b,m68_cmp_d_n_w,m68_cmp_d_n_l,m68_cmpa__n_w,
/* 10110111ss */ m68_eor_e_n_b,m68_eor_e_n_w,m68_eor_e_n_l,m68_cmpa__n_l,
/* 10111000ss */ m68_cmp_d_n_b,m68_cmp_d_n_w,m68_cmp_d_n_l,m68_cmpa__n_w,
/* 10111001ss */ m68_eor_e_n_b,m68_eor_e_n_w,m68_eor_e_n_l,m68_cmpa__n_l,
/* 10111010ss */ m68_cmp_d_n_b,m68_cmp_d_n_w,m68_cmp_d_n_l,m68_cmpa__n_w,
/* 10111011ss */ m68_eor_e_n_b,m68_eor_e_n_w,m68_eor_e_n_l,m68_cmpa__n_l,
/* 10111100ss */ m68_cmp_d_n_b,m68_cmp_d_n_w,m68_cmp_d_n_l,m68_cmpa__n_w,
/* 10111101ss */ m68_eor_e_n_b,m68_eor_e_n_w,m68_eor_e_n_l,m68_cmpa__n_l,
/* 10111110ss */ m68_cmp_d_n_b,m68_cmp_d_n_w,m68_cmp_d_n_l,m68_cmpa__n_w,
/* 10111111ss */ m68_eor_e_n_b,m68_eor_e_n_w,m68_eor_e_n_l,m68_cmpa__n_l,
/* 11000000ss */ m68_and_d_n_b,m68_and_d_n_w,m68_and_d_n_l,m68_mulu__n_w,
/* 11000001ss */ m68_and_e_n_b,m68_and_e_n_w,m68_and_e_n_l,m68_muls__n_w,
/* 11000010ss */ m68_and_d_n_b,m68_and_d_n_w,m68_and_d_n_l,m68_mulu__n_w,
/* 11000011ss */ m68_and_e_n_b,m68_and_e_n_w,m68_and_e_n_l,m68_muls__n_w,
/* 11000100ss */ m68_and_d_n_b,m68_and_d_n_w,m68_and_d_n_l,m68_mulu__n_w,
/* 11000101ss */ m68_and_e_n_b,m68_and_e_n_w,m68_and_e_n_l,m68_muls__n_w,
/* 11000110ss */ m68_and_d_n_b,m68_and_d_n_w,m68_and_d_n_l,m68_mulu__n_w,
/* 11000111ss */ m68_and_e_n_b,m68_and_e_n_w,m68_and_e_n_l,m68_muls__n_w,
/* 11001000ss */ m68_and_d_n_b,m68_and_d_n_w,m68_and_d_n_l,m68_mulu__n_w,
/* 11001001ss */ m68_and_e_n_b,m68_and_e_n_w,m68_and_e_n_l,m68_muls__n_w,
/* 11001010ss */ m68_and_d_n_b,m68_and_d_n_w,m68_and_d_n_l,m68_mulu__n_w,
/* 11001011ss */ m68_and_e_n_b,m68_and_e_n_w,m68_and_e_n_l,m68_muls__n_w,
/* 11001100ss */ m68_and_d_n_b,m68_and_d_n_w,m68_and_d_n_l,m68_mulu__n_w,
/* 11001101ss */ m68_and_e_n_b,m68_and_e_n_w,m68_and_e_n_l,m68_muls__n_w,
/* 11001110ss */ m68_and_d_n_b,m68_and_d_n_w,m68_and_d_n_l,m68_mulu__n_w,
/* 11001111ss */ m68_and_e_n_b,m68_and_e_n_w,m68_and_e_n_l,m68_muls__n_w,
/* 11010000ss */ m68_add_d_n_b,m68_add_d_n_w,m68_add_d_n_l,m68_adda__n_w,
/* 11010001ss */ m68_add_e_n_b,m68_add_e_n_w,m68_add_e_n_l,m68_adda__n_l,
/* 11010010ss */ m68_add_d_n_b,m68_add_d_n_w,m68_add_d_n_l,m68_adda__n_w,
/* 11010011ss */ m68_add_e_n_b,m68_add_e_n_w,m68_add_e_n_l,m68_adda__n_l,
/* 11010100ss */ m68_add_d_n_b,m68_add_d_n_w,m68_add_d_n_l,m68_adda__n_w,
/* 11010101ss */ m68_add_e_n_b,m68_add_e_n_w,m68_add_e_n_l,m68_adda__n_l,
/* 11010110ss */ m68_add_d_n_b,m68_add_d_n_w,m68_add_d_n_l,m68_adda__n_w,
/* 11010111ss */ m68_add_e_n_b,m68_add_e_n_w,m68_add_e_n_l,m68_adda__n_l,
/* 11011000ss */ m68_add_d_n_b,m68_add_d_n_w,m68_add_d_n_l,m68_adda__n_w,
/* 11011001ss */ m68_add_e_n_b,m68_add_e_n_w,m68_add_e_n_l,m68_adda__n_l,
/* 11011010ss */ m68_add_d_n_b,m68_add_d_n_w,m68_add_d_n_l,m68_adda__n_w,
/* 11011011ss */ m68_add_e_n_b,m68_add_e_n_w,m68_add_e_n_l,m68_adda__n_l,
/* 11011100ss */ m68_add_d_n_b,m68_add_d_n_w,m68_add_d_n_l,m68_adda__n_w,
/* 11011101ss */ m68_add_e_n_b,m68_add_e_n_w,m68_add_e_n_l,m68_adda__n_l,
/* 11011110ss */ m68_add_d_n_b,m68_add_d_n_w,m68_add_d_n_l,m68_adda__n_w,
/* 11011111ss */ m68_add_e_n_b,m68_add_e_n_w,m68_add_e_n_l,m68_adda__n_l,
/* 11100000ss */ m68_shr_r_n_b,m68_shr_r_n_w,m68_shr_r_n_l,m68_asr_m___w,
/* 11100001ss */ m68_shl_r_n_b,m68_shl_r_n_w,m68_shl_r_n_l,m68_asl_m___w,
/* 11100010ss */ m68_shr_r_n_b,m68_shr_r_n_w,m68_shr_r_n_l,m68_lsr_m___w,
/* 11100011ss */ m68_shl_r_n_b,m68_shl_r_n_w,m68_shl_r_n_l,m68_lsl_m___w,
/* 11100100ss */ m68_shr_r_n_b,m68_shr_r_n_w,m68_shr_r_n_l,m68_roxr_m__w,
/* 11100101ss */ m68_shl_r_n_b,m68_shl_r_n_w,m68_shl_r_n_l,m68_roxl_m__w,
/* 11100110ss */ m68_shr_r_n_b,m68_shr_r_n_w,m68_shr_r_n_l,m68_ror_m___w,
/* 11100111ss */ m68_shl_r_n_b,m68_shl_r_n_w,m68_shl_r_n_l,m68_rol_m___w,
/* 11101000ss */ m68_shr_r_n_b,m68_shr_r_n_w,m68_shr_r_n_l,m68_unrecog_x,
/* 11101001ss */ m68_shl_r_n_b,m68_shl_r_n_w,m68_shl_r_n_l,m68_unrecog_x,
/* 11101010ss */ m68_shr_r_n_b,m68_shr_r_n_w,m68_shr_r_n_l,m68_unrecog_x,
/* 11101011ss */ m68_shl_r_n_b,m68_shl_r_n_w,m68_shl_r_n_l,m68_unrecog_x,
/* 11101100ss */ m68_shr_r_n_b,m68_shr_r_n_w,m68_shr_r_n_l,m68_unrecog_x,
/* 11101101ss */ m68_shl_r_n_b,m68_shl_r_n_w,m68_shl_r_n_l,m68_unrecog_x,
/* 11101110ss */ m68_shr_r_n_b,m68_shr_r_n_w,m68_shr_r_n_l,m68_unrecog_x,
/* 11101111ss */ m68_shl_r_n_b,m68_shl_r_n_w,m68_shl_r_n_l,m68_unrecog_x,
/* 11110000ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11110001ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11110010ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11110011ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11110100ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11110101ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11110110ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11110111ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11111000ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11111001ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11111010ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11111011ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11111100ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11111101ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11111110ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11111111ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x};

static void cpudebug_disassemble(int n) {
  while(n--) {
    dword addr = debugpc;
    cpudebug_printf("%08X: ", addr);
    isize = 0;
    inst = fetch();
    debugmap[inst >> 6]();
    while(addr < debugpc) {
      cpudebug_printf("%04X ", s68000fetch(starcontext,addr) & 0xFFFF);
      addr += 2;
    }
    while(isize < 10) {
      cpudebug_printf("     ");
      isize += 2;
    }
    cpudebug_printf("%s\n", sdebug);
  }
}

static void cpudebug_hexdump(void) {
  byte c, tmpchar[16];
  dword tmpaddr;
  int i, j, k;
  tmpaddr = hexaddr & 0xFFFFFFF0;
  for(i = 0; i < 8; i++) {
    cpudebug_printf("%08X: %c", tmpaddr,
      (hexaddr == tmpaddr) ? '>' : ' '
    );
    for(j = 0; j < 16; j += 2) {
      k = s68000fetch(starcontext,tmpaddr) & 0xFFFF;
      tmpchar[j    ] = k >> 8;
      tmpchar[j + 1] = k & 0xFF;
      tmpaddr += 2;
      cpudebug_printf("%02X%02X%c",
        tmpchar[j], tmpchar[j + 1],
        (( hexaddr            == tmpaddr   )&&(j!=14))?'>':
        (((hexaddr&0xFFFFFFFE)==(tmpaddr-2))?'<':' ')
      );
    }
    cpudebug_printf("  ");
    for(j = 0; j < 16; j++) {
      c = tmpchar[j];
      if((c<32)||(c>126))c='.';
      cpudebug_printf("%c", c);
    }
    cpudebug_printf("\n");
  }
  hexaddr += 0x80;
}

static void cpudebug_registerdump(void) {
  int i;
  unsigned dreg[8];
  unsigned areg[8];
  unsigned asp;
  unsigned sr;
  unsigned pc;

  for(i=0;i<8;i++) {
    dreg[i] = s68000readReg(starcontext, STARSCREAM_REG_DATA+i);
    areg[i] = s68000readReg(starcontext, STARSCREAM_REG_ADDRESS+i);
  }
  asp = s68000readReg(starcontext, STARSCREAM_REG_ASP);
  sr = s68000readSR(starcontext);
  pc = s68000readPC(starcontext);

  cpudebug_printf(
    "d0=%08X   d4=%08X   a0=%08X   a4=%08X   %c%c%c%c%c\n",
    dreg[0],dreg[4],
    areg[0],areg[4],
    (sr >> 4) & 1 ? 'X' : '-',
    (sr >> 3) & 1 ? 'N' : '-',
    (sr >> 2) & 1 ? 'Z' : '-',
    (sr >> 1) & 1 ? 'V' : '-',
    (sr     ) & 1 ? 'C' : '-'
  );
  cpudebug_printf(
    "d1=%08X   d5=%08X   a1=%08X   a5=%08X   intmask=%d\n",
      dreg[1],   dreg[5],
      areg[1],   areg[5],
    (  sr >> 8) & 7
  );
  cpudebug_printf(
    "d2=%08X   d6=%08X   a2=%08X   a6=%08X   ",
      dreg[2],   dreg[6],
      areg[2],   areg[6]
  );
  for(i = 1; i <= 7; i++) {
    if(  interrupts[0] & (1 << i)) {
      cpudebug_printf("%02X",   interrupts[i]);
    } else {
      cpudebug_printf("--");
    }
  }
  cpudebug_printf(
    "\nd3=%08X   d7=%08X   a3=%08X   a7=%08X   %csp=%08X\n",
      dreg[3],   dreg[7],
      areg[3],   areg[7],
    ((  sr)&0x2000)?'u':'s',   asp
  );
  debugpc = pc;
  cpudebug_disassemble(1);
  debugpc = pc;
}

int cpudebug_interactive(
  int cpun,
  void (*execstep)(void *execcontext),
  void *execcontext,
  void *scontext
) {
  char inputstr[80];
  char *cmd, *args, *argsend;
  dword tmppc;
  hexaddr = 0;
  starcontext = scontext;
  for(;;) {
    s68000flushInterrupts(starcontext);
    cpudebug_printf("cpu%d-", cpun);
    cpudebug_gets(inputstr, sizeof(inputstr));
    cmd = inputstr;
    while((tolower(*cmd) < 'a') && (tolower(*cmd) > 'z')) {
      if(!(*cmd)) break;
      cmd++;
    }
    if(!(*cmd)) continue;
    *cmd = tolower(*cmd);
    args = cmd + 1;
    while((*args) && ((*args) < 32)) args++;
    switch(*cmd) {
    case '?':
      cpudebug_printf(
"b [address]           Run continuously, break at PC=[address]\n"
"d [address]           Dump memory, starting at [address]\n"
"i [number]            Generate hardware interrupt [number]\n"
"q                     Quit\n"
"r                     Show register dump and next instruction\n"
"s [n]                 Switch to CPU [n]\n"
"t [hex number]        Trace through [hex number] instructions\n"
"u [address]           Unassemble code, starting at [address]\n"
      );
      break;
    case 'b':
      if(*args) {
        tmppc = strtoul(args, &argsend, 16);
        if(argsend != args) {
          while(s68000readPC(starcontext) != tmppc)
          if(execstep) execstep(execcontext);
          else s68000exec(starcontext, 1);
          cpudebug_registerdump();
        } else {
          cpudebug_printf("Invalid address\n");
        }
      } else {
        cpudebug_printf("Need an address\n");
      }
      break;
    case 'u':
      if(*args) {
        tmppc = strtoul(args, &argsend, 16);
        if(argsend != args) debugpc = tmppc;
      }
      cpudebug_disassemble(16);
      break;
    case 'd':
      if(*args) {
        tmppc = strtoul(args,&argsend,16);
        if(argsend != args) hexaddr = tmppc;
      }
      cpudebug_hexdump();
      break;
    case 'i':
      if(*args) {
        tmppc = strtoul(args, &argsend, 10);
        if(argsend != args) {
          cpudebug_printf("Interrupt %d generated\n", tmppc);
          s68000interrupt(starcontext, tmppc, -1);
          s68000flushInterrupts(starcontext);
          cpudebug_registerdump();
          debugpc = s68000readPC(starcontext);
        } else {
          cpudebug_printf("Invalid interrupt number\n");
        }
      } else {
        cpudebug_printf("Need an interrupt number\n");
      }
      break;
    case 't':
      tmppc = 1;
      if(*args) {
        tmppc = strtoul(args, &argsend, 16);
        if(argsend == args) tmppc = 1;
      }
      if(tmppc > 0) {
        while(tmppc--) {
          if(execstep) execstep();
          else s68000exec(starcontext, 1);
        }
        cpudebug_registerdump();
      }
      break;
    case 'r':
      cpudebug_registerdump();
      break;
    case 'q':
      return -1;
    case 's':
      if(*args) {
        tmppc = strtoul(args, &argsend, 10);
        if(tmppc > 0) return tmppc;
        else cpudebug_printf("Invalid CPU number\n");
      } else return 0;
      break;
    default:
/*      cpudebug_printf("Unknown command\n");*/
      break;
    }
  }
  return -1;
}
