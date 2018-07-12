/******************************************************************************\
* Authors:  Iconoclast                                                         *
* Release:  2013.12.11                                                         *
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
#include "rsp.h"

#include "su.h"
#include "vu/vu.h"

#include "r4300/interupt.h"

#define FIT_IMEM(PC)    (PC & 0xFFF & 0xFFC)

NOINLINE void run_task(usf_state_t * state)
{
    register int PC;
    int wrap_count = 0;
#ifdef SP_EXECUTE_LOG
    int last_PC;
#endif

    if (CFG_WAIT_FOR_CPU_HOST != 0)
    {
        register int i;

        for (i = 0; i < 32; i++)
            state->MFC0_count[i] = 0;
    }
    PC = FIT_IMEM(state->g_sp.regs2[SP_PC_REG]);
    while ((state->g_sp.regs[SP_STATUS_REG] & 0x00000001) == 0x00000000)
    {
        register uint32_t inst;

        inst = *(uint32_t *)(state->IMEM + FIT_IMEM(PC));
#ifdef SP_EXECUTE_LOG
        last_PC = PC;
#endif
#ifdef EMULATE_STATIC_PC
        PC = (PC + 0x004);
        if ( FIT_IMEM(PC) == 0 && ++wrap_count == 32 )
        {
            message( state, "RSP execution presumably caught in an infinite loop", 3 );
            break;
        }
EX:
#endif
#ifdef SP_EXECUTE_LOG
        step_SP_commands(state, last_PC, inst);
#endif
        if (inst >> 25 == 0x25) /* is a VU instruction */
        {
            const int opcode = inst % 64; /* inst.R.func */
            const int vd = (inst & 0x000007FF) >> 6; /* inst.R.sa */
            const int vs = (unsigned short)(inst) >> 11; /* inst.R.rd */
            const int vt = (inst >> 16) & 31; /* inst.R.rt */
            const int e  = (inst >> 21) & 0xF; /* rs & 0xF */

            COP2_C2[opcode](state, vd, vs, vt, e);
        }
        else
        {
            const int op = inst >> 26;
            const int rs = inst >> 21; /* &= 31 */
            const int rt = (inst >> 16) & 31;
            const int rd = (unsigned short)(inst) >> 11;
            const int element = (inst & 0x000007FF) >> 7;
            const int base = (inst >> 21) & 31;

#if (0)
            state->SR[0] = 0x00000000; /* already handled on per-instruction basis */
#endif
            switch (op)
            {
                signed int offset;
                register uint32_t addr;

                case 000: /* SPECIAL */
                    switch (inst % 64)
                    {
                        case 000: /* SLL */
                            state->SR[rd] = state->SR[rt] << MASK_SA(inst >> 6);
                            state->SR[0] = 0x00000000;
                            CONTINUE
                        case 002: /* SRL */
                            state->SR[rd] = (unsigned)(state->SR[rt]) >> MASK_SA(inst >> 6);
                            state->SR[0] = 0x00000000;
                            CONTINUE
                        case 003: /* SRA */
                            state->SR[rd] = (signed)(state->SR[rt]) >> MASK_SA(inst >> 6);
                            state->SR[0] = 0x00000000;
                            CONTINUE
                        case 004: /* SLLV */
                            state->SR[rd] = state->SR[rt] << MASK_SA(state->SR[rs]);
                            state->SR[0] = 0x00000000;
                            CONTINUE
                        case 006: /* SRLV */
                            state->SR[rd] = (unsigned)(state->SR[rt]) >> MASK_SA(state->SR[rs]);
                            state->SR[0] = 0x00000000;
                            CONTINUE
                        case 007: /* SRAV */
                            state->SR[rd] = (signed)(state->SR[rt]) >> MASK_SA(state->SR[rs]);
                            state->SR[0] = 0x00000000;
                            CONTINUE
                        case 011: /* JALR */
                            state->SR[rd] = (PC + LINK_OFF) & 0x00000FFC;
                            state->SR[0] = 0x00000000;
                        case 010: /* JR */
                            set_PC(state, state->SR[rs]);
                            JUMP
                        case 015: /* BREAK */
                            state->g_sp.regs[SP_STATUS_REG] |= 0x00000003; /* BROKE | HALT */
                            if (state->g_sp.regs[SP_STATUS_REG] & 0x00000040)
                            { /* SP_STATUS_INTR_BREAK */
                                state->g_r4300.mi.regs[MI_INTR_REG] |= 0x00000001;
                                //check_interupt(state);
                            }
                            CONTINUE
                        case 040: /* ADD */
                        case 041: /* ADDU */
                            state->SR[rd] = state->SR[rs] + state->SR[rt];
                            state->SR[0] = 0x00000000; /* needed for Rareware ucodes */
                            CONTINUE
                        case 042: /* SUB */
                        case 043: /* SUBU */
                            state->SR[rd] = state->SR[rs] - state->SR[rt];
                            state->SR[0] = 0x00000000;
                            CONTINUE
                        case 044: /* AND */
                            state->SR[rd] = state->SR[rs] & state->SR[rt];
                            state->SR[0] = 0x00000000; /* needed for Rareware ucodes */
                            CONTINUE
                        case 045: /* OR */
                            state->SR[rd] = state->SR[rs] | state->SR[rt];
                            state->SR[0] = 0x00000000;
                            CONTINUE
                        case 046: /* XOR */
                            state->SR[rd] = state->SR[rs] ^ state->SR[rt];
                            state->SR[0] = 0x00000000;
                            CONTINUE
                        case 047: /* NOR */
                            state->SR[rd] = ~(state->SR[rs] | state->SR[rt]);
                            state->SR[0] = 0x00000000;
                            CONTINUE
                        case 052: /* SLT */
                            state->SR[rd] = ((signed)(state->SR[rs]) < (signed)(state->SR[rt]));
                            state->SR[0] = 0x00000000;
                            CONTINUE
                        case 053: /* SLTU */
                            state->SR[rd] = ((unsigned)(state->SR[rs]) < (unsigned)(state->SR[rt]));
                            state->SR[0] = 0x00000000;
                            CONTINUE
                        default:
                            res_S(state);
                            CONTINUE
                    }
                    CONTINUE
                case 001: /* REGIMM */
                    switch (rt)
                    {
                        case 020: /* BLTZAL */
                            state->SR[31] = (PC + LINK_OFF) & 0x00000FFC;
                        case 000: /* BLTZ */
                            if (!(state->SR[base] < 0))
                                CONTINUE
                            set_PC(state, PC + 4*inst + SLOT_OFF);
                            JUMP
                        case 021: /* BGEZAL */
                            state->SR[31] = (PC + LINK_OFF) & 0x00000FFC;
                        case 001: /* BGEZ */
                            if (!(state->SR[base] >= 0))
                                CONTINUE
                            set_PC(state, PC + 4*inst + SLOT_OFF);
                            JUMP
                        default:
                            res_S(state);
                            CONTINUE
                    }
                    CONTINUE
                case 003: /* JAL */
                    state->SR[31] = (PC + LINK_OFF) & 0x00000FFC;
                case 002: /* J */
                    set_PC(state, 4*inst);
                    JUMP
                case 004: /* BEQ */
                    if (!(state->SR[base] == state->SR[rt]))
                        CONTINUE
                    set_PC(state, PC + 4*inst + SLOT_OFF);
                    JUMP
                case 005: /* BNE */
                    if (!(state->SR[base] != state->SR[rt]))
                        CONTINUE
                    set_PC(state, PC + 4*inst + SLOT_OFF);
                    JUMP
                case 006: /* BLEZ */
                    if (!((signed)state->SR[base] <= 0x00000000))
                        CONTINUE
                    set_PC(state, PC + 4*inst + SLOT_OFF);
                    JUMP
                case 007: /* BGTZ */
                    if (!((signed)state->SR[base] >  0x00000000))
                        CONTINUE
                    set_PC(state, PC + 4*inst + SLOT_OFF);
                    JUMP
                case 010: /* ADDI */
                case 011: /* ADDIU */
                    state->SR[rt] = state->SR[base] + (signed short)(inst);
                    state->SR[0] = 0x00000000;
                    CONTINUE
                case 012: /* SLTI */
                    state->SR[rt] = ((signed)(state->SR[base]) < (signed short)(inst));
                    state->SR[0] = 0x00000000;
                    CONTINUE
                case 013: /* SLTIU */
                    state->SR[rt] = ((unsigned)(state->SR[base]) < (unsigned short)(inst));
                    state->SR[0] = 0x00000000;
                    CONTINUE
                case 014: /* ANDI */
                    state->SR[rt] = state->SR[base] & (unsigned short)(inst);
                    state->SR[0] = 0x00000000;
                    CONTINUE
                case 015: /* ORI */
                    state->SR[rt] = state->SR[base] | (unsigned short)(inst);
                    state->SR[0] = 0x00000000;
                    CONTINUE
                case 016: /* XORI */
                    state->SR[rt] = state->SR[base] ^ (unsigned short)(inst);
                    state->SR[0] = 0x00000000;
                    CONTINUE
                case 017: /* LUI */
                    state->SR[rt] = inst << 16;
                    state->SR[0] = 0x00000000;
                    CONTINUE
                case 020: /* COP0 */
                    switch (base)
                    {
                        case 000: /* MFC0 */
                            MFC0(state, rt, rd & 0xF);
                            CONTINUE
                        case 004: /* MTC0 */
                            MTC0[rd & 0xF](state, rt);
                            CONTINUE
                        default:
                            res_S(state);
                            CONTINUE
                    }
                    CONTINUE
                case 022: /* COP2 */
                    switch (base)
                    {
                        case 000: /* MFC2 */
                            MFC2(state, rt, rd, element);
                            CONTINUE
                        case 002: /* CFC2 */
                            CFC2(state, rt, rd);
                            CONTINUE
                        case 004: /* MTC2 */
                            MTC2(state, rt, rd, element);
                            CONTINUE
                        case 006: /* CTC2 */
                            CTC2(state, rt, rd);
                            CONTINUE
                        default:
                            res_S(state);
                            CONTINUE
                    }
                    CONTINUE
                case 040: /* LB */
                    offset = (signed short)(inst);
                    addr = (state->SR[base] + offset) & 0x00000FFF;
                    state->SR[rt] = state->DMEM[BES(addr)];
                    state->SR[rt] = (signed char)(state->SR[rt]);
                    state->SR[0] = 0x00000000;
                    CONTINUE
                case 041: /* LH */
                    offset = (signed short)(inst);
                    addr = (state->SR[base] + offset) & 0x00000FFF;
                    if (addr%0x004 == 0x003)
                    {
                        SR_B(rt, 2) = state->DMEM[addr - BES(0x000)];
                        addr = (addr + 0x00000001) & 0x00000FFF;
                        SR_B(rt, 3) = state->DMEM[addr + BES(0x000)];
                        state->SR[rt] = (signed short)(state->SR[rt]);
                    }
                    else
                    {
                        addr -= HES(0x000)*(addr%0x004 - 1);
                        state->SR[rt] = *(signed short *)(state->DMEM + addr);
                    }
                    state->SR[0] = 0x00000000;
                    CONTINUE
                case 043: /* LW */
                    offset = (signed short)(inst);
                    addr = (state->SR[base] + offset) & 0x00000FFF;
                    if (addr%0x004 != 0x000)
                        ULW(state, rt, addr);
                    else
                        state->SR[rt] = *(int32_t *)(state->DMEM + addr);
                    state->SR[0] = 0x00000000;
                    CONTINUE
                case 044: /* LBU */
                    offset = (signed short)(inst);
                    addr = (state->SR[base] + offset) & 0x00000FFF;
                    state->SR[rt] = state->DMEM[BES(addr)];
                    state->SR[rt] = (unsigned char)(state->SR[rt]);
                    state->SR[0] = 0x00000000;
                    CONTINUE
                case 045: /* LHU */
                    offset = (signed short)(inst);
                    addr = (state->SR[base] + offset) & 0x00000FFF;
                    if (addr%0x004 == 0x003)
                    {
                        SR_B(rt, 2) = state->DMEM[addr - BES(0x000)];
                        addr = (addr + 0x00000001) & 0x00000FFF;
                        SR_B(rt, 3) = state->DMEM[addr + BES(0x000)];
                        state->SR[rt] = (unsigned short)(state->SR[rt]);
                    }
                    else
                    {
                        addr -= HES(0x000)*(addr%0x004 - 1);
                        state->SR[rt] = *(unsigned short *)(state->DMEM + addr);
                    }
                    state->SR[0] = 0x00000000;
                    CONTINUE
                case 050: /* SB */
                    offset = (signed short)(inst);
                    addr = (state->SR[base] + offset) & 0x00000FFF;
                    state->DMEM[BES(addr)] = (unsigned char)(state->SR[rt]);
                    CONTINUE
                case 051: /* SH */
                    offset = (signed short)(inst);
                    addr = (state->SR[base] + offset) & 0x00000FFF;
                    if (addr%0x004 == 0x003)
                    {
                        state->DMEM[addr - BES(0x000)] = SR_B(rt, 2);
                        addr = (addr + 0x00000001) & 0x00000FFF;
                        state->DMEM[addr + BES(0x000)] = SR_B(rt, 3);
                        CONTINUE
                    }
                    addr -= HES(0x000)*(addr%0x004 - 1);
                    *(short *)(state->DMEM + addr) = (short)(state->SR[rt]);
                    CONTINUE
                case 053: /* SW */
                    offset = (signed short)(inst);
                    addr = (state->SR[base] + offset) & 0x00000FFF;
                    if (addr%0x004 != 0x000)
                        USW(state, rt, addr);
                    else
                        *(int32_t *)(state->DMEM + addr) = state->SR[rt];
                    CONTINUE
                case 062: /* LWC2 */
                    offset = SE(inst, 6);
                    switch (rd)
                    {
                        case 000: /* LBV */
                            LBV(state, rt, element, offset, base);
                            CONTINUE
                        case 001: /* LSV */
                            LSV(state, rt, element, offset, base);
                            CONTINUE
                        case 002: /* LLV */
                            LLV(state, rt, element, offset, base);
                            CONTINUE
                        case 003: /* LDV */
                            LDV(state, rt, element, offset, base);
                            CONTINUE
                        case 004: /* LQV */
                            LQV(state, rt, element, offset, base);
                            CONTINUE
                        case 005: /* LRV */
                            LRV(state, rt, element, offset, base);
                            CONTINUE
                        case 006: /* LPV */
                            LPV(state, rt, element, offset, base);
                            CONTINUE
                        case 007: /* LUV */
                            LUV(state, rt, element, offset, base);
                            CONTINUE
                        case 010: /* LHV */
                            LHV(state, rt, element, offset, base);
                            CONTINUE
                        case 011: /* LFV */
                            LFV(state, rt, element, offset, base);
                            CONTINUE
                        case 013: /* LTV */
                            LTV(state, rt, element, offset, base);
                            CONTINUE
                        default:
                            res_S(state);
                            CONTINUE
                    }
                    CONTINUE
                case 072: /* SWC2 */
                    offset = SE(inst, 6);
                    switch (rd)
                    {
                        case 000: /* SBV */
                            SBV(state, rt, element, offset, base);
                            CONTINUE
                        case 001: /* SSV */
                            SSV(state, rt, element, offset, base);
                            CONTINUE
                        case 002: /* SLV */
                            SLV(state, rt, element, offset, base);
                            CONTINUE
                        case 003: /* SDV */
                            SDV(state, rt, element, offset, base);
                            CONTINUE
                        case 004: /* SQV */
                            SQV(state, rt, element, offset, base);
                            CONTINUE
                        case 005: /* SRV */
                            SRV(state, rt, element, offset, base);
                            CONTINUE
                        case 006: /* SPV */
                            SPV(state, rt, element, offset, base);
                            CONTINUE
                        case 007: /* SUV */
                            SUV(state, rt, element, offset, base);
                            CONTINUE
                        case 010: /* SHV */
                            SHV(state, rt, element, offset, base);
                            CONTINUE
                        case 011: /* SFV */
                            SFV(state, rt, element, offset, base);
                            CONTINUE
                        case 012: /* SWV */
                            SWV(state, rt, element, offset, base);
                            CONTINUE
                        case 013: /* STV */
                            STV(state, rt, element, offset, base);
                            CONTINUE
                        default:
                            res_S(state);
                            CONTINUE
                    }
                    CONTINUE
                default:
                    res_S(state);
                    CONTINUE
            }
        }
#ifndef EMULATE_STATIC_PC
        if (state->stage == 2) /* branch phase of scheduler */
        {
            state->stage = 0*stage;
            PC = state->temp_PC & 0x00000FFC;
            state->g_sp.regs2[SP_PC_REG] = state->temp_PC;
        }
        else
        {
            state->stage = 2*state->stage; /* next IW in branch delay slot? */
            PC = (PC + 0x004) & 0xFFC;
            if ( FIT_IMEM(PC) == 0 && ++wrap_count == 32 )
            {
                message( state, "RSP execution presumably caught in an infinite loop", 3 );
                break;
            }
            state->g_sp.regs2[SP_PC_REG] = PC;
        }
        continue;
#else
        continue;
BRANCH:
        inst = *(uint32_t *)(state->IMEM + FIT_IMEM(PC));
#ifdef SP_EXECUTE_LOG
        last_PC = PC;
#endif
        PC = state->temp_PC & 0x00000FFC;
        goto EX;
#endif
    }
    state->g_sp.regs2[SP_PC_REG] = FIT_IMEM(PC);
    if (state->g_sp.regs[SP_STATUS_REG] & 0x00000002) /* normal exit, from executing BREAK */
        return;
    else if (state->g_r4300.mi.regs[MI_INTR_REG] & 0x00000001) /* interrupt set by MTC0 to break */
        /*check_interupt(state)*/;
    else if (CFG_WAIT_FOR_CPU_HOST != 0) /* plugin system hack to re-sync */
        {}
    else if (state->g_sp.regs[SP_SEMAPHORE_REG] != 0x00000000) /* semaphore lock fixes */
        {}
    else /* ??? unknown, possibly external intervention from CPU memory map */
    {
        message(state, "SP_SET_HALT", 3);
        return;
    }
    state->g_sp.regs[SP_STATUS_REG] &= ~0x00000001; /* CPU restarts with the correct SIGs. */
    return;
}
