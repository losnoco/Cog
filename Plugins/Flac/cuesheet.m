/* grabbag - Convenience lib for various routines common to several tools
 * Copyright (C) 2002-2009  Josh Coalson
 * Copyright (C) 2011-2016  Xiph.Org Foundation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#import <Foundation/Foundation.h>

#include <stdlib.h>
#include <string.h>

#include <FLAC/FLAC_assert.h>
#include <FLAC/all.h>

uint32_t grabbag__cuesheet_msf_to_frame(uint32_t minutes, uint32_t seconds, uint32_t frames) {
	return ((minutes * 60) + seconds) * 75 + frames;
}

void grabbag__cuesheet_frame_to_msf(uint32_t frame, uint32_t *minutes, uint32_t *seconds, uint32_t *frames) {
	*frames = frame % 75;
	frame /= 75;
	*seconds = frame % 60;
	frame /= 60;
	*minutes = frame;
}

void grabbag__cuesheet_emit(NSString **out, const FLAC__StreamMetadata *cuesheet, const char *file_reference) {
	const FLAC__StreamMetadata_CueSheet *cs;
	uint32_t track_num, index_num;

	FLAC__ASSERT(0 != out);
	FLAC__ASSERT(0 != cuesheet);
	FLAC__ASSERT(cuesheet->type == FLAC__METADATA_TYPE_CUESHEET);

	NSMutableArray *stringList = [NSMutableArray new];

	cs = &cuesheet->data.cue_sheet;

	if(*(cs->media_catalog_number))
		[stringList addObject:[NSString stringWithFormat:@"CATALOG %s\n", cs->media_catalog_number]];
	[stringList addObject:[NSString stringWithFormat:@"FILE %s\n", file_reference]];

	for(track_num = 0; track_num < cs->num_tracks - 1; track_num++) {
		const FLAC__StreamMetadata_CueSheet_Track *track = cs->tracks + track_num;

		[stringList addObject:[NSString stringWithFormat:@"  TRACK %02u %s\n", (uint32_t)track->number, track->type == 0 ? "AUDIO" : "DATA"]];

		if(track->pre_emphasis)
			[stringList addObject:@"    FLAGS PRE\n"];
		if(*(track->isrc))
			[stringList addObject:[NSString stringWithFormat:@"    ISRC %s\n", track->isrc]];

		for(index_num = 0; index_num < track->num_indices; index_num++) {
			const FLAC__StreamMetadata_CueSheet_Index *indx = track->indices + index_num;

			[stringList addObject:[NSString stringWithFormat:@"    INDEX %02u ", (uint32_t)indx->number]];
			if(cs->is_cd) {
				const uint32_t logical_frame = (uint32_t)((track->offset + indx->offset) / (44100 / 75));
				uint32_t m, s, f;
				grabbag__cuesheet_frame_to_msf(logical_frame, &m, &s, &f);
				[stringList addObject:[NSString stringWithFormat:@"%02u:%02u:%02u\n", m, s, f]];
			} else
				[stringList addObject:[NSString stringWithFormat:@"%" PRIu64 "\n", (track->offset + indx->offset)]];
		}
	}

	[stringList addObject:[NSString stringWithFormat:@"REM FLAC__lead-in %" PRIu64 "\n", cs->lead_in]];
	[stringList addObject:[NSString stringWithFormat:@"REM FLAC__lead-out %u %" PRIu64 "\n", (uint32_t)cs->tracks[track_num].number, cs->tracks[track_num].offset]];

	*out = [stringList componentsJoinedByString:@""];
}
