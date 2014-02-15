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
#include "divrom.h"

static void VRSQL(usf_state_t * state, int vd, int de, int vt, int e)
{
    state->DivIn &= -state->DPH;
    state->DivIn |= (unsigned short)state->VR[vt][e & 07];
    do_div(state, state->DivIn, SP_DIV_SQRT_YES, state->DPH);
    SHUFFLE_VECTOR(VACC_L, state->VR[vt], e);
    state->VR[vd][de &= 07] = (short)state->DivOut;
    state->DPH = SP_DIV_PRECISION_SINGLE;
    return;
}
