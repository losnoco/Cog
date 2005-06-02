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

#ifdef USE_LAYER_2

#ifndef LAYER2_H_INCLUDED
#define LAYER2_H_INCLUDED


struct al_table2 
{
  short bits;
  short d;
};



void init_layer2(void);
void II_step_one(unsigned int *bit_alloc,int *scale,struct frame *fr);
void II_step_two(unsigned int *bit_alloc,real fraction[2][4][SBLIMIT],int *scale,struct frame *fr,int x1);
int  do_layer2( PMPSTR mp,unsigned char *pcm_sample,int *pcm_point);

#endif

#endif

