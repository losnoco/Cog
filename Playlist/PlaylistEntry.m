//
//  PlaylistEntry.m
//  Cog
//
//  Created by Vincent Spader on 3/14/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Foundation/Foundation.h>

#import <CoreData/CoreData.h>

#import "PlaylistEntry.h"

#import "AVIFDecoder.h"
#import "SHA256Digest.h"
#import "SecondsFormatter.h"

extern NSPersistentContainer *kPersistentContainer;
extern NSMutableDictionary<NSString *, AlbumArtwork *> *kArtworkDictionary;

NSNotificationName const CogPlaylistEntryMetadataLoadedNotification = @"CogPlaylistEntryMetadataLoadedNotification";

@implementation PlaylistEntry (Extension)

// What an entry is wherever Cog runs (its URL, its tags, what is shown of
// them) is in CogPlaylist's PlaylistEntry+Extension.swift. What remains here
// needs AppKit, the app's art cache and Core Data stack, or its strings.

// The following read-only keys depend on the values of other properties

+ (NSSet *)keyPathsForValuesAffectingStatusMessage {
	return [NSSet setWithObjects:@"current", @"queued", @"queuePosition", @"error", @"errorMessage", @"stopAfter", nil];
}

+ (NSSet *)keyPathsForValuesAffectingSpam {
	return [NSSet setWithObjects:@"albumartist", @"artist", @"rawTitle", @"album", @"track", @"disc", @"totalFrames", @"currentPosition", @"bitrate", nil];
}

+ (NSSet *)keyPathsForValuesAffectingIndexedSpam {
	return [NSSet setWithObjects:@"albumartist", @"artist", @"rawTitle", @"album", @"track", @"disc", @"totalFrames", @"currentPosition", @"bitrate", @"index", nil];
}

+ (NSSet *)keyPathsForValuesAffectingPositionText {
	return [NSSet setWithObject:@"currentPosition"];
}

+ (NSSet *)keyPathsForValuesAffectingLengthText {
	return [NSSet setWithObject:@"length"];
}

+ (NSSet *)keyPathsForValuesAffectingLengthInfo {
	return [NSSet setWithObject:@"length"];
}

+ (NSSet *)keyPathsForValuesAffectingAlbumArt {
	return [NSSet setWithObjects:@"albumArtInternal", @"artId", nil];
}

+ (NSSet *)keyPathsForValuesAffectingGainCorrection {
	return [NSSet setWithObjects:@"replayGainAlbumGain", @"replayGainAlbumPeak", @"replayGainTrackGain", @"replayGainTrackPeak", @"soundcheck", @"volume", nil];
}

+ (NSSet *)keyPathsForValuesAffectingGainInfo {
	return [NSSet setWithObjects:@"replayGainAlbumGain", @"replayGainAlbumPeak", @"replayGainTrackGain", @"replayGainTrackPeak", @"soundcheck", @"volume", nil];
}

- (NSString *)description {
	return [NSString stringWithFormat:@"PlaylistEntry %lli:(%@)", self.index, self.url];
}

@dynamic indexedSpam;
- (NSString *)indexedSpam {
	return [NSString stringWithFormat:@"%llu. %@", self.index, self.spam];
}

@dynamic spam;
- (NSString *)spam {
	BOOL hasBitrate = (self.bitrate != 0);
	BOOL hasArtist = (self.artist != nil) && (![self.artist isEqualToString:@""]);
	BOOL hasAlbumArtist = (self.albumartist != nil) && (![self.albumartist isEqualToString:@""]);
	BOOL hasTrackArtist = (hasArtist && hasAlbumArtist) && (![self.albumartist isEqualToString:self.artist]);
	BOOL hasAlbum = (self.album != nil) && (![self.album isEqualToString:@""]);
	BOOL hasTrack = (self.track != 0);
	BOOL hasLength = (self.totalFrames != 0);
	BOOL hasCurrentPosition = (self.currentPosition != 0) && (self.current);
	BOOL hasExtension = NO;
	BOOL hasTitle = (self.rawTitle != nil) && (![self.rawTitle isEqualToString:@""]);
	BOOL hasCodec = (self.codec != nil) && (![self.codec isEqualToString:@""]);

	NSMutableString *filename = [NSMutableString stringWithString:self.filename];
	NSRange dotPosition = [filename rangeOfString:@"." options:NSBackwardsSearch];
	NSString *extension = nil;

	if(dotPosition.length > 0) {
		dotPosition.location++;
		dotPosition.length = [filename length] - dotPosition.location;
		extension = [filename substringWithRange:dotPosition];
		dotPosition.location--;
		dotPosition.length++;
		[filename deleteCharactersInRange:dotPosition];
		hasExtension = YES;
	}

	NSMutableArray *elements = [NSMutableArray array];

	if(hasExtension) {
		[elements addObject:@"["];
		if(hasCodec) {
			[elements addObject:self.codec];
		} else {
			[elements addObject:[extension uppercaseString]];
		}
		if(hasBitrate) {
			[elements addObject:@"@"];
			[elements addObject:[NSString stringWithFormat:@"%u", self.bitrate]];
			[elements addObject:@"kbps"];
		}
		[elements addObject:@"] "];
	}

	if(hasArtist) {
		if(hasAlbumArtist) {
			[elements addObject:self.albumartist];
		} else {
			[elements addObject:self.artist];
		}
		[elements addObject:@" - "];
	}

	if(hasAlbum) {
		[elements addObject:@"["];
		[elements addObject:self.album];
		if(hasTrack) {
			[elements addObject:@" #"];
			[elements addObject:self.trackText];
		}
		[elements addObject:@"] "];
	}

	if(hasTitle) {
		[elements addObject:self.rawTitle];
	} else {
		[elements addObject:filename];
	}

	if(hasTrackArtist) {
		[elements addObject:@" // "];
		[elements addObject:self.artist];
	}

	if(hasCurrentPosition || hasLength) {
		SecondsFormatter *secondsFormatter = [SecondsFormatter new];
		[elements addObject:@" ("];
		if(hasCurrentPosition) {
			[elements addObject:[secondsFormatter stringForObjectValue:@(self.currentPosition)]];
		}
		if(hasLength) {
			if(hasCurrentPosition) {
				[elements addObject:@" / "];
			}
			[elements addObject:[secondsFormatter stringForObjectValue:[self length]]];
		}
		[elements addObject:@")"];
	}

	return [elements componentsJoinedByString:@""];
}

@dynamic gainCorrection;
- (NSString *)gainCorrection {
	if(self.replayGainAlbumGain) {
		if(self.replayGainAlbumPeak)
			return NSLocalizedStringFromTableInBundle(@"GainAlbumGainPeak", nil, [NSBundle bundleForClass:[self class]], @"");
		else
			return NSLocalizedStringFromTableInBundle(@"GainAlbumGain", nil, [NSBundle bundleForClass:[self class]], @"");
	} else if(self.replayGainTrackGain) {
		if(self.replayGainTrackPeak)
			return NSLocalizedStringFromTableInBundle(@"GainTrackGainPeak", nil, [NSBundle bundleForClass:[self class]], @"");
		else
			return NSLocalizedStringFromTableInBundle(@"GainTrackGain", nil, [NSBundle bundleForClass:[self class]], @"");
	} else if(self.soundcheck && self.soundcheck.length) {
		return NSLocalizedStringFromTableInBundle(@"GainSoundcheck", nil, [NSBundle bundleForClass:[self class]], @"");
	} else if(self.volume && self.volume != 1.0) {
		return NSLocalizedStringFromTableInBundle(@"GainVolumeScale", nil, [NSBundle bundleForClass:[self class]], @"");
	} else {
		return NSLocalizedStringFromTableInBundle(@"GainNone", nil, [NSBundle bundleForClass:[self class]], @"");
	}
}

@dynamic gainInfo;
- (NSString *)gainInfo {
	NSMutableArray *gainItems = [NSMutableArray new];
	if(self.replayGainAlbumGain) {
		[gainItems addObject:[NSString stringWithFormat:@"%@: %+.2f dB", NSLocalizedStringFromTableInBundle(@"GainAlbumGain", nil, [NSBundle bundleForClass:[self class]], @""), self.replayGainAlbumGain]];
	}
	if(self.replayGainAlbumPeak) {
		[gainItems addObject:[NSString stringWithFormat:@"%@: %.6f", NSLocalizedStringFromTableInBundle(@"GainAlbumPeak", nil, [NSBundle bundleForClass:[self class]], @""), self.replayGainAlbumPeak]];
	}
	if(self.replayGainTrackGain) {
		[gainItems addObject:[NSString stringWithFormat:@"%@: %+.2f dB", NSLocalizedStringFromTableInBundle(@"GainTrackGain", nil, [NSBundle bundleForClass:[self class]], @""), self.replayGainTrackGain]];
	}
	if(self.replayGainTrackPeak) {
		[gainItems addObject:[NSString stringWithFormat:@"%@: %.6f", NSLocalizedStringFromTableInBundle(@"GainTrackPeak", nil, [NSBundle bundleForClass:[self class]], @""), self.replayGainTrackPeak]];
	}
	if(self.soundcheck && self.soundcheck.length) {
		NSString *scdisplay = self.soundcheckDisplay;
		if(scdisplay && scdisplay.length)
			[gainItems addObject:[NSString stringWithFormat:@"%@: %@", NSLocalizedStringFromTableInBundle(@"GainSoundcheck", nil, [NSBundle bundleForClass:[self class]], @""), scdisplay]];
	}
	if(self.volume && self.volume != 1) {
		[gainItems addObject:[NSString stringWithFormat:@"%@: %.2f%C", NSLocalizedStringFromTableInBundle(@"GainVolumeScale", nil, [NSBundle bundleForClass:[self class]], @""), self.volume, (unichar)0x00D7]];
	}
	return [gainItems componentsJoinedByString:@"\n"];
}

@dynamic positionText;
- (NSString *)positionText {
	SecondsFormatter *secondsFormatter = [SecondsFormatter new];
	NSString *time = [secondsFormatter stringForObjectValue:@(self.currentPosition)];
	return time;
}

@dynamic lengthText;
- (NSString *)lengthText {
	SecondsFormatter *secondsFormatter = [SecondsFormatter new];
	NSString *time = [secondsFormatter stringForObjectValue:self.length];
	return time;
}

@dynamic lengthInfo;
- (NSString *)lengthInfo {
	SecondsFractionFormatter * secondsFormatter = [SecondsFractionFormatter new];
	NSString *time = [secondsFormatter stringForObjectValue:self.length];
	return time;
}

@dynamic albumArt;
- (NSImage *)albumArt {
	if(!self.albumArtInternal || ![self.albumArtInternal length]) return nil;

	NSString *imageCacheTag = self.artHash;
	NSImage *image = [NSImage imageNamed:imageCacheTag];

	if(image == nil) {
		if(@available(macOS 13.0, *)) {
			image = [[NSImage alloc] initWithData:self.albumArtInternal];
		} else {
			if([AVIFDecoder isAVIFFormatForData:self.albumArtInternal]) {
				CGImageRef imageRef = [AVIFDecoder createAVIFImageWithData:self.albumArtInternal];
				if(imageRef) {
					image = [[NSImage alloc] initWithCGImage:imageRef size:NSZeroSize];
					CFRelease(imageRef);
				}
			} else {
				image = [[NSImage alloc] initWithData:self.albumArtInternal];
			}
		}
		[image setName:imageCacheTag];
	}

	return image;
}

- (void)setAlbumArt:(id)data {
	if([data isKindOfClass:[NSData class]]) {
		[self setAlbumArtInternal:data];
	}
}

@dynamic albumArtInternal;
- (NSData *)albumArtInternal {
	NSString *imageCacheTag = self.artHash;
	return [kArtworkDictionary objectForKey:imageCacheTag].artData;
}

- (void)setAlbumArtInternal:(NSData *)albumArtInternal {
	if(!albumArtInternal || [albumArtInternal length] == 0) return;

	Class shaClass = NSClassFromString(@"SHA256Digest"); // CogAudio
	NSString *imageCacheTag = [shaClass digestDataAsString:albumArtInternal];

	self.artHash = imageCacheTag;

	if(![kArtworkDictionary objectForKey:imageCacheTag]) {
		AlbumArtwork *art = [NSEntityDescription insertNewObjectForEntityForName:@"AlbumArtwork" inManagedObjectContext:kPersistentContainer.viewContext];
		art.artHash = imageCacheTag;
		art.artData = albumArtInternal;

		[kArtworkDictionary setObject:art forKey:imageCacheTag];
	}
}

@dynamic urlBookmark;

@dynamic statusMessage;
- (NSString *)statusMessage {
	if(self.stopAfter) {
		return NSLocalizedStringFromTableInBundle(@"StatusStopAfter", nil, [NSBundle bundleForClass:[self class]], @"");
	} else if(self.current) {
		return NSLocalizedStringFromTableInBundle(@"StatusPlaying", nil, [NSBundle bundleForClass:[self class]], @"");
	} else if(self.queued) {
		return [NSString stringWithFormat:NSLocalizedStringFromTableInBundle(@"StatusQueued", nil, [NSBundle bundleForClass:[self class]], @""), self.queuePosition + 1];
	} else if(self.error) {
		return self.errorMessage;
	}

	return nil;
}

- (void)setMetadata:(NSDictionary *)metadata {
	if(metadata == nil) {
		self.error = YES;
		self.errorMessage = NSLocalizedStringFromTableInBundle(@"ErrorMetadata", nil, [NSBundle bundleForClass:[self class]], @"");
	} else {
		NSDictionary *originalDict = (NSDictionary * _Nullable) self.metadataBlob;
		NSMutableDictionary *metaDict;
		if(originalDict) {
			metaDict = [originalDict mutableCopy];
		} else {
			metaDict = [NSMutableDictionary new];
		}
		self.volume = 1;
		for(NSString *key in metadata) {
			NSString *tagName = [PlaylistEntry metaTagForKey:key];
			NSString *lowerKey = [tagName lowercaseString];
			id valueObj = [metadata objectForKey:key];
			id genericValue;
			NSArray *values = nil;
			NSString *firstValue = nil;
			NSData *dataValue = nil;
			if([valueObj isKindOfClass:[NSArray class]]) {
				values = (NSArray *)valueObj;
				if([values count]) {
					firstValue = values[0];
				}
				genericValue = values;
			} else if([valueObj isKindOfClass:[NSString class]]) {
				firstValue = (NSString *)valueObj;
				values = @[firstValue];
				genericValue = values;
			} else if([valueObj isKindOfClass:[NSNumber class]]) {
				NSNumber *numberValue = (NSNumber *)valueObj;
				firstValue = [numberValue stringValue];
				values = @[firstValue];
				genericValue = values;
			} else if([valueObj isKindOfClass:[NSData class]]) {
				dataValue = (NSData *)valueObj;
				genericValue = dataValue;
			} else {
				// Unknown object in metadata block
				genericValue = valueObj;
			}
			if([lowerKey isEqualToString:@"bitrate"]) {
				self.bitrate = [firstValue intValue];
			} else if([lowerKey isEqualToString:@"bitspersample"]) {
				self.bitsPerSample = [firstValue intValue];
			} else if([lowerKey isEqualToString:@"channelconfig"]) {
				self.channelConfig = [firstValue intValue];
			} else if([lowerKey isEqualToString:@"channels"]) {
				self.channels = [firstValue intValue];
			} else if([lowerKey isEqualToString:@"codec"]) {
				self.codec = firstValue;
			} else if([lowerKey isEqualToString:@"cuesheet"]) {
				self.cuesheet = firstValue;
			} else if([lowerKey isEqualToString:@"encoding"]) {
				self.encoding = firstValue;
			} else if([lowerKey isEqualToString:@"endian"]) {
				self.endian = firstValue;
			} else if([lowerKey isEqualToString:@"floatingpoint"]) {
				self.floatingPoint = [firstValue boolValue];
			} else if([lowerKey isEqualToString:@"samplerate"]) {
				self.sampleRate = [firstValue floatValue];
			} else if([lowerKey isEqualToString:@"seekable"]) {
				self.seekable = [firstValue boolValue];
			} else if([lowerKey isEqualToString:@"totalframes"]) {
				self.totalFrames = [firstValue integerValue];
			} else if([lowerKey isEqualToString:@"unsigned"]) {
				self.unSigned = [firstValue boolValue];
			} else if([lowerKey isEqualToString:@"replaygain_album_gain"]) {
				self.replayGainAlbumGain = [firstValue floatValue];
			} else if([lowerKey isEqualToString:@"replaygain_album_peak"]) {
				self.replayGainAlbumPeak = [firstValue floatValue];
			} else if([lowerKey isEqualToString:@"replaygain_track_gain"]) {
				self.replayGainTrackGain = [firstValue floatValue];
			} else if([lowerKey isEqualToString:@"replaygain_track_peak"]) {
				self.replayGainTrackPeak = [firstValue floatValue];
			} else if([lowerKey isEqualToString:@"soundcheck"]) {
				self.soundcheck = firstValue;
			} else if([lowerKey isEqualToString:@"volume"]) {
				self.volume = [firstValue floatValue];
			} else if([lowerKey isEqualToString:@"albumart"]) {
				self.albumArt = dataValue;
			} else {
				[metaDict setObject:genericValue forKey:key];
			}
		}
		self.metadataBlob = [NSDictionary dictionaryWithDictionary:metaDict];
	}

	[self setMetadataLoaded:YES];

	[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaylistEntryMetadataLoadedNotification object:self];
}

@dynamic playCountItem;
- (PlayCount *)playCountItem {
	NSPredicate *albumPredicate = [NSPredicate predicateWithFormat:@"album == %@", self.album];
	NSPredicate *artistPredicate = [NSPredicate predicateWithFormat:@"artist == %@", self.artist];
	NSPredicate *titlePredicate = [NSPredicate predicateWithFormat:@"title == %@", self.title];

	NSCompoundPredicate *predicate = [NSCompoundPredicate andPredicateWithSubpredicates:@[albumPredicate, artistPredicate, titlePredicate]];

	__block BOOL fixtags = NO;

	__block PlayCount *item = nil;

	[kPersistentContainer.viewContext performBlockAndWait:^{
		NSFetchRequest *request = [NSFetchRequest fetchRequestWithEntityName:@"PlayCount"];
		request.predicate = predicate;

		NSError *error = nil;
		NSArray *results = [kPersistentContainer.viewContext executeFetchRequest:request error:&error];

		if(!results || [results count] < 1) {
			NSPredicate *filenamePredicate = [NSPredicate predicateWithFormat:@"filename == %@", self.filenameFragment];

			request = [NSFetchRequest fetchRequestWithEntityName:@"PlayCount"];
			request.predicate = filenamePredicate;

			results = [kPersistentContainer.viewContext executeFetchRequest:request error:&error];
			if(!results || [results count] < 1) {
				filenamePredicate = [NSPredicate predicateWithFormat:@"filename == %@", self.filename];

				request = [NSFetchRequest fetchRequestWithEntityName:@"PlayCount"];
				request.predicate = filenamePredicate;

				results = [kPersistentContainer.viewContext executeFetchRequest:request error:&error];
			}

			if(results && [results count] >= 1) {
				fixtags = YES;
			}
		}

		if(!results || [results count] < 1) return;

		item = results[0];
	}];

	if(fixtags) {
		// shoot, something inserted the play counts without the tags
		[kPersistentContainer.viewContext performBlockAndWait:^{
			item.album = self.album;
			item.artist = self.artist;
			item.title = self.title;
			item.filename = self.filenameFragment;
		}];

		NSError *error = nil;
		[kPersistentContainer.viewContext save:&error];
	}

	return item;
}

@dynamic playCount;
- (NSString *)playCount {
	PlayCount *pc = self.playCountItem;
	if(pc)
		return [NSString stringWithFormat:@"%llu", pc.count];
	else
		return @"0";
}

@dynamic playCountInfo;
- (NSString *)playCountInfo {
	PlayCount *pc = self.playCountItem;
	if(pc) {
		NSDateFormatter *dateFormatter = [NSDateFormatter new];
		dateFormatter.dateStyle = NSDateFormatterMediumStyle;
		dateFormatter.timeStyle = NSDateFormatterShortStyle;

		if(pc.count) {
			return [NSString stringWithFormat:@"%@: %@\n%@: %@", NSLocalizedStringFromTableInBundle(@"TimeFirstSeen", nil, [NSBundle bundleForClass:[self class]], @""), [dateFormatter stringFromDate:pc.firstSeen], NSLocalizedStringFromTableInBundle(@"TimeLastPlayed", nil, [NSBundle bundleForClass:[self class]], @""), [dateFormatter stringFromDate:pc.lastPlayed]];
		} else {
			return [NSString stringWithFormat:@"%@: %@", NSLocalizedStringFromTableInBundle(@"TimeFirstSeen", nil, [NSBundle bundleForClass:[self class]], @""), [dateFormatter stringFromDate:pc.firstSeen]];
		}
	}
	return @"";
}

@dynamic rating;
- (float)rating {
	PlayCount *pc = self.playCountItem;
	if(pc) {
		return pc.rating;
	} else {
		return 0;
	}
}

- (AudioScrobblerTrack *_Nonnull)audioScrobblerTrack {
    AudioScrobblerTrack *track = [[AudioScrobblerTrack alloc] initWithTitle:self.title
                                                                     artist:self.artist
                                                                albumArtist:self.albumartist
                                                                      album:self.album
                                                                trackNumber:self.track
                                                                     length:[self.length doubleValue]];
    // TagLib, FLAC, Vorbis and Opus lowercase the Picard names; FFmpeg keeps
    // the MP4/ID3 spelling.
    track.recordingMBID = [self readAllValuesAsString:@"musicbrainz_trackid"] ?: [self readAllValuesAsString:@"musicbrainz track id"];
    track.releaseMBID = [self readAllValuesAsString:@"musicbrainz_albumid"] ?: [self readAllValuesAsString:@"musicbrainz album id"];
    NSString *artistIDs = [self readAllValuesAsString:@"musicbrainz_artistid"] ?: [self readAllValuesAsString:@"musicbrainz artist id"];
    if([artistIDs length]) {
        NSMutableArray<NSString *> *ids = [NSMutableArray array];
        for(NSString *part in [artistIDs componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@",/;"]]) {
            NSString *trimmed = [part stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
            if([trimmed length]) {
                [ids addObject:trimmed];
            }
        }
        track.artistMBIDs = ids;
    }
    return track;
}
@end
