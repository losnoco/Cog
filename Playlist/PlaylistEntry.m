//
//  PlaylistEntry.m
//  Cog
//
//  Created by Vincent Spader on 3/14/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
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
	[filename release];
	[display release];
	
	[super dealloc];
}


-(void)setIndex:(int)i
{
	idx = i;
}
-(int)index
{
	return idx;
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

- (void)readInfo
{
	SoundFile *sp;
	SoundFile *sf = [SoundFile readInfo:filename];
	sp= sf;
	
	length = [sf length];
	bitRate = [sf bitRate];
	channels = [sf channels];
	bitsPerSample = [sf bitsPerSample];
	sampleRate = [sf frequency];
	
	[self setLengthString:length];
//	DBLog(@"Length: %f bitRate: %i channels: %i bps: %i samplerate: %f", length, bitRate, channels, bitsPerSample, sampleRate);
	
	//[(SoundFile *)sf close];
//	[sp close];
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

- (double)length
{
	return length;
}
- (int)bitRate
{
	return bitRate;
}

- (int)channels
{
	return channels;
}
- (int)bitsPerSample
{
	return bitsPerSample;
}
- (float)sampleRate
{
	return sampleRate;
}

-(void)readTags
{
	TagLib_File *tagFile = taglib_file_new((const char *)[filename UTF8String]);
	if (tagFile)
	{
		TagLib_Tag *tag = taglib_file_tag(tagFile);
		
		if (tag)
		{
			DBLog(@"TAG: %i", tag);
			
			char *pArtist, *pTitle, *pAlbum, *pGenre, *pComment;
			
			pArtist = taglib_tag_artist(tag);
			pTitle = taglib_tag_title(tag);
			pAlbum = taglib_tag_album(tag);
			pGenre = taglib_tag_genre(tag);
			pComment = taglib_tag_comment(tag);
			
			year = taglib_tag_year(tag);
			track = taglib_tag_track(tag);
			
			
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
				[self setDisplay:[filename lastPathComponent]];
			else
				[self setDisplay:[NSString stringWithFormat:@"%@ - %@", artist, title]];
			
			
			taglib_tag_free_strings();
		}
		
		taglib_file_free(tagFile);
	}
	else
	{
		[self setDisplay:[filename lastPathComponent]];
	}
}

@end
