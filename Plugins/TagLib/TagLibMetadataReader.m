//
//  TagLibMetadataReader.m
//  TagLib
//
//  Created by Vincent Spader on 2/24/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "TagLibMetadataReader.h"
#import "TagLib/tag_c.h"


@implementation TagLibMetadataReader

+ (NSDictionary *)metadataForURL:(NSURL *)url
{
	NSString *lArtist = @"", *lTitle = @"", *lAlbum = @"", *lGenre = @"";
	int lYear = 0, lTrack = 0;
	
	TagLib_File *tagFile = taglib_file_new((const char *)[[url path] UTF8String]);
	if (tagFile)
	{
		TagLib_Tag *tag = taglib_file_tag(tagFile);
		
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
				lTitle = [NSString stringWithUTF8String:(char *)pTitle];
			else
				lTitle = @"";
			
			if (pGenre != NULL)	
				lGenre = [NSString stringWithUTF8String:(char *)pGenre];
			else
				lGenre = @"";
				
			taglib_tag_free_strings();
		}
		
		taglib_file_free(tagFile);
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
	return [NSArray arrayWithObjects:@"shn",@"wv",@"ogg",@"mpc",@"flac",@"ape",@"mp3",@"m4a",nil];
}

@end
