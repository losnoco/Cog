/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "cnst.h"

/*--------------------------------------------------------------------------*/
/*  Function  window_ola                                                    */
/*  ~~~~~~~~~~~~~~~~~~~~                                                    */
/*                                                                          */
/*  Windowing, Overlap and Add                                              */
/*--------------------------------------------------------------------------*/
/*  float     ImdctOut[]  (i)   input                                       */
/*  short     auOut[]     (o)   output audio                                */
/*  float     OldauOut[]  (i/o) audio from previous frame                   */
/*--------------------------------------------------------------------------*/
void window_ola(float ImdctOut[], short auOut[], float OldauOut[])
{
    short i;
    float OutputVal;
    float ImdctOutWin[2*FRAME_LENGTH];
   
    for (i=0 ; i < 2*FRAME_LENGTH ; i++)
    {
        ImdctOutWin[i] = ImdctOut[i] * window[i];
    }

    for (i=0 ; i < FRAME_LENGTH ; i++)
    {
        OutputVal = ImdctOutWin[i] + OldauOut[i];
        if (OutputVal > 32767.0f)
        {
          OutputVal = 32767.0f;
        }
        else if (OutputVal < -32768.0f)
        {
          OutputVal = -32768.0f;
        }
        auOut[i] = (short)OutputVal;
    }

    for (i=0 ; i < FRAME_LENGTH ; i++)
    {
        OldauOut[i] = ImdctOutWin[i+FRAME_LENGTH];
    }
}
