/*
 * Copyright (C) 2002      Michael Smith <msmith@xiph.org>
 * Copyright (C) 2015-2021 Philipp Schafft <lion@lion.leolix.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <string.h>

#include "base64.h"

static const signed char base64decode[256] = {
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 62, -2, -2, -2, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -2, -2, -2, -1, -2, -2,
	-2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -2, -2, -2, -2, -2,
	-2, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -2, -2, -2, -2, -2,
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2
};

int base64_decode(const char *in, void **out, size_t *len) {
	const unsigned char *input = (const unsigned char *)in;
	size_t todo = strlen(in);
	char *output;
	size_t opp = 0; // output pointer

	if(todo < 4 || (todo % 4) != 0)
		return -1;

	output = calloc(1, todo * 3 / 4 + 5);
	if(!output)
		return -1;

	while(todo) {
		signed char vals[4];
		size_t i;

		for(i = 0; i < (sizeof(vals) / sizeof(*vals)); i++)
			vals[i] = base64decode[*input++];

		if(vals[0] < 0 || vals[1] < 0 || vals[2] < -1 || vals[3] < -1) {
			todo -= 4;
			continue;
		}

		output[opp++] = vals[0] << 2 | vals[1] >> 4;

		/* vals[3] and (if that is) vals[2] can be '=' as padding, which is
		 * looked up in the base64decode table as '-1'. Check for this case,
		 * and output zero-terminators instead of characters if we've got
		 * padding. */
		if(vals[2] >= 0) {
			output[opp++] = ((vals[1] & 0x0F) << 4) | (vals[2] >> 2);
		} else {
			break;
		}

		if(vals[3] >= 0) {
			output[opp++] = ((vals[2] & 0x03) << 6) | (vals[3]);
		} else {
			break;
		}

		todo -= 4;
	}
	output[opp++] = 0;

	*out = output;
	*len = opp - 1;

	return 0;
}
