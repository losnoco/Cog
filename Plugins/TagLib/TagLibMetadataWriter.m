//
//  TagLibMetadataWriter.m
//  TagLib
//
//  Created by Safari on 08/11/17.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "TagLibMetadataWriter.h"

#import <tag/fileref.h>
#import <tag/tag.h>

#import "Logging.h"

static NSString *toString(id value) {
	if([value isKindOfClass:[NSString class]])
		return (NSString *)value;
	else if([value isKindOfClass:[NSArray class]]) {
		NSArray *arrayValue = value;
		return [arrayValue componentsJoinedByString:@", "];
	} else if([value isKindOfClass:[NSNumber class]]) {
		NSNumber *numberValue = value;
		return [NSString stringWithFormat:@"%@", numberValue];
	} else {
		return @"";
	}
}

static int toInt(id value) {
	if([value isKindOfClass:[NSNumber class]]) {
		NSNumber *numberValue = value;
		return [numberValue intValue];
	} else {
		return 0;
	}
}

static TagLib::Tag::ReplayGain toReplaygain(NSDictionary *tags) {
	TagLib::Tag::ReplayGain rg;
	NSNumber *lReplaygainAlbumGain = [tags valueForKey:@"replaygain_album_gain"];
	NSNumber *lReplaygainAlbumPeak = [tags valueForKey:@"replaygain_album_peak"];
	NSNumber *lReplaygainTrackGain = [tags valueForKey:@"replaygain_track_gain"];
	NSNumber *lReplaygainTrackPeak = [tags valueForKey:@"replaygain_track_peak"];
	if(lReplaygainAlbumGain)
		rg.setAlbumGain([lReplaygainAlbumGain floatValue]);
	if(lReplaygainAlbumPeak)
		rg.setAlbumPeak([lReplaygainAlbumPeak floatValue]);
	if(lReplaygainTrackGain)
		rg.setTrackGain([lReplaygainTrackGain floatValue]);
	if(lReplaygainTrackPeak)
		rg.setTrackPeak([lReplaygainTrackPeak floatValue]);
	return rg;
}

@implementation TagLibMetadataWriter

+ (int)putMetadataInURL:(NSURL *)url tagData:(NSDictionary *)tagData {
	NSString *lTitle = @"", *lAlbumArtist = @"", *lArtist = @"", *lComposer = @"", *lAlbum = @"", *lUnsyncedLyrics = @"", *lComment = @"", *lGenre = @"", *lCuesheet = @"";
	int lYear = 0, lTrack = 0, lDisc = 0;
	TagLib::Tag::ReplayGain lReplaygain;

	try {
		TagLib::FileRef f((const char *)[[url path] UTF8String], false);
		if(!f.isNull()) {
			const TagLib::Tag *tag = f.tag();

			if(tag) {
				bool altered = false;

				TagLib::String pTitle, pAlbumArtist, pArtist, pComposer, pAlbum, pUnsyncedLyrics, pComment, pGenre, pCuesheet;
				int pYear, pTrack, pDisc;
				TagLib::Tag::ReplayGain pReplaygain;

				lTitle = toString([tagData valueForKey:@"title"]);
				lAlbumArtist = toString([tagData valueForKey:@"albumartist"]);
				lArtist = toString([tagData valueForKey:@"artist"]);
				lComposer = toString([tagData valueForKey:@"composer"]);
				lAlbum = toString([tagData valueForKey:@"album"]);
				lUnsyncedLyrics = toString([tagData valueForKey:@"unsyncedlyrics"]);
				lComment = toString([tagData valueForKey:@"comment"]);
				lGenre = toString([tagData valueForKey:@"genre"]);
				lCuesheet = toString([tagData valueForKey:@"cuesheet"]);
				lYear = toInt([tagData valueForKey:@"year"]);
				lTrack = toInt([tagData valueForKey:@"track"]);
				lDisc = toInt([tagData valueForKey:@"disc"]);
				lReplaygain = toReplaygain(tagData);

				pTitle = tag->title();
				pAlbumArtist = tag->albumartist();
				pArtist = tag->artist();
				pComposer = tag->composer();
				pAlbum = tag->album();
				pUnsyncedLyrics = tag->unsyncedlyrics();
				pComment = tag->comment();
				pGenre = tag->genre();
				pCuesheet = tag->cuesheet();
				pYear = tag->year();
				pTrack = tag->track();
				pDisc = tag->disc();
				pReplaygain = tag->replaygain();

				if(pTitle.isEmpty() != ![lTitle length] ||
				   ![lTitle isEqualToString:[NSString stringWithUTF8String:pTitle.toCString(true)]]) {
					f.tag()->setTitle(TagLib::String([lTitle UTF8String]));
					altered = true;
				}

				if(pAlbumArtist.isEmpty() != ![lAlbumArtist length] ||
				   ![lAlbumArtist isEqualToString:[NSString stringWithUTF8String:pAlbumArtist.toCString(true)]]) {
					f.tag()->setAlbumArtist(TagLib::String([lAlbumArtist UTF8String]));
					altered = true;
				}

				if(pArtist.isEmpty() != ![lArtist length] ||
				   ![lArtist isEqualToString:[NSString stringWithUTF8String:pArtist.toCString(true)]]) {
					f.tag()->setArtist(TagLib::String([lArtist UTF8String]));
					altered = true;
				}

				if(pComposer.isEmpty() != ![lComposer length] ||
				   ![lComposer isEqualToString:[NSString stringWithUTF8String:pComposer.toCString(true)]]) {
					f.tag()->setComposer(TagLib::String([lComposer UTF8String]));
					altered = true;
				}

				if(pAlbum.isEmpty() != ![lAlbum length] ||
				   ![lAlbum isEqualToString:[NSString stringWithUTF8String:pAlbum.toCString(true)]]) {
					f.tag()->setAlbum(TagLib::String([lComposer UTF8String]));
					altered = true;
				}

				if(pUnsyncedLyrics.isEmpty() != ![lUnsyncedLyrics length] ||
				   ![lUnsyncedLyrics isEqualToString:[NSString stringWithUTF8String:pUnsyncedLyrics.toCString(true)]]) {
					f.tag()->setUnsyncedLyrics(TagLib::String([lUnsyncedLyrics UTF8String]));
					altered = true;
				}

				if(pComment.isEmpty() != ![lComment length] ||
				   ![lComment isEqualToString:[NSString stringWithUTF8String:pComment.toCString(true)]]) {
					f.tag()->setComment(TagLib::String([lComment UTF8String]));
					altered = true;
				}

				if(pGenre.isEmpty() != ![lGenre length] ||
				   ![lGenre isEqualToString:[NSString stringWithUTF8String:pGenre.toCString(true)]]) {
					f.tag()->setGenre(TagLib::String([lGenre UTF8String]));
					altered = true;
				}

				if(pCuesheet.isEmpty() != ![lCuesheet length] ||
				   ![lCuesheet isEqualToString:[NSString stringWithUTF8String:pCuesheet.toCString(true)]]) {
					f.tag()->setCuesheet(TagLib::String([lCuesheet UTF8String]));
					altered = true;
				}

				if(pYear != lYear) {
					f.tag()->setYear(lYear);
					altered = true;
				}

				if(pTrack != lTrack) {
					f.tag()->setTrack(lTrack);
					altered = true;
				}

				if(pDisc != lDisc) {
					f.tag()->setDisc(lDisc);
					altered = true;
				}

				if(pReplaygain != lReplaygain) {
					f.tag()->setReplaygain(lReplaygain);
					altered = true;
				}

				if(altered)
					f.save();
			}
		}
	} catch (std::exception &e) {
		ALog(@"Exception caught writing with TagLib: %s", e.what());
		return -1;
	}

	return 0;
}

@end
