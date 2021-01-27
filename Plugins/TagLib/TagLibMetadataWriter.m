//
//  TagLibMetadataWriter.m
//  TagLib
//
//  Created by Safari on 08/11/17.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "TagLibMetadataWriter.h"

#import <taglib/fileref.h>
#import <taglib/tag.h>

#import "Logging.h"

@implementation TagLibMetadataWriter

+ (int)putMetadataInURL:(NSURL *)url tagData:(NSDictionary *)tagData
{
	
	NSString *lArtist = @"", *lTitle = @"", *lAlbum = @"", *lGenre = @"";
	//int lYear = 0, lTrack = 0;
	
	TagLib::FileRef f((const char *)[[url path] UTF8String], false);
	if (!f.isNull())
	{
		const TagLib::Tag *tag = f.tag();
		
		if (tag)
		{
			const TagLib::String pArtist, pTitle, pAlbum, pGenre, pComment;
			
			lArtist = [tagData valueForKey:@"artist"];
			lTitle = [tagData valueForKey:@"title"];
			lAlbum = [tagData valueForKey:@"album"];
			lGenre = [tagData valueForKey:@"genre"];
			
			f.tag()->setTitle([lTitle UTF8String]);
			f.tag()->setArtist([lArtist UTF8String]);
			
		}
			
			/*
			 
			 NSArray *keys = [NSArray arrayWithObjects:@"key1", @"key2", @"key3", nil];
			 NSArray *objects = [NSArray arrayWithObjects:@"value1", @"value2", @"value3", nil];
			 NSDictionary *dictionary = [NSDictionary dictionaryWithObjects:objects forKeys:keys];
			 
			 for (id key in dictionary)
			 {
			 DLog(@"key: %@, value: %@", key, [dictionary objectForKey:key]);
			 }
			 
			pArtist = tag->artist();
			pTitle = tag->title();;
			pAlbum = tag->album();
			pGenre = tag->genre();
			pComment = tag->comment();
			
			lYear = tag->year();
			lTrack = tag->track();
			
			if (!pArtist.isNull())
				lArtist = [NSString stringWithUTF8String:pArtist.toCString(true)];
			
			if (!pAlbum.isNull())
				lAlbum = [NSString stringWithUTF8String:pAlbum.toCString(true)];
			
			if (!pTitle.isNull())
				lTitle = [NSString stringWithUTF8String:pTitle.toCString(true)];
			
			if (!pGenre.isNull())
				lGenre = [NSString stringWithUTF8String:pGenre.toCString(true)];
		*/
	}
		

	
	return 0;

}

@end
