/*
 * Copyright (C) 2021 Philipp Schafft <lion@lion.leolix.org>
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

#ifndef __PICTURE_H__
#define __PICTURE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef enum {
	FLAC_PICTURE_INVALID = -1,
	FLAC_PICTURE_OTHER = 0,
	FLAC_PICTURE_FILE_ICON = 1,
	FLAC_PICTURE_OTHER_FILE_ICON = 2,
	FLAC_PICTURE_COVER_FRONT = 3,
	FLAC_PICTURE_COVER_BACK = 4,
	FLAC_PICTURE_LEAFLET_PAGE = 5,
	FLAC_PICTURE_MEDIA = 6,
	FLAC_PICTURE_LEAD = 7,
	FLAC_PICTURE_ARTIST = 8,
	FLAC_PICTURE_CONDUCTOR = 9,
	FLAC_PICTURE_BAND = 10,
	FLAC_PICTURE_COMPOSER = 11,
	FLAC_PICTURE_LYRICIST = 12,
	FLAC_PICTURE_RECORDING_LOCATION = 13,
	FLAC_PICTURE_DURING_RECORDING = 14,
	FLAC_PICTURE_DURING_PREFORMANCE = 15,
	FLAC_PICTURE_SCREEN_CAPTURE = 16,
	FLAC_PICTURE_A_BRIGHT_COLOURED_FISH = 17,
	FLAC_PICTURE_ILLUSTRATION = 18,
	FLAC_PICTURE_BAND_LOGOTYPE = 19,
	FLAC_PICTURE_PUBLISHER_LOGOTYPE = 20
} flac_picture_type;

typedef struct {
	flac_picture_type type;
	const char *media_type;
	const char *description;
	unsigned int width;
	unsigned int height;
	unsigned int depth;
	unsigned int colors;
	const void *binary;
	size_t binary_length;
	const char *uri;
	void *private_data;
	size_t private_data_length;
} flac_picture_t;

const char *flac_picture_type_string(flac_picture_type type);

flac_picture_t *flac_picture_parse_from_base64(const char *str);
flac_picture_t *flac_picture_parse_from_blob(const void *in, size_t len);

void flac_picture_free(flac_picture_t *picture);

#ifdef __cplusplus
}
#endif

#endif
