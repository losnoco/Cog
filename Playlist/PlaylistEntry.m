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
@synthesize dbIndex;
@synthesize entryId;

@synthesize current;
@synthesize removed;

@synthesize stopAfter;

@synthesize queued;
@synthesize queuePosition;

@synthesize error;
@synthesize errorMessage;

@synthesize URL;

@synthesize artist;
@synthesize albumartist;
@synthesize album;
@synthesize genre;
@synthesize year;
@synthesize track;
@synthesize disc;

@synthesize cuesheet;

@synthesize totalFrames;
@synthesize bitrate;
@synthesize channels;
@synthesize bitsPerSample;
@synthesize floatingPoint;
@synthesize Unsigned;
@synthesize sampleRate;

@synthesize codec;

@synthesize replayGainAlbumGain;
@synthesize replayGainAlbumPeak;
@synthesize replayGainTrackGain;
@synthesize replayGainTrackPeak;
@synthesize volume;

@synthesize currentPosition;

@synthesize endian;

@synthesize encoding;

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
    return [NSSet setWithObjects:@"albumartist", @"artist", @"title", @"album", @"track", @"disc", @"totalFrames", @"currentPosition", @"bitrate", nil];
}

+ (NSSet *)keyPathsForValuesAffectingTrackText
{
    return [NSSet setWithObjects:@"track", @"disc", nil];
}

+ (NSSet *)keyPathsForValuesAffectingYearText
{
    return [NSSet setWithObject:@"year"];
}

+ (NSSet *)keyPathsForValuesAffectingCuesheetPresent
{
    return [NSSet setWithObject:@"cuesheet"];
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

+ (NSSet *)keyPathsForValuesAffectingGainCorrection
{
    return [NSSet setWithObjects:@"replayGainAlbumGain", @"replayGainAlbumPeak", @"replayGainTrackGain", @"replayGainTrackPeak", @"volume", nil];
}

+ (NSSet *)keyPathsForValuesAffectingGainInfo
{
    return [NSSet setWithObjects:@"replayGainAlbumGain", @"replayGainAlbumPeak", @"replayGainTrackGain", @"replayGainTrackPeak", @"volume", nil];
}

- (NSString *)description
{
    return [NSString stringWithFormat:@"PlaylistEntry %li:(%@)", self.index, self.URL];
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
    self.albumartist = nil;
	self.album = nil;
	self.title = nil;
	self.genre = nil;
	self.year = nil;
	self.track = nil;
    self.disc = nil;
	self.albumArtInternal = nil;
    
    self.cuesheet = nil;
	
	self.endian = nil;
    self.codec = nil;
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

@synthesize rawTitle;
- (NSString *)rawTitle
{
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
    BOOL hasAlbumArtist = (self.albumartist != nil) && (![self.albumartist isEqualToString:@""]);
    BOOL hasAlbum = (self.album != nil) && (![self.album isEqualToString:@""]);
    BOOL hasTrack = (self.track != 0);
    BOOL hasLength = (self.totalFrames != 0);
    BOOL hasCurrentPosition = (self.currentPosition != 0) && (self.current);
    BOOL hasExtension = NO;
    BOOL hasTitle = (title != nil) && (![title isEqualToString:@""]);
    BOOL hasCodec = (self.codec != nil) && (![self.codec isEqualToString:@""]);
    
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
        if (hasCodec) {
            [elements addObject:self.codec];
        }
        else {
            [elements addObject:[extension uppercaseString]];
        }
        if (hasBitrate) {
            [elements addObject:@"@"];
            [elements addObject:[NSString stringWithFormat:@"%u", self.bitrate]];
            [elements addObject:@"kbps"];
        }
        [elements addObject:@"] "];
    }
    
    if (hasArtist) {
        if (hasAlbumArtist) {
            [elements addObject:self.albumartist];
        }
        else {
            [elements addObject:self.artist];
        }
        [elements addObject:@" - "];
    }
    
    if (hasAlbum) {
        [elements addObject:@"["];
        [elements addObject:self.album];
        if (hasTrack) {
            [elements addObject:@" #"];
            [elements addObject:self.trackText];
        }
        [elements addObject:@"] "];
    }
    
    if (hasTitle) {
        [elements addObject:title];
    }
    else {
        [elements addObject:filename];
    }
    
    if (hasAlbumArtist && hasArtist) {
        [elements addObject:@" // "];
        [elements addObject:self.artist];
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

@dynamic trackText;
-(NSString *)trackText
{
    if ([self.track intValue])
    {
        if ([self.disc intValue])
        {
            return [NSString stringWithFormat:@"%@.%02u", self.disc, [self.track intValue]];
        }
        else
        {
            return [NSString stringWithFormat:@"%02u", [self.track intValue]];
        }
    }
    else
    {
        return @"";
    }
}

@dynamic yearText;
-(NSString *)yearText
{
    if ([self.year intValue])
    {
        return [NSString stringWithFormat:@"%@", self.year];
    }
    else
    {
        return @"";
    }
}

@dynamic cuesheetPresent;
-(NSString *)cuesheetPresent
{
    if (cuesheet && [cuesheet length])
    {
        return @"yes";
    }
    else
    {
        return @"no";
    }
}

@dynamic gainCorrection;
- (NSString *)gainCorrection
{
    if (replayGainAlbumGain)
    {
        if (replayGainAlbumPeak)
            return @"Album Gain plus Peak";
        else
            return @"Album Gain";
    }
    else if (replayGainTrackGain)
    {
        if (replayGainTrackPeak)
            return @"Track Gain plus Peak";
        else
            return @"Track Gain";
    }
    else if (volume && volume != 1)
    {
        return @"Volume scale";
    }
    else
    {
        return @"None";
    }
}

@dynamic gainInfo;
- (NSString *)gainInfo
{
    NSMutableArray * gainItems = [[NSMutableArray alloc] init];
    if (replayGainAlbumGain)
    {
        [gainItems addObject:[NSString stringWithFormat:@"Album Gain: %+.2f dB", replayGainAlbumGain]];
    }
    if (replayGainAlbumPeak)
    {
        [gainItems addObject:[NSString stringWithFormat:@"Album Peak: %.6f", replayGainAlbumPeak]];
    }
    if (replayGainTrackGain)
    {
        [gainItems addObject:[NSString stringWithFormat:@"Track Gain: %+.2f dB", replayGainTrackGain]];
    }
    if (replayGainTrackPeak)
    {
        [gainItems addObject:[NSString stringWithFormat:@"Track Peak: %.6f", replayGainTrackPeak]];
    }
    if (volume && volume != 1)
    {
        [gainItems addObject:[NSString stringWithFormat:@"Volume Scale: %.2f%C", volume, (unichar)0x00D7]];
    }
    return [gainItems componentsJoinedByString:@"\n"];
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
    return [NSNumber numberWithDouble:(self.metadataLoaded)?((double)self.totalFrames / self.sampleRate):0.0];
}

@dynamic path;
- (NSString *)path
{
    if ([self.URL isFileURL])
        return [[self.URL path] stringByAbbreviatingWithTildeInPath];
    else
        return [self.URL absoluteString];
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
        return [NSString stringWithFormat:@"Queued: %li", self.queuePosition + 1];
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

- (id)copyWithZone:(NSZone *)zone {
    PlaylistEntry *pe = [[[self class] allocWithZone:zone] init];
    
    if (pe) {
        pe->index = index;
        pe->shuffleIndex = shuffleIndex;
        pe->dbIndex = dbIndex;
        pe->entryId = entryId;

        pe->current = current;
        pe->removed = removed;

        pe->stopAfter = stopAfter;

        pe->queued = queued;
        pe->queuePosition = queuePosition;

        pe->error = error;
        pe->errorMessage = [errorMessage copyWithZone:zone];

        pe->URL = [URL copyWithZone:zone];

        pe->artist = [artist copyWithZone:zone];
        pe->albumartist = [albumartist copyWithZone:zone];
        pe->album = [album copyWithZone:zone];
        pe->title = [title copyWithZone:zone];
        pe->genre = [genre copyWithZone:zone];
        pe->year = [year copyWithZone:zone];
        pe->track = [track copyWithZone:zone];
        pe->disc = [disc copyWithZone:zone];

        pe->cuesheet = [cuesheet copyWithZone:zone];

        pe->albumArtInternal = [albumArtInternal copyWithZone:zone];

        pe->replayGainAlbumGain = replayGainAlbumGain;
        pe->replayGainAlbumPeak = replayGainAlbumPeak;
        pe->replayGainTrackGain = replayGainTrackGain;
        pe->replayGainTrackPeak = replayGainTrackPeak;
        pe->volume = volume;

        currentPosition = pe->currentPosition;

        pe->totalFrames = totalFrames;
        pe->bitrate = bitrate;
        pe->channels = channels;
        pe->bitsPerSample = bitsPerSample;
        pe->floatingPoint = floatingPoint;
        pe->Unsigned = Unsigned;
        pe->sampleRate = sampleRate;

        pe->codec = [codec copyWithZone:zone];

        pe->endian = [endian copyWithZone:zone];
        
        pe->encoding = [encoding copyWithZone:zone];

        pe->seekable = seekable;

        pe->metadataLoaded = metadataLoaded;
    }

    return pe;
}

@end
