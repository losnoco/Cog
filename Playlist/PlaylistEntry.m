//
//  PlaylistEntry.m
//  Cog
//
//  Created by Vincent Spader on 3/14/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "PlaylistEntry.h"
#import "SecondsFormatter.h"

@implementation PlaylistEntry

@synthesize index;
@synthesize shuffleIndex;

@synthesize current;
@synthesize removed;

@synthesize stopAfter;

@synthesize queued;
@synthesize queuePosition;

@synthesize error;
@synthesize errorMessage;

@synthesize URL;

@synthesize artist;
@synthesize album;
@synthesize genre;
@synthesize year;
@synthesize track;

@synthesize totalFrames;
@synthesize bitrate;
@synthesize channels;
@synthesize bitsPerSample;
@synthesize floatingPoint;
@synthesize Unsigned;
@synthesize sampleRate;

@synthesize replayGainAlbumGain;
@synthesize replayGainAlbumPeak;
@synthesize replayGainTrackGain;
@synthesize replayGainTrackPeak;
@synthesize volume;

@synthesize currentPosition;

@synthesize endian;

@synthesize seekable;

@synthesize metadataLoaded;

// The following read-only keys depend on the values of other properties

+ (NSSet *)keyPathsForValuesAffectingDisplay
{
    return [NSSet setWithObjects:@"artist",@"title",nil];
}

+ (NSSet *)keyPathsForValuesAffectingLength
{
    return [NSSet setWithObject:@"totalFrames"];
}

+ (NSSet *)keyPathsForValuesAffectingPath
{
    return [NSSet setWithObject:@"URL"];
}

+ (NSSet *)keyPathsForValuesAffectingFilename
{
    return [NSSet setWithObject:@"URL"];
}

+ (NSSet *)keyPathsForValuesAffectingStatus
{
	return [NSSet setWithObjects:@"current",@"queued", @"error", @"stopAfter", nil];
}

+ (NSSet *)keyPathsForValuesAffectingStatusMessage
{
	return [NSSet setWithObjects:@"current", @"queued", @"queuePosition", @"error", @"errorMessage", @"stopAfter", nil];
}

+ (NSSet *)keyPathsForValuesAffectingSpam
{
    return [NSSet setWithObjects:@"artist", @"title", @"album", @"track", @"totalFrames", @"currentPosition", @"bitrate", nil];
}

+ (NSSet *)keyPathsForValuesAffectingPositionText
{
    return [NSSet setWithObject:@"currentPosition"];
}

+ (NSSet *)keyPathsForValuesAffectingLengthText
{
    return [NSSet setWithObject:@"length"];
}

+ (NSSet *)keyPathsForValuesAffectingAlbumArt
{
    return [NSSet setWithObject:@"albumArtInternal"];
}

- (NSString *)description
{
	return [NSString stringWithFormat:@"PlaylistEntry %i:(%@)", self.index, self.URL];
}

- (id)init
{
    if (self = [super init]) {
        self.replayGainAlbumGain = 0;
        self.replayGainAlbumPeak = 0;
        self.replayGainTrackGain = 0;
        self.replayGainTrackPeak = 0;
        self.volume = 1;
    }
    return self;
}

- (void)dealloc
{
	self.errorMessage = nil;
	
	self.URL = nil;
	
	self.artist = nil;
	self.album = nil;
	self.title = nil;
	self.genre = nil;
	self.year = nil;
	self.track = nil;
	self.albumArtInternal = nil;
	
	self.endian = nil;
}

// Get the URL if the title is blank
@synthesize title;
- (NSString *)title
{
    if((title == nil || [title isEqualToString:@""]) && self.URL)
    {
        return [[self.URL path] lastPathComponent];
    }
    return title;
}

@dynamic display;
- (NSString *)display
{
	if ((self.artist == NULL) || ([self.artist isEqualToString:@""]))
		return self.title;
	else {
		return [NSString stringWithFormat:@"%@ - %@", self.artist, self.title];
	}
}

@dynamic spam;
- (NSString *)spam
{
    BOOL hasBitrate = (self.bitrate != 0);
    BOOL hasArtist = (self.artist != nil) && (![self.artist isEqualToString:@""]);
    BOOL hasAlbum = (self.album != nil) && (![self.album isEqualToString:@""]);
    BOOL hasTrack = (self.track != 0);
    BOOL hasLength = (self.totalFrames != 0);
    BOOL hasCurrentPosition = (self.currentPosition != 0) && (self.current);
    BOOL hasExtension = NO;
    BOOL hasTitle = (title != nil) && (![title isEqualToString:@""]);
    
    NSMutableString * filename = [NSMutableString stringWithString:[self filename]];
    NSRange dotPosition = [filename rangeOfString:@"." options:NSBackwardsSearch];
    NSString * extension = nil;
    
    if (dotPosition.length > 0) {
        dotPosition.location++;
        dotPosition.length = [filename length] - dotPosition.location;
        extension = [filename substringWithRange:dotPosition];
        dotPosition.location--;
        dotPosition.length++;
        [filename deleteCharactersInRange:dotPosition];
        hasExtension = YES;
    }
    
    NSMutableArray * elements = [NSMutableArray array];

    if (hasExtension) {
        [elements addObject:@"["];
        [elements addObject:[extension uppercaseString]];
        if (hasBitrate) {
            [elements addObject:@"@"];
            [elements addObject:[NSString stringWithFormat:@"%u", self.bitrate]];
            [elements addObject:@"kbps"];
        }
        [elements addObject:@"] "];
    }
    
    if (hasArtist) {
        [elements addObject:self.artist];
        [elements addObject:@" - "];
    }
    
    if (hasAlbum) {
        [elements addObject:@"["];
        [elements addObject:self.album];
        if (hasTrack) {
            [elements addObject:@" #"];
            [elements addObject:[NSString stringWithFormat:@"%@", self.track]];
        }
        [elements addObject:@"] "];
    }
    
    if (hasTitle) {
        [elements addObject:title];
    }
    else {
        [elements addObject:filename];
    }
    
    if (hasCurrentPosition || hasLength) {
        SecondsFormatter *secondsFormatter = [[SecondsFormatter alloc] init];
        [elements addObject:@" ("];
        if (hasCurrentPosition) {
            [elements addObject:[secondsFormatter stringForObjectValue:[NSNumber numberWithFloat:currentPosition]]];
        }
        if (hasLength) {
            if (hasCurrentPosition) {
                [elements addObject:@" / "];
            }
            [elements addObject:[secondsFormatter stringForObjectValue:[self length]]];
        }
        [elements addObject:@")"];
    }
    
    return [elements componentsJoinedByString:@""];
}

@dynamic positionText;
- (NSString *)positionText
{
    SecondsFormatter *secondsFormatter = [[SecondsFormatter alloc] init];
    NSString *time = [secondsFormatter stringForObjectValue:[NSNumber numberWithFloat:currentPosition]];
    return time;
}

@dynamic lengthText;
- (NSString *)lengthText
{
    SecondsFormatter *secondsFormatter = [[SecondsFormatter alloc] init];
    NSString *time = [secondsFormatter stringForObjectValue:[self length]];
    return time;
}

@synthesize albumArtInternal;

@dynamic albumArt;
- (NSImage *)albumArt
{
    if (!albumArtInternal) return nil;

    NSString *imageCacheTag = [NSString stringWithFormat:@"%@-%@-%@-%@", album, artist, genre, year];
    NSImage *image = [NSImage imageNamed:imageCacheTag];
    
    if (image == nil)
    {
        image = [[NSImage alloc] initWithData:albumArtInternal];
        [image setName:imageCacheTag];
    }
    
    return image;
}

- (void)setAlbumArt:(id)data
{
    if ([data isKindOfClass:[NSData class]])
    {
        [self setAlbumArtInternal:data];
    }
}

@dynamic length;
- (NSNumber *)length
{
    return [NSNumber numberWithDouble:((double)self.totalFrames / self.sampleRate)];
}

@dynamic path;
- (NSString *)path
{
	return [[self.URL path] stringByAbbreviatingWithTildeInPath];
}

@dynamic filename;
- (NSString *)filename
{
	return [[self.URL path] lastPathComponent];
}

@dynamic status;
- (NSString *)status
{
	if (self.stopAfter)
	{
		return @"stopAfter";
	}
	else if (self.current)
	{
		return @"playing";
	}
	else if (self.queued)
	{
		return @"queued";
	}
	else if (self.error)
	{
		return @"error";
	}
	
	return nil;
}

@dynamic statusMessage;
- (NSString *)statusMessage
{
	if (self.stopAfter)
	{
		return @"Stopping once finished...";
	}
	else if (self.current)
	{
		return @"Playing...";
	}
	else if (self.queued)
	{
		return [NSString stringWithFormat:@"Queued: %i", self.queuePosition + 1];
	}
	else if (self.error)
	{
		return errorMessage;
	}
	
	return nil;
}

- (void)setMetadata:(NSDictionary *)metadata
{
    if (metadata == nil)
    {
        self.error = YES;
        self.errorMessage = @"Unable to retrieve metadata.";
    }
    else
    {
		[self setValuesForKeysWithDictionary:metadata];
    }
	
	metadataLoaded = YES;
}

@end
