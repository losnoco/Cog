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

static void VRSQH(usf_state_t * state, int vd, int de, int vt, int e)
{
    state->DivIn = state->VR[vt][e & 07] << 16;
    SHUFFLE_VECTOR(VACC_L, state->VR[vt], e);
    state->VR[vd][de &= 07] = state->DivOut >> 16;
    state->DPH = SP_DIV_PRECISION_DOUBLE;
    return;
}
