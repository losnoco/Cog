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
#import <tag/tag.h>
#import <tag/tpropertymap.h>

#import "Logging.h"

#import "SandboxBroker.h"

@implementation TagLibMetadataReader

+ (NSDictionary *)readMetadataFromFile:(const TagLib::FileRef &)f {
	NSMutableDictionary *dict = [NSMutableDictionary new];

	try {
		TagLib::String title;

		TagLib::Tag *tag = f.tag();
		if(tag) {
			title = tag->title();
		}

		TagLib::PropertyMap tags = f.properties();
		
		NSDictionary *tagmap = @{@"tracknumber": @"track",
								 @"discnumber": @"disc",
								 @"itunnorm": @"soundcheck"};

		for(auto i = tags.cbegin(); i != tags.cend(); ++i) {
			NSString *name = [guess_encoding_of_string(i->first.toCString(true)) lowercaseString];
			NSMutableArray *valuelist = [NSMutableArray new];
			for(auto j = i->second.begin(); j != i->second.end(); ++j) {
				[valuelist addObject:guess_encoding_of_string(j->toCString(true))];
			}
			NSString *mapped = [tagmap valueForKey:name];
			if(mapped) {
				[dict setValue:valuelist forKey:mapped];
			} else {
				[dict setValue:valuelist forKey:name];
			}
		}

		TagLib::StringList names = f.complexPropertyKeys();

		if(!title.isEmpty()) {
			NSString *titlename;
			if(names.contains("CHAPTERS")) {
				titlename = @"album";
			} else {
				titlename = @"title";
			}
			[dict setValue:guess_encoding_of_string(title.toCString(true)) forKey:titlename];
		}
		
		// Try to load the image.
		NSData *image = nil;
		
		if(names.contains("PICTURE")) {
			const TagLib::List<TagLib::VariantMap> &props = f.complexProperties("PICTURE");
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
			dict = [self readMetadataFromFile:f];
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
	NSArray *skiplist = @[@"mp4", @"m4a"];
	NSMutableArray *extlist = [NSMutableArray new];
	TagLib::StringList exts = TagLib::FileRef::defaultFileExtensions();
	for(const auto &ext : exts) {
		NSString *_ext = [NSString stringWithUTF8String:ext.toCString(true)];
		if([skiplist containsObject:_ext])
			continue;
		[extlist addObject:_ext];
	}
	return [NSArray arrayWithArray:extlist];
}

+ (NSArray *)mimeTypes {
	return @[@"audio/x-ms-wma", @"audio/x-musepack", @"audio/mpeg", @"audio/x-mp3", @"audio/mp4", @"audio/x-apl", @"audio/wav", @"audio/aiff", @"audio/x-wavpack", @"audio/ogg", @"audio/matroska"];
}

+ (float)priority {
	return 1.0f;
}

@end
