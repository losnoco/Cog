//
//  PlaylistEntry.m
//  Cog
//
//  Created by Vincent Spader on 3/14/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "PlaylistEntry.h"
#import "TagLib/tag_c.h"

@implementation PlaylistEntry

- (id)init
{
	self = [super init];
	if (self)
	{
		[self setIndex:0];
		[self setFilename:@""];
		[self setDisplay:@"Untitled"];
	}

	return self;
}

- (void)dealloc
{
	NSLog(@"DEALLOCATING A PLAYLIST ENTRY: %@", display);
	
	[filename release];
	[display release];
	
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

-(void)setFilename:(NSString *)f
{
	f = [f copy];
	[filename release];
	filename = f;
/*	
	//GO THROUGH HELLA SHIT TO DETERMINE FILE...NEED TO MAKE SOME KIND OF REGISTERING MECHANISM
	if ([[filename pathExtension] isEqualToString:@"wav"] || [[filename pathExtension] isEqualToString:@"aiff"])
	{
		soundFile = [[WaveFile alloc] init];
	}
	else if ([[filename pathExtension] isEqualToString:@"ogg"])
	{
		soundFile = [[VorbisFile alloc] init];
	}
	else if ([[filename pathExtension] isEqualToString:@"mpc"])
	{
		soundFile = [[MusepackFile alloc] init];
	}
	else if ([[filename pathExtension] isEqualToString:@"flac"])
	{
		soundFile = [[FlacFile alloc] init];
	}
	else if ([[filename pathExtension] isEqualToString:@"ape"])
	{
		soundFile = [[MonkeysFile alloc] init];
	}
	else if ([[filename pathExtension] isEqualToString:@"mp3"])
	{
		soundFile = [[MPEGFile alloc] init];
	}
	else
	{
		soundFile = nil;
	}
	
	[soundFile open:[filename UTF8String]];	
*/
}

-(NSString *)filename
{
	return filename;
}

-(void)setDisplay:(NSString *)d
{
	d = [d copy];
	[display release];
	display = d;
}

-(NSString *)display
{
	return display;
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

- (void)readInfo
{
	SoundFile *sf = [SoundFile readInfo:filename];
	if (sf == nil)
		return;
	
	length = [sf length];
	bitrate = [sf bitrate];
	channels = [sf channels];
	bitsPerSample = [sf bitsPerSample];
	sampleRate = [sf frequency];
	
	[self setLengthString:length];

	[sf release];
//	DBLog(@"Length: %f bitrate: %i channels: %i bps: %i samplerate: %f", length, bitrate, channels, bitsPerSample, sampleRate);
	
	//[(SoundFile *)sf close];
//	[sp close];
}

- (void)readInfoThreadSetVariables:(SoundFile *)sf
{
	[self setLength:[sf length]];
	[self setBitrate:[sf bitrate]];
	[self setChannels:[sf channels]];
	[self setBitsPerSample:[sf bitsPerSample]];
	[self setSampleRate:(float)[sf frequency]];
	
	[self setLengthString:length];

	[sf release];
}

- (void)readInfoThread
{
	SoundFile *sf = [SoundFile readInfo:filename];
	if (sf == nil)
		return;

	[self performSelectorOnMainThread:@selector(readInfoThreadSetVariables:) withObject:sf waitUntilDone:YES];
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

-(void)readTags
{
	TagLib_File *tagFile = taglib_file_new((const char *)[filename UTF8String]);
	DBLog(@"Does it have a file? %i %s", tagFile, (const char *)[filename UTF8String]);
	if (tagFile)
	{
		TagLib_Tag *tag = taglib_file_tag(tagFile);
		DBLog(@"Does it have a tag? %i", tag);

		if (tag)
		{
			char *pArtist, *pTitle, *pAlbum, *pGenre, *pComment;
			
			pArtist = taglib_tag_artist(tag);
			pTitle = taglib_tag_title(tag);
			pAlbum = taglib_tag_album(tag);
			pGenre = taglib_tag_genre(tag);
			pComment = taglib_tag_comment(tag);
			
			[self setYear:[[NSNumber numberWithInt:taglib_tag_year(tag)] stringValue]];
			[self setTrack:taglib_tag_track(tag)];
			
			
			if (pArtist != NULL)
				[self setArtist:[NSString stringWithUTF8String:(char *)pArtist]];
			else
				[self setArtist:nil];
			
			if (pAlbum != NULL)
				[self setAlbum:[NSString stringWithUTF8String:(char *)pAlbum]];
			else
				[self setAlbum:nil];
			
			if (pTitle != NULL)
				[self setTitle:[NSString stringWithUTF8String:(char *)pTitle]];
			else
				[self setTitle:nil];
			
			if (pGenre != NULL)	
				[self setGenre:[NSString stringWithUTF8String:(char *)pGenre]];
			else
				[self setGenre:nil];
			
			if ([artist isEqualToString:@""] || [title isEqualToString:@""])
			{
				[self setDisplay:[filename lastPathComponent]];
				[self setTitle:[filename lastPathComponent]];
			}
			else
			{
				[self setDisplay:[NSString stringWithFormat:@"%@ - %@", artist, title]];
			}
			
			taglib_tag_free_strings();
		}
		
		taglib_file_free(tagFile);
	}
	else
	{
		[self setDisplay:[filename lastPathComponent]];
		[self setTitle:[filename lastPathComponent]];
	}
}

- (void)readTagsThreadSetVariables: (NSArray *)a
{
	NSLog(@"SETTING TITLE TO: %@", [a objectAtIndex:0]);
	[self setDisplay:[a objectAtIndex:0]];
	NSLog(@"SETTING TITLE TO: %@", [a objectAtIndex:1]);
	[self setTitle:[a objectAtIndex:1]];
	NSLog(@"SETTING TITLE TO: %@", [a objectAtIndex:2]);
	[self setArtist:[a objectAtIndex:2]];
	NSLog(@"SETTING TITLE TO: %@", [a objectAtIndex:3]);
	[self setAlbum:[a objectAtIndex:3]];
	NSLog(@"SETTING TITLE TO: %@", [a objectAtIndex:4]);
	[self setGenre:[a objectAtIndex:4]];
	NSLog(@"SETTING TITLE TO: %@", [a objectAtIndex:5]);
	[self setYear:[[a objectAtIndex:5] stringValue]];
	NSLog(@"SETTING TITLE TO: %@", [a objectAtIndex:6]);
	[self setTrack:[[a objectAtIndex:6] intValue]];
}	

- (void)readTagsThread
{
	NSString *lDisplay = @"", *lArtist = @"", *lTitle = @"", *lAlbum = @"", *lGenre = @"";
	int lYear = 0, lTrack = 0;
	
	TagLib_File *tagFile = taglib_file_new((const char *)[filename UTF8String]);
	DBLog(@"Does it have a file? %i %s", tagFile, (const char *)[filename UTF8String]);
	if (tagFile)
	{
		TagLib_Tag *tag = taglib_file_tag(tagFile);
		DBLog(@"Does it have a tag? %i", tag);
		
		if (tag)
		{
			char *pArtist, *pTitle, *pAlbum, *pGenre, *pComment;
			
			pArtist = taglib_tag_artist(tag);
			pTitle = taglib_tag_title(tag);
			pAlbum = taglib_tag_album(tag);
			pGenre = taglib_tag_genre(tag);
			pComment = taglib_tag_comment(tag);
			
			lYear = taglib_tag_year(tag);
			lTrack = taglib_tag_track(tag);
			
			if (pArtist != NULL)
				lArtist = [NSString stringWithUTF8String:(char *)pArtist];
			else
				lArtist = @"";
			
			if (pAlbum != NULL)
				lAlbum = [NSString stringWithUTF8String:(char *)pAlbum];
			else
				lAlbum = @"";
			
			if (pTitle != NULL)
			{
				NSLog(@"SET TITLE PROPERLY");
				lTitle = [NSString stringWithUTF8String:(char *)pTitle];
			}
			else
				lTitle = @"";
			
			if (pGenre != NULL)	
				lGenre = [NSString stringWithUTF8String:(char *)pGenre];
			else
				lGenre = @"";
				
			if ([lArtist isEqualToString:@""] || [lTitle isEqualToString:@""])
			{
				NSLog(@"SET TITLE IMPROPERLY");

				lDisplay = [filename lastPathComponent];
				lTitle = [filename lastPathComponent];
			}
			else
			{
				lDisplay = [NSString stringWithFormat:@"%@ - %@", lArtist, lTitle];
			}
			
			taglib_tag_free_strings();
		}
		
		taglib_file_free(tagFile);
	}
	else
	{
		NSLog(@"SET TITLE IMPROPERLY2");
		lDisplay = [filename lastPathComponent];
		lTitle = [filename lastPathComponent];
	}
	NSLog(@"TITLE IS: %@", lTitle);
	[self performSelectorOnMainThread:@selector(readTagsThreadSetVariables:) withObject:
		[NSArray arrayWithObjects:
			lDisplay,
			lTitle,
			lArtist,
			lAlbum,
			lGenre,
			[NSNumber numberWithInt:lYear],
			[NSNumber numberWithInt:lTrack],
			nil]

		waitUntilDone:YES];
}

- (NSString *)description
{
	return [NSString stringWithFormat:@"PlaylistEntry %i:(%@)",idx, filename];
}

@end
