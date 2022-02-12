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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "base64.h"
#include "picture.h"

const char *flac_picture_type_string(flac_picture_type type) {
	switch(type) {
		case FLAC_PICTURE_OTHER:
			return "Other";
			break;
		case FLAC_PICTURE_FILE_ICON:
			return "32x32 pixels file icon (PNG)";
			break;
		case FLAC_PICTURE_OTHER_FILE_ICON:
			return "Other file icon";
			break;
		case FLAC_PICTURE_COVER_FRONT:
			return "Cover (front)";
			break;
		case FLAC_PICTURE_COVER_BACK:
			return "Cover (back)";
			break;
		case FLAC_PICTURE_LEAFLET_PAGE:
			return "Leaflet page";
			break;
		case FLAC_PICTURE_MEDIA:
			return "Media";
			break;
		case FLAC_PICTURE_LEAD:
			return "Lead artist/lead performer/soloist";
			break;
		case FLAC_PICTURE_ARTIST:
			return "Artist/performer";
			break;
		case FLAC_PICTURE_CONDUCTOR:
			return "Conductor";
			break;
		case FLAC_PICTURE_BAND:
			return "Band/Orchestra";
			break;
		case FLAC_PICTURE_COMPOSER:
			return "Composer";
			break;
		case FLAC_PICTURE_LYRICIST:
			return "Lyricist/text writer";
			break;
		case FLAC_PICTURE_RECORDING_LOCATION:
			return "Recording Location";
			break;
		case FLAC_PICTURE_DURING_RECORDING:
			return "During recording";
			break;
		case FLAC_PICTURE_DURING_PREFORMANCE:
			return "During performance";
			break;
		case FLAC_PICTURE_SCREEN_CAPTURE:
			return "Movie/video screen capture";
			break;
		case FLAC_PICTURE_A_BRIGHT_COLOURED_FISH:
			return "A bright coloured fish";
			break;
		case FLAC_PICTURE_ILLUSTRATION:
			return "Illustration";
			break;
		case FLAC_PICTURE_BAND_LOGOTYPE:
			return "Band/artist logotype";
			break;
		case FLAC_PICTURE_PUBLISHER_LOGOTYPE:
			return "Publisher/Studio logotype";
			break;
		default:
			return "<unknown>";
			break;
	}
}

static uint32_t read32be(unsigned char *buf) {
	uint32_t ret = 0;

	ret = (ret << 8) | *buf++;
	ret = (ret << 8) | *buf++;
	ret = (ret << 8) | *buf++;
	ret = (ret << 8) | *buf++;

	return ret;
}

static flac_picture_t *flac_picture_parse_eat(void *data, size_t len) {
	size_t expected_length = 32; // 8*32 bit
	size_t offset = 0;
	flac_picture_t *ret;
	uint32_t tmp;

	if(len < expected_length)
		return NULL;

	ret = calloc(1, sizeof(*ret));
	if(!ret)
		return NULL;

	ret->private_data = data;
	ret->private_data_length = len;

	ret->type = read32be(data);

	/*
	const char *media_type;
	const char *description;
	unsigned int width;
	unsigned int height;
	unsigned int depth;
	unsigned int colors;
	const void *binary;
	size_t binary_length;
	const char *uri;
	*/
	tmp = read32be(data + 4);
	expected_length += tmp;
	if(len < expected_length) {
		free(ret);
		return NULL;
	}

	ret->media_type = data + 8;
	offset = 8 + tmp;
	tmp = read32be(data + offset);
	expected_length += tmp;
	if(len < expected_length) {
		free(ret);
		return NULL;
	}

	*(char *)(data + offset) = 0;
	offset += 4;
	ret->description = data + offset;
	offset += tmp;
	ret->width = read32be(data + offset);
	*(char *)(data + offset) = 0;
	offset += 4;
	ret->height = read32be(data + offset);
	offset += 4;
	ret->depth = read32be(data + offset);
	offset += 4;
	ret->colors = read32be(data + offset);
	offset += 4;
	ret->binary_length = read32be(data + offset);
	expected_length += ret->binary_length;
	if(len < expected_length) {
		free(ret);
		return NULL;
	}
	offset += 4;
	ret->binary = data + offset;

	if(strcmp(ret->media_type, "-->") == 0) {
		// Note: it is ensured ret->binary[ret->binary_length] == 0.
		ret->media_type = NULL;
		ret->uri = ret->binary;
		ret->binary = NULL;
		ret->binary_length = 0;
	}

	return ret;
}

flac_picture_t *flac_picture_parse_from_base64(const char *str) {
	flac_picture_t *ret;
	void *data;
	size_t len;

	if(!str || !*str)
		return NULL;

	if(base64_decode(str, &data, &len) != 0)
		return NULL;

	ret = flac_picture_parse_eat(data, len);

	if(!ret) {
		free(data);
		return NULL;
	}

	return ret;
}

flac_picture_t *flac_picture_parse_from_blob(const void *in, size_t len) {
	flac_picture_t *ret;
	void *data;

	if(!in || !len)
		return NULL;

	data = calloc(1, len + 1);
	if(!data)
		return NULL;

	memcpy(data, in, len);

	ret = flac_picture_parse_eat(data, len);

	if(!ret) {
		free(data);
		return NULL;
	}

	return ret;
}

void flac_picture_free(flac_picture_t *picture) {
	if(!picture)
		return;

	free(picture->private_data);
	free(picture);
}
