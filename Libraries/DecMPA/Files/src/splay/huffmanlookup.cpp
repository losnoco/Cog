    /*

    Copyright (C) 2000 Stefan Westerfeld
                       stefan@space.twc.de

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
  
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
   
    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

    */

//changes 8/11/2002 (by Hauke Duden):
//	- removed unnecessary includes

#include "huffmanlookup.h"

//#include <assert.h>




struct HuffmanLookup::decodeData HuffmanLookup::qdecode[32][256];
/* for initialization */
static HuffmanLookup l;

HuffmanLookup::HuffmanLookup()
{
	int table,p,x,y;

	for(table = 0; table < 32; table++)
	{
		// 8 bits pattern
		for(p = 0; p < 256; p++)
		{
			bits = 24;
			pattern = (p << 16);

			huffmandecoder_1(&Mpegtoraw::ht[table], &x,&y);

			int used = 24 - bits;
			qdecode[table][p].skip = (used <= 8)?used:0;
			qdecode[table][p].x = x;
			qdecode[table][p].y = y;
		}
	}
}

int HuffmanLookup::wgetbit()   
{
	return (pattern >> --bits) & 1;
}

int HuffmanLookup::wgetbits (int b)
{
   	bits -= b;
	return (pattern >> bits) & ((1 << b) - 1);
}

void HuffmanLookup::huffmandecoder_1(const HUFFMANCODETABLE *h, int *x, int *y)
{  
  typedef unsigned int HUFFBITS;

  HUFFBITS level=(1<<(sizeof(HUFFBITS)*8-1));
  int point=0;
  /* Lookup in Huffman table. */
  for(;;)
  {
    if(h->val[point][0]==0)
    {   /*end of tree*/
      int xx,yy;

      xx=h->val[point][1]>>4;
      yy=h->val[point][1]&0xf;

      if(h->linbits)
      {
	if((h->xlen)==(unsigned)xx)xx+=wgetbits(h->linbits);
	if(xx)if(wgetbit())xx=-xx;
	if((h->ylen)==(unsigned)yy)yy+=wgetbits(h->linbits);
	if(yy)if(wgetbit())yy=-yy;
      }
      else
      {
	if(xx)if(wgetbit())xx=-xx;
	if(yy)if(wgetbit())yy=-yy;
      }
      *x=xx;*y=yy;
      break;
    } 

    point+=h->val[point][wgetbit()];
    
    level>>=1;
    if(!(level || ((unsigned)point<Mpegtoraw::ht->treelen)))
    {
      register int xx,yy;

      xx=(h->xlen<<1);// set x and y to a medium value as a simple concealment
      yy=(h->ylen<<1);

      // h->xlen and h->ylen can't be 1 under tablename 32
      //      if(xx)
	if(wgetbit())xx=-xx;
      //      if(yy)
	if(wgetbit())yy=-yy;

      *x=xx;*y=yy;
      break;
    }
  }
}


