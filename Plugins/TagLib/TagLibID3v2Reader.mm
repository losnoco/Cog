//
//  TagLibID3v2Reader.m
//  TagLib Plugin
//
//  Created by Christopher Snowhill on 2/8/22.
//

#import "TagLibID3v2Reader.h"

#import <taglib/fileref.h>
#import <taglib/mpeg/id3v2/frames/attachedpictureframe.h>
#import <taglib/mpeg/id3v2/id3v2tag.h>
#import <taglib/mpeg/mpegfile.h>
#import <taglib/toolkit/tbytevectorstream.h>

@implementation TagLibID3v2Reader

+ (NSDictionary *)metadataForTag:(NSData *)tagBlock {
	NSMutableDictionary *dict = [[NSMutableDictionary alloc] init];

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

	TagLib::ByteVector vector((const char *)[tagBlock bytes], (unsigned int)[tagBlock length]);
	TagLib::ByteVectorStream vectorStream(vector);

	TagLib::FileRef f(&vectorStream, false);
	if(!f.isNull()) {
		const TagLib::Tag *tag = f.tag();

		if(tag) {
			TagLib::String artist, albumartist, title, album, genre, comment, unsyncedlyrics;
			int year, track, disc;
			float rgAlbumGain, rgAlbumPeak, rgTrackGain, rgTrackPeak;
			TagLib::String cuesheet;
			TagLib::String soundcheck;

			artist = tag->artist();
			albumartist = tag->albumartist();
			title = tag->title();
			album = tag->album();
			genre = tag->genre();
			comment = tag->comment();
			cuesheet = tag->cuesheet();
			unsyncedlyrics = tag->unsyncedlyrics();

			year = tag->year();
			[dict setObject:@(year) forKey:@"year"];

			track = tag->track();
			[dict setObject:@(track) forKey:@"track"];

			disc = tag->disc();
			[dict setObject:@(disc) forKey:@"disc"];

			rgAlbumGain = tag->rgAlbumGain();
			rgAlbumPeak = tag->rgAlbumPeak();
			rgTrackGain = tag->rgTrackGain();
			rgTrackPeak = tag->rgTrackPeak();
			[dict setObject:@(rgAlbumGain) forKey:@"replaygain_album_gain"];
			[dict setObject:@(rgAlbumPeak) forKey:@"replaygain_album_peak"];
			[dict setObject:@(rgTrackGain) forKey:@"replaygain_track_gain"];
			[dict setObject:@(rgTrackPeak) forKey:@"replaygain_track_peak"];

			soundcheck = tag->soundcheck();
			if(!soundcheck.isEmpty()) {
				TagLib::StringList tag = soundcheck.split(" ");
				TagLib::StringList wantedTag;
				for(int i = 0, count = tag.size(); i < count; i++) {
					if(tag[i].length() == 8)
						wantedTag.append(tag[i]);
				}

				if(wantedTag.size() >= 10) {
					float volume1 = -log10((double)((uint32_t)wantedTag[0].toInt(16)) / 1000) * 10;
					float volume2 = -log10((double)((uint32_t)wantedTag[1].toInt(16)) / 1000) * 10;
					float volumeToUse = MIN(volume1, volume2);
					float volumeScale = pow(10, volumeToUse / 20);
					[dict setObject:@(volumeScale) forKey:@"volume"];
				}
			}

			if(!artist.isEmpty())
				[dict setObject:[NSString stringWithUTF8String:artist.toCString(true)] forKey:@"artist"];

			if(!albumartist.isEmpty())
				[dict setObject:[NSString stringWithUTF8String:albumartist.toCString(true)] forKey:@"albumartist"];

			if(!album.isEmpty())
				[dict setObject:[NSString stringWithUTF8String:album.toCString(true)] forKey:@"album"];

			if(!title.isEmpty())
				[dict setObject:[NSString stringWithUTF8String:title.toCString(true)] forKey:@"title"];

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

			TagLib::MPEG::File *mf = dynamic_cast<TagLib::MPEG::File *>(f.file());
			if(mf) {
				TagLib::ID3v2::FrameList pictures = mf->ID3v2Tag()->frameListMap()["APIC"];
				if(!pictures.isEmpty()) {
					TagLib::ID3v2::AttachedPictureFrame *pic = static_cast<TagLib::ID3v2::AttachedPictureFrame *>(pictures.front());

					image = [NSData dataWithBytes:pic->picture().data() length:pic->picture().size()];
				}
			}

			if(nil != image) {
				[dict setObject:image forKey:@"albumArt"];
			}
		}
	}

	return [NSDictionary dictionaryWithDictionary:dict];
}

@end
