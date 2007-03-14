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

- (id)init
{
	self = [super init];
	if (self)
	{
		[self setIndex:nil];
		[self setURL:nil];
	}

	return self;
}

- (void)dealloc
{
	[url release];
	
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
	NSLog(@"INDEX: %@", i);
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

-(void)setCurrent:(BOOL) b
{
	current = b;
}

-(BOOL)current
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
//	DBLog(@"HERE FUCK: %@", title);
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
	[s release];
	
	sampleRate = s;
}
- (NSNumber *)sampleRate
{
	return sampleRate;
}

- (void)setMetadata: (NSDictionary *)m
{
	NSString *ti = [m objectForKey:@"title"];

	if ([ti isEqualToString:@""]) {
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
