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

INLINE static void do_mudl(usf_state_t * state, short* VD, short* VS, short* VT)
{
    register int i;

    for (i = 0; i < N; i++)
        VACC_L[i] = (unsigned short)(VS[i])*(unsigned short)(VT[i]) >> 16;
    for (i = 0; i < N; i++)
        VACC_M[i] = 0x0000;
    for (i = 0; i < N; i++)
        VACC_H[i] = 0x0000;
    vector_copy(VD, VACC_L); /* no possibilities to clamp */
    return;
}

static void VMUDL(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    do_mudl(state, state->VR[vd], state->VR[vs], ST);
    return;
}
