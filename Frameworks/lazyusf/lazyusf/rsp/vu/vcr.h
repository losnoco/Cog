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

INLINE static void do_cr(usf_state_t * state, short* VD, short* VS, short* VT)
{
    ALIGNED short ge[N], le[N], sn[N];
    ALIGNED short VC[N];
    ALIGNED short cmp[N];
    register int i;

    for (i = 0; i < N; i++)
        VC[i] = VT[i];
    for (i = 0; i < N; i++)
        sn[i] = (signed short)(VS[i] ^ VT[i]) >> 15;
#if (0)
    for (i = 0; i < N; i++)
        le[i] = sn[i] ? (VT[i] <= ~VS[i]) : (VT[i] <= ~0x0000);
    for (i = 0; i < N; i++)
        ge[i] = sn[i] ? (~0x0000 >= VT[i]) : (VS[i] >= VT[i]);
#else
    for (i = 0; i < N; i++)
        cmp[i] = ~(VS[i] & sn[i]);
    for (i = 0; i < N; i++)
        le[i] = (VT[i] <= cmp[i]);
    for (i = 0; i < N; i++)
        cmp[i] =  (VS[i] | sn[i]);
    for (i = 0; i < N; i++)
        ge[i] = (cmp[i] >= VT[i]);
#endif
    for (i = 0; i < N; i++)
        VC[i] ^= sn[i]; /* if (sn == ~0) {VT = ~VT;} else {VT =  VT;} */
    merge(VACC_L, le, VC, VS);
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

static void VCR(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    do_cr(state, state->VR[vd], state->VR[vs], ST);
    return;
}
