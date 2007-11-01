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

+ (void)initialize { 
	[self setKeys:[NSArray arrayWithObjects:@"artist",@"title",nil] triggerChangeNotificationsForDependentKey:@"display"]; 
}

- (id)init
{
	self = [super init];
	if (self)
	{
		url = nil;

		artist = nil;
		album = nil;
		title = nil;
		genre = nil;

		year = nil;
		track = nil;
		length = nil;
		bitrate = nil;
		channels = nil;
		bitsPerSample = nil;
		sampleRate = nil;
		
		current = nil;
		
		idx = nil;
		shuffleIndex = nil;
	}

	return self;
}

- (void)dealloc
{
	[url release];
	[artist release];
	[album release];
	[title release];
	[genre release];
	[year release];
	[track release];
	[length release];
	[bitrate release];
	[channels release];
	[bitsPerSample release];
	[sampleRate release];
	[current release];
	[idx release];
	[shuffleIndex release];
	
	[super dealloc];
}

-(void)setShuffleIndex:(NSNumber *)si
{
	[si retain];
	[shuffleIndex release];
	
	shuffleIndex = si;
}

-(NSNumber *)shuffleIndex
{
	return shuffleIndex;
}

-(void)setIndex:(NSNumber *)i
{
	[i retain];
	[idx release];

	idx = i;
}

-(NSNumber *)index
{
	return idx;
}

-(void)setURL:(NSURL *)u
{
	[u retain];
	[url release];
	url = u;
}

-(NSURL *)url
{
	return url;
}

-(void)setCurrent:(NSNumber *) b
{
	[b retain];
	[current release];
	current = b;
}

-(NSNumber *)current
{
	return current;
}


- (void)setArtist:(NSString *)s
{
	[s retain];
	[artist release];
	
	artist = s;
}

- (NSString *)artist
{
	return artist;
}

- (void)setAlbum:(NSString *)s
{
	[s retain];
	[album release];
	
	album = s;
}

- (NSString *)album
{
	return album;
}

- (void)setTitle:(NSString *)s
{
	[s retain];
	[title release];
	
	title = s;
}

- (NSString *)title
{
	return title;
}

- (void)setGenre:(NSString *)s
{
	[s retain];
	[genre release];
	
	genre = s;
}

- (NSString *)genre
{
	return genre;
}

- (void)setYear:(NSString *)y
{
	[y retain];
	[year release];
	
	if ([y intValue] == 0)
	{
		y = @"";
	}
	
	year = y;
}
- (NSString *)year
{
	return year;
}

- (void)setTrack:(NSNumber *)t
{
	[t retain];
	[track release];
	
	track = t;
}
- (NSNumber *)track
{
	return track;
}

- (void)setProperties:(NSDictionary *)dict
{
	[self setLength:		[dict objectForKey:@"length"		]];
	[self setBitrate:		[dict objectForKey:@"bitrate"		]];
	[self setChannels:		[dict objectForKey:@"channels"		]];
	[self setBitsPerSample:	[dict objectForKey:@"bitsPerSample" ]];
	[self setSampleRate:	[dict objectForKey:@"sampleRate"	]];
	[self setSeekable:		[dict objectForKey:@"seekable"		]];
}

- (void)readPropertiesThread
{
	NSDictionary *properties = [AudioPropertiesReader propertiesForURL:url];

	[self performSelectorOnMainThread:@selector(setProperties:) withObject:properties waitUntilDone:YES];
}

- (void)setLength:(NSNumber *)l
{
	[l retain];
	[length release];
	
	length = l;
}
- (NSNumber *)length
{
	return length;
}

- (void)setBitrate:(NSNumber *) br
{
	[br retain];
	[bitrate release];
	
	bitrate = br;
}
- (NSNumber *)bitrate
{
	return bitrate;
}

- (void)setChannels:(NSNumber *)c
{
	[c retain];
	[channels release];
	
	channels = c;
}
- (NSNumber *)channels
{
	return channels;
}

- (void)setBitsPerSample:(NSNumber *)bps
{
	[bps retain];
	[bitsPerSample release];
	
	bitsPerSample = bps;
}
- (NSNumber *)bitsPerSample
{
	return bitsPerSample;
}

- (void)setSampleRate:(NSNumber *)s
{
	[s retain];
	[sampleRate release];

	sampleRate = s;
}
- (NSNumber *)sampleRate
{
	return sampleRate;
}

- (NSString *)display
{
	if ([[self artist] isEqualToString:@""]) {
			return title;
	}
	else {
		return [NSString stringWithFormat:@"%@ - %@", artist, title];
	}
}

- (void)setSeekable:(NSNumber *)s
{
	[s retain];
	[seekable release];
	
	seekable = s;
}
	
- (NSNumber *)seekable
{
	return seekable;
}

- (void)setMetadata: (NSDictionary *)m
{
	NSString *ti = [m objectForKey:@"title"];

	if (ti == nil || [ti isEqualToString:@""]) {
		[self setTitle:[[url path] lastPathComponent]];
	}
	else {
		[self setTitle:ti];
	}
	
	[self setArtist:[m objectForKey:@"artist"]];
	[self setAlbum:[m objectForKey:@"album"]];
	[self setGenre:[m objectForKey:@"genre"]];
	[self setYear:[m objectForKey:@"year"]];
	[self setTrack:[m objectForKey:@"track"]];
}	

- (void)readMetadataThread
{
	NSDictionary *metadata = [AudioMetadataReader metadataForURL:url];
	
	[self performSelectorOnMainThread:@selector(setMetadata:) withObject:metadata waitUntilDone:YES];

}

- (NSString *)description
{
	return [NSString stringWithFormat:@"PlaylistEntry %i:(%@)", idx, url];
}

@end
