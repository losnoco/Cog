//
//  PlaylistEntry.m
//  Cog
//
//  Created by Vincent Spader on 3/14/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "PlaylistEntry.h"

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
@synthesize sampleRate;

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

- (NSString *)description
{
	return [NSString stringWithFormat:@"PlaylistEntry %i:(%@)", self.index, self.URL];
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
	
	self.endian = nil;
	
	[super dealloc];
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
