//
//  TagLibMetadataReader.m
//  TagLib
//
//  Created by Vincent Spader on 2/24/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "TagLibMetadataReader.h"
#import <TagLib/fileref.h>
#import <TagLib/tag.h>

@implementation TagLibMetadataReader

+ (NSDictionary *)metadataForURL:(NSURL *)url
{
	NSString *lArtist = @"", *lTitle = @"", *lAlbum = @"", *lGenre = @"";
	int lYear = 0, lTrack = 0;
	
	TagLib::FileRef f((const char *)[[url path] UTF8String], false);
	if (!f.isNull())
	{
		const TagLib::Tag *tag = f.tag();
		
		if (tag)
		{
			TagLib::String pArtist, pTitle, pAlbum, pGenre, pComment;
			
			pArtist = tag->artist();
			pTitle = tag->title();;
			pAlbum = tag->album();
			pGenre = tag->genre();
			pComment = tag->comment();
			
			lYear = tag->year();
			lTrack = tag->track();
			
			lArtist = [NSString stringWithUTF8String:pArtist.toCString(true)];

			lAlbum = [NSString stringWithUTF8String:pAlbum.toCString(true)];
			
			lTitle = [NSString stringWithUTF8String:pTitle.toCString(true)];
			
			lGenre = [NSString stringWithUTF8String:pGenre.toCString(true)];
		}
	}
	
	return [NSDictionary dictionaryWithObjectsAndKeys:
		lArtist, @"artist",
		lTitle, @"title",
		lAlbum, @"album",
		lGenre, @"genre",
		[[NSNumber numberWithInt: lYear] stringValue], @"year",
		[NSNumber numberWithInt: lTrack], @"track",
		nil];
}

+ (NSArray *)fileTypes
{
	//May be a way to get a list of supported formats
	return [NSArray arrayWithObjects:@"ogg", @"mpc", @"flac", @"m4a", @"mp3", nil];
}

+ (NSArray *)mimeTypes
{
	return [NSArray arrayWithObjects:@"application/ogg", @"application/x-ogg", @"audio/x-vorbis+ogg", @"audio/x-musepack", @"audio/x-flac", @"audio/x-m4a", @"audio/mpeg", @"audio/x-mp3", nil];
}

@end
