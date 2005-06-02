/*  DecMPA decoding routines from Lame/HIP (Myers W. Carpenter)	

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	For more information look at the file License.txt in this package.

  
	email: hazard_hd@users.sourceforge.net
*/

/*
** Copyright (C) 2000 Albert L. Faber
**/


#ifndef DECODE_I386_H_INCLUDED
#define DECODE_I386_H_INCLUDED

#include "common.h"

int synth_1to1_mono(PMPSTR mp, real *bandPtr,unsigned char *samples,int *pnt);
int synth_1to1(PMPSTR mp, real *bandPtr,int channel,unsigned char *out,int *pnt);

#endif
