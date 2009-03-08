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
#import <Taglib/mpegfile.h>
#import <Taglib/id3v2tag.h>
#import <Taglib/attachedpictureframe.h>

@implementation TagLibMetadataReader

+ (NSDictionary *)metadataForURL:(NSURL *)url
{
	if (![url isFileURL]) {
		return [NSDictionary dictionary];
	}
	
	NSMutableDictionary *dict = [[NSMutableDictionary alloc] init];
	
	TagLib::FileRef f((const char *)[[url path] UTF8String], false);
	if (!f.isNull())
	{
		const TagLib::Tag *tag = f.tag();
		
		if (tag)
		{
			TagLib::String artist, title, album, genre, comment;
			int year, track;
			
			artist = tag->artist();
			title = tag->title();;
			album = tag->album();
			genre = tag->genre();
			comment = tag->comment();
			
			year = tag->year();
			[dict setObject:[NSNumber numberWithInt:year] forKey:@"year"];
			
			track = tag->track();
			[dict setObject:[NSNumber numberWithInt:track] forKey:@"track"];
			
			if (!artist.isNull())
				[dict setObject:[NSString stringWithUTF8String:artist.toCString(true)] forKey:@"artist"];

			if (!album.isNull())
				[dict setObject:[NSString stringWithUTF8String:album.toCString(true)] forKey:@"album"];
			
			if (!title.isNull())
				[dict setObject:[NSString stringWithUTF8String:title.toCString(true)] forKey:@"title"];
			
			if (!genre.isNull())
				[dict setObject:[NSString stringWithUTF8String:genre.toCString(true)] forKey:@"genre"];
		}
		
		NSString *imageCacheTag = [NSString stringWithFormat:@"%@-%@-%@-%@", [dict objectForKey:@"album"], [dict objectForKey:@"artist"], [dict objectForKey:@"genre"], [dict objectForKey:@"year"]];
		NSImage *image = [NSImage imageNamed:imageCacheTag];
		
		if (nil == image) {
			// Try to load the image.
		
			// WARNING: HACK
			TagLib::MPEG::File *mf = dynamic_cast<TagLib::MPEG::File *>(f.file());
			if (mf) {
				TagLib::ID3v2::Tag *tag = mf->ID3v2Tag();
				if (tag) {
					TagLib::ID3v2::FrameList pictures = mf->ID3v2Tag()->frameListMap()["APIC"];
					if (!pictures.isEmpty()) {
						TagLib::ID3v2::AttachedPictureFrame *pic = static_cast<TagLib::ID3v2::AttachedPictureFrame *>(pictures.front());
						
						NSData *data = [[NSData alloc] initWithBytes:pic->picture().data() length:pic->picture().size()];
						image = [[[NSImage alloc] initWithData:data] autorelease];
						[data release];
					}
				}
			}
			
			if (nil != image) {
				[image setName:imageCacheTag];
			}
		}

		if (nil != image) {
			[dict setObject:image forKey:@"albumArt"];
		}
	}

	return [dict autorelease];
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
