//
//  j2b.c
//  Dumb J2B Archive parser
//
//  Created by Christopher Snowhill on 10/4/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#include <stdint.h>
#include <stdlib.h>
#include <zlib.h>

#include "j2b.h"

void *unpackJ2b(const void *in, long *size) {
	uint32_t fileLength;
	uint32_t checksum;
	uint32_t lenCompressed;
	uint32_t lenUncompressed;
	const uint8_t *in8 = (const uint8_t *)in;
	Bytef *uncompressedData;
	int zErr;
	uLong dataUncompressed;

	if(*size < 8)
		return 0;

	if(in8[0] != 'M' || in8[1] != 'U' || in8[2] != 'S' || in8[3] != 'E' ||
	   in8[4] != 0xDE || in8[5] != 0xAD ||
	   ((in8[6] != 0xBE || in8[7] != 0xAF) && (in8[6] != 0xBA || in8[7] != 0xBE))) {
		return 0;
	}

	if(*size < 12)
		return 0;

	fileLength = in8[8] | (in8[9] << 8) | (in8[10] << 16) | (in8[11] << 24);

	if(fileLength < 12 || fileLength + 12 > *size)
		return 0;

	checksum = in8[12] | (in8[13] << 8) | (in8[14] << 16) | (in8[15] << 24);
	lenCompressed = in8[16] | (in8[17] << 8) | (in8[18] << 16) | (in8[19] << 24);
	lenUncompressed = in8[20] | (in8[21] << 8) | (in8[22] << 16) | (in8[23] << 24);

	if(lenCompressed + 12 > fileLength)
		return 0;

	if(crc32(0, in8 + 24, lenCompressed) != checksum)
		return 0;

	uncompressedData = (Bytef *)malloc(lenUncompressed);

	if(!uncompressedData)
		return 0;

	dataUncompressed = lenUncompressed;

	zErr = uncompress(uncompressedData, &dataUncompressed, in8 + 24, lenCompressed);
	if(zErr != Z_OK) {
		free(uncompressedData);
		return 0;
	}

	*size = dataUncompressed;

	return uncompressedData;
}
