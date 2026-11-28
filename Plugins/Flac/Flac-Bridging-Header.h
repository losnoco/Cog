//
//  Flac-Bridging-Header.h
//  Flac
//
//  The C APIs the Swift decoder reads through.
//

#import <Foundation/Foundation.h>

#import <FLAC/all.h>

#import "../../Utils/RedundantPlaylistDataStore.h"
#import "../../Utils/NSDictionary+Merge.h"

// The cuesheet grabbag in cuesheet.m.
void grabbag__cuesheet_emit(NSString **out, const FLAC__StreamMetadata *cuesheet, const char *file_reference);

static const size_t FlacBlockBufferSize = ((FLAC__MAX_BLOCK_SIZE + 512) * FLAC__MAX_CHANNELS * (FLAC__MAX_BITS_PER_SAMPLE / 8));
