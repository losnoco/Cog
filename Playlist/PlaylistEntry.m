//
//  PlaylistEntry.m
//  Cog
//
//  Created by Vincent Spader on 3/14/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "PlaylistEntry.h"
#import "CogAudio/AudioPropertiesReader.h"
#import "CogAudio/AudioMetadataReader.h"

@implementation PlaylistEntry

@synthesize index;
@synthesize shuffleIndex;
@synthesize status;
@synthesize statusMessage;
@synthesize queuePosition;

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

@synthesize seekable;

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

- (void)readPropertiesThread
{
	NSDictionary *properties = [AudioPropertiesReader propertiesForURL:self.URL];
	if (!properties) {
		self.status = kCogEntryError;
		self.statusMessage = @"Failed to read properties!";

		return;
	}
	
	[self performSelectorOnMainThread:@selector(setValuesForKeysWithDictionary:) withObject:properties waitUntilDone:YES];
}

- (void)readMetadataThread
{
	NSDictionary *metadata = [AudioMetadataReader metadataForURL:self.URL];

	[self performSelectorOnMainThread:@selector(setValuesForKeysWithDictionary:) withObject:metadata waitUntilDone:YES];

}

- (NSString *)description
{
	return [NSString stringWithFormat:@"PlaylistEntry %i:(%@)", self.index, self.URL];
}

// Get the URL if the title is blank
@synthesize title;
- (NSString *)title
{
    if(self.URL && (title == nil || [title isEqualToString:@""]))
    {
        return [[self.URL path] lastPathComponent];
    }
    return [[title retain] autorelease];
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

@end
