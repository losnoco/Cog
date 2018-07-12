/******************************************************************************\
* Project:  RSP Disassembler                                                   *
* Authors:  Iconoclast                                                         *
* Release:  2013.09.12                                                         *
* License:  CC0 Public Domain Dedication                                       *
*                                                                              *
* To the extent possible under law, the author(s) have dedicated all copyright *
* and related and neighboring rights to this software to the public domain     *
* worldwide. This software is distributed without any warranty.                *
*                                                                              *
* You should have received a copy of the CC0 Public Domain Dedication along    *
* with this software.                                                          *
* If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.             *
\******************************************************************************/

#ifndef MATRIX_H
#define MATRIX_H

#define reserved_       "illegal"
/* Or, "invalid".  Change it to whatever, but it must NOT exceed 7 letters. */

static const char mnemonics_PRIMARY[64][8] = {
"SPECIAL","REGIMM ","J      ","JAL    ","BEQ    ","BNE    ","BLEZ   ","BGTZ   ",
"ADDI   ","ADDIU  ","SLTI   ","SLTIU  ","ANDI   ","ORI    ","XORI   ","LUI    ",
"COP0   ",reserved_,"COP2   ",reserved_,reserved_,reserved_,reserved_,reserved_,
reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,
"LB     ","LH     ",reserved_,"LW     ","LBU    ","LHU    ",reserved_,reserved_,
"SB     ","SH     ",reserved_,"SW     ",reserved_,reserved_,reserved_,reserved_,
reserved_,reserved_,"LWC2   ",reserved_,reserved_,reserved_,reserved_,reserved_,
reserved_,reserved_,"SWC2   ",reserved_,reserved_,reserved_,reserved_,reserved_
};
static const char mnemonics_SPECIAL[64][8] = {
"SLL    ",reserved_,"SRL    ","SRA    ","SLLV   ",reserved_,"SRLV   ","SRAV   ",
"JR     ","JALR   ",reserved_,reserved_,reserved_,"BREAK  ",reserved_,reserved_,
reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,
reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,
"ADD    ","ADDU   ","SUB    ","SUBU   ","AND    ","OR     ","XOR    ","NOR    ",
reserved_,reserved_,"SLT    ","SLTU   ",reserved_,reserved_,reserved_,reserved_,
reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,
reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_
};
static const char mnemonics_REGIMM[32][8] = {
"BLTZ   ","BGEZ   ",reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,
reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,
"BLTZAL ","BGEZAL ",reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,
reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_
};
static const char mnemonics_COP0[32][8] = {
"MFC0   ",reserved_,reserved_,reserved_,"MTC0   ",reserved_,reserved_,reserved_,
reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,
reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,
reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_
};
static const char mnemonics_COP2[32][8] = {
"MFC2   ",reserved_,"CFC2   ",reserved_,"MTC2   ",reserved_,"CTC2   ",reserved_,
reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,
"C2     ","C2     ","C2     ","C2     ","C2     ","C2     ","C2     ","C2     ",
"C2     ","C2     ","C2     ","C2     ","C2     ","C2     ","C2     ","C2     "
};
static const char mnemonics_LWC2[32][8] = {
"LBV    ","LSV    ","LLV    ","LDV    ","LQV    ","LRV    ","LPV    ","LUV    ",
"LHV    ","LFV    ",reserved_,"LTV    ",reserved_,reserved_,reserved_,reserved_,
reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,
reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_
};
static const char mnemonics_SWC2[32][8] = {
"SBV    ","SSV    ","SLV    ","SDV    ","SQV    ","SRV    ","SPV    ","SUV    ",
"SHV    ","SFV    ","SWV    ","STV    ",reserved_,reserved_,reserved_,reserved_,
reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,
reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_
};
static const char mnemonics_C2[64][8] = {
"VMULF  ","VMULU  ",reserved_,reserved_,"VMUDL  ","VMUDM  ","VMUDN  ","VMUDH  ",
"VMACF  ","VMACU  ",reserved_,"VMACQ  ","VMADL  ","VMADM  ","VMADN  ","VMADH  ",
"VADD   ","VSUB   ",reserved_,"VABS   ","VADDC  ","VSUBC  ",reserved_,reserved_,
reserved_,reserved_,reserved_,reserved_,reserved_,"VSAW   ",reserved_,reserved_,
"VLT    ","VEQ    ","VNE    ","VGE    ","VCL    ","VCH    ","VCR    ","VMRG   ",
"VAND   ","VNAND  ","VOR    ","VNOR   ","VXOR   ","VNXOR  ",reserved_,reserved_,
"VRCP   ","VRCPL  ","VRCPH  ","VMOV   ","VRSQ   ","VRSQL  ","VRSQH  ","VNOP   ",
reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_,reserved_
};

/*
 * This is a dynamic-style disassembler.  If it was really coded entirely for
 * speed then the instruction decoding fetching would be more static,
 * but this was mostly written to be accurate and small on size, not fastest.
 */

static const char tokens_CR_V[4][4] = {
    "vco",
    "vcc",
    "vce", "vce" /* exception override:  only three control registers */
};
static const char* computational_elements[16] = {
    "", /* vector operand */
    "[]",
    "[0q]", "[1q]", /* scalar quarter */
    "[0h]", "[1h]", "[2h]", "[3h]", /* scalar half */
    "[0]", "[1]", "[2]", "[3]", "[4]", "[5]", "[6]", "[7]" /* scalar whole */
};
/*
 * Syntax above is strict RSP assembler:
 *     1.  The letters must be lower-case, not upper-case.
 *     2.  It is not legal to append a 'w' for the scalar whole elements.
 *     3.  e==0x1 is impossible to set in the assembler; "[]" is just for debug.
 */

void disassemble(char disasm[32], int IW)
{
    register int ID;
    unsigned short imm = (IW & 0x0000FFFF);
    const signed int offset = -(IW & 0x00008000) | imm;
    const unsigned int target = IW%0x04000000 << 2;
    const int func = IW % 64;
    const int sa = (IW >>  6) & 31;
    const int rd = (IW & 0x0000FFFF) >> 11;
    const int rt = (IW >> 16) & 31;
    const int rs = (IW >> 21) & 31;
    const int op = (IW >> 26) & 63;

    if ((op & ~001) == 000) /* SPECIAL/REGIMM */
        ID = op + 1;
    else if ((op & 075) == 020) /* COPz */
        ID = (op & 002) ? 04 + ((IW & 0x02000000) ? 3 : 0) : 03;
    else if ((op & 067) == 062) /* ?WC2 */
        ID = 05 + ((op & 010) ? 1 : 0);
    else
        ID = 00;

    ID = (ID % 8) & 07; /* Help compiler see this as an aligned jump: */
    switch (ID)
    {
        char opcode[8];

        case 00:
            strcpy(opcode, mnemonics_PRIMARY[op]);
            if (op & 32) /* op > 31:  scalar loads and stores */
                sprintf(disasm, "%s $%u, %i($%u)", opcode, rt, offset, rs);
            else if (op & 8) /* 16 > op > 8:  arithmetic/logical immediate op */
                if (op == 0xF) /* LUI does not encode rs. */
                    sprintf(disasm, "%s $%u, 0x%04X", opcode, rt, imm);
                else
                    sprintf(disasm, "%s $%u, $%u, 0x%04X", opcode, rt, rs, imm);
            else if (op & 4) /* 8 > op > 4:  primary branches */
                if (op & 2) /* BLEZ, BGTZ */
                    sprintf(disasm, "%s $%u, %i", opcode, rs, offset);
                else /* BEQ, BNE */
                    sprintf(disasm, "%s $%u, $%u, %i", opcode, rs, rt, offset);
            else if (op & 2) /* J and JAL */
                sprintf(disasm, "%s 0x%07X", opcode, target);
            else /* RESERVED */
                sprintf(disasm, "%s:%08X", opcode, IW);
            return;
        case 01:
            strcpy(opcode, mnemonics_SPECIAL[func]);
            if (func & 040) /* func > 31:  arithmetic/logic R-op (omit traps) */
                sprintf(disasm, "%s $%u, $%u, $%u", opcode, rd, rs, rt);
            else if (func < 8) /* scalar shifts */
                if (func & 4) /* variable */
                    sprintf(disasm, "%s $%u, $%u, $%u", opcode, rd, rt, rs);
                else
                    sprintf(disasm, "%s $%u, $%u, %u", opcode, rd, rt, sa);
            else if (func == 010) /* JR */
                sprintf(disasm, "%s $%u", opcode, rs);
            else if (func == 011) /* JALR */
                sprintf(disasm, "%s $%u, $%u", opcode, rd, rs);
            else if (func == 015) /* BREAK */
                strcpy(disasm, opcode); /* sprintf "BREAK" + secret code mask */
            else /* RESERVED */
                sprintf(disasm, "%s:%08X", opcode, IW);
            return;
        case 02:
            strcpy(opcode, mnemonics_REGIMM[rt]);
            if ((rt & 016) == 000) /* BLTZ[AL] and BGEZ[AL] */
                sprintf(disasm, "%s $%u, %i", opcode, rs, offset);
            else /* RESERVED */
                sprintf(disasm, "%s:%08X", opcode, IW);
            return;
        case 03:
            strcpy(opcode, mnemonics_COP0[rs]);
            if ((rs & 033) != 000)
                sprintf(disasm, "%s:%08X", opcode, IW); /* RESERVED */
            else /* M?C0 */
                sprintf(disasm, "%s $%u, $c%u", opcode, rt, rd & 0xF);
            return;
        case 04:
            strcpy(opcode, mnemonics_COP2[rs]);
            if (opcode[0] == 'M') /* M?C2 */
                sprintf(disasm, "%s $%u, $v%u[0x%X]", opcode, rt, rd, sa >> 1);
            else if (opcode[0] == 'C') /* C?C2 */
                sprintf(disasm, "%s $%u, $%s", opcode, rt, tokens_CR_V[rd % 4]);
            else /* RESERVED */
                sprintf(disasm, "%s:%08X", opcode, IW);
            return;
        case 05:
            strcpy(opcode, mnemonics_LWC2[rd]);
            if (opcode[0] != 'L')
                sprintf(disasm, "%s:%08X", opcode, IW); /* RESERVED */
            else
                sprintf(disasm, "%s $v%u[0x%X], %i($%u)", opcode, rt, sa >> 1,
                    -(IW & 0x00000040) | func, rs);
            return;
        case 06:
            strcpy(opcode, mnemonics_SWC2[rd]);
            if (opcode[0] != 'S')
                sprintf(disasm, "%s:%08X", opcode, IW); /* RESERVED */
            else
                sprintf(disasm, "%s $v%u[0x%X], %i($%u)", opcode, rt, sa >> 1,
                    -(IW & 0x00000040) | func, rs);
            return;
        case 07:
            strcpy(opcode, mnemonics_C2[func]);
            if (opcode[0] != 'V')
                sprintf(disasm, "%s:%08X", opcode, IW); /* RESERVED */
            else if (func == 067) /* VNOP */
                strcpy(disasm, opcode);
            else if (func >= 060) /* VRCP?, VRSQ?, and VMOV */
                sprintf(disasm, "%s $v%u[%u], $v%u[0x%X]", opcode, sa, rd & 07,
                    rt, rs & 0xF);
            else /* the most common vectors */
                if (func == 035) /* VSAR "VSAW", unconditional element syntax */
                    sprintf(disasm, "%s $v%u, $v%u, $v%u[%u]", opcode, sa, rd,
                        rt, rs & 07); /* rs&7:  00"High", 01"Middle", 02"Low" */
                else /* conditional element syntax:  e != 0x0 */
                    sprintf(disasm, "%s $v%u, $v%u, $v%u%s", opcode, sa, rd, rt,
                        computational_elements[rs & 0xF]);
            return;
    }
}

#endif
