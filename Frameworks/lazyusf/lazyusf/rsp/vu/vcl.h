/******************************************************************************\
* Authors:  Iconoclast                                                         *
* Release:  2013.11.26                                                         *
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
#include "vu.h"

INLINE static void do_cl(usf_state_t * state, short* VD, short* VS, short* VT)
{
    ALIGNED short eq[N], ge[N], le[N];
    ALIGNED short gen[N], len[N], lz[N], uz[N], sn[N];
    ALIGNED short diff[N];
    ALIGNED short cmp[N];
    ALIGNED unsigned short VB[N], VC[N];
    register int i;

    for (i = 0; i < N; i++)
        VB[i] = VS[i];
    for (i = 0; i < N; i++)
        VC[i] = VT[i];

/*
    for (i = 0; i < N; i++)
        ge[i] = clip[i];
    for (i = 0; i < N; i++)
        le[i] = comp[i];
*/
    for (i = 0; i < N; i++)
        eq[i] = state->ne[i] ^ 1;
    for (i = 0; i < N; i++)
        sn[i] = state->co[i];
/*
 * Now that we have extracted all the flags, we will essentially be masking
 * them back in where they came from redundantly, unless the corresponding
 * NOTEQUAL bit from VCO upper was not set....
 */
    for (i = 0; i < N; i++)
        VC[i] = VC[i] ^ -sn[i];
    for (i = 0; i < N; i++)
        VC[i] = VC[i] + sn[i]; /* conditional negation, if sn */
    for (i = 0; i < N; i++)
        diff[i] = VB[i] - VC[i];
    for (i = 0; i < N; i++)
        uz[i] = (VB[i] - VC[i] - 0xFFFF) >> 31;
    for (i = 0; i < N; i++)
        lz[i] = (diff[i] == 0x0000);
    for (i = 0; i < N; i++)
        gen[i] = lz[i] | uz[i];
    for (i = 0; i < N; i++)
        len[i] = lz[i] & uz[i];
    for (i = 0; i < N; i++)
        gen[i] = gen[i] & state->vce[i];
    for (i = 0; i < N; i++)
        len[i] = len[i] & (state->vce[i] ^ 1);
    for (i = 0; i < N; i++)
        len[i] = len[i] | gen[i];
    for (i = 0; i < N; i++)
        gen[i] = (VB[i] >= VC[i]);

    for (i = 0; i < N; i++)
        cmp[i] = eq[i] & sn[i];
    merge(le, cmp, len, state->comp);

    for (i = 0; i < N; i++)
        cmp[i] = eq[i] & (sn[i] ^ 1);
    merge(ge, cmp, gen, state->clip);

    merge(cmp, sn, le, ge);
    merge(VACC_L, cmp, (short *)VC, VS);
    vector_copy(VD, VACC_L);

    for (i = 0; i < N; i++)
        state->clip[i] = ge[i];
    for (i = 0; i < N; i++)
        state->comp[i] = le[i];
    for (i = 0; i < N; i++)
        state->ne[i] = 0;
    for (i = 0; i < N; i++)
        state->co[i] = 0;
    for (i = 0; i < N; i++)
        state->vce[i] = 0;
    return;
}

static void VCL(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    do_cl(state, state->VR[vd], state->VR[vs], ST);
    return;
}
