//
//  TagLibMetadataReader.m
//  TagLib
//
//  Created by Vincent Spader on 2/24/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "TagLibMetadataReader.h"
#import <tag/audioproperties.h>
#import <tag/fileref.h>
#import <tag/flacfile.h>
#import <tag/mpcproperties.h>
#import <tag/tag.h>

#import "Logging.h"

#import "SandboxBroker.h"

@implementation TagLibMetadataReader

+ (NSDictionary *)readMetadataFromTag:(const TagLib::Tag *)tag {
	NSMutableDictionary *dict = [NSMutableDictionary new];

	try {
		TagLib::String artist, albumartist, composer, title, album, genre, comment, unsyncedlyrics;
		int year, track, disc;
		TagLib::Tag::ReplayGain rg;
		TagLib::String cuesheet;
		TagLib::String soundcheck;
		
		artist = tag->artist();
		albumartist = tag->albumartist();
		composer = tag->composer();
		title = tag->title();
		
		album = tag->album();
		genre = tag->genre();
		comment = tag->comment();
		cuesheet = tag->cuesheet();
		
		unsyncedlyrics = tag->unsyncedlyrics();
		
		year = tag->year();
		if(year)
			[dict setObject:@(year) forKey:@"year"];
		
		track = tag->track();
		if(track)
			[dict setObject:@(track) forKey:@"track"];
		
		disc = tag->disc();
		if(disc)
			[dict setObject:@(disc) forKey:@"disc"];
		
		rg = tag->replaygain();
		if(!rg.isEmpty()) {
			if(rg.albumGainSet())
				[dict setObject:@(rg.albumGain()) forKey:@"replaygain_album_gain"];
			if(rg.albumPeakSet())
				[dict setObject:@(rg.albumPeak()) forKey:@"replaygain_album_peak"];
			if(rg.trackGainSet())
				[dict setObject:@(rg.trackGain()) forKey:@"replaygain_track_gain"];
			if(rg.trackPeakSet())
				[dict setObject:@(rg.trackPeak()) forKey:@"replaygain_track_peak"];
		}
		
		soundcheck = tag->soundcheck();
		if(!soundcheck.isEmpty()) {
			[dict setObject:[NSString stringWithUTF8String:soundcheck.toCString(true)] forKey:@"soundcheck"];
		}
		
		if(!artist.isEmpty())
			[dict setObject:[NSString stringWithUTF8String:artist.toCString(true)] forKey:@"artist"];
		
		if(!albumartist.isEmpty())
			[dict setObject:[NSString stringWithUTF8String:albumartist.toCString(true)] forKey:@"albumartist"];
		
		if(!composer.isEmpty())
			[dict setObject:[NSString stringWithUTF8String:composer.toCString(true)] forKey:@"composer"];
		
		if(!album.isEmpty())
			[dict setObject:[NSString stringWithUTF8String:album.toCString(true)] forKey:@"album"];
		
		if(!title.isEmpty()) {
			// XXX workaround for colliding with chapterized inputs' readers
			if(album.isEmpty() || title != album)
				[dict setObject:[NSString stringWithUTF8String:title.toCString(true)] forKey:@"title"];
		}
		
		if(!genre.isEmpty())
			[dict setObject:[NSString stringWithUTF8String:genre.toCString(true)] forKey:@"genre"];
		
		if(!cuesheet.isEmpty())
			[dict setObject:[NSString stringWithUTF8String:cuesheet.toCString(true)] forKey:@"cuesheet"];
		
		if(!comment.isEmpty())
			[dict setObject:[NSString stringWithUTF8String:comment.toCString(true)] forKey:@"comment"];
		
		if(!unsyncedlyrics.isEmpty())
			[dict setObject:[NSString stringWithUTF8String:unsyncedlyrics.toCString(true)] forKey:@"unsyncedlyrics"];
		
		// Try to load the image.
		NSData *image = nil;
		
		TagLib::StringList properties = tag->complexPropertyKeys();
		if(properties.contains("PICTURE")) {
			const TagLib::List<TagLib::VariantMap> &props = tag->complexProperties("PICTURE");
			if(!props.isEmpty()) {
				const TagLib::VariantMap &picture = props.front();
				TagLib::ByteVector data = picture["data"].toByteVector();
				
				image = [NSData dataWithBytes:data.data() length:data.size()];
				[dict setObject:image forKey:@"albumArt"];
			}
		}
	} catch (std::exception &e) {
		ALog(@"Exception caught reading properties from TagLib: %s", e.what());
		return @{};
	}

	return [NSDictionary dictionaryWithDictionary:dict];
}

+ (NSDictionary *)metadataForURL:(NSURL *)url {
	if(![url isFileURL]) {
		return @{};
	}

	id sandboxBrokerClass = NSClassFromString(@"SandboxBroker");
	id sandboxBroker = [sandboxBrokerClass sharedSandboxBroker];

	const void *sbHandle = [sandboxBroker beginFolderAccess:url];

	NSDictionary *dict = @{};

	//	if ( !*TagLib::ascii_encoding ) {
	//		NSStringEncoding enc = [NSString defaultCStringEncoding];
	//		CFStringEncoding cfenc = CFStringConvertNSStringEncodingToEncoding(enc);
	//		NSString *ref = (NSString *)CFStringConvertEncodingToIANACharSetName(cfenc);
	//		UInt32 cp = CFStringConvertEncodingToWindowsCodepage(cfenc);
	//
	//		// Most tags are using windows codepage, so remap OS X codepage to Windows one.
	//
	//		static struct {
	//			UInt32 from, to;
	//		} codepage_remaps[] = {
	//			{ 10001, 932 },		// Japanese Shift-JIS
	//			{ 10002, 950 },		// Traditional Chinese
	//			{ 10003, 949 },		// Korean
	//			{ 10004, 1256 },	// Arabic
	//			{ 10005, 1255 },	// Hebrew
	//			{ 10006, 1253 },	// Greek
	//			{ 10007, 1251 },	// Cyrillic
	//			{ 10008, 936 },		// Simplified Chinese
	//			{ 10029, 1250 },	// Central European (latin2)
	//		};
	//
	//		int i;
	//		int max = sizeof(codepage_remaps)/sizeof(codepage_remaps[0]);
	//		for ( i=0; i<max; i++ )
	//			if ( codepage_remaps[i].from == cp )
	//				break;
	//		if ( i < max )
	//			sprintf(TagLib::ascii_encoding, "windows-%d", codepage_remaps[i].to);
	//		else
	//			strcpy(TagLib::ascii_encoding, [ref UTF8String]);
	//
	//	}

	try {
		TagLib::FileRef f((const char *)[[url path] UTF8String], false);
		if(!f.isNull()) {
			const TagLib::Tag *tag = f.tag();

			if(tag) {
				dict = [self readMetadataFromTag:tag];
			}
		}
	} catch (std::exception &e) {
		ALog(@"Exception caught reading file with TagLib: %s", e.what());
		[sandboxBroker endFolderAccess:sbHandle];
		return @{};
	}

	[sandboxBroker endFolderAccess:sbHandle];

	return dict;
}

+ (NSArray *)fileTypes {
	NSMutableArray *extlist = [NSMutableArray new];
	TagLib::StringList exts = TagLib::FileRef::defaultFileExtensions();
	for(const auto &ext : exts) {
		[extlist addObject:[NSString stringWithUTF8String:ext.toCString(true)]];
	}
	return [NSArray arrayWithArray:extlist];
}

+ (NSArray *)mimeTypes {
	return @[@"audio/x-ms-wma", @"audio/x-musepack", @"audio/mpeg", @"audio/x-mp3", @"audio/mp4", @"audio/x-apl", @"audio/wav", @"audio/aiff", @"audio/x-wavpack", @"audio/ogg"];
}

+ (float)priority {
	return 1.0f;
}

@end
