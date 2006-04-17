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

#ifndef __HUFFMANLOOKUP_H
#define __HUFFMANLOOKUP_H
#include "mpegsound.h"

/*
 * This class speeds up the huffman table decoding by largely replacing it
 * by a table lookup. It uses the fact that the huffman tables don't change,
 * and that given a byte of the bitstream, we can predict what the result
 * will be (without reading it bit by bit).
 */

class HuffmanLookup {
private:
	long pattern, bits;
	int wgetbit();
	int wgetbits (int b);
	void huffmandecoder_1(const HUFFMANCODETABLE *h,int *x, int *y);

	ATTR_ALIGN(64) static struct decodeData {
		int x : 8;
		int y : 8;
		int skip : 16;
	} qdecode[32][256];
public:
	HuffmanLookup();

	static int decode(int table, int pattern, int* x, int* y)
	{
		*x = qdecode[table][pattern].x;
		*y = qdecode[table][pattern].y;
		return qdecode[table][pattern].skip;
	}
};

#endif
