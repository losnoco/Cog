/******************************************************************************\
* Project:  Simple Vector Unit Benchmark                                       *
* Authors:  Iconoclast                                                         *
* Release:  2013.12.12                                                         *
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

/*
 * Since operations on scalar registers are much more predictable,
 * standardized, and documented, we don't really need to bench those.
 *
 * I was thinking I should also add MTC0 (SP DMA and writes to SP_STATUS_REG)
 * and, due to the SSE-style flags register file, CFC2 and CTC2, but I mostly
 * just wanted to hurry up and make this header quick with all the basics. :P
 *
 * Fortunately, because all the methods are static (no conditional jumps),
 * we can lazily leave the instruction word set to 0x00000000 for all the
 * op-codes we are benching, and it will make no difference in speed.
 */

#include <time.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "../usf.h"

#include "../usf_internal.h"

#undef JUMP

#define DisplayError(...)

#include "config.h"
#include "matrix.h"
#include "rsp.h"

#define NUMBER_OF_VU_OPCODES    38

ALIGNED usf_state_t state;

void CheckInterrupts(usf_state_t * state)
{
    (void)state;
    return;
}

void SP_DMA_READ(usf_state_t * state)
{
    (void)state;
    return;
}

void SP_DMA_WRITE(usf_state_t * state)
{
    (void)state;
    return;
}

static void (*bench_tests[NUMBER_OF_VU_OPCODES])(usf_state_t *, int, int, int, int) = {
    VMULF, VMACF, /* signed single-precision fractions */
    VMULU, VMACU, /* unsigned single-precision fractions */

    VMUDL, VMADL, /* double-precision multiplies using partial products */
    VMUDM, VMADM,
    VMUDN, VMADN,
    VMUDH, VMADH,

    VADD, VSUB, VABS,
    VADDC, VSUBC,
    VSAW,

    VEQ, VNE, VLT, VGE, /* normal select compares */
    VCH, VCL, /* double-precision clip select */
    VCR, /* single-precision, one's complement */
    VMRG,

    VAND, VNAND,
    VOR , VNOR ,
    VXOR, VNXOR,

    VRCPL, VRSQL, /* double-precision reciprocal look-ups */
    VRCPH, VRSQH,

    VMOV, VNOP
};

enum {
    SP_VMULF =  000,
    SP_VMULU =  001,
    SP_VRNDP =  002,
    SP_VMULQ =  003,
    SP_VMUDL =  004,
    SP_VMUDM =  005,
    SP_VMUDN =  006,
    SP_VMUDH =  007,
    SP_VMACF =  010,
    SP_VMACU =  011,
    SP_VRNDN =  012,
    SP_VMACQ =  013,
    SP_VMADL =  014,
    SP_VMADM =  015,
    SP_VMADN =  016,
    SP_VMADH =  017,
    SP_VADD  =  020,
    SP_VSUB  =  021,
    SP_VSUT  =  022,
    SP_VABS  =  023,
    SP_VADDC =  024,
    SP_VSUBC =  025,
    SP_VADDB =  026,
    SP_VSUBB =  027,
    SP_VACCB =  030,
    SP_VSUCB =  031,
    SP_VSAD  =  032,
    SP_VSAC  =  033,
    SP_VSUM  =  034,
    SP_VSAW  =  035,


    SP_VLT   =  040,
    SP_VEQ   =  041,
    SP_VNE   =  042,
    SP_VGE   =  043,
    SP_VCL   =  044,
    SP_VCH   =  045,
    SP_VCR   =  046,
    SP_VMRG  =  047,
    SP_VAND  =  050,
    SP_VNAND =  051,
    SP_VOR   =  052,
    SP_VNOR  =  053,
    SP_VXOR  =  054,
    SP_VNXOR =  055,


    SP_VRCP  =  060,
    SP_VRCPL =  061,
    SP_VRCPH =  062,
    SP_VMOV  =  063,
    SP_VRSQ  =  064,
    SP_VRSQL =  065,
    SP_VRSQH =  066,
    SP_VNOP  =  067,
    SP_VEXTT =  070,
    SP_VEXTQ =  071,
    SP_VEXTN =  072,

    SP_VINST =  074,
    SP_VINSQ =  075,
    SP_VINSN =  076,
    SP_VNULLOP= 077
};
const char* test_names[NUMBER_OF_VU_OPCODES] = {
    mnemonics_C2[SP_VMULF], mnemonics_C2[SP_VMACF],
    mnemonics_C2[SP_VMULU], mnemonics_C2[SP_VMACU],

    mnemonics_C2[SP_VMUDL], mnemonics_C2[SP_VMADL],
    mnemonics_C2[SP_VMUDM], mnemonics_C2[SP_VMADM],
    mnemonics_C2[SP_VMUDN], mnemonics_C2[SP_VMADN],
    mnemonics_C2[SP_VMUDH], mnemonics_C2[SP_VMADH],

    mnemonics_C2[SP_VADD], mnemonics_C2[SP_VSUB], mnemonics_C2[SP_VABS],
    mnemonics_C2[SP_VADDC], mnemonics_C2[SP_VSUBC],
    mnemonics_C2[SP_VSAW],

    mnemonics_C2[SP_VEQ], mnemonics_C2[SP_VNE],
    mnemonics_C2[SP_VLT], mnemonics_C2[SP_VGE],
    mnemonics_C2[SP_VCH], mnemonics_C2[SP_VCL],
    mnemonics_C2[SP_VCR],
    mnemonics_C2[SP_VMRG],

    mnemonics_C2[SP_VAND], mnemonics_C2[SP_VNAND],
    mnemonics_C2[SP_VOR] , mnemonics_C2[SP_VNOR] ,
    mnemonics_C2[SP_VXOR], mnemonics_C2[SP_VNXOR],

    mnemonics_C2[SP_VRCPL], mnemonics_C2[SP_VRSQL],
    mnemonics_C2[SP_VRCPH], mnemonics_C2[SP_VRSQH],

    mnemonics_C2[SP_VMOV], mnemonics_C2[SP_VNOP],
};

const char* notice_starting =
    "Ready to start benchmarks.\n"\
    "Close this message to commence tests.  Testing could take minutes.";
const char* notice_finished =
    "Finished writing benchmark results.\n"\
    "Check working emulator directory for \"sp_bench.txt\".";

int main(void)
{
    FILE* log;
    clock_t t1, t2;
    register int i, j;
    register float delta, total;

    fputs(notice_starting, stderr);
    log = fopen("sp_bench.txt", "w");
    fprintf(log, "RSP Vector Benchmarks Log\n\n");

    total = 0.0;
    for (i = 0; i < NUMBER_OF_VU_OPCODES; i++)
    {
        t1 = clock();
        for (j = -0x1000000; j < 0; j++)
            bench_tests[i](&state, 0, 0, 0, 8);
        t2 = clock();
        delta = (float)(t2 - t1) / CLOCKS_PER_SEC;
        fprintf(log, "%s:  %.3f s\n", test_names[i], delta);
	fprintf(stderr, "%s:  %.3f s\n", test_names[i], delta);
        total += delta;
    }
    fprintf(log, "Total time spent:  %.3f s\n", total);
    fprintf(stderr, "Total time spent:  %.3f s\n", total);
    fclose(log);
    fputs(notice_finished, stderr);
    return 0;
}

