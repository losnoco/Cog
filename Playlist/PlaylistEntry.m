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
		[self setIndex:0];
		[self setURL:nil];
	}

	return self;
}

- (void)dealloc
{
	[url release];
	
	[super dealloc];
}

-(void)setShuffleIndex:(int)si
{
	shuffleIdx = si;
}

-(int)shuffleIndex
{
	return shuffleIdx;
}

-(void)setIndex:(int)i
{
	idx = i;
	[self setDisplayIndex:i+1];
}

-(int)index
{
	return idx;
}

-(void)setDisplayIndex:(int)i
{
	displayIdx=i;
}

-(int)displayIndex
{
	return displayIdx;
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

- (void)setTrack:(int)t
{
	track = t;
}
- (int)track
{
	return track;
}

- (void)setProperties:(NSDictionary *)dict
{
	[self setLength:		[[dict objectForKey:@"length"		] doubleValue]];
	[self setBitrate:		[[dict objectForKey:@"bitrate"		] intValue]];
	[self setChannels:		[[dict objectForKey:@"channels"		] intValue]];
	[self setBitsPerSample:	[[dict objectForKey:@"bitsPerSample"] intValue]];
	[self setSampleRate:	[[dict objectForKey:@"sampleRate"	] floatValue]];
	
	[self setLengthString:[[dict objectForKey:@"length"] doubleValue]];
}

- (void)readPropertiesThread
{
	NSDictionary *properties = [AudioPropertiesReader propertiesForURL:url];

	[self performSelectorOnMainThread:@selector(setProperties:) withObject:properties waitUntilDone:YES];
}

- (NSString *)lengthString
{
	return lengthString;
}
- (void)setLengthString:(double)l
{
	int sec = (int)(l/1000.0);

	[lengthString release];
	lengthString = [[NSString alloc] initWithFormat:@"%i:%02i",sec/60,sec%60]; 
}


- (void)setLength:(double)l
{
	length = l;
}
- (double)length
{
	return length;
}

- (void)setBitrate:(int) br
{
	bitrate = br;
}
- (int)bitrate
{
	return bitrate;
}

- (void)setChannels:(int)c
{
	channels = c;
}
- (int)channels
{
	return channels;
}

- (void)setBitsPerSample:(int)bps
{
	bitsPerSample = bps;
}
- (int)bitsPerSample
{
	return bitsPerSample;
}

- (void)setSampleRate:(float)s
{
	sampleRate = s;
}
- (float)sampleRate
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
	[self setTrack:[[m objectForKey:@"track"] intValue]];
}	

- (void)readMetadataThread
{
	NSDictionary *metadata = [AudioMetadataReader metadataForURL:url];
	
	[self performSelectorOnMainThread:@selector(setMetadata:) withObject:metadata waitUntilDone:YES];

}

- (NSString *)description
{
	return [NSString stringWithFormat:@"PlaylistEntry %i:(%@)",idx, url];
}

@end
