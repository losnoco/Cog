//
//  TagLibID3v2Reader.m
//  TagLib Plugin
//
//  Created by Christopher Snowhill on 2/8/22.
//

#import "TagLibID3v2Reader.h"

#import "TagLibMetadataReader.h"

#import <tag/fileref.h>
#import <tag/tfilestream.h>
#import <tag/tbytevectorstream.h>

#import "Logging.h"

@implementation TagLibID3v2Reader

+ (NSDictionary *)metadataForTag:(NSData *)tagBlock {
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
		TagLib::ByteVector vector((const char *)[tagBlock bytes], (unsigned int)[tagBlock length]);
		TagLib::ByteVectorStream vectorStream(vector);

		TagLib::FileRef f((TagLib::IOStream *)&vectorStream, false);
		if(!f.isNull()) {
			const TagLib::Tag *tag = f.tag();

			if(tag) {
				dict = [TagLibMetadataReader readMetadataFromTag:tag];
			}
		}
	} catch (std::exception &e) {
		ALog(@"Exception caught processing ID3v2 tag with TagLib: %s", e.what());
		return @{};
	}

	return dict;
}

@end
