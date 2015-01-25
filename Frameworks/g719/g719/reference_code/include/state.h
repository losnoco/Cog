/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#ifndef _STATE_H
#define _STATE_H

#include <stdio.h>
#include "cnst.h"

typedef struct 
{
   float                old_wtda[FRAME_LENGTH/2];
   float				   	old_hpfilt_in;
   float				   	old_hpfilt_out;
   float			      	EnergyLT;

   short                TransientHangOver;

   short                num_bits;
   short                num_bits_spectrum_stationary ;
   short                num_bits_spectrum_transient  ;
} CoderState;

typedef struct {
   float             old_out[FRAME_LENGTH];
   float             old_coeffs[FRAME_LENGTH];

   short             num_bits;
   short             num_bits_spectrum_stationary;
   short             num_bits_spectrum_transient;

   short             old_is_transient;
} DecoderState;

#endif
