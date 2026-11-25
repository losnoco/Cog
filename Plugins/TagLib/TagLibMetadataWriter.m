//
//  TagLibMetadataWriter.m
//  TagLib
//
//  Writes tags, cover art and chapters back to files through TagLib, for
//  every format it can save. Compiled as Objective-C++.
//
//  * Tags are named as TagLibMetadataReader names them: TagLib's property
//    names lowercased, with tracknumber, discnumber and itunnorm read as
//    track, disc and soundcheck. A write maps those back.
//  * A save is a copy, saved, then swapped in for the file by a rename, never
//    a rewrite in place: TagLib rewrites files in place, which an
//    interrupted save would leave damaged, and the file may be playing. A
//    decoder reading it keeps the old file's data until it closes it, and
//    the next open gets the new one, where a tag that grew in place would
//    shift the audio under the decoder. The copy goes in a temporary folder
//    on the file's own volume, or else beside it; when neither can be
//    written, the save fails and the file is untouched.
//  * MPEG files save their ID3v2 tag alone and strip the others: TagLib
//    reads ID3v1 and APE tags too, so a stale one would read back over the
//    new tag. WAV and DSDIFF keep their second tag kind (RIFF INFO, DIIN),
//    which setProperties writes the same values to.
//  * Chapters are ID3v2 CHAP/CTOC frames, Matroska chapters, and MP4's two
//    kinds: a QuickTime chapter track, which Apple's players read, and a
//    Nero chpl list, which others do. A save writes both, from one list.
//

#import "TagLibMetadataWriter.h"

#import "TagLibChapters.h"

#import <tag/aifffile.h>
#import <tag/apefile.h>
#import <tag/asffile.h>
#import <tag/dsdifffile.h>
#import <tag/dsffile.h>
#import <tag/fileref.h>
#import <tag/flacfile.h>
#import <tag/id3v2tag.h>
#import <tag/matroskafile.h>
#import <tag/mp4file.h>
#import <tag/mpcfile.h>
#import <tag/mpegfile.h>
#import <tag/oggfile.h>
#import <tag/tpropertymap.h>
#import <tag/trueaudiofile.h>
#import <tag/wavfile.h>
#import <tag/wavpackfile.h>

#import "Logging.h"

#import "SandboxBroker.h"

#include <algorithm>
#include <vector>

namespace {

// How each kind of file keeps tags, which decides what a save can promise.
enum class Family {
	Unwritable, // TagLib reads it but doesn't save it (Shorten, modules)
	Xiph, // Vorbis comments, APE and ASF tags: any name, repeated values
	ID3v2, // MPEG, WAV, AIFF, DSF, DSDIFF, TrueAudio
	MP4,
	Matroska,
};

Family familyOf(TagLib::File *file) {
	if(!file) return Family::Unwritable;
	if(dynamic_cast<TagLib::MPEG::File *>(file) || dynamic_cast<TagLib::RIFF::WAV::File *>(file) ||
	   dynamic_cast<TagLib::RIFF::AIFF::File *>(file) || dynamic_cast<TagLib::DSF::File *>(file) ||
	   dynamic_cast<TagLib::DSDIFF::File *>(file) || dynamic_cast<TagLib::TrueAudio::File *>(file))
		return Family::ID3v2;
	if(dynamic_cast<TagLib::FLAC::File *>(file) || dynamic_cast<TagLib::Ogg::File *>(file) ||
	   dynamic_cast<TagLib::APE::File *>(file) || dynamic_cast<TagLib::WavPack::File *>(file) ||
	   dynamic_cast<TagLib::MPC::File *>(file) || dynamic_cast<TagLib::ASF::File *>(file))
		return Family::Xiph;
	if(dynamic_cast<TagLib::MP4::File *>(file)) return Family::MP4;
	if(dynamic_cast<TagLib::Matroska::File *>(file)) return Family::Matroska;
	return Family::Unwritable;
}

NSDictionary *capabilitiesOf(Family family) {
	switch(family) {
		case Family::Unwritable:
			return @{};
		case Family::Xiph:
			return @{ CogTagWriterWritable: @YES, CogTagWriterArbitraryKeys: @YES, CogTagWriterMultiValue: @YES, CogTagWriterPictures: @YES };
		case Family::ID3v2:
			// Names without a frame of their own become TXXX frames.
			return @{ CogTagWriterWritable: @YES, CogTagWriterFoldsKeys: @YES, CogTagWriterMultiValue: @YES, CogTagWriterPictures: @YES, CogTagWriterChapters: @YES };
		case Family::MP4:
			// Names without an atom of their own become ---- atoms.
			return @{ CogTagWriterWritable: @YES, CogTagWriterFoldsKeys: @YES, CogTagWriterMultiValue: @YES, CogTagWriterPictures: @YES, CogTagWriterChapters: @YES };
		case Family::Matroska:
			return @{ CogTagWriterWritable: @YES, CogTagWriterArbitraryKeys: @YES, CogTagWriterMultiValue: @YES, CogTagWriterPictures: @YES, CogTagWriterChapters: @YES };
	}
	return @{};
}

// The reader's names for TagLib's properties, and back.
NSString *tagNameForProperty(const TagLib::String &property) {
	NSString *name = [[NSString stringWithUTF8String:property.toCString(true)] lowercaseString];
	static NSDictionary *names = @{ @"tracknumber": @"track", @"discnumber": @"disc", @"itunnorm": @"soundcheck" };
	return names[name] ?: name;
}

TagLib::String propertyForTagName(NSString *name) {
	static NSDictionary *properties = @{ @"track": @"TRACKNUMBER", @"disc": @"DISCNUMBER", @"soundcheck": @"ITUNNORM" };
	NSString *property = properties[[name lowercaseString]] ?: [name uppercaseString];
	return TagLib::String([property UTF8String], TagLib::String::UTF8);
}

TagLib::String toString(NSString *string) {
	return TagLib::String([string UTF8String] ?: "", TagLib::String::UTF8);
}

NSString *toNSString(const TagLib::String &string) {
	return [NSString stringWithUTF8String:string.toCString(true)] ?: @"";
}

TagLib::StringList toStringList(id value) {
	TagLib::StringList list;
	NSArray *values = [value isKindOfClass:[NSArray class]] ? value : (value ? @[value] : @[]);
	for(id item in values) {
		if([item isKindOfClass:[NSString class]])
			list.append(toString(item));
		else if([item isKindOfClass:[NSNumber class]])
			list.append(toString([item stringValue]));
	}
	return list;
}

// The mime type a picture block is labeled with. FLAC reads an empty one
// as a link to art elsewhere, so there has to be one.
NSString *mimeTypeOfImage(NSData *data) {
	const unsigned char *bytes = (const unsigned char *)[data bytes];
	NSUInteger length = [data length];
	if(length >= 3 && bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF) return @"image/jpeg";
	if(length >= 8 && memcmp(bytes, "\x89PNG\r\n\x1a\n", 8) == 0) return @"image/png";
	if(length >= 6 && (memcmp(bytes, "GIF87a", 6) == 0 || memcmp(bytes, "GIF89a", 6) == 0)) return @"image/gif";
	if(length >= 12 && memcmp(bytes, "RIFF", 4) == 0 && memcmp(bytes + 8, "WEBP", 4) == 0) return @"image/webp";
	if(length >= 2 && bytes[0] == 'B' && bytes[1] == 'M') return @"image/bmp";
	return @"image/png";
}

// MARK: - Tags

NSDictionary *readTags(const TagLib::FileRef &f) {
	NSMutableDictionary *dict = [NSMutableDictionary new];
	const TagLib::PropertyMap properties = f.properties();
	for(auto i = properties.cbegin(); i != properties.cend(); ++i) {
		NSMutableArray *values = [NSMutableArray new];
		for(const auto &value : i->second) {
			[values addObject:toNSString(value)];
		}
		dict[tagNameForProperty(i->first)] = values;
	}
	const TagLib::List<TagLib::VariantMap> pictures = f.complexProperties("PICTURE");
	if(!pictures.isEmpty()) {
		const TagLib::ByteVector data = pictures.front()["data"].toByteVector();
		dict[CogTagAlbumArt] = [NSData dataWithBytes:data.data() length:data.size()];
	}
	Family family = familyOf(f.file());
	if([capabilitiesOf(family)[CogTagWriterChapters] boolValue]) {
		dict[CogTagChapters] = chapterArray(readChapters(f.file()));
	}
	return dict;
}

void applyPicture(TagLib::FileRef &f, NSData *art) {
	TagLib::List<TagLib::VariantMap> pictures = f.complexProperties("PICTURE");
	if(![art length]) {
		f.setComplexProperties("PICTURE", TagLib::List<TagLib::VariantMap>());
		return;
	}
	NSString *mimeType = mimeTypeOfImage(art);
	TagLib::VariantMap picture;
	picture["data"] = TagLib::ByteVector((const char *)[art bytes], (unsigned int)[art length]);
	picture["mimeType"] = toString(mimeType);
	picture["description"] = TagLib::String();
	picture["pictureType"] = TagLib::String("Front Cover");
	// Matroska keeps pictures as attachments, which need a file name.
	picture["fileName"] = toString([@"cover." stringByAppendingString:[[mimeType componentsSeparatedByString:@"/"] lastObject]]);

	// The cover replaces the front cover, or else the first picture, the
	// one the reader shows. Other pictures stay.
	bool replaced = false;
	for(auto it = pictures.begin(); it != pictures.end(); ++it) {
		if((*it)["pictureType"].toString() == "Front Cover") {
			*it = picture;
			replaced = true;
			break;
		}
	}
	if(!replaced) {
		if(pictures.isEmpty())
			pictures.append(picture);
		else
			pictures.front() = picture;
	}
	f.setComplexProperties("PICTURE", pictures);
}

bool applyTags(TagLib::FileRef &f, NSDictionary *tags, BOOL replace) {
	TagLib::PropertyMap map = replace ? TagLib::PropertyMap() : f.properties();
	for(NSString *name in tags) {
		if(![name isKindOfClass:[NSString class]] || [name isEqualToString:CogTagAlbumArt] || [name isEqualToString:CogTagChapters]) continue;
		// PropertyMap's keys are case-insensitive, so this also takes
		// "ARTIST" for "artist".
		const TagLib::String property = propertyForTagName(name);
		map.erase(property);
		const TagLib::StringList values = toStringList(tags[name]);
		if(!values.isEmpty()) map.replace(property, values);
	}
	const TagLib::PropertyMap unsupported = f.setProperties(map);
	if(!unsupported.isEmpty()) {
		DLog(@"TagLib could not store these tags: %s", unsupported.toString().toCString(true));
	}

	NSData *art = tags[CogTagAlbumArt];
	if([art isKindOfClass:[NSData class]]) applyPicture(f, art);

	NSArray *chapters = tags[CogTagChapters];
	if([chapters isKindOfClass:[NSArray class]]) {
		TagLib::File *file = f.file();
		const unsigned long long length = f.audioProperties() ? (unsigned long long)f.audioProperties()->lengthInMilliseconds() : 0;
		const std::vector<Chapter> list = chaptersFromArray(chapters, length);
		if(auto matroska = dynamic_cast<TagLib::Matroska::File *>(file)) {
			writeMatroskaChapters(matroska, list);
		} else if(auto mp4 = dynamic_cast<TagLib::MP4::File *>(file)) {
			writeMP4Chapters(mp4, list);
		} else if(auto tag = id3v2TagOf(file, !list.empty())) {
			writeID3v2Chapters(tag, list);
		}
	}

	TagLib::File *file = f.file();
	if(auto mpeg = dynamic_cast<TagLib::MPEG::File *>(file)) {
		return mpeg->save(TagLib::MPEG::File::ID3v2, TagLib::File::StripOthers, TagLib::ID3v2::v4, TagLib::File::DoNotDuplicate);
	}
	if(auto wav = dynamic_cast<TagLib::RIFF::WAV::File *>(file)) {
		return wav->save(TagLib::RIFF::WAV::File::AllTags, TagLib::File::StripOthers, TagLib::ID3v2::v4);
	}
	if(auto dsdiff = dynamic_cast<TagLib::DSDIFF::File *>(file)) {
		return dsdiff->save(TagLib::DSDIFF::File::AllTags, TagLib::File::StripOthers, TagLib::ID3v2::v4);
	}
	return f.save();
}

NSError *saveError(NSURL *url, NSString *reason) {
	return [NSError errorWithDomain:NSCocoaErrorDomain
	                           code:NSFileWriteUnknownError
	                       userInfo:@{ NSURLErrorKey: url, NSLocalizedFailureReasonErrorKey: reason }];
}

// Opens `path`, applies the tags and saves. False with `reason` set when
// TagLib can't.
bool saveTagsAtPath(NSString *path, NSDictionary *tags, BOOL replace, NSString **reason) {
	try {
		TagLib::FileRef f([path fileSystemRepresentation], true);
		if(f.isNull()) {
			*reason = @"TagLib could not open the file.";
			return false;
		}
		if(familyOf(f.file()) == Family::Unwritable) {
			*reason = @"TagLib cannot save tags to this kind of file.";
			return false;
		}
		if(f.file()->readOnly()) {
			*reason = @"The file is read-only.";
			return false;
		}
		if(!applyTags(f, tags, replace)) {
			*reason = @"TagLib could not save the file.";
			return false;
		}
		return true;
	} catch(std::exception &e) {
		*reason = [NSString stringWithFormat:@"TagLib failed: %s", e.what()];
		return false;
	}
}

} // namespace

bool TagLibFileHasChapters(TagLib::File *file) {
	try {
		return !readChapters(file).empty();
	} catch(std::exception &e) {
		return false;
	}
}

@implementation TagLibMetadataWriter

+ (NSArray *)fileTypes {
	NSMutableArray *types = [NSMutableArray new];
	for(const auto &ext : TagLib::FileRef::defaultFileExtensions()) {
		[types addObject:[NSString stringWithUTF8String:ext.toCString(true)]];
	}
	return types;
}

+ (float)priority {
	return 1.0f;
}

+ (NSDictionary *)capabilitiesForURL:(NSURL *)url {
	if(![url isFileURL] || [url fragment]) return @{};

	id sandboxBroker = [NSClassFromString(@"SandboxBroker") sharedSandboxBroker];
	const void *sbHandle = [sandboxBroker beginFolderAccess:url];

	NSDictionary *capabilities = @{};
	try {
		TagLib::FileRef f([[url path] fileSystemRepresentation], false);
		if(!f.isNull() && !f.file()->readOnly()) {
			capabilities = capabilitiesOf(familyOf(f.file()));
		}
	} catch(std::exception &e) {
		ALog(@"Exception caught probing file with TagLib: %s", e.what());
	}

	[sandboxBroker endFolderAccess:sbHandle];
	return capabilities;
}

+ (NSDictionary *)tagsForURL:(NSURL *)url {
	if(![url isFileURL] || [url fragment]) return nil;

	id sandboxBroker = [NSClassFromString(@"SandboxBroker") sharedSandboxBroker];
	const void *sbHandle = [sandboxBroker beginFolderAccess:url];

	NSDictionary *tags = nil;
	try {
		// With audio properties: the last MP4 chapter ends at the length.
		TagLib::FileRef f([[url path] fileSystemRepresentation], true);
		if(!f.isNull()) tags = readTags(f);
	} catch(std::exception &e) {
		ALog(@"Exception caught reading file with TagLib: %s", e.what());
	}

	[sandboxBroker endFolderAccess:sbHandle];
	return tags;
}

+ (BOOL)writeTags:(NSDictionary *)tags toURL:(NSURL *)url replace:(BOOL)replace error:(NSError **)error {
	if(![url isFileURL] || [url fragment]) {
		if(error) *error = saveError(url, @"Only a file's own tags can be saved.");
		return NO;
	}

	id sandboxBroker = [NSClassFromString(@"SandboxBroker") sharedSandboxBroker];
	const void *sbHandle = [sandboxBroker beginFolderAccess:url];

	NSFileManager *fileManager = [NSFileManager defaultManager];
	// A symbolic link stays one: its target is what gets the new tags.
	NSURL *target = [url URLByResolvingSymlinksInPath];
	NSString *reason = nil;
	BOOL saved = NO;

	// Where the copy can be made: the volume's own folder for safe saves,
	// or else beside the file. Either way on the file's volume, so the swap
	// is a rename. The copy keeps the file's extension, which is how TagLib
	// picks the format.
	NSURL *replacementFolder = [fileManager URLForDirectory:NSItemReplacementDirectory inDomain:NSUserDomainMask appropriateForURL:target create:YES error:nil];
	NSString *besideName = [NSString stringWithFormat:@".%@.cog-%@.%@", [[target lastPathComponent] stringByDeletingPathExtension], [[NSUUID UUID] UUIDString], [target pathExtension]];
	NSMutableArray<NSURL *> *places = [NSMutableArray new];
	if(replacementFolder) [places addObject:[replacementFolder URLByAppendingPathComponent:[target lastPathComponent]]];
	[places addObject:[[target URLByDeletingLastPathComponent] URLByAppendingPathComponent:besideName]];

	NSURL *copyURL = nil;
	NSError *copyError = nil;
	for(NSURL *place in places) {
		if([fileManager copyItemAtURL:target toURL:place error:&copyError]) {
			copyURL = place;
			break;
		}
	}

	if(copyURL) {
		if(saveTagsAtPath([copyURL path], tags, replace, &reason)) {
			NSError *replaceError = nil;
			// Keeps the file's own attributes: permissions, dates made,
			// extended attributes.
			saved = [fileManager replaceItemAtURL:target withItemAtURL:copyURL backupItemName:nil options:0 resultingItemURL:nil error:&replaceError];
			if(!saved) reason = [replaceError localizedDescription] ?: @"The saved copy could not replace the file.";
		}
		if(!saved) [fileManager removeItemAtURL:copyURL error:nil];
	} else {
		reason = [NSString stringWithFormat:@"Cog can't write a copy of the file to save into. %@", [copyError localizedFailureReason] ?: [copyError localizedDescription] ?: @""];
	}
	if(replacementFolder) [fileManager removeItemAtURL:replacementFolder error:nil];

	[sandboxBroker endFolderAccess:sbHandle];

	if(!saved && error) *error = saveError(url, reason ?: @"The tags could not be saved.");
	return saved;
}

@end
