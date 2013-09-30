/////////////////////////////////////////////////////////////////////////////
//
// r3000asm - R3000 quick assembler (no symbols, no macro instructions)
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EMU_COMPILE
#error "Hi I forgot to set EMU_COMPILE"
#endif

#include "r3000asm.h"

/////////////////////////////////////////////////////////////////////////////

enum {
  TOKEN_NONE,
  TOKEN_REG,
  TOKEN_C0REG,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_COMMA,
  TOKEN_NUMBER,
  TOKEN_UNKNOWN
};

/////////////////////////////////////////////////////////////////////////////

static int myisspace(unsigned char c) {
  return (c == 9 || c == 10 || c == 13 || c == 32);
}

static int myisdigit(unsigned char c) {
  return (c >= '0' && c <= '9');
}
static int myisalpha(unsigned char c) {
  return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

static int myisalnum(unsigned char c) {
  return (myisalpha(c) || myisdigit(c));
}

/////////////////////////////////////////////////////////////////////////////
//
// Returns TRUE on match
//
static int alphacompare(const char *text, const char *pattern) {
  for(; *pattern; pattern++, text++) {
    if(!(*text)) return 0;
    if(tolower(*text) != tolower(*pattern)) return 0;
  }
  // The key: The trailing text must be non-alnum
  if(myisalnum(*text)) return 0;
  return 1;
}

/////////////////////////////////////////////////////////////////////////////

static const char *scan(const char *text, uint32 *token, uint32 *subtoken) {
  const char *s;
  *token = TOKEN_UNKNOWN;
  //
  // Skip opening space
  //
  while(*text && myisspace(*text)) text++;
  s = text;
  //
  // Token: None
  //
  if(!(*s)) {
    *token = TOKEN_NONE;
    return text;
  }
  //
  // Token: Register
  //
  if(*s == '$') {
    int regnum = 0;
    s++;
    if(myisdigit(*s)) {
      regnum = *(s++) - '0';
      if(myisdigit(*s)) {
        regnum = (regnum * 10) + (*(s++) - '0');
      }
      if(regnum >= 32) return text;
    } else {
      char a = 0;
      char b = 0;
      if(*s) { a = tolower(*s); s++; }
      if(*s) { b = tolower(*s); s++; }
           if(a=='a'&&b=='t') { regnum =  1; }
      else if(a=='v'&&b=='0') { regnum =  2; }
      else if(a=='v'&&b=='1') { regnum =  3; }
      else if(a=='a'&&b=='0') { regnum =  4; }
      else if(a=='a'&&b=='1') { regnum =  5; }
      else if(a=='a'&&b=='2') { regnum =  6; }
      else if(a=='a'&&b=='3') { regnum =  7; }
      else if(a=='t'&&b=='0') { regnum =  8; }
      else if(a=='t'&&b=='1') { regnum =  9; }
      else if(a=='t'&&b=='2') { regnum = 10; }
      else if(a=='t'&&b=='3') { regnum = 11; }
      else if(a=='t'&&b=='4') { regnum = 12; }
      else if(a=='t'&&b=='5') { regnum = 13; }
      else if(a=='t'&&b=='6') { regnum = 14; }
      else if(a=='t'&&b=='7') { regnum = 15; }
      else if(a=='s'&&b=='0') { regnum = 16; }
      else if(a=='s'&&b=='1') { regnum = 17; }
      else if(a=='s'&&b=='2') { regnum = 18; }
      else if(a=='s'&&b=='3') { regnum = 19; }
      else if(a=='s'&&b=='4') { regnum = 20; }
      else if(a=='s'&&b=='5') { regnum = 21; }
      else if(a=='s'&&b=='6') { regnum = 22; }
      else if(a=='s'&&b=='7') { regnum = 23; }
      else if(a=='t'&&b=='8') { regnum = 24; }
      else if(a=='t'&&b=='9') { regnum = 25; }
      else if(a=='k'&&b=='0') { regnum = 26; }
      else if(a=='k'&&b=='1') { regnum = 27; }
      else if(a=='g'&&b=='p') { regnum = 28; }
      else if(a=='s'&&b=='p') { regnum = 29; }
      else if(a=='f'&&b=='p') { regnum = 30; }
      else if(a=='r'&&b=='a') { regnum = 31; }
      else { return text; }
    }
    *token = TOKEN_REG;
    *subtoken = regnum;
    return s;
  }
  //
  // Token: C0 register
  //
  if(tolower(s[0]) == 'C' && s[1] == '0' && s[2] == '_') {
    int regnum = 0;
    s += 3;
    if(myisdigit(*s)) {
      regnum = *(s++) - '0';
      if(myisdigit(*s)) {
        regnum = (regnum * 10) + (*(s++) - '0');
      }
      if(regnum >= 32) return text;
    } else {
           if(alphacompare(s, "status")) { regnum = 12; }
      else if(alphacompare(s, "cause" )) { regnum = 13; }
      else if(alphacompare(s, "epc"   )) { regnum = 14; }
      else { return text; }
    }
    *token = TOKEN_C0REG;
    *subtoken = regnum;
    return s;
  }
  //
  // Token: Various punctuation
  //
  switch(*s) {
  case ',': s++; *token = TOKEN_COMMA ; return s;
  case '(': s++; *token = TOKEN_LPAREN; return s;
  case ')': s++; *token = TOKEN_RPAREN; return s;
  }
  //
  // Token: Number
  //
  if(myisdigit(*s) || *s == '-') {
    int bNegative = 0;
    int radix = 10;
    int num = 0;
    if(*s == '-') { bNegative = 1; s++; }
    if(*s == '0') {
      s++;
      if(tolower(*s) == 'x') {
        radix = 16;
        s++;
      } else {
        radix = 8;
      }
    }
    for(;;) {
      int digit = 0;
      char c = *s;
      if(!c) break;
           if(c >= '0' && c <= '9') { digit =      c - '0'; }
      else if(c >= 'a' && c <= 'f') { digit = 10 + c - 'a'; }
      else if(c >= 'A' && c <= 'F') { digit = 10 + c - 'A'; }
      else { break; }
      if(digit >= radix) return text;
      num *= radix;
      num += digit;
      s++;
    }
    if(bNegative) { num = -num; }
    *token = TOKEN_NUMBER;
    *subtoken = num;
    return s;
  }
  //
  // Token was unknown
  //
  return text;
}

/////////////////////////////////////////////////////////////////////////////

static void replaceinsfield(uint32 *ins, int nBitPosition, int nBits, uint32 dwValue) {
  uint32 dwMask = (1 << nBits) - 1;
  dwValue &= dwMask;
  (*ins) &= ~(dwMask  << nBitPosition);
  (*ins) |=  (dwValue << nBitPosition);
}

/////////////////////////////////////////////////////////////////////////////

static const char *assemblepattern(
  const char *pattern,
  uint32 pc,
  const char *text,
  uint32 *ins
) {
  uint32 token;
  uint32 subtoken;
  for(;;) {
    char p = *pattern++;
    text = scan(text, &token, &subtoken);
    switch(p) {
    case 0:
      if(token != TOKEN_NONE) return "Expected end-of-line";
      return NULL;
    case ',':
      if(token != TOKEN_COMMA) return "Expected ','";
      break;
    case '(':
      if(token != TOKEN_LPAREN) return "Expected '('";
      break;
    case ')':
      if(token != TOKEN_RPAREN) return "Expected ')'";
      break;
    //
    // S is 21
    // T is 16
    // D is 11
    //
    case 'S':
      if(token != TOKEN_REG) return "Expected register name";
      replaceinsfield(ins, 21, 5, subtoken);
      break;
    case 'T':
      if(token != TOKEN_REG) return "Expected register name";
      replaceinsfield(ins, 16, 5, subtoken);
      break;
    case 'D':
      if(token != TOKEN_REG) return "Expected register name";
      replaceinsfield(ins, 11, 5, subtoken);
      break;
    //
    // E = D and T
    // F = D and S
    // G = T and S
    //
    case 'E':
      if(token != TOKEN_REG) return "Expected register name";
      replaceinsfield(ins, 11, 5, subtoken);
      replaceinsfield(ins, 16, 5, subtoken);
      break;
    case 'F':
      if(token != TOKEN_REG) return "Expected register name";
      replaceinsfield(ins, 11, 5, subtoken);
      replaceinsfield(ins, 21, 5, subtoken);
      break;
    case 'G':
      if(token != TOKEN_REG) return "Expected register name";
      replaceinsfield(ins, 16, 5, subtoken);
      replaceinsfield(ins, 21, 5, subtoken);
      break;
    case 'H':
      if(token != TOKEN_NUMBER) return "Expected shift constant";
      replaceinsfield(ins, 6, 5, subtoken);
      break;
    case 'I':
      if(token != TOKEN_NUMBER) return "Expected number";
      replaceinsfield(ins, 0, 16, subtoken);
      break;
    case 'C':
      if(token != TOKEN_C0REG) return "Expected C0 register name";
      replaceinsfield(ins, 11, 5, subtoken);
      break;
    case 'J':
      if(token != TOKEN_NUMBER) return "Expected address";
      replaceinsfield(ins, 0, 26, subtoken>>2);
      break;
    case 'B':
      if(token != TOKEN_NUMBER) return "Expected address";
      replaceinsfield(ins, 0, 16, (subtoken-(pc+4))>>2);
      break;
    }
  }
}

/////////////////////////////////////////////////////////////////////////////

const char *assemblepatterns(
  const char *patterns,
  uint32 pc,
  const char *text,
  uint32 *ins,
  uint32 insTemplate
) {
  int i;
  const char *complaint = NULL;
  do {
    char pattemp[20];
    *ins = insTemplate;
    for(i = 0;; i++) {
      char c = *patterns;
      if(!c) break;
      patterns++;
      if(c == '/') break;
      pattemp[i] = c;
    }
    pattemp[i] = 0;
    complaint = assemblepattern(pattemp, pc, text, ins);
    if(!complaint) break;
  } while(*patterns);
  return complaint;
}

/////////////////////////////////////////////////////////////////////////////

static const char *assemble(uint32 pc, const char *text, uint32 *ins) {
  //
  // Skip opening space
  //
  while(*text && myisspace(*text)) text++;

  //
  // E = D and T
  // F = D and S
  // G = T and S
  //

  if(alphacompare(text, "nop"    )) return assemblepatterns(""         , pc, text + 3, ins, 0x00);

  if(alphacompare(text, "sll"    )) return assemblepatterns("E,H/D,T,H", pc, text + 3, ins, 0x00);
  if(alphacompare(text, "srl"    )) return assemblepatterns("E,H/D,T,H", pc, text + 3, ins, 0x02);
  if(alphacompare(text, "sra"    )) return assemblepatterns("E,H/D,T,H", pc, text + 3, ins, 0x03);
  if(alphacompare(text, "sllv"   )) return assemblepatterns("E,S/D,T,S", pc, text + 4, ins, 0x04);
  if(alphacompare(text, "srlv"   )) return assemblepatterns("E,S/D,T,S", pc, text + 4, ins, 0x06);
  if(alphacompare(text, "srav"   )) return assemblepatterns("E,S/D,T,S", pc, text + 4, ins, 0x07);

  if(alphacompare(text, "jr"     )) return assemblepatterns("S"        , pc, text + 2, ins, 0x08);
  if(alphacompare(text, "jalr"   )) return assemblepatterns("S/D,S"    , pc, text + 4, ins, 0x09|(0x1F<<11));

  if(alphacompare(text, "syscall")) return assemblepatterns(""         , pc, text + 7, ins, 0x0C);

  if(alphacompare(text, "mfhi"   )) return assemblepatterns("D"        , pc, text + 4, ins, 0x10);
  if(alphacompare(text, "mthi"   )) return assemblepatterns("D"        , pc, text + 4, ins, 0x11);
  if(alphacompare(text, "mflo"   )) return assemblepatterns("D"        , pc, text + 4, ins, 0x12);
  if(alphacompare(text, "mtlo"   )) return assemblepatterns("D"        , pc, text + 4, ins, 0x13);

  if(alphacompare(text, "mult"   )) return assemblepatterns("S,T"      , pc, text + 4, ins, 0x18);
  if(alphacompare(text, "multu"  )) return assemblepatterns("S,T"      , pc, text + 5, ins, 0x19);
  if(alphacompare(text, "div"    )) return assemblepatterns("S,T"      , pc, text + 3, ins, 0x1A);
  if(alphacompare(text, "divu"   )) return assemblepatterns("S,T"      , pc, text + 4, ins, 0x1B);

  if(alphacompare(text, "add"    )) return assemblepatterns("F,T/D,S,T", pc, text + 3, ins, 0x20);
  if(alphacompare(text, "addu"   )) return assemblepatterns("F,T/D,S,T", pc, text + 4, ins, 0x21);
  if(alphacompare(text, "sub"    )) return assemblepatterns("F,T/D,S,T", pc, text + 3, ins, 0x22);
  if(alphacompare(text, "subu"   )) return assemblepatterns("F,T/D,S,T", pc, text + 4, ins, 0x23);
  if(alphacompare(text, "and"    )) return assemblepatterns("F,T/D,S,T", pc, text + 3, ins, 0x24);
  if(alphacompare(text, "or"     )) return assemblepatterns("F,T/D,S,T", pc, text + 2, ins, 0x25);
  if(alphacompare(text, "xor"    )) return assemblepatterns("F,T/D,S,T", pc, text + 3, ins, 0x26);
  if(alphacompare(text, "nor"    )) return assemblepatterns("F,T/D,S,T", pc, text + 3, ins, 0x27);

  if(alphacompare(text, "slt"    )) return assemblepatterns("D,S,T"    , pc, text + 3, ins, 0x2A);
  if(alphacompare(text, "sltu"   )) return assemblepatterns("D,S,T"    , pc, text + 4, ins, 0x2B);

  if(alphacompare(text, "bltz"   )) return assemblepatterns("S,B"      , pc, text + 4, ins, (0x01<<26)|(0x00<<16));
  if(alphacompare(text, "bgez"   )) return assemblepatterns("S,B"      , pc, text + 4, ins, (0x01<<26)|(0x01<<16));
  if(alphacompare(text, "bltzal" )) return assemblepatterns("S,B"      , pc, text + 6, ins, (0x01<<26)|(0x10<<16));
  if(alphacompare(text, "bgezal" )) return assemblepatterns("S,B"      , pc, text + 6, ins, (0x01<<26)|(0x11<<16));

  if(alphacompare(text, "j"      )) return assemblepatterns("J"        , pc, text + 1, ins, (0x02<<26));
  if(alphacompare(text, "jal"    )) return assemblepatterns("J"        , pc, text + 3, ins, (0x03<<26));

  if(alphacompare(text, "beq"    )) return assemblepatterns("S,T,B"    , pc, text + 3, ins, (0x04<<26));
  if(alphacompare(text, "bne"    )) return assemblepatterns("S,T,B"    , pc, text + 3, ins, (0x05<<26));
  if(alphacompare(text, "beqz"   )) return assemblepatterns("S,B"      , pc, text + 4, ins, (0x04<<26));
  if(alphacompare(text, "bnez"   )) return assemblepatterns("S,B"      , pc, text + 4, ins, (0x05<<26));

  if(alphacompare(text, "blez"   )) return assemblepatterns("S,B"      , pc, text + 4, ins, (0x06<<26));
  if(alphacompare(text, "bgtz"   )) return assemblepatterns("S,B"      , pc, text + 4, ins, (0x07<<26));

  if(alphacompare(text, "addi"   )) return assemblepatterns("G,I/T,S,I", pc, text + 4, ins, (0x08<<26));
  if(alphacompare(text, "addiu"  )) return assemblepatterns("G,I/T,S,I", pc, text + 5, ins, (0x09<<26));
  if(alphacompare(text, "slti"   )) return assemblepatterns("T,S,I"    , pc, text + 4, ins, (0x0A<<26));
  if(alphacompare(text, "sltiu"  )) return assemblepatterns("T,S,I"    , pc, text + 5, ins, (0x0B<<26));

  if(alphacompare(text, "andi"   )) return assemblepatterns("G,I/T,S,I", pc, text + 4, ins, (0x0C<<26));
  if(alphacompare(text, "ori"    )) return assemblepatterns("G,I/T,S,I", pc, text + 3, ins, (0x0D<<26));
  if(alphacompare(text, "xori"   )) return assemblepatterns("G,I/T,S,I", pc, text + 4, ins, (0x0E<<26));
  if(alphacompare(text, "lui"    )) return assemblepatterns("T,I"      , pc, text + 3, ins, (0x0F<<26));

  if(alphacompare(text, "mfc0"   )) return assemblepatterns("T,C"      , pc, text + 4, ins, (0x10<<26)|(0x00<<21));
  if(alphacompare(text, "mtc0"   )) return assemblepatterns("T,C"      , pc, text + 4, ins, (0x10<<26)|(0x04<<21));
  if(alphacompare(text, "rfe"    )) return assemblepatterns(""         , pc, text + 3, ins, (0x10<<26)|(0x10<<21));

  if(alphacompare(text, "lb"     )) return assemblepatterns("T,(S)/T,I(S)", pc, text + 2, ins, (uint32)(0x20<<26));
  if(alphacompare(text, "lh"     )) return assemblepatterns("T,(S)/T,I(S)", pc, text + 2, ins, (uint32)(0x21<<26));
  if(alphacompare(text, "lw"     )) return assemblepatterns("T,(S)/T,I(S)", pc, text + 2, ins, (uint32)(0x23<<26));
  if(alphacompare(text, "lbu"    )) return assemblepatterns("T,(S)/T,I(S)", pc, text + 3, ins, (uint32)(0x24<<26));
  if(alphacompare(text, "lhu"    )) return assemblepatterns("T,(S)/T,I(S)", pc, text + 3, ins, (uint32)(0x25<<26));
  if(alphacompare(text, "sb"     )) return assemblepatterns("T,(S)/T,I(S)", pc, text + 2, ins, (uint32)(0x28<<26));
  if(alphacompare(text, "sh"     )) return assemblepatterns("T,(S)/T,I(S)", pc, text + 2, ins, (uint32)(0x29<<26));
  if(alphacompare(text, "sw"     )) return assemblepatterns("T,(S)/T,I(S)", pc, text + 2, ins, (uint32)(0x2B<<26));

  //
  // Friendly instructions like not, move, neg, negu, li
  // "li" always expands to a signed addiu - not an ori.
  //
  if(alphacompare(text, "move"   )) return assemblepatterns("D,T"  , pc, text + 4, ins, 0x21);
  if(alphacompare(text, "neg"    )) return assemblepatterns("E/D,T", pc, text + 3, ins, 0x22);
  if(alphacompare(text, "negu"   )) return assemblepatterns("E/D,T", pc, text + 4, ins, 0x23);
  if(alphacompare(text, "not"    )) return assemblepatterns("E/D,T", pc, text + 3, ins, 0x27);
  if(alphacompare(text, "li"     )) return assemblepatterns("T,I"  , pc, text + 2, ins, (0x09<<26));

  return "Unknown instuction name";
}

/////////////////////////////////////////////////////////////////////////////
//
// Returns negative on error (and fills the error string buffer)
// Must be 256 bytes in the error string buffer
//
sint32 EMU_CALL r3000asm(uint32 pc, const char *text, uint32 *ins, char *errorstring) {
  const char *complaint = assemble(pc, text, ins);
  if(complaint) {
    int i;
    for(i = 0; i < 255 && complaint[i]; i++) errorstring[i] = complaint[i];
    errorstring[i] = 0;
    return -1;
  }
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
