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

static NSMutableDictionary *kMetadataCache = nil;

@implementation PlaylistEntry (Extension)

+ (void)initialize {
	if(!kMetadataCache) {
		kMetadataCache = [[NSMutableDictionary alloc] init];
	}
}

// The following read-only keys depend on the values of other properties

+ (NSSet *)keyPathsForValuesAffectingUrl {
	return [NSSet setWithObject:@"urlString"];
}

+ (NSSet *)keyPathsForValuesAffectingTrashUrl {
	return [NSSet setWithObject:@"trashUrlString"];
}

+ (NSSet *)keyPathsForValuesAffectingTitle {
	return [NSSet setWithObject:@"rawTitle"];
}

+ (NSSet *)keyPathsForValuesAffectingDisplay {
	return [NSSet setWithObjects:@"artist", @"title", nil];
}

+ (NSSet *)keyPathsForValuesAffectingLength {
	return [NSSet setWithObjects:@"metadataLoaded", @"totalFrames", @"sampleRate", nil];
}

+ (NSSet *)keyPathsForValuesAffectingPath {
	return [NSSet setWithObject:@"url"];
}

+ (NSSet *)keyPathsForValuesAffectingFilename {
	return [NSSet setWithObject:@"url"];
}

+ (NSSet *)keyPathsForValuesAffectingFilenameFragment {
	return [NSSet setWithObject:@"url"];
}

+ (NSSet *)keyPathsForValuesAffectingStatus {
	return [NSSet setWithObjects:@"current", @"queued", @"error", @"stopAfter", nil];
}

+ (NSSet *)keyPathsForValuesAffectingStatusMessage {
	return [NSSet setWithObjects:@"current", @"queued", @"queuePosition", @"error", @"errorMessage", @"stopAfter", nil];
}

+ (NSSet *)keyPathsForValuesAffectingSpam {
	return [NSSet setWithObjects:@"albumartist", @"artist", @"rawTitle", @"album", @"track", @"disc", @"totalFrames", @"currentPosition", @"bitrate", nil];
}

+ (NSSet *)keyPathsForValuesAffectingIndexedSpam {
	return [NSSet setWithObjects:@"albumartist", @"artist", @"rawTitle", @"album", @"track", @"disc", @"totalFrames", @"currentPosition", @"bitrate", @"index", nil];
}

+ (NSSet *)keyPathsForValuesAffectingTrackText {
	return [NSSet setWithObjects:@"track", @"disc", nil];
}

+ (NSSet *)keyPathsForValuesAffectingYearText {
	return [NSSet setWithObject:@"year"];
}

+ (NSSet *)keyPathsForValuesAffectingCuesheetPresent {
	return [NSSet setWithObject:@"cuesheet"];
}

+ (NSSet *)keyPathsForValuesAffectingPositionText {
	return [NSSet setWithObject:@"currentPosition"];
}

+ (NSSet *)keyPathsForValuesAffectingLengthText {
	return [NSSet setWithObject:@"length"];
}

+ (NSSet *)keyPathsForValuesAffectingAlbumArt {
	return [NSSet setWithObjects:@"albumArtInternal", @"artId", nil];
}

+ (NSSet *)keyPathsForValuesAffectingGainCorrection {
	return [NSSet setWithObjects:@"replayGainAlbumGain", @"replayGainAlbumPeak", @"replayGainTrackGain", @"replayGainTrackPeak", @"volume", nil];
}

+ (NSSet *)keyPathsForValuesAffectingGainInfo {
	return [NSSet setWithObjects:@"replayGainAlbumGain", @"replayGainAlbumPeak", @"replayGainTrackGain", @"replayGainTrackPeak", @"volume", nil];
}

+ (NSSet *)keyPathsForValuesAffectingUnsigned {
	return [NSSet setWithObject:@"unSigned"];
}

- (NSString *)description {
	return [NSString stringWithFormat:@"PlaylistEntry %lli:(%@)", self.index, self.url];
}

// Get the URL if the title is blank
@dynamic title;
- (NSString *)title {
	if((self.rawTitle == nil || [self.rawTitle isEqualToString:@""]) && self.url) {
		return [[self.url path] lastPathComponent];
	}
	return self.rawTitle;
}

- (void)setTitle:(NSString *)title {
	self.rawTitle = title;
}

@dynamic display;
- (NSString *)display {
	if((self.artist == NULL) || ([self.artist isEqualToString:@""]))
		return self.title;
	else {
		return [NSString stringWithFormat:@"%@ - %@", self.artist, self.title];
	}
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
		SecondsFormatter *secondsFormatter = [[SecondsFormatter alloc] init];
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

@dynamic trackText;
- (NSString *)trackText {
	if(self.track != 0) {
		if(self.disc != 0) {
			return [NSString stringWithFormat:@"%u.%02u", self.disc, self.track];
		} else {
			return [NSString stringWithFormat:@"%02u", self.track];
		}
	} else {
		return @"";
	}
}

@dynamic yearText;
- (NSString *)yearText {
	if(self.year != 0) {
		return [NSString stringWithFormat:@"%u", self.year];
	} else {
		return @"";
	}
}

@dynamic cuesheetPresent;
- (NSString *)cuesheetPresent {
	if(self.cuesheet && [self.cuesheet length]) {
		return @"yes";
	} else {
		return @"no";
	}
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
	} else if(self.volume && self.volume != 1.0) {
		return NSLocalizedStringFromTableInBundle(@"GainVolumeScale", nil, [NSBundle bundleForClass:[self class]], @"");
	} else {
		return NSLocalizedStringFromTableInBundle(@"GainNone", nil, [NSBundle bundleForClass:[self class]], @"");
	}
}

@dynamic gainInfo;
- (NSString *)gainInfo {
	NSMutableArray *gainItems = [[NSMutableArray alloc] init];
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
	if(self.volume && self.volume != 1) {
		[gainItems addObject:[NSString stringWithFormat:@"%@: %.2f%C", NSLocalizedStringFromTableInBundle(@"GainVolumeScale", nil, [NSBundle bundleForClass:[self class]], @""), self.volume, (unichar)0x00D7]];
	}
	return [gainItems componentsJoinedByString:@"\n"];
}

@dynamic positionText;
- (NSString *)positionText {
	SecondsFormatter *secondsFormatter = [[SecondsFormatter alloc] init];
	NSString *time = [secondsFormatter stringForObjectValue:@(self.currentPosition)];
	return time;
}

@dynamic lengthText;
- (NSString *)lengthText {
	SecondsFormatter *secondsFormatter = [[SecondsFormatter alloc] init];
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

	NSString *imageCacheTag = [SHA256Digest digestDataAsString:albumArtInternal];

	self.artHash = imageCacheTag;

	if(![kArtworkDictionary objectForKey:imageCacheTag]) {
		AlbumArtwork *art = [NSEntityDescription insertNewObjectForEntityForName:@"AlbumArtwork" inManagedObjectContext:kPersistentContainer.viewContext];
		art.artHash = imageCacheTag;
		art.artData = albumArtInternal;

		[kArtworkDictionary setObject:art forKey:imageCacheTag];
	}
}

@dynamic length;
- (NSNumber *)length {
	return (self.metadataLoaded) ? @(((double)self.totalFrames / self.sampleRate)) : @(0.0);
}

NSURL *_Nullable urlForPath(NSString *_Nullable path) {
	if(!path || ![path length]) {
		return nil;
	}

	NSRange protocolRange = [path rangeOfString:@"://"];
	if(protocolRange.location != NSNotFound) {
		return [NSURL URLWithString:path];
	}

	NSMutableString *unixPath = [path mutableCopy];

	// Get the fragment
	NSString *fragment = @"";
	NSScanner *scanner = [NSScanner scannerWithString:unixPath];
	NSCharacterSet *characterSet = [NSCharacterSet characterSetWithCharactersInString:@"#1234567890"];
	while(![scanner isAtEnd]) {
		NSString *possibleFragment;
		[scanner scanUpToString:@"#" intoString:nil];

		if([scanner scanCharactersFromSet:characterSet intoString:&possibleFragment] && [scanner isAtEnd]) {
			fragment = possibleFragment;
			[unixPath deleteCharactersInRange:NSMakeRange([scanner scanLocation] - [possibleFragment length], [possibleFragment length])];
			break;
		}
	}

	// Append the fragment
	NSURL *url = [NSURL URLWithString:[[[NSURL fileURLWithPath:unixPath] absoluteString] stringByAppendingString:fragment]];
	return url;
}

@dynamic url;
- (NSURL *)url {
	return urlForPath(self.urlString);
}

- (void)setUrl:(NSURL *)url {
	self.urlString = url ? [url absoluteString] : nil;
}

@dynamic trashUrl;
- (NSURL *)trashUrl {
	return urlForPath(self.trashUrlString);
}

- (void)setTrashUrl:(NSURL *)trashUrl {
	self.trashUrlString = trashUrl ? [trashUrl absoluteString] : nil;
}

@dynamic path;
- (NSString *)path {
	if([self.url isFileURL])
		return [[self.url path] stringByAbbreviatingWithTildeInPath];
	else
		return [self.url absoluteString];
}

@dynamic filename;
- (NSString *)filename {
	return [[self.url path] lastPathComponent];
}

@dynamic filenameFragment;
- (NSString *)filenameFragment {
	if([self.url fragment]) {
		return [[[self.url path] lastPathComponent] stringByAppendingFormat:@"#%@", [self.url fragment]];
	} else {
		return self.filename;
	}
}

@dynamic status;
- (NSString *)status {
	if(self.stopAfter) {
		return @"stopAfter";
	} else if(self.current) {
		return @"playing";
	} else if(self.queued) {
		return @"queued";
	} else if(self.error) {
		return @"error";
	}

	return nil;
}

@dynamic statusMessage;
- (NSString *)statusMessage {
	if(self.stopAfter) {
		return @"Stopping once finished...";
	} else if(self.current) {
		return @"Playing...";
	} else if(self.queued) {
		return [NSString stringWithFormat:@"Queued: %lli", self.queuePosition + 1];
	} else if(self.error) {
		return self.errorMessage;
	}

	return nil;
}

// Gotta love that requirement of Core Data that everything starts with a lower case letter
@dynamic Unsigned;
- (BOOL)Unsigned {
	return self.unSigned;
}

- (void)setUnsigned:(BOOL)Unsigned {
	self.unSigned = Unsigned;
}

// More of the same
@dynamic URL;
- (NSURL *)URL {
	return self.url;
}

- (void)setURL:(NSURL *)URL {
	self.url = URL;
}

- (void)setMetadata:(NSDictionary *)metadata {
	if(metadata == nil) {
		self.error = YES;
		self.errorMessage = NSLocalizedStringFromTableInBundle(@"ErrorMetadata", nil, [NSBundle bundleForClass:[self class]], @"");
	} else {
		NSMutableDictionary *metaDict = [[NSMutableDictionary alloc] init];
		self.volume = 1;
		for(NSString *key in metadata) {
			NSString *lowerKey = [key lowercaseString];
			id valueObj = [metadata objectForKey:key];
			NSArray *values = nil;
			NSString *firstValue = nil;
			NSData *dataValue = nil;
			if([valueObj isKindOfClass:[NSArray class]]) {
				values = (NSArray *)valueObj;
				if([values count]) {
					firstValue = values[0];
				}
			} else if([valueObj isKindOfClass:[NSString class]]) {
				firstValue = (NSString *)valueObj;
				values = @[firstValue];
			} else if([valueObj isKindOfClass:[NSNumber class]]) {
				NSNumber *numberValue = (NSNumber *)valueObj;
				firstValue = [numberValue stringValue];
				values = @[firstValue];
			} else if([valueObj isKindOfClass:[NSData class]]) {
				dataValue = (NSData *)valueObj;
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
			} else if([lowerKey isEqualToString:@"volume"]) {
				self.volume = [firstValue floatValue];
			} else if([lowerKey isEqualToString:@"albumart"]) {
				self.albumArt = dataValue;
			} else {
				[metaDict setObject:values forKey:lowerKey];
			}
		}
		self.metadataBlob = [NSDictionary dictionaryWithDictionary:metaDict];
	}

	[self setMetadataLoaded:YES];
}

@dynamic playCountItem;
- (PlayCount *)playCountItem {
	NSPredicate *albumPredicate = [NSPredicate predicateWithFormat:@"album == %@", self.album];
	NSPredicate *artistPredicate = [NSPredicate predicateWithFormat:@"artist == %@", self.artist];
	NSPredicate *titlePredicate = [NSPredicate predicateWithFormat:@"title == %@", self.title];

	NSCompoundPredicate *predicate = [NSCompoundPredicate andPredicateWithSubpredicates:@[albumPredicate, artistPredicate, titlePredicate]];

	NSFetchRequest *request = [NSFetchRequest fetchRequestWithEntityName:@"PlayCount"];
	request.predicate = predicate;

	NSError *error = nil;
	NSArray *results = [kPersistentContainer.viewContext executeFetchRequest:request error:&error];

	if(!results || [results count] < 1) {
		NSPredicate *filenamePredicate = [NSPredicate predicateWithFormat:@"filename == %@", self.filenameFragment];

		request = [NSFetchRequest fetchRequestWithEntityName:@"PlayCount"];
		request.predicate = filenamePredicate;

		results = [kPersistentContainer.viewContext executeFetchRequest:request error:&error];
	}

	if(!results || [results count] < 1) return nil;

	return results[0];
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
		NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
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

@dynamic album;
- (NSString *)album {
	return [self readAllValuesAsString:@"album"];
}

- (void)setAlbum:(NSString *)album {
	[self setValue:@"album" fromString:album];
}

@dynamic albumartist;
- (NSString *)albumartist {
	NSString *value = [self readAllValuesAsString:@"albumartist"];
	if(!value) {
		value = [self readAllValuesAsString:@"album artist"];
	}
	if(!value) {
		value = [self readAllValuesAsString:@"album_artist"];
	}
	return value;
}

- (void)setAlbumartist:(NSString *)albumartist {
	[self setValue:@"albumartist" fromString:albumartist];
	[self setValue:@"album artist" fromString:nil];
	[self setValue:@"album_artist" fromString:nil];
}

@dynamic artist;
- (NSString *)artist {
	return [self readAllValuesAsString:@"artist"];
}

- (void)setArtist:(NSString *)artist {
	[self setValue:@"artist" fromString:artist];
}

@dynamic rawTitle;
- (NSString *)rawTitle {
	return [self readAllValuesAsString:@"title"];
}

- (void)setRawTitle:(NSString *)rawTitle {
	[self setValue:@"title" fromString:rawTitle];
}

@dynamic genre;
- (NSString *)genre {
	return [self readAllValuesAsString:@"genre"];
}

- (void)setGenre:(NSString *)genre {
	[self setValue:@"genre" fromString:genre];
}

@dynamic disc;
- (int32_t)disc {
	NSString *value = [self readAllValuesAsString:@"discnumber"];
	if(!value) {
		value = [self readAllValuesAsString:@"discnum"];
	}
	if(!value) {
		value = [self readAllValuesAsString:@"disc"];
	}
	if(value) {
		return [value intValue];
	} else {
		return 0;
	}
}

- (void)setDisc:(int32_t)disc {
	[self setValue:@"discnumber" fromString:[NSString stringWithFormat:@"%u", disc]];
	[self setValue:@"discnum" fromString:nil];
	[self setValue:@"disc" fromString:nil];
}

@dynamic track;
- (int32_t)track {
	NSString *value = [self readAllValuesAsString:@"tracknumber"];
	if(!value) {
		value = [self readAllValuesAsString:@"tracknum"];
	}
	if(!value) {
		value = [self readAllValuesAsString:@"track"];
	}
	if(value) {
		return [value intValue];
	} else {
		return 0;
	}
}

@dynamic year;
- (int32_t)year {
	NSString *value = [self readAllValuesAsString:@"date"];
	if(!value) {
		value = [self readAllValuesAsString:@"recording_date"];
	}
	if(!value) {
		value = [self readAllValuesAsString:@"year"];
	}
	if(value) {
		return [value intValue];
	} else {
		return 0;
	}
}

- (void)setYear:(int32_t)year {
	NSString *svalue = [NSString stringWithFormat:@"%u", year];
	[self setValue:@"year" fromString:svalue];
	[self setValue:@"date" fromString:nil];
	[self setValue:@"recording_date" fromString:nil];
}

@dynamic date;
- (NSString *)date {
	NSString *value = [self readAllValuesAsString:@"date"];
	if(!value) {
		value = [self readAllValuesAsString:@"recording_date"];
	}
	if(!value) {
		value = [self readAllValuesAsString:@"year"];
	}
	return value;
}

- (void)setDate:(NSString *)date {
	[self setValue:@"date" fromString:date];
	[self setValue:@"recording_date" fromString:nil];
	[self setValue:@"year" fromString:nil];
}

@dynamic comment;
- (NSString *)comment {
	return [self readAllValuesAsString:@"comment"];
}

- (void)setComment:(NSString *)comment {
	[self setValue:@"comment" fromString:comment];
}

- (NSString *_Nullable)readAllValuesAsString:(NSString *_Nonnull)tagName {
	NSMutableDictionary *dict = [kMetadataCache objectForKey:self.urlString];
	if(dict) {
		NSString *value = [dict objectForKey:tagName];
		if(value) {
			return value;
		}
	}

	id metaObj = self.metadataBlob;

	if(metaObj && [metaObj isKindOfClass:[NSDictionary class]]) {
		NSDictionary *metaDict = (NSDictionary *)metaObj;

		NSArray *values = [metaDict objectForKey:tagName];

		if(values) {
			NSString *value = [values componentsJoinedByString:@", "];
			if(!dict) {
				dict = [[NSMutableDictionary alloc] init];
				[kMetadataCache setObject:dict forKey:self.urlString];
			}
			[dict setObject:value forKey:tagName];
			return value;
		}
	}

	return nil;
}

- (void)deleteAllValues {
	self.metadataBlob = nil;
	[kMetadataCache removeObjectForKey:self.urlString];
}

- (void)deleteValue:(NSString *_Nonnull)tagName {
	id metaObj = self.metadataBlob;

	if(metaObj && [metaObj isKindOfClass:[NSDictionary class]]) {
		NSDictionary *metaDict = (NSDictionary *)metaObj;
		NSMutableDictionary *metaDictCopy = [metaDict mutableCopy];

		[metaDictCopy removeObjectForKey:tagName];

		self.metadataBlob = [NSDictionary dictionaryWithDictionary:metaDictCopy];
	}
}

- (void)setValue:(NSString *_Nonnull)tagName fromString:(NSString *_Nullable)value {
	if(!value) {
		[self deleteValue:tagName];
		return;
	}

	NSArray *values = [value componentsSeparatedByString:@", "];

	id metaObj = self.metadataBlob;

	if(metaObj && [metaObj isKindOfClass:[NSDictionary class]]) {
		NSDictionary *metaDict = (NSDictionary *)metaObj;
		NSMutableDictionary *metaDictCopy = [metaDict mutableCopy];

		[metaDictCopy setObject:values forKey:tagName];

		self.metadataBlob = [NSDictionary dictionaryWithDictionary:metaDictCopy];
	}
}

- (void)addValue:(NSString *_Nonnull)tagName fromString:(NSString *_Nonnull)value {
	NSMutableDictionary *dict = [kMetadataCache objectForKey:self.urlString];
	if(dict) {
		[dict removeObjectForKey:tagName];
	}

	id metaObj = self.metadataBlob;

	if(metaObj && [metaObj isKindOfClass:[NSDictionary class]]) {
		NSDictionary *metaDict = (NSDictionary *)metaObj;
		NSMutableDictionary *metaDictCopy = [metaDict mutableCopy];

		NSArray *values = [metaDictCopy objectForKey:tagName];
		NSMutableArray *valuesCopy;
		if(values) {
			valuesCopy = [values mutableCopy];
		} else {
			valuesCopy = [[NSMutableArray alloc] init];
		}
		[valuesCopy addObject:value];
		values = [NSArray arrayWithArray:valuesCopy];
		[metaDictCopy setObject:values forKey:tagName];

		self.metadataBlob = [NSDictionary dictionaryWithDictionary:metaDictCopy];
	}
}

@end
