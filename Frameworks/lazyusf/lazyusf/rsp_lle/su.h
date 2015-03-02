/******************************************************************************\
* Project:  MSP Emulation Table for Scalar Unit Operations                     *
* Authors:  Iconoclast                                                         *
* Release:  2013.12.10                                                         *
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
#ifndef _SU_H
#define _SU_H

/*
 * RSP virtual registers (of scalar unit)
 * The most important are the 32 general-purpose scalar registers.
 * We have the convenience of using a 32-bit machine (Win32) to emulate
 * another 32-bit machine (MIPS/N64), so the most natural way to accurately
 * emulate the scalar GPRs is to use the standard `int` type.  Situations
 * specifically requiring sign-extension or lack thereof are forcibly
 * applied as defined in the MIPS quick reference card and user manuals.
 * Remember that these are not the same "GPRs" as in the MIPS ISA and totally
 * abandon their designated purposes on the master CPU host (the VR4300),
 * hence most of the MIPS names "k0, k1, t0, t1, v0, v1 ..." no longer apply.
 */

#include "rsp.h"

NOINLINE static void res_S(usf_state_t * state)
{
    (void)state;
    return;
}

#ifdef EMULATE_STATIC_PC
#define BASE_OFF    0x000
#else
#define BASE_OFF    0x004
#endif

#define SLOT_OFF    (BASE_OFF + 0x000)
#define LINK_OFF    (BASE_OFF + 0x004)
static void set_PC(usf_state_t * state, int address)
{
    state->temp_PC = (address & 0xFFC);
#ifndef EMULATE_STATIC_PC
    state->stage = 1;
#endif
    return;
}

#if ANDROID
#define MASK_SA(sa) (sa & 31)
/* Force masking in software. */
#else
#define MASK_SA(sa) (sa)
/* Let hardware architecture do the mask for us. */
#endif

#if (0)
#define ENDIAN   0
#else
#define ENDIAN  ~0
#endif
#define BES(address)    ((address) ^ ((ENDIAN) & 03))
#define HES(address)    ((address) ^ ((ENDIAN) & 02))
#define MES(address)    ((address) ^ ((ENDIAN) & 01))
#define WES(address)    ((address) ^ ((ENDIAN) & 00))
#define SR_B(s, i)      (*(byte *)(((byte *)(state->SR + s)) + BES(i)))
#define SR_S(s, i)      (*(short *)(((byte *)(state->SR + s)) + HES(i)))
#define SE(x, b)        (-((signed int)x & (1 << b)) | (x & ~(~0 << b)))
#define ZE(x, b)        (+(x & (1 << b)) | (x & ~(~0 << b)))

static void ULW(usf_state_t *, int rd, uint32_t addr);
static void USW(usf_state_t *, int rs, uint32_t addr);

/*
 * All other behaviors defined below this point in the file are specific to
 * the SGI N64 extension to the MIPS R4000 and are not entirely implemented.
 */

/*** Scalar, Coprocessor Operations (system control) ***/

static void MFC0(usf_state_t * state, int rt, int rd)
{
    state->SR[rt] = *(state->CR[rd]);
    state->SR[0] = 0x00000000;
    if (rd == 0x7) /* SP_SEMAPHORE_REG */
    {
        if (CFG_MEND_SEMAPHORE_LOCK == 0)
            return;
        state->g_sp.regs[SP_SEMAPHORE_REG] = 0x00000001;
        state->g_sp.regs[SP_STATUS_REG] |= 0x00000001; /* temporary bit to break CPU */
        return;
    }
    if (rd == 0x4) /* SP_STATUS_REG */
    {
        if (CFG_WAIT_FOR_CPU_HOST == 0)
            return;
#ifdef WAIT_FOR_CPU_HOST
        ++state->MFC0_count[rt];
        if (state->MFC0_count[rt] > 07)
            state->g_sp.regs[SP_STATUS_REG] |= 0x00000001; /* Let OS restart the task. */
#endif
    }
    return;
}

static void MT_DMA_CACHE(usf_state_t * state, int rt)
{
    state->g_sp.regs[SP_MEM_ADDR_REG] = state->SR[rt] & 0xFFFFFFF8; /* & 0x00001FF8 */
    return; /* Reserved upper bits are ignored during DMA R/W. */
}
static void MT_DMA_DRAM(usf_state_t * state, int rt)
{
    state->g_sp.regs[SP_DRAM_ADDR_REG] = state->SR[rt] & 0xFFFFFFF8; /* & 0x00FFFFF8 */
    return; /* Let the reserved bits get sent, but the pointer is 24-bit. */
}
void SP_DMA_READ(usf_state_t * state);
static void MT_DMA_READ_LENGTH(usf_state_t * state, int rt)
{
    state->g_sp.regs[SP_RD_LEN_REG] = state->SR[rt] | 07;
    SP_DMA_READ(state);
    return;
}
void SP_DMA_WRITE(usf_state_t * state);
static void MT_DMA_WRITE_LENGTH(usf_state_t * state, int rt)
{
    state->g_sp.regs[SP_WR_LEN_REG] = state->SR[rt] | 07;
    SP_DMA_WRITE(state);
    return;
}
static void MT_SP_STATUS(usf_state_t * state, int rt)
{
    if (state->SR[rt] & 0xFE000040)
        message(state, "MTC0\nSP_STATUS", 2);
    state->g_sp.regs[SP_STATUS_REG] &= ~(!!(state->SR[rt] & 0x00000001) <<  0);
    state->g_sp.regs[SP_STATUS_REG] |=  (!!(state->SR[rt] & 0x00000002) <<  0);
    state->g_sp.regs[SP_STATUS_REG] &= ~(!!(state->SR[rt] & 0x00000004) <<  1);
    state->g_r4300.mi.regs[MI_INTR_REG] &= ~((state->SR[rt] & 0x00000008) >> 3); /* SP_CLR_INTR */
    state->g_r4300.mi.regs[MI_INTR_REG] |=  ((state->SR[rt] & 0x00000010) >> 4); /* SP_SET_INTR */
    state->g_sp.regs[SP_STATUS_REG] |= (state->SR[rt] & 0x00000010) >> 4; /* int set halt */
    state->g_sp.regs[SP_STATUS_REG] &= ~(!!(state->SR[rt] & 0x00000020) <<  5);
 /* state->g_sp.regs[SP_STATUS_REG] |=  (!!(state->SR[rt] & 0x00000040) <<  5); */
    state->g_sp.regs[SP_STATUS_REG] &= ~(!!(state->SR[rt] & 0x00000080) <<  6);
    state->g_sp.regs[SP_STATUS_REG] |=  (!!(state->SR[rt] & 0x00000100) <<  6);
    state->g_sp.regs[SP_STATUS_REG] &= ~(!!(state->SR[rt] & 0x00000200) <<  7);
    state->g_sp.regs[SP_STATUS_REG] |=  (!!(state->SR[rt] & 0x00000400) <<  7);
    state->g_sp.regs[SP_STATUS_REG] &= ~(!!(state->SR[rt] & 0x00000800) <<  8);
    state->g_sp.regs[SP_STATUS_REG] |=  (!!(state->SR[rt] & 0x00001000) <<  8);
    state->g_sp.regs[SP_STATUS_REG] &= ~(!!(state->SR[rt] & 0x00002000) <<  9);
    state->g_sp.regs[SP_STATUS_REG] |=  (!!(state->SR[rt] & 0x00004000) <<  9);
    state->g_sp.regs[SP_STATUS_REG] &= ~(!!(state->SR[rt] & 0x00008000) << 10);
    state->g_sp.regs[SP_STATUS_REG] |=  (!!(state->SR[rt] & 0x00010000) << 10);
    state->g_sp.regs[SP_STATUS_REG] &= ~(!!(state->SR[rt] & 0x00020000) << 11);
    state->g_sp.regs[SP_STATUS_REG] |=  (!!(state->SR[rt] & 0x00040000) << 11);
    state->g_sp.regs[SP_STATUS_REG] &= ~(!!(state->SR[rt] & 0x00080000) << 12);
    state->g_sp.regs[SP_STATUS_REG] |=  (!!(state->SR[rt] & 0x00100000) << 12);
    state->g_sp.regs[SP_STATUS_REG] &= ~(!!(state->SR[rt] & 0x00200000) << 13);
    state->g_sp.regs[SP_STATUS_REG] |=  (!!(state->SR[rt] & 0x00400000) << 13);
    state->g_sp.regs[SP_STATUS_REG] &= ~(!!(state->SR[rt] & 0x00800000) << 14);
    state->g_sp.regs[SP_STATUS_REG] |=  (!!(state->SR[rt] & 0x01000000) << 14);
    return;
}
static void MT_SP_RESERVED(usf_state_t * state, int rt)
{
    const uint32_t source = state->SR[rt] & 0x00000000; /* forced (zilmar, dox) */

    state->g_sp.regs[SP_SEMAPHORE_REG] = source;
    return;
}
static void MT_CMD_START(usf_state_t * state, int rt)
{
    const uint32_t source = state->SR[rt] & 0xFFFFFFF8; /* Funnelcube demo */

    if (state->g_dp.dpc_regs[DPC_BUFBUSY_REG]) /* lock hazards not implemented */
        message(state, "MTC0\nCMD_START", 0);
    state->g_dp.dpc_regs[DPC_END_REG] = state->g_dp.dpc_regs[DPC_CURRENT_REG] = state->g_dp.dpc_regs[DPC_START_REG] = source;
    return;
}
static void MT_CMD_END(usf_state_t * state, int rt)
{
    if (state->g_dp.dpc_regs[DPC_BUFBUSY_REG])
        message(state, "MTC0\nCMD_END", 0); /* This is just CA-related. */
    state->g_dp.dpc_regs[DPC_END_REG] = state->SR[rt] & 0xFFFFFFF8;
    return;
}
static void MT_CMD_STATUS(usf_state_t * state, int rt)
{
    if (state->SR[rt] & 0xFFFFFD80) /* unsupported or reserved bits */
        message(state, "MTC0\nCMD_STATUS", 2);
    state->g_dp.dpc_regs[DPC_STATUS_REG] &= ~(!!(state->SR[rt] & 0x00000001) << 0);
    state->g_dp.dpc_regs[DPC_STATUS_REG] |=  (!!(state->SR[rt] & 0x00000002) << 0);
    state->g_dp.dpc_regs[DPC_STATUS_REG] &= ~(!!(state->SR[rt] & 0x00000004) << 1);
    state->g_dp.dpc_regs[DPC_STATUS_REG] |=  (!!(state->SR[rt] & 0x00000008) << 1);
    state->g_dp.dpc_regs[DPC_STATUS_REG] &= ~(!!(state->SR[rt] & 0x00000010) << 2);
    state->g_dp.dpc_regs[DPC_STATUS_REG] |=  (!!(state->SR[rt] & 0x00000020) << 2);
/* Some NUS-CIC-6105 SP tasks try to clear some zeroed DPC registers. */
    state->g_dp.dpc_regs[DPC_TMEM_REG]     &= !(state->SR[rt] & 0x00000040) * -1;
 /* state->g_dp.dpc_regs[DPC_PIPEBUSY_REG] &= !(state->SR[rt] & 0x00000080) * -1; */
 /* state->g_dp.dpc_regs[DPC_BUFBUSY_REG]  &= !(state->SR[rt] & 0x00000100) * -1; */
    state->g_dp.dpc_regs[DPC_CLOCK_REG]    &= !(state->SR[rt] & 0x00000200) * -1;
    return;
}
static void MT_CMD_CLOCK(usf_state_t * state, int rt)
{
    message(state, "MTC0\nCMD_CLOCK", 1); /* read-only?? */
    state->g_dp.dpc_regs[DPC_CLOCK_REG] = state->SR[rt];
    return; /* Appendix says this is RW; elsewhere it says R. */
}
static void MT_READ_ONLY(usf_state_t * state, int rt)
{
    (void)state;
    (void)rt;
    //char text[64];

    //sprintf(text, "MTC0\nInvalid write attempt.\nstate->SR[%i] = 0x%08X", rt, state->SR[rt]);
    //message(state, text, 2);
    return;
}

static void (*MTC0[16])(usf_state_t *, int) = {
MT_DMA_CACHE       ,MT_DMA_DRAM        ,MT_DMA_READ_LENGTH ,MT_DMA_WRITE_LENGTH,
MT_SP_STATUS       ,MT_READ_ONLY       ,MT_READ_ONLY       ,MT_SP_RESERVED,
MT_CMD_START       ,MT_CMD_END         ,MT_READ_ONLY       ,MT_CMD_STATUS,
MT_CMD_CLOCK       ,MT_READ_ONLY       ,MT_READ_ONLY       ,MT_READ_ONLY
};
void SP_DMA_READ(usf_state_t * state)
{
    // Write to RSP, read from RDRAM
    dma_sp_write(&state->g_sp);
    state->g_sp.regs[SP_DMA_BUSY_REG] = 0x00000000;
    state->g_sp.regs[SP_STATUS_REG] &= ~0x00000004; /* SP_STATUS_DMABUSY */
}
void SP_DMA_WRITE(usf_state_t * state)
{
    // Read from RSP, write to RDRAM
    dma_sp_read(&state->g_sp);
    state->g_sp.regs[SP_DMA_BUSY_REG] = 0x00000000;
    state->g_sp.regs[SP_STATUS_REG] &= ~0x00000004; /* SP_STATUS_DMABUSY */
}

/*** Scalar, Coprocessor Operations (vector unit) ***/

/*
 * Since RSP vectors are stored 100% accurately as big-endian arrays for the
 * proper vector operation math to be done, LWC2 and SWC2 emulation code will
 * have to look a little different.  zilmar's method is to distort the endian
 * using an array of unions, permitting hacked byte- and halfword-precision.
 */

/*
 * Universal byte-access macro for 16*8 halfword vectors.
 * Use this macro if you are not sure whether the element is odd or even.
 */
#define VR_B(vt,element)    (*(byte *)((byte *)(state->VR[vt]) + MES(element)))

/*
 * Optimized byte-access macros for the vector registers.
 * Use these ONLY if you know the element is even (or odd in the second).
 */
#define VR_A(vt,element)    (*(byte *)((byte *)(state->VR[vt]) + element + MES(0x0)))
#define VR_U(vt,element)    (*(byte *)((byte *)(state->VR[vt]) + element - MES(0x0)))

/*
 * Optimized halfword-access macro for indexing eight-element vectors.
 * Use this ONLY if you know the element is even, not odd.
 *
 * If the four-bit element is odd, then there is no solution in one hit.
 */
#define VR_S(vt,element)    (*(short *)((byte *)(state->VR[vt]) + element))

static unsigned short get_VCO(usf_state_t * state);
static unsigned short get_VCC(usf_state_t * state);
static unsigned char get_VCE(usf_state_t * state);
static void set_VCO(usf_state_t * state, unsigned short VCO);
static void set_VCC(usf_state_t * state, unsigned short VCC);
static void set_VCE(usf_state_t * state, unsigned char VCE);

static unsigned short rwR_VCE(usf_state_t * state)
{ /* never saw a game try to read VCE out to a scalar GPR yet */
    register unsigned short ret_slot;

    ret_slot = 0x00 | (unsigned short)get_VCE(state);
    return (ret_slot);
}
static void rwW_VCE(usf_state_t * state, unsigned short VCE)
{ /* never saw a game try to write VCE using a scalar GPR yet */
    register int i;

    VCE = 0x00 | (VCE & 0xFF);
    for (i = 0; i < 8; i++)
        state->vce[i] = (VCE >> i) & 1;
    return;
}

static unsigned short (*R_VCF[32])(usf_state_t *) = {
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
/* Hazard reaction barrier:  RD = (UINT16)(inst) >> 11, without &= 3. */
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
    get_VCO,get_VCC,rwR_VCE,rwR_VCE
};
static void (*W_VCF[32])(usf_state_t *, unsigned short) = {
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
/* Hazard reaction barrier:  RD = (UINT16)(inst) >> 11, without &= 3. */
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
    set_VCO,set_VCC,rwW_VCE,rwW_VCE
};
static void MFC2(usf_state_t * state, int rt, int vs, int e)
{
    SR_B(rt, 2) = VR_B(vs, e);
    e = (e + 0x1) & 0xF;
    SR_B(rt, 3) = VR_B(vs, e);
    state->SR[rt] = (signed short)(state->SR[rt]);
    state->SR[0] = 0x00000000;
    return;
}
static void MTC2(usf_state_t * state, int rt, int vd, int e)
{
    VR_B(vd, e+0x0) = SR_B(rt, 2);
    VR_B(vd, e+0x1) = SR_B(rt, 3);
    return; /* If element == 0xF, it does not matter; loads do not wrap over. */
}
static void CFC2(usf_state_t * state, int rt, int rd)
{
    state->SR[rt] = (signed short)R_VCF[rd](state);
    state->SR[0] = 0x00000000;
    return;
}
static void CTC2(usf_state_t * state, int rt, int rd)
{
    W_VCF[rd](state, state->SR[rt] & 0x0000FFFF);
    return;
}

/*** Scalar, Coprocessor Operations (vector unit, scalar cache transfers) ***/
INLINE static void LBV(usf_state_t * state, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    const int e = element;

    addr = (state->SR[base] + 1*offset) & 0x00000FFF;
    VR_B(vt, e) = state->DMEM[BES(addr)];
    return;
}
INLINE static void LSV(usf_state_t * state, int vt, int element, int offset, int base)
{
    int correction;
    register uint32_t addr;
    const int e = element;

    if (e & 0x1)
    {
        message(state, "LSV\nIllegal element.", 3);
        return;
    }
    addr = (state->SR[base] + 2*offset) & 0x00000FFF;
    correction = addr % 0x004;
    if (correction == 0x003)
    {
        message(state, "LSV\nWeird addr.", 3);
        return;
    }
    VR_S(vt, e) = *(short *)(state->DMEM + addr - HES(0x000)*(correction - 1));
    return;
}
INLINE static void LLV(usf_state_t * state, int vt, int element, int offset, int base)
{
    int correction;
    register uint32_t addr;
    const int e = element;

    if (e & 0x1)
    {
        message(state, "LLV\nOdd element.", 3);
        return;
    } /* Illegal (but still even) elements are used by Boss Game Studios. */
    addr = (state->SR[base] + 4*offset) & 0x00000FFF;
    if (addr & 0x00000001)
    {
        message(state, "LLV\nOdd addr.", 3);
        return;
    }
    correction = HES(0x000)*(addr%0x004 - 1);
    VR_S(vt, e+0x0) = *(short *)(state->DMEM + addr - correction);
    addr = (addr + 0x00000002) & 0x00000FFF; /* F3DLX 1.23:  addr%4 is 0x002. */
    VR_S(vt, e+0x2) = *(short *)(state->DMEM + addr + correction);
    return;
}
INLINE static void LDV(usf_state_t * state, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    const int e = element;

    if (e & 0x1)
    {
        message(state, "LDV\nOdd element.", 3);
        return;
    } /* Illegal (but still even) elements are used by Boss Game Studios. */
    addr = (state->SR[base] + 8*offset) & 0x00000FFF;
    switch (addr & 07)
    {
        case 00:
            VR_S(vt, e+0x0) = *(short *)(state->DMEM + addr + HES(0x000));
            VR_S(vt, e+0x2) = *(short *)(state->DMEM + addr + HES(0x002));
            VR_S(vt, e+0x4) = *(short *)(state->DMEM + addr + HES(0x004));
            VR_S(vt, e+0x6) = *(short *)(state->DMEM + addr + HES(0x006));
            return;
        case 01: /* standard ABI ucodes (unlike e.g. MusyX w/ even addresses) */
            VR_S(vt, e+0x0) = *(short *)(state->DMEM + addr + 0x000);
            VR_A(vt, e+0x2) = state->DMEM[addr + 0x002 - BES(0x000)];
            VR_U(vt, e+0x3) = state->DMEM[addr + 0x003 + BES(0x000)];
            VR_S(vt, e+0x4) = *(short *)(state->DMEM + addr + 0x004);
            VR_A(vt, e+0x6) = state->DMEM[addr + 0x006 - BES(0x000)];
            addr += 0x007 + BES(00);
            addr &= 0x00000FFF;
            VR_U(vt, e+0x7) = state->DMEM[addr];
            return;
        case 02:
            VR_S(vt, e+0x0) = *(short *)(state->DMEM + addr + 0x000 - HES(0x000));
            VR_S(vt, e+0x2) = *(short *)(state->DMEM + addr + 0x002 + HES(0x000));
            VR_S(vt, e+0x4) = *(short *)(state->DMEM + addr + 0x004 - HES(0x000));
            addr += 0x006 + HES(00);
            addr &= 0x00000FFF;
            VR_S(vt, e+0x6) = *(short *)(state->DMEM + addr);
            return;
        case 03: /* standard ABI ucodes (unlike e.g. MusyX w/ even addresses) */
            VR_A(vt, e+0x0) = state->DMEM[addr + 0x000 - BES(0x000)];
            VR_U(vt, e+0x1) = state->DMEM[addr + 0x001 + BES(0x000)];
            VR_S(vt, e+0x2) = *(short *)(state->DMEM + addr + 0x002);
            VR_A(vt, e+0x4) = state->DMEM[addr + 0x004 - BES(0x000)];
            addr += 0x005 + BES(00);
            addr &= 0x00000FFF;
            VR_U(vt, e+0x5) = state->DMEM[addr];
            VR_S(vt, e+0x6) = *(short *)(state->DMEM + addr + 0x001 - BES(0x000));
            return;
        case 04:
            VR_S(vt, e+0x0) = *(short *)(state->DMEM + addr + HES(0x000));
            VR_S(vt, e+0x2) = *(short *)(state->DMEM + addr + HES(0x002));
            addr += 0x004 + WES(00);
            addr &= 0x00000FFF;
            VR_S(vt, e+0x4) = *(short *)(state->DMEM + addr + HES(0x000));
            VR_S(vt, e+0x6) = *(short *)(state->DMEM + addr + HES(0x002));
            return;
        case 05: /* standard ABI ucodes (unlike e.g. MusyX w/ even addresses) */
            VR_S(vt, e+0x0) = *(short *)(state->DMEM + addr + 0x000);
            VR_A(vt, e+0x2) = state->DMEM[addr + 0x002 - BES(0x000)];
            addr += 0x003;
            addr &= 0x00000FFF;
            VR_U(vt, e+0x3) = state->DMEM[addr + BES(0x000)];
            VR_S(vt, e+0x4) = *(short *)(state->DMEM + addr + 0x001);
            VR_A(vt, e+0x6) = state->DMEM[addr + BES(0x003)];
            VR_U(vt, e+0x7) = state->DMEM[addr + BES(0x004)];
            return;
        case 06:
            VR_S(vt, e+0x0) = *(short *)(state->DMEM + addr - HES(0x000));
            addr += 0x002;
            addr &= 0x00000FFF;
            VR_S(vt, e+0x2) = *(short *)(state->DMEM + addr + HES(0x000));
            VR_S(vt, e+0x4) = *(short *)(state->DMEM + addr + HES(0x002));
            VR_S(vt, e+0x6) = *(short *)(state->DMEM + addr + HES(0x004));
            return;
        case 07: /* standard ABI ucodes (unlike e.g. MusyX w/ even addresses) */
            VR_A(vt, e+0x0) = state->DMEM[addr - BES(0x000)];
            addr += 0x001;
            addr &= 0x00000FFF;
            VR_U(vt, e+0x1) = state->DMEM[addr + BES(0x000)];
            VR_S(vt, e+0x2) = *(short *)(state->DMEM + addr + 0x001);
            VR_A(vt, e+0x4) = state->DMEM[addr + BES(0x003)];
            VR_U(vt, e+0x5) = state->DMEM[addr + BES(0x004)];
            VR_S(vt, e+0x6) = *(short *)(state->DMEM + addr + 0x005);
            return;
    }
}
INLINE static void SBV(usf_state_t * state, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    const int e = element;

    addr = (state->SR[base] + 1*offset) & 0x00000FFF;
    state->DMEM[BES(addr)] = VR_B(vt, e);
    return;
}
INLINE static void SSV(usf_state_t * state, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    const int e = element;

    addr = (state->SR[base] + 2*offset) & 0x00000FFF;
    state->DMEM[BES(addr)] = VR_B(vt, (e + 0x0));
    addr = (addr + 0x00000001) & 0x00000FFF;
    state->DMEM[BES(addr)] = VR_B(vt, (e + 0x1) & 0xF);
    return;
}
INLINE static void SLV(usf_state_t * state, int vt, int element, int offset, int base)
{
    int correction;
    register uint32_t addr;
    const int e = element;

    if ((e & 0x1) || e > 0xC) /* must support illegal even elements in F3DEX2 */
    {
        message(state, "SLV\nIllegal element.", 3);
        return;
    }
    addr = (state->SR[base] + 4*offset) & 0x00000FFF;
    if (addr & 0x00000001)
    {
        message(state, "SLV\nOdd addr.", 3);
        return;
    }
    correction = HES(0x000)*(addr%0x004 - 1);
    *(short *)(state->DMEM + addr - correction) = VR_S(vt, e+0x0);
    addr = (addr + 0x00000002) & 0x00000FFF; /* F3DLX 0.95:  "Mario Kart 64" */
    *(short *)(state->DMEM + addr + correction) = VR_S(vt, e+0x2);
    return;
}
INLINE static void SDV(usf_state_t * state, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    const int e = element;

    addr = (state->SR[base] + 8*offset) & 0x00000FFF;
    if (e > 0x8 || (e & 0x1))
    { /* Illegal elements with Boss Game Studios publications. */
        register int i;

        for (i = 0; i < 8; i++)
            state->DMEM[BES(addr &= 0x00000FFF)] = VR_B(vt, (e+i)&0xF);
        return;
    }
    switch (addr & 07)
    {
        case 00:
            *(short *)(state->DMEM + addr + HES(0x000)) = VR_S(vt, e+0x0);
            *(short *)(state->DMEM + addr + HES(0x002)) = VR_S(vt, e+0x2);
            *(short *)(state->DMEM + addr + HES(0x004)) = VR_S(vt, e+0x4);
            *(short *)(state->DMEM + addr + HES(0x006)) = VR_S(vt, e+0x6);
            return;
        case 01: /* "Tetrisphere" audio ucode */
            *(short *)(state->DMEM + addr + 0x000) = VR_S(vt, e+0x0);
            state->DMEM[addr + 0x002 - BES(0x000)] = VR_A(vt, e+0x2);
            state->DMEM[addr + 0x003 + BES(0x000)] = VR_U(vt, e+0x3);
            *(short *)(state->DMEM + addr + 0x004) = VR_S(vt, e+0x4);
            state->DMEM[addr + 0x006 - BES(0x000)] = VR_A(vt, e+0x6);
            addr += 0x007 + BES(0x000);
            addr &= 0x00000FFF;
            state->DMEM[addr] = VR_U(vt, e+0x7);
            return;
        case 02:
            *(short *)(state->DMEM + addr + 0x000 - HES(0x000)) = VR_S(vt, e+0x0);
            *(short *)(state->DMEM + addr + 0x002 + HES(0x000)) = VR_S(vt, e+0x2);
            *(short *)(state->DMEM + addr + 0x004 - HES(0x000)) = VR_S(vt, e+0x4);
            addr += 0x006 + HES(0x000);
            addr &= 0x00000FFF;
            *(short *)(state->DMEM + addr) = VR_S(vt, e+0x6);
            return;
        case 03: /* "Tetrisphere" audio ucode */
            state->DMEM[addr + 0x000 - BES(0x000)] = VR_A(vt, e+0x0);
            state->DMEM[addr + 0x001 + BES(0x000)] = VR_U(vt, e+0x1);
            *(short *)(state->DMEM + addr + 0x002) = VR_S(vt, e+0x2);
            state->DMEM[addr + 0x004 - BES(0x000)] = VR_A(vt, e+0x4);
            addr += 0x005 + BES(0x000);
            addr &= 0x00000FFF;
            state->DMEM[addr] = VR_U(vt, e+0x5);
            *(short *)(state->DMEM + addr + 0x001 - BES(0x000)) = VR_S(vt, 0x6);
            return;
        case 04:
            *(short *)(state->DMEM + addr + HES(0x000)) = VR_S(vt, e+0x0);
            *(short *)(state->DMEM + addr + HES(0x002)) = VR_S(vt, e+0x2);
            addr = (addr + 0x004) & 0x00000FFF;
            *(short *)(state->DMEM + addr + HES(0x000)) = VR_S(vt, e+0x4);
            *(short *)(state->DMEM + addr + HES(0x002)) = VR_S(vt, e+0x6);
            return;
        case 05: /* "Tetrisphere" audio ucode */
            *(short *)(state->DMEM + addr + 0x000) = VR_S(vt, e+0x0);
            state->DMEM[addr + 0x002 - BES(0x000)] = VR_A(vt, e+0x2);
            addr = (addr + 0x003) & 0x00000FFF;
            state->DMEM[addr + BES(0x000)] = VR_U(vt, e+0x3);
            *(short *)(state->DMEM + addr + 0x001) = VR_S(vt, e+0x4);
            state->DMEM[addr + BES(0x003)] = VR_A(vt, e+0x6);
            state->DMEM[addr + BES(0x004)] = VR_U(vt, e+0x7);
            return;
        case 06:
            *(short *)(state->DMEM + addr - HES(0x000)) = VR_S(vt, e+0x0);
            addr = (addr + 0x002) & 0x00000FFF;
            *(short *)(state->DMEM + addr + HES(0x000)) = VR_S(vt, e+0x2);
            *(short *)(state->DMEM + addr + HES(0x002)) = VR_S(vt, e+0x4);
            *(short *)(state->DMEM + addr + HES(0x004)) = VR_S(vt, e+0x6);
            return;
        case 07: /* "Tetrisphere" audio ucode */
            state->DMEM[addr - BES(0x000)] = VR_A(vt, e+0x0);
            addr = (addr + 0x001) & 0x00000FFF;
            state->DMEM[addr + BES(0x000)] = VR_U(vt, e+0x1);
            *(short *)(state->DMEM + addr + 0x001) = VR_S(vt, e+0x2);
            state->DMEM[addr + BES(0x003)] = VR_A(vt, e+0x4);
            state->DMEM[addr + BES(0x004)] = VR_U(vt, e+0x5);
            *(short *)(state->DMEM + addr + 0x005) = VR_S(vt, e+0x6);
            return;
    }
}

/*
 * Group II vector loads and stores:
 * PV and UV (As of RCP implementation, XV and ZV are reserved opcodes.)
 */
INLINE static void LPV(usf_state_t * state, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    register int b;
    const int e = element;

    if (e != 0x0)
    {
        message(state, "LPV\nIllegal element.", 3);
        return;
    }
    addr = (state->SR[base] + 8*offset) & 0x00000FFF;
    b = addr & 07;
    addr &= ~07;
    switch (b)
    {
        case 00:
            state->VR[vt][07] = state->DMEM[addr + BES(0x007)] << 8;
            state->VR[vt][06] = state->DMEM[addr + BES(0x006)] << 8;
            state->VR[vt][05] = state->DMEM[addr + BES(0x005)] << 8;
            state->VR[vt][04] = state->DMEM[addr + BES(0x004)] << 8;
            state->VR[vt][03] = state->DMEM[addr + BES(0x003)] << 8;
            state->VR[vt][02] = state->DMEM[addr + BES(0x002)] << 8;
            state->VR[vt][01] = state->DMEM[addr + BES(0x001)] << 8;
            state->VR[vt][00] = state->DMEM[addr + BES(0x000)] << 8;
            return;
        case 01: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            state->VR[vt][00] = state->DMEM[addr + BES(0x001)] << 8;
            state->VR[vt][01] = state->DMEM[addr + BES(0x002)] << 8;
            state->VR[vt][02] = state->DMEM[addr + BES(0x003)] << 8;
            state->VR[vt][03] = state->DMEM[addr + BES(0x004)] << 8;
            state->VR[vt][04] = state->DMEM[addr + BES(0x005)] << 8;
            state->VR[vt][05] = state->DMEM[addr + BES(0x006)] << 8;
            state->VR[vt][06] = state->DMEM[addr + BES(0x007)] << 8;
            addr += BES(0x008);
            addr &= 0x00000FFF;
            state->VR[vt][07] = state->DMEM[addr] << 8;
            return;
        case 02: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            state->VR[vt][00] = state->DMEM[addr + BES(0x002)] << 8;
            state->VR[vt][01] = state->DMEM[addr + BES(0x003)] << 8;
            state->VR[vt][02] = state->DMEM[addr + BES(0x004)] << 8;
            state->VR[vt][03] = state->DMEM[addr + BES(0x005)] << 8;
            state->VR[vt][04] = state->DMEM[addr + BES(0x006)] << 8;
            state->VR[vt][05] = state->DMEM[addr + BES(0x007)] << 8;
            addr += 0x008;
            addr &= 0x00000FFF;
            state->VR[vt][06] = state->DMEM[addr + BES(0x000)] << 8;
            state->VR[vt][07] = state->DMEM[addr + BES(0x001)] << 8;
            return;
        case 03: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            state->VR[vt][00] = state->DMEM[addr + BES(0x003)] << 8;
            state->VR[vt][01] = state->DMEM[addr + BES(0x004)] << 8;
            state->VR[vt][02] = state->DMEM[addr + BES(0x005)] << 8;
            state->VR[vt][03] = state->DMEM[addr + BES(0x006)] << 8;
            state->VR[vt][04] = state->DMEM[addr + BES(0x007)] << 8;
            addr += 0x008;
            addr &= 0x00000FFF;
            state->VR[vt][05] = state->DMEM[addr + BES(0x000)] << 8;
            state->VR[vt][06] = state->DMEM[addr + BES(0x001)] << 8;
            state->VR[vt][07] = state->DMEM[addr + BES(0x002)] << 8;
            return;
        case 04: /* "Resident Evil 2" in-game 3-D, F3DLX 2.08--"WWF No Mercy" */
            state->VR[vt][00] = state->DMEM[addr + BES(0x004)] << 8;
            state->VR[vt][01] = state->DMEM[addr + BES(0x005)] << 8;
            state->VR[vt][02] = state->DMEM[addr + BES(0x006)] << 8;
            state->VR[vt][03] = state->DMEM[addr + BES(0x007)] << 8;
            addr += 0x008;
            addr &= 0x00000FFF;
            state->VR[vt][04] = state->DMEM[addr + BES(0x000)] << 8;
            state->VR[vt][05] = state->DMEM[addr + BES(0x001)] << 8;
            state->VR[vt][06] = state->DMEM[addr + BES(0x002)] << 8;
            state->VR[vt][07] = state->DMEM[addr + BES(0x003)] << 8;
            return;
        case 05: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            state->VR[vt][00] = state->DMEM[addr + BES(0x005)] << 8;
            state->VR[vt][01] = state->DMEM[addr + BES(0x006)] << 8;
            state->VR[vt][02] = state->DMEM[addr + BES(0x007)] << 8;
            addr += 0x008;
            addr &= 0x00000FFF;
            state->VR[vt][03] = state->DMEM[addr + BES(0x000)] << 8;
            state->VR[vt][04] = state->DMEM[addr + BES(0x001)] << 8;
            state->VR[vt][05] = state->DMEM[addr + BES(0x002)] << 8;
            state->VR[vt][06] = state->DMEM[addr + BES(0x003)] << 8;
            state->VR[vt][07] = state->DMEM[addr + BES(0x004)] << 8;
            return;
        case 06: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            state->VR[vt][00] = state->DMEM[addr + BES(0x006)] << 8;
            state->VR[vt][01] = state->DMEM[addr + BES(0x007)] << 8;
            addr += 0x008;
            addr &= 0x00000FFF;
            state->VR[vt][02] = state->DMEM[addr + BES(0x000)] << 8;
            state->VR[vt][03] = state->DMEM[addr + BES(0x001)] << 8;
            state->VR[vt][04] = state->DMEM[addr + BES(0x002)] << 8;
            state->VR[vt][05] = state->DMEM[addr + BES(0x003)] << 8;
            state->VR[vt][06] = state->DMEM[addr + BES(0x004)] << 8;
            state->VR[vt][07] = state->DMEM[addr + BES(0x005)] << 8;
            return;
        case 07: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            state->VR[vt][00] = state->DMEM[addr + BES(0x007)] << 8;
            addr += 0x008;
            addr &= 0x00000FFF;
            state->VR[vt][01] = state->DMEM[addr + BES(0x000)] << 8;
            state->VR[vt][02] = state->DMEM[addr + BES(0x001)] << 8;
            state->VR[vt][03] = state->DMEM[addr + BES(0x002)] << 8;
            state->VR[vt][04] = state->DMEM[addr + BES(0x003)] << 8;
            state->VR[vt][05] = state->DMEM[addr + BES(0x004)] << 8;
            state->VR[vt][06] = state->DMEM[addr + BES(0x005)] << 8;
            state->VR[vt][07] = state->DMEM[addr + BES(0x006)] << 8;
            return;
    }
}
INLINE static void LUV(usf_state_t * state, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    register int b;
    int e = element;

    addr = (state->SR[base] + 8*offset) & 0x00000FFF;
    if (e != 0x0)
    { /* "Mia Hamm Soccer 64" SP exception override (zilmar) */
        addr += -e & 0xF;
        for (b = 0; b < 8; b++)
        {
            state->VR[vt][b] = state->DMEM[BES(addr &= 0x00000FFF)] << 7;
            --e;
            addr -= 16 * (e == 0x0);
            ++addr;
        }
        return;
    }
    b = addr & 07;
    addr &= ~07;
    switch (b)
    {
        case 00:
            state->VR[vt][07] = state->DMEM[addr + BES(0x007)] << 7;
            state->VR[vt][06] = state->DMEM[addr + BES(0x006)] << 7;
            state->VR[vt][05] = state->DMEM[addr + BES(0x005)] << 7;
            state->VR[vt][04] = state->DMEM[addr + BES(0x004)] << 7;
            state->VR[vt][03] = state->DMEM[addr + BES(0x003)] << 7;
            state->VR[vt][02] = state->DMEM[addr + BES(0x002)] << 7;
            state->VR[vt][01] = state->DMEM[addr + BES(0x001)] << 7;
            state->VR[vt][00] = state->DMEM[addr + BES(0x000)] << 7;
            return;
        case 01: /* PKMN Puzzle League HVQM decoder */
            state->VR[vt][00] = state->DMEM[addr + BES(0x001)] << 7;
            state->VR[vt][01] = state->DMEM[addr + BES(0x002)] << 7;
            state->VR[vt][02] = state->DMEM[addr + BES(0x003)] << 7;
            state->VR[vt][03] = state->DMEM[addr + BES(0x004)] << 7;
            state->VR[vt][04] = state->DMEM[addr + BES(0x005)] << 7;
            state->VR[vt][05] = state->DMEM[addr + BES(0x006)] << 7;
            state->VR[vt][06] = state->DMEM[addr + BES(0x007)] << 7;
            addr += BES(0x008);
            addr &= 0x00000FFF;
            state->VR[vt][07] = state->DMEM[addr] << 7;
            return;
        case 02: /* PKMN Puzzle League HVQM decoder */
            state->VR[vt][00] = state->DMEM[addr + BES(0x002)] << 7;
            state->VR[vt][01] = state->DMEM[addr + BES(0x003)] << 7;
            state->VR[vt][02] = state->DMEM[addr + BES(0x004)] << 7;
            state->VR[vt][03] = state->DMEM[addr + BES(0x005)] << 7;
            state->VR[vt][04] = state->DMEM[addr + BES(0x006)] << 7;
            state->VR[vt][05] = state->DMEM[addr + BES(0x007)] << 7;
            addr += 0x008;
            addr &= 0x00000FFF;
            state->VR[vt][06] = state->DMEM[addr + BES(0x000)] << 7;
            state->VR[vt][07] = state->DMEM[addr + BES(0x001)] << 7;
            return;
        case 03: /* PKMN Puzzle League HVQM decoder */
            state->VR[vt][00] = state->DMEM[addr + BES(0x003)] << 7;
            state->VR[vt][01] = state->DMEM[addr + BES(0x004)] << 7;
            state->VR[vt][02] = state->DMEM[addr + BES(0x005)] << 7;
            state->VR[vt][03] = state->DMEM[addr + BES(0x006)] << 7;
            state->VR[vt][04] = state->DMEM[addr + BES(0x007)] << 7;
            addr += 0x008;
            addr &= 0x00000FFF;
            state->VR[vt][05] = state->DMEM[addr + BES(0x000)] << 7;
            state->VR[vt][06] = state->DMEM[addr + BES(0x001)] << 7;
            state->VR[vt][07] = state->DMEM[addr + BES(0x002)] << 7;
            return;
        case 04: /* PKMN Puzzle League HVQM decoder */
            state->VR[vt][00] = state->DMEM[addr + BES(0x004)] << 7;
            state->VR[vt][01] = state->DMEM[addr + BES(0x005)] << 7;
            state->VR[vt][02] = state->DMEM[addr + BES(0x006)] << 7;
            state->VR[vt][03] = state->DMEM[addr + BES(0x007)] << 7;
            addr += 0x008;
            addr &= 0x00000FFF;
            state->VR[vt][04] = state->DMEM[addr + BES(0x000)] << 7;
            state->VR[vt][05] = state->DMEM[addr + BES(0x001)] << 7;
            state->VR[vt][06] = state->DMEM[addr + BES(0x002)] << 7;
            state->VR[vt][07] = state->DMEM[addr + BES(0x003)] << 7;
            return;
        case 05: /* PKMN Puzzle League HVQM decoder */
            state->VR[vt][00] = state->DMEM[addr + BES(0x005)] << 7;
            state->VR[vt][01] = state->DMEM[addr + BES(0x006)] << 7;
            state->VR[vt][02] = state->DMEM[addr + BES(0x007)] << 7;
            addr += 0x008;
            addr &= 0x00000FFF;
            state->VR[vt][03] = state->DMEM[addr + BES(0x000)] << 7;
            state->VR[vt][04] = state->DMEM[addr + BES(0x001)] << 7;
            state->VR[vt][05] = state->DMEM[addr + BES(0x002)] << 7;
            state->VR[vt][06] = state->DMEM[addr + BES(0x003)] << 7;
            state->VR[vt][07] = state->DMEM[addr + BES(0x004)] << 7;
            return;
        case 06: /* PKMN Puzzle League HVQM decoder */
            state->VR[vt][00] = state->DMEM[addr + BES(0x006)] << 7;
            state->VR[vt][01] = state->DMEM[addr + BES(0x007)] << 7;
            addr += 0x008;
            addr &= 0x00000FFF;
            state->VR[vt][02] = state->DMEM[addr + BES(0x000)] << 7;
            state->VR[vt][03] = state->DMEM[addr + BES(0x001)] << 7;
            state->VR[vt][04] = state->DMEM[addr + BES(0x002)] << 7;
            state->VR[vt][05] = state->DMEM[addr + BES(0x003)] << 7;
            state->VR[vt][06] = state->DMEM[addr + BES(0x004)] << 7;
            state->VR[vt][07] = state->DMEM[addr + BES(0x005)] << 7;
            return;
        case 07: /* PKMN Puzzle League HVQM decoder */
            state->VR[vt][00] = state->DMEM[addr + BES(0x007)] << 7;
            addr += 0x008;
            addr &= 0x00000FFF;
            state->VR[vt][01] = state->DMEM[addr + BES(0x000)] << 7;
            state->VR[vt][02] = state->DMEM[addr + BES(0x001)] << 7;
            state->VR[vt][03] = state->DMEM[addr + BES(0x002)] << 7;
            state->VR[vt][04] = state->DMEM[addr + BES(0x003)] << 7;
            state->VR[vt][05] = state->DMEM[addr + BES(0x004)] << 7;
            state->VR[vt][06] = state->DMEM[addr + BES(0x005)] << 7;
            state->VR[vt][07] = state->DMEM[addr + BES(0x006)] << 7;
            return;
    }
}
INLINE static void SPV(usf_state_t * state, int vt, int element, int offset, int base)
{
    register int b;
    register uint32_t addr;
    const int e = element;

    if (e != 0x0)
    {
        message(state, "SPV\nIllegal element.", 3);
        return;
    }
    addr = (state->SR[base] + 8*offset) & 0x00000FFF;
    b = addr & 07;
    addr &= ~07;
    switch (b)
    {
        case 00:
            state->DMEM[addr + BES(0x007)] = (unsigned char)(state->VR[vt][07] >> 8);
            state->DMEM[addr + BES(0x006)] = (unsigned char)(state->VR[vt][06] >> 8);
            state->DMEM[addr + BES(0x005)] = (unsigned char)(state->VR[vt][05] >> 8);
            state->DMEM[addr + BES(0x004)] = (unsigned char)(state->VR[vt][04] >> 8);
            state->DMEM[addr + BES(0x003)] = (unsigned char)(state->VR[vt][03] >> 8);
            state->DMEM[addr + BES(0x002)] = (unsigned char)(state->VR[vt][02] >> 8);
            state->DMEM[addr + BES(0x001)] = (unsigned char)(state->VR[vt][01] >> 8);
            state->DMEM[addr + BES(0x000)] = (unsigned char)(state->VR[vt][00] >> 8);
            return;
        case 01: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            state->DMEM[addr + BES(0x001)] = (unsigned char)(state->VR[vt][00] >> 8);
            state->DMEM[addr + BES(0x002)] = (unsigned char)(state->VR[vt][01] >> 8);
            state->DMEM[addr + BES(0x003)] = (unsigned char)(state->VR[vt][02] >> 8);
            state->DMEM[addr + BES(0x004)] = (unsigned char)(state->VR[vt][03] >> 8);
            state->DMEM[addr + BES(0x005)] = (unsigned char)(state->VR[vt][04] >> 8);
            state->DMEM[addr + BES(0x006)] = (unsigned char)(state->VR[vt][05] >> 8);
            state->DMEM[addr + BES(0x007)] = (unsigned char)(state->VR[vt][06] >> 8);
            addr += BES(0x008);
            addr &= 0x00000FFF;
            state->DMEM[addr] = (unsigned char)(state->VR[vt][07] >> 8);
            return;
        case 02: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            state->DMEM[addr + BES(0x002)] = (unsigned char)(state->VR[vt][00] >> 8);
            state->DMEM[addr + BES(0x003)] = (unsigned char)(state->VR[vt][01] >> 8);
            state->DMEM[addr + BES(0x004)] = (unsigned char)(state->VR[vt][02] >> 8);
            state->DMEM[addr + BES(0x005)] = (unsigned char)(state->VR[vt][03] >> 8);
            state->DMEM[addr + BES(0x006)] = (unsigned char)(state->VR[vt][04] >> 8);
            state->DMEM[addr + BES(0x007)] = (unsigned char)(state->VR[vt][05] >> 8);
            addr += 0x008;
            addr &= 0x00000FFF;
            state->DMEM[addr + BES(0x000)] = (unsigned char)(state->VR[vt][06] >> 8);
            state->DMEM[addr + BES(0x001)] = (unsigned char)(state->VR[vt][07] >> 8);
            return;
        case 03: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            state->DMEM[addr + BES(0x003)] = (unsigned char)(state->VR[vt][00] >> 8);
            state->DMEM[addr + BES(0x004)] = (unsigned char)(state->VR[vt][01] >> 8);
            state->DMEM[addr + BES(0x005)] = (unsigned char)(state->VR[vt][02] >> 8);
            state->DMEM[addr + BES(0x006)] = (unsigned char)(state->VR[vt][03] >> 8);
            state->DMEM[addr + BES(0x007)] = (unsigned char)(state->VR[vt][04] >> 8);
            addr += 0x008;
            addr &= 0x00000FFF;
            state->DMEM[addr + BES(0x000)] = (unsigned char)(state->VR[vt][05] >> 8);
            state->DMEM[addr + BES(0x001)] = (unsigned char)(state->VR[vt][06] >> 8);
            state->DMEM[addr + BES(0x002)] = (unsigned char)(state->VR[vt][07] >> 8);
            return;
        case 04: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            state->DMEM[addr + BES(0x004)] = (unsigned char)(state->VR[vt][00] >> 8);
            state->DMEM[addr + BES(0x005)] = (unsigned char)(state->VR[vt][01] >> 8);
            state->DMEM[addr + BES(0x006)] = (unsigned char)(state->VR[vt][02] >> 8);
            state->DMEM[addr + BES(0x007)] = (unsigned char)(state->VR[vt][03] >> 8);
            addr += 0x008;
            addr &= 0x00000FFF;
            state->DMEM[addr + BES(0x000)] = (unsigned char)(state->VR[vt][04] >> 8);
            state->DMEM[addr + BES(0x001)] = (unsigned char)(state->VR[vt][05] >> 8);
            state->DMEM[addr + BES(0x002)] = (unsigned char)(state->VR[vt][06] >> 8);
            state->DMEM[addr + BES(0x003)] = (unsigned char)(state->VR[vt][07] >> 8);
            return;
        case 05: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            state->DMEM[addr + BES(0x005)] = (unsigned char)(state->VR[vt][00] >> 8);
            state->DMEM[addr + BES(0x006)] = (unsigned char)(state->VR[vt][01] >> 8);
            state->DMEM[addr + BES(0x007)] = (unsigned char)(state->VR[vt][02] >> 8);
            addr += 0x008;
            addr &= 0x00000FFF;
            state->DMEM[addr + BES(0x000)] = (unsigned char)(state->VR[vt][03] >> 8);
            state->DMEM[addr + BES(0x001)] = (unsigned char)(state->VR[vt][04] >> 8);
            state->DMEM[addr + BES(0x002)] = (unsigned char)(state->VR[vt][05] >> 8);
            state->DMEM[addr + BES(0x003)] = (unsigned char)(state->VR[vt][06] >> 8);
            state->DMEM[addr + BES(0x004)] = (unsigned char)(state->VR[vt][07] >> 8);
            return;
        case 06: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            state->DMEM[addr + BES(0x006)] = (unsigned char)(state->VR[vt][00] >> 8);
            state->DMEM[addr + BES(0x007)] = (unsigned char)(state->VR[vt][01] >> 8);
            addr += 0x008;
            addr &= 0x00000FFF;
            state->DMEM[addr + BES(0x000)] = (unsigned char)(state->VR[vt][02] >> 8);
            state->DMEM[addr + BES(0x001)] = (unsigned char)(state->VR[vt][03] >> 8);
            state->DMEM[addr + BES(0x002)] = (unsigned char)(state->VR[vt][04] >> 8);
            state->DMEM[addr + BES(0x003)] = (unsigned char)(state->VR[vt][05] >> 8);
            state->DMEM[addr + BES(0x004)] = (unsigned char)(state->VR[vt][06] >> 8);
            state->DMEM[addr + BES(0x005)] = (unsigned char)(state->VR[vt][07] >> 8);
            return;
        case 07: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            state->DMEM[addr + BES(0x007)] = (unsigned char)(state->VR[vt][00] >> 8);
            addr += 0x008;
            addr &= 0x00000FFF;
            state->DMEM[addr + BES(0x000)] = (unsigned char)(state->VR[vt][01] >> 8);
            state->DMEM[addr + BES(0x001)] = (unsigned char)(state->VR[vt][02] >> 8);
            state->DMEM[addr + BES(0x002)] = (unsigned char)(state->VR[vt][03] >> 8);
            state->DMEM[addr + BES(0x003)] = (unsigned char)(state->VR[vt][04] >> 8);
            state->DMEM[addr + BES(0x004)] = (unsigned char)(state->VR[vt][05] >> 8);
            state->DMEM[addr + BES(0x005)] = (unsigned char)(state->VR[vt][06] >> 8);
            state->DMEM[addr + BES(0x006)] = (unsigned char)(state->VR[vt][07] >> 8);
            return;
    }
}
INLINE static void SUV(usf_state_t * state, int vt, int element, int offset, int base)
{
    register int b;
    register uint32_t addr;
    const int e = element;

    if (e != 0x0)
    {
        message(state, "SUV\nIllegal element.", 3);
        return;
    }
    addr = (state->SR[base] + 8*offset) & 0x00000FFF;
    b = addr & 07;
    addr &= ~07;
    switch (b)
    {
        case 00:
            state->DMEM[addr + BES(0x007)] = (unsigned char)(state->VR[vt][07] >> 7);
            state->DMEM[addr + BES(0x006)] = (unsigned char)(state->VR[vt][06] >> 7);
            state->DMEM[addr + BES(0x005)] = (unsigned char)(state->VR[vt][05] >> 7);
            state->DMEM[addr + BES(0x004)] = (unsigned char)(state->VR[vt][04] >> 7);
            state->DMEM[addr + BES(0x003)] = (unsigned char)(state->VR[vt][03] >> 7);
            state->DMEM[addr + BES(0x002)] = (unsigned char)(state->VR[vt][02] >> 7);
            state->DMEM[addr + BES(0x001)] = (unsigned char)(state->VR[vt][01] >> 7);
            state->DMEM[addr + BES(0x000)] = (unsigned char)(state->VR[vt][00] >> 7);
            return;
        case 04: /* "Indiana Jones and the Infernal Machine" in-game */
            state->DMEM[addr + BES(0x004)] = (unsigned char)(state->VR[vt][00] >> 7);
            state->DMEM[addr + BES(0x005)] = (unsigned char)(state->VR[vt][01] >> 7);
            state->DMEM[addr + BES(0x006)] = (unsigned char)(state->VR[vt][02] >> 7);
            state->DMEM[addr + BES(0x007)] = (unsigned char)(state->VR[vt][03] >> 7);
            addr += 0x008;
            addr &= 0x00000FFF;
            state->DMEM[addr + BES(0x000)] = (unsigned char)(state->VR[vt][04] >> 7);
            state->DMEM[addr + BES(0x001)] = (unsigned char)(state->VR[vt][05] >> 7);
            state->DMEM[addr + BES(0x002)] = (unsigned char)(state->VR[vt][06] >> 7);
            state->DMEM[addr + BES(0x003)] = (unsigned char)(state->VR[vt][07] >> 7);
            return;
        default: /* Completely legal, just never seen it be done. */
            message(state, "SUV\nWeird addr.", 3);
            return;
    }
}

/*
 * Group III vector loads and stores:
 * HV, FV, and AV (As of RCP implementation, AV opcodes are reserved.)
 */
static void LHV(usf_state_t * state, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    const int e = element;

    if (e != 0x0)
    {
        message(state, "LHV\nIllegal element.", 3);
        return;
    }
    addr = (state->SR[base] + 16*offset) & 0x00000FFF;
    if (addr & 0x0000000E)
    {
        message(state, "LHV\nIllegal addr.", 3);
        return;
    }
    addr ^= MES(00);
    state->VR[vt][07] = state->DMEM[addr + HES(0x00E)] << 7;
    state->VR[vt][06] = state->DMEM[addr + HES(0x00C)] << 7;
    state->VR[vt][05] = state->DMEM[addr + HES(0x00A)] << 7;
    state->VR[vt][04] = state->DMEM[addr + HES(0x008)] << 7;
    state->VR[vt][03] = state->DMEM[addr + HES(0x006)] << 7;
    state->VR[vt][02] = state->DMEM[addr + HES(0x004)] << 7;
    state->VR[vt][01] = state->DMEM[addr + HES(0x002)] << 7;
    state->VR[vt][00] = state->DMEM[addr + HES(0x000)] << 7;
    return;
}
NOINLINE static void LFV(usf_state_t * state, int vt, int element, int offset, int base)
{
    (void)state;
    (void)vt;
    (void)element;
    (void)offset;
    (void)base;
    /* Dummy implementation only:  Do any games execute this? */
    /*char debugger[32];

    sprintf(debugger, "%s     $v%i[0x%X], 0x%03X($%i)", "LFV",
        vt, element, offset & 0xFFF, base);
    message(state, debugger, 3);*/
    return;
}
static void SHV(usf_state_t * state, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    const int e = element;

    if (e != 0x0)
    {
        message(state, "SHV\nIllegal element.", 3);
        return;
    }
    addr = (state->SR[base] + 16*offset) & 0x00000FFF;
    if (addr & 0x0000000E)
    {
        message(state, "SHV\nIllegal addr.", 3);
        return;
    }
    addr ^= MES(00);
    state->DMEM[addr + HES(0x00E)] = (unsigned char)(state->VR[vt][07] >> 7);
    state->DMEM[addr + HES(0x00C)] = (unsigned char)(state->VR[vt][06] >> 7);
    state->DMEM[addr + HES(0x00A)] = (unsigned char)(state->VR[vt][05] >> 7);
    state->DMEM[addr + HES(0x008)] = (unsigned char)(state->VR[vt][04] >> 7);
    state->DMEM[addr + HES(0x006)] = (unsigned char)(state->VR[vt][03] >> 7);
    state->DMEM[addr + HES(0x004)] = (unsigned char)(state->VR[vt][02] >> 7);
    state->DMEM[addr + HES(0x002)] = (unsigned char)(state->VR[vt][01] >> 7);
    state->DMEM[addr + HES(0x000)] = (unsigned char)(state->VR[vt][00] >> 7);
    return;
}
static void SFV(usf_state_t * state, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    const int e = element;

    addr = (state->SR[base] + 16*offset) & 0x00000FFF;
    addr &= 0x00000FF3;
    addr ^= BES(00);
    switch (e)
    {
        case 0x0:
            state->DMEM[addr + 0x000] = (unsigned char)(state->VR[vt][00] >> 7);
            state->DMEM[addr + 0x004] = (unsigned char)(state->VR[vt][01] >> 7);
            state->DMEM[addr + 0x008] = (unsigned char)(state->VR[vt][02] >> 7);
            state->DMEM[addr + 0x00C] = (unsigned char)(state->VR[vt][03] >> 7);
            return;
        case 0x8:
            state->DMEM[addr + 0x000] = (unsigned char)(state->VR[vt][04] >> 7);
            state->DMEM[addr + 0x004] = (unsigned char)(state->VR[vt][05] >> 7);
            state->DMEM[addr + 0x008] = (unsigned char)(state->VR[vt][06] >> 7);
            state->DMEM[addr + 0x00C] = (unsigned char)(state->VR[vt][07] >> 7);
            return;
        default:
            message(state, "SFV\nIllegal element.", 3);
            return;
    }
}

/*
 * Group IV vector loads and stores:
 * QV and RV
 */
INLINE static void LQV(usf_state_t * state, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    register int b;
    const int e = element; /* Boss Game Studios illegal elements */

    if (e & 0x1)
    {
        message(state, "LQV\nOdd element.", 3);
        return;
    }
    addr = (state->SR[base] + 16*offset) & 0x00000FFF;
    if (addr & 0x00000001)
    {
        message(state, "LQV\nOdd addr.", 3);
        return;
    }
    b = addr & 0x0000000F;
    addr &= ~0x0000000F;
    switch (b/2) /* mistake in SGI patent regarding LQV */
    {
        case 0x0/2:
            VR_S(vt,e+0x0) = *(short *)(state->DMEM + addr + HES(0x000));
            VR_S(vt,e+0x2) = *(short *)(state->DMEM + addr + HES(0x002));
            VR_S(vt,e+0x4) = *(short *)(state->DMEM + addr + HES(0x004));
            VR_S(vt,e+0x6) = *(short *)(state->DMEM + addr + HES(0x006));
            VR_S(vt,e+0x8) = *(short *)(state->DMEM + addr + HES(0x008));
            VR_S(vt,e+0xA) = *(short *)(state->DMEM + addr + HES(0x00A));
            VR_S(vt,e+0xC) = *(short *)(state->DMEM + addr + HES(0x00C));
            VR_S(vt,e+0xE) = *(short *)(state->DMEM + addr + HES(0x00E));
            return;
        case 0x2/2:
            VR_S(vt,e+0x0) = *(short *)(state->DMEM + addr + HES(0x002));
            VR_S(vt,e+0x2) = *(short *)(state->DMEM + addr + HES(0x004));
            VR_S(vt,e+0x4) = *(short *)(state->DMEM + addr + HES(0x006));
            VR_S(vt,e+0x6) = *(short *)(state->DMEM + addr + HES(0x008));
            VR_S(vt,e+0x8) = *(short *)(state->DMEM + addr + HES(0x00A));
            VR_S(vt,e+0xA) = *(short *)(state->DMEM + addr + HES(0x00C));
            VR_S(vt,e+0xC) = *(short *)(state->DMEM + addr + HES(0x00E));
            return;
        case 0x4/2:
            VR_S(vt,e+0x0) = *(short *)(state->DMEM + addr + HES(0x004));
            VR_S(vt,e+0x2) = *(short *)(state->DMEM + addr + HES(0x006));
            VR_S(vt,e+0x4) = *(short *)(state->DMEM + addr + HES(0x008));
            VR_S(vt,e+0x6) = *(short *)(state->DMEM + addr + HES(0x00A));
            VR_S(vt,e+0x8) = *(short *)(state->DMEM + addr + HES(0x00C));
            VR_S(vt,e+0xA) = *(short *)(state->DMEM + addr + HES(0x00E));
            return;
        case 0x6/2:
            VR_S(vt,e+0x0) = *(short *)(state->DMEM + addr + HES(0x006));
            VR_S(vt,e+0x2) = *(short *)(state->DMEM + addr + HES(0x008));
            VR_S(vt,e+0x4) = *(short *)(state->DMEM + addr + HES(0x00A));
            VR_S(vt,e+0x6) = *(short *)(state->DMEM + addr + HES(0x00C));
            VR_S(vt,e+0x8) = *(short *)(state->DMEM + addr + HES(0x00E));
            return;
        case 0x8/2: /* "Resident Evil 2" cinematics and Boss Game Studios */
            VR_S(vt,e+0x0) = *(short *)(state->DMEM + addr + HES(0x008));
            VR_S(vt,e+0x2) = *(short *)(state->DMEM + addr + HES(0x00A));
            VR_S(vt,e+0x4) = *(short *)(state->DMEM + addr + HES(0x00C));
            VR_S(vt,e+0x6) = *(short *)(state->DMEM + addr + HES(0x00E));
            return;
        case 0xA/2: /* "Conker's Bad Fur Day" audio microcode by Rareware */
            VR_S(vt,e+0x0) = *(short *)(state->DMEM + addr + HES(0x00A));
            VR_S(vt,e+0x2) = *(short *)(state->DMEM + addr + HES(0x00C));
            VR_S(vt,e+0x4) = *(short *)(state->DMEM + addr + HES(0x00E));
            return;
        case 0xC/2: /* "Conker's Bad Fur Day" audio microcode by Rareware */
            VR_S(vt,e+0x0) = *(short *)(state->DMEM + addr + HES(0x00C));
            VR_S(vt,e+0x2) = *(short *)(state->DMEM + addr + HES(0x00E));
            return;
        case 0xE/2: /* "Conker's Bad Fur Day" audio microcode by Rareware */
            VR_S(vt,e+0x0) = *(short *)(state->DMEM + addr + HES(0x00E));
            return;
    }
}
static void LRV(usf_state_t * state, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    register int b;
    const int e = element;

    if (e != 0x0)
    {
        message(state, "LRV\nIllegal element.", 3);
        return;
    }
    addr = (state->SR[base] + 16*offset) & 0x00000FFF;
    if (addr & 0x00000001)
    {
        message(state, "LRV\nOdd addr.", 3);
        return;
    }
    b = addr & 0x0000000F;
    addr &= ~0x0000000F;
    switch (b/2)
    {
        case 0xE/2:
            state->VR[vt][01] = *(short *)(state->DMEM + addr + HES(0x000));
            state->VR[vt][02] = *(short *)(state->DMEM + addr + HES(0x002));
            state->VR[vt][03] = *(short *)(state->DMEM + addr + HES(0x004));
            state->VR[vt][04] = *(short *)(state->DMEM + addr + HES(0x006));
            state->VR[vt][05] = *(short *)(state->DMEM + addr + HES(0x008));
            state->VR[vt][06] = *(short *)(state->DMEM + addr + HES(0x00A));
            state->VR[vt][07] = *(short *)(state->DMEM + addr + HES(0x00C));
            return;
        case 0xC/2:
            state->VR[vt][02] = *(short *)(state->DMEM + addr + HES(0x000));
            state->VR[vt][03] = *(short *)(state->DMEM + addr + HES(0x002));
            state->VR[vt][04] = *(short *)(state->DMEM + addr + HES(0x004));
            state->VR[vt][05] = *(short *)(state->DMEM + addr + HES(0x006));
            state->VR[vt][06] = *(short *)(state->DMEM + addr + HES(0x008));
            state->VR[vt][07] = *(short *)(state->DMEM + addr + HES(0x00A));
            return;
        case 0xA/2:
            state->VR[vt][03] = *(short *)(state->DMEM + addr + HES(0x000));
            state->VR[vt][04] = *(short *)(state->DMEM + addr + HES(0x002));
            state->VR[vt][05] = *(short *)(state->DMEM + addr + HES(0x004));
            state->VR[vt][06] = *(short *)(state->DMEM + addr + HES(0x006));
            state->VR[vt][07] = *(short *)(state->DMEM + addr + HES(0x008));
            return;
        case 0x8/2:
            state->VR[vt][04] = *(short *)(state->DMEM + addr + HES(0x000));
            state->VR[vt][05] = *(short *)(state->DMEM + addr + HES(0x002));
            state->VR[vt][06] = *(short *)(state->DMEM + addr + HES(0x004));
            state->VR[vt][07] = *(short *)(state->DMEM + addr + HES(0x006));
            return;
        case 0x6/2:
            state->VR[vt][05] = *(short *)(state->DMEM + addr + HES(0x000));
            state->VR[vt][06] = *(short *)(state->DMEM + addr + HES(0x002));
            state->VR[vt][07] = *(short *)(state->DMEM + addr + HES(0x004));
            return;
        case 0x4/2:
            state->VR[vt][06] = *(short *)(state->DMEM + addr + HES(0x000));
            state->VR[vt][07] = *(short *)(state->DMEM + addr + HES(0x002));
            return;
        case 0x2/2:
            state->VR[vt][07] = *(short *)(state->DMEM + addr + HES(0x000));
            return;
        case 0x0/2:
            return;
    }
}
INLINE static void SQV(usf_state_t * state, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    register int b;
    const int e = element;

    addr = (state->SR[base] + 16*offset) & 0x00000FFF;
    if (e != 0x0)
    { /* happens with "Mia Hamm Soccer 64" */
        register int i;

        for (i = 0; i < (int)(16 - addr%16); i++)
            state->DMEM[BES((addr + i) & 0xFFF)] = VR_B(vt, (e + i) & 0xF);
        return;
    }
    b = addr & 0x0000000F;
    addr &= ~0x0000000F;
    switch (b)
    {
        case 00:
            *(short *)(state->DMEM + addr + HES(0x000)) = state->VR[vt][00];
            *(short *)(state->DMEM + addr + HES(0x002)) = state->VR[vt][01];
            *(short *)(state->DMEM + addr + HES(0x004)) = state->VR[vt][02];
            *(short *)(state->DMEM + addr + HES(0x006)) = state->VR[vt][03];
            *(short *)(state->DMEM + addr + HES(0x008)) = state->VR[vt][04];
            *(short *)(state->DMEM + addr + HES(0x00A)) = state->VR[vt][05];
            *(short *)(state->DMEM + addr + HES(0x00C)) = state->VR[vt][06];
            *(short *)(state->DMEM + addr + HES(0x00E)) = state->VR[vt][07];
            return;
        case 02:
            *(short *)(state->DMEM + addr + HES(0x002)) = state->VR[vt][00];
            *(short *)(state->DMEM + addr + HES(0x004)) = state->VR[vt][01];
            *(short *)(state->DMEM + addr + HES(0x006)) = state->VR[vt][02];
            *(short *)(state->DMEM + addr + HES(0x008)) = state->VR[vt][03];
            *(short *)(state->DMEM + addr + HES(0x00A)) = state->VR[vt][04];
            *(short *)(state->DMEM + addr + HES(0x00C)) = state->VR[vt][05];
            *(short *)(state->DMEM + addr + HES(0x00E)) = state->VR[vt][06];
            return;
        case 04:
            *(short *)(state->DMEM + addr + HES(0x004)) = state->VR[vt][00];
            *(short *)(state->DMEM + addr + HES(0x006)) = state->VR[vt][01];
            *(short *)(state->DMEM + addr + HES(0x008)) = state->VR[vt][02];
            *(short *)(state->DMEM + addr + HES(0x00A)) = state->VR[vt][03];
            *(short *)(state->DMEM + addr + HES(0x00C)) = state->VR[vt][04];
            *(short *)(state->DMEM + addr + HES(0x00E)) = state->VR[vt][05];
            return;
        case 06:
            *(short *)(state->DMEM + addr + HES(0x006)) = state->VR[vt][00];
            *(short *)(state->DMEM + addr + HES(0x008)) = state->VR[vt][01];
            *(short *)(state->DMEM + addr + HES(0x00A)) = state->VR[vt][02];
            *(short *)(state->DMEM + addr + HES(0x00C)) = state->VR[vt][03];
            *(short *)(state->DMEM + addr + HES(0x00E)) = state->VR[vt][04];
            return;
        default:
            message(state, "SQV\nWeird addr.", 3);
            return;
    }
}
static void SRV(usf_state_t * state, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    register int b;
    const int e = element;

    if (e != 0x0)
    {
        message(state, "SRV\nIllegal element.", 3);
        return;
    }
    addr = (state->SR[base] + 16*offset) & 0x00000FFF;
    if (addr & 0x00000001)
    {
        message(state, "SRV\nOdd addr.", 3);
        return;
    }
    b = addr & 0x0000000F;
    addr &= ~0x0000000F;
    switch (b/2)
    {
        case 0xE/2:
            *(short *)(state->DMEM + addr + HES(0x000)) = state->VR[vt][01];
            *(short *)(state->DMEM + addr + HES(0x002)) = state->VR[vt][02];
            *(short *)(state->DMEM + addr + HES(0x004)) = state->VR[vt][03];
            *(short *)(state->DMEM + addr + HES(0x006)) = state->VR[vt][04];
            *(short *)(state->DMEM + addr + HES(0x008)) = state->VR[vt][05];
            *(short *)(state->DMEM + addr + HES(0x00A)) = state->VR[vt][06];
            *(short *)(state->DMEM + addr + HES(0x00C)) = state->VR[vt][07];
            return;
        case 0xC/2:
            *(short *)(state->DMEM + addr + HES(0x000)) = state->VR[vt][02];
            *(short *)(state->DMEM + addr + HES(0x002)) = state->VR[vt][03];
            *(short *)(state->DMEM + addr + HES(0x004)) = state->VR[vt][04];
            *(short *)(state->DMEM + addr + HES(0x006)) = state->VR[vt][05];
            *(short *)(state->DMEM + addr + HES(0x008)) = state->VR[vt][06];
            *(short *)(state->DMEM + addr + HES(0x00A)) = state->VR[vt][07];
            return;
        case 0xA/2:
            *(short *)(state->DMEM + addr + HES(0x000)) = state->VR[vt][03];
            *(short *)(state->DMEM + addr + HES(0x002)) = state->VR[vt][04];
            *(short *)(state->DMEM + addr + HES(0x004)) = state->VR[vt][05];
            *(short *)(state->DMEM + addr + HES(0x006)) = state->VR[vt][06];
            *(short *)(state->DMEM + addr + HES(0x008)) = state->VR[vt][07];
            return;
        case 0x8/2:
            *(short *)(state->DMEM + addr + HES(0x000)) = state->VR[vt][04];
            *(short *)(state->DMEM + addr + HES(0x002)) = state->VR[vt][05];
            *(short *)(state->DMEM + addr + HES(0x004)) = state->VR[vt][06];
            *(short *)(state->DMEM + addr + HES(0x006)) = state->VR[vt][07];
            return;
        case 0x6/2:
            *(short *)(state->DMEM + addr + HES(0x000)) = state->VR[vt][05];
            *(short *)(state->DMEM + addr + HES(0x002)) = state->VR[vt][06];
            *(short *)(state->DMEM + addr + HES(0x004)) = state->VR[vt][07];
            return;
        case 0x4/2:
            *(short *)(state->DMEM + addr + HES(0x000)) = state->VR[vt][06];
            *(short *)(state->DMEM + addr + HES(0x002)) = state->VR[vt][07];
            return;
        case 0x2/2:
            *(short *)(state->DMEM + addr + HES(0x000)) = state->VR[vt][07];
            return;
        case 0x0/2:
            return;
    }
}

/*
 * Group V vector loads and stores
 * TV and SWV (As of RCP implementation, LTWV opcode was undesired.)
 */
INLINE static void LTV(usf_state_t * state, int vt, int element, int offset, int base)
{
    register int i;
    register uint32_t addr;
    const int e = element;

    if (e & 1)
    {
        message(state, "LTV\nIllegal element.", 3);
        return;
    }
    if (vt & 07)
    {
        message(state, "LTV\nUncertain case!", 3);
        return; /* For LTV I am not sure; for STV I have an idea. */
    }
    addr = (state->SR[base] + 16*offset) & 0x00000FFF;
    if (addr & 0x0000000F)
    {
        message(state, "LTV\nIllegal addr.", 3);
        return;
    }
    for (i = 0; i < 8; i++) /* SGI screwed LTV up on N64.  See STV instead. */
        state->VR[vt+i][(-e/2 + i) & 07] = *(short *)(state->DMEM + addr + HES(2*i));
    return;
}
NOINLINE static void SWV(usf_state_t * state, int vt, int element, int offset, int base)
{
    (void)state;
    (void)vt;
    (void)element;
    (void)offset;
    (void)base;
    /* Dummy implementation only:  Do any games execute this? */
    /*char debugger[32];

    sprintf(debugger, "%s     $v%i[0x%X], 0x%03X($%i)", "SWV",
        vt, element, offset & 0xFFF, base);
    message(state, debugger, 3);*/
    return;
}
INLINE static void STV(usf_state_t * state, int vt, int element, int offset, int base)
{
    register int i;
    register uint32_t addr;
    const int e = element;

    if (e & 1)
    {
        message(state, "STV\nIllegal element.", 3);
        return;
    }
    if (vt & 07)
    {
        message(state, "STV\nUncertain case!", 2);
        return; /* vt &= 030; */
    }
    addr = (state->SR[base] + 16*offset) & 0x00000FFF;
    if (addr & 0x0000000F)
    {
        message(state, "STV\nIllegal addr.", 3);
        return;
    }
    for (i = 0; i < 8; i++)
        *(short *)(state->DMEM + addr + HES(2*i)) = state->VR[vt + (e/2 + i)%8][i];
    return;
}

/*** Modern pseudo-operations (not real instructions, but nice shortcuts) ***/
void ULW(usf_state_t * state, int rd, uint32_t addr)
{ /* "Unaligned Load Word" */
    union {
        unsigned char B[4];
        signed char SB[4];
        unsigned short H[2];
        signed short SH[2];
        unsigned W:  32;
    } SR_temp;
    if (addr & 0x00000001)
    {
        SR_temp.B[03] = state->DMEM[BES(addr)];
        addr = (addr + 0x001) & 0xFFF;
        SR_temp.B[02] = state->DMEM[BES(addr)];
        addr = (addr + 0x001) & 0xFFF;
        SR_temp.B[01] = state->DMEM[BES(addr)];
        addr = (addr + 0x001) & 0xFFF;
        SR_temp.B[00] = state->DMEM[BES(addr)];
    }
    else /* addr & 0x00000002 */
    {
        SR_temp.H[01] = *(short *)(state->DMEM + addr - HES(0x000));
        addr = (addr + 0x002) & 0xFFF;
        SR_temp.H[00] = *(short *)(state->DMEM + addr + HES(0x000));
    }
    state->SR[rd] = SR_temp.W;
 /* state->SR[0] = 0x00000000; */
    return;
}
void USW(usf_state_t * state, int rs, uint32_t addr)
{ /* "Unaligned Store Word" */
    union {
        unsigned char B[4];
        signed char SB[4];
        unsigned short H[2];
        signed short SH[2];
        unsigned W:  32;
    } SR_temp;
    SR_temp.W = state->SR[rs];
    if (addr & 0x00000001)
    {
        state->DMEM[BES(addr)] = SR_temp.B[03];
        addr = (addr + 0x001) & 0xFFF;
        state->DMEM[BES(addr)] = SR_temp.B[02];
        addr = (addr + 0x001) & 0xFFF;
        state->DMEM[BES(addr)] = SR_temp.B[01];
        addr = (addr + 0x001) & 0xFFF;
        state->DMEM[BES(addr)] = SR_temp.B[00];
    }
    else /* addr & 0x00000002 */
    {
        *(short *)(state->DMEM + addr - HES(0x000)) = SR_temp.H[01];
        addr = (addr + 0x002) & 0xFFF;
        *(short *)(state->DMEM + addr + HES(0x000)) = SR_temp.H[00];
    }
    return;
}

#endif
