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


#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include "HIPDefines.h"

#include "mpg123.h"
#include "mpglib.h"

extern const int  tabsel_123[2][3][16];
extern const long freqs[9];
extern unsigned char *wordpointer;
extern int bitindex;


#if defined( USE_LAYER_1 ) || defined ( USE_LAYER_2 )
  extern real muls[27][64];
#endif

int  head_check(unsigned long head,int check_layer);
int  decode_header(struct frame *fr,unsigned long newhead);
void print_header(struct frame *fr);
void print_header_compact(struct frame *fr);
unsigned int getbits(int number_of_bits);
unsigned int getbits_fast(int number_of_bits);
int set_pointer( PMPSTR mp, long backstep);

#endif
