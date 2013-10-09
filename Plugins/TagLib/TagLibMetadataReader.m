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
            float rgAlbumGain, rgAlbumPeak, rgTrackGain, rgTrackPeak;
			
			artist = tag->artist();
			title = tag->title();;
			album = tag->album();
			genre = tag->genre();
			comment = tag->comment();
			
			year = tag->year();
			[dict setObject:[NSNumber numberWithInt:year] forKey:@"year"];
			
			track = tag->track();
			[dict setObject:[NSNumber numberWithInt:track] forKey:@"track"];

            rgAlbumGain = tag->rgAlbumGain();
            rgAlbumPeak = tag->rgAlbumPeak();
            rgTrackGain = tag->rgTrackGain();
            rgTrackPeak = tag->rgTrackPeak();
            [dict setObject:[NSNumber numberWithFloat:rgAlbumGain] forKey:@"replayGainAlbumGain"];
            [dict setObject:[NSNumber numberWithFloat:rgAlbumPeak] forKey:@"replayGainAlbumPeak"];
            [dict setObject:[NSNumber numberWithFloat:rgTrackGain] forKey:@"replayGainTrackGain"];
            [dict setObject:[NSNumber numberWithFloat:rgTrackPeak] forKey:@"replayGainTrackPeak"];
			
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

		// Try to load the image.
		NSData * image = nil;
        
		// WARNING: HACK
		TagLib::MPEG::File *mf = dynamic_cast<TagLib::MPEG::File *>(f.file());
		if (mf) {
			TagLib::ID3v2::Tag *tag = mf->ID3v2Tag();
			if (tag) {
				TagLib::ID3v2::FrameList pictures = mf->ID3v2Tag()->frameListMap()["APIC"];
				if (!pictures.isEmpty()) {
					TagLib::ID3v2::AttachedPictureFrame *pic = static_cast<TagLib::ID3v2::AttachedPictureFrame *>(pictures.front());
						
						image = [NSData dataWithBytes:pic->picture().data() length:pic->picture().size()];
				}
			}
		}

		if (nil == image) {
			// Try to load image from external file

			NSString *path = [[url path] stringByDeletingLastPathComponent];

				// Gather list of candidate image files
				
			NSArray *fileNames = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:path error:nil];
			NSArray *imageFileNames = [fileNames pathsMatchingExtensions:[NSImage imageFileTypes]];

			NSEnumerator *imageEnumerator = [imageFileNames objectEnumerator];
			NSString *fileName;
				
			while (fileName = [imageEnumerator nextObject]) {
				if ([TagLibMetadataReader isCoverFile:fileName]) {
					image = [NSData dataWithContentsOfFile:[path stringByAppendingPathComponent:fileName]];
					break;
				}
			}
		}
		
		if (nil != image) {
			[dict setObject:image forKey:@"albumArt"];
		}
	}

	return [dict autorelease];
}

+ (BOOL)isCoverFile:(NSString *)fileName
{
	NSEnumerator *coverEnumerator = [[TagLibMetadataReader coverNames] objectEnumerator];
	NSString *coverFileName;
	
	while (coverFileName = [coverEnumerator nextObject]) {
		if ([[[[fileName lastPathComponent] stringByDeletingPathExtension] lowercaseString] hasSuffix:coverFileName]) {
			return true;
		}
	}
	return false;
			
}

+ (NSArray *)coverNames
{
	return [NSArray arrayWithObjects:@"cover", @"folder", @"album", @"front", nil];
}

+ (NSArray *)fileTypes
{
	//May be a way to get a list of supported formats
	return [NSArray arrayWithObjects:@"ape", @"asf", @"wma", @"ogg", @"opus", @"mpc", @"flac", @"m4a", @"mp3", @"tak", nil];
}

+ (NSArray *)mimeTypes
{
	return [NSArray arrayWithObjects:@"audio/x-ape", @"audio/x-ms-wma", @"application/ogg", @"application/x-ogg", @"audio/x-vorbis+ogg", @"audio/x-musepack", @"audio/x-flac", @"audio/x-m4a", @"audio/mpeg", @"audio/x-mp3", @"audio/x-tak", nil];
}

@end
