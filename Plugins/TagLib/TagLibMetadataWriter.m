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
//  * Saving goes to a copy beside the file, which then replaces it: TagLib
//    rewrites files in place, so a failed or interrupted save would
//    otherwise leave a damaged file. Where the folder can't be written
//    (a sandboxed app given only the file), the file is saved in place
//    with a copy in the temporary folder to restore from on failure.
//  * MPEG files save their ID3v2 tag alone and strip the others: TagLib
//    reads ID3v1 and APE tags too, so a stale one would read back over the
//    new tag. WAV and DSDIFF keep their second tag kind (RIFF INFO, DIIN),
//    which setProperties writes the same values to.
//  * Chapters exist for TagLib 2.2.1 only as ID3v2 CHAP/CTOC frames and
//    Matroska chapters. MP4 chapters it neither reads nor writes, and a
//    save leaves them alone.
//

#import "TagLibMetadataWriter.h"

#import <tag/aifffile.h>
#import <tag/apefile.h>
#import <tag/asffile.h>
#import <tag/chapterframe.h>
#import <tag/dsdifffile.h>
#import <tag/dsffile.h>
#import <tag/fileref.h>
#import <tag/flacfile.h>
#import <tag/id3v2tag.h>
#import <tag/matroskachapter.h>
#import <tag/matroskachapteredition.h>
#import <tag/matroskachapters.h>
#import <tag/matroskafile.h>
#import <tag/mp4file.h>
#import <tag/mpcfile.h>
#import <tag/mpegfile.h>
#import <tag/oggfile.h>
#import <tag/tableofcontentsframe.h>
#import <tag/textidentificationframe.h>
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
			return @{ CogTagWriterWritable: @YES, CogTagWriterFoldsKeys: @YES, CogTagWriterMultiValue: @YES, CogTagWriterPictures: @YES };
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

// MARK: - Chapters

struct Chapter {
	unsigned long long start = 0; // milliseconds
	unsigned long long end = 0;
	NSString *title = @"";
	id identifier = nil; // NSString element ID (ID3v2) or NSNumber UID (Matroska)
};

TagLib::ID3v2::Tag *id3v2TagOf(TagLib::File *file, bool create) {
	if(auto mpeg = dynamic_cast<TagLib::MPEG::File *>(file)) return mpeg->ID3v2Tag(create);
	if(auto wav = dynamic_cast<TagLib::RIFF::WAV::File *>(file)) return (create || wav->hasID3v2Tag()) ? wav->ID3v2Tag() : nullptr;
	if(auto aiff = dynamic_cast<TagLib::RIFF::AIFF::File *>(file)) return (create || aiff->hasID3v2Tag()) ? aiff->tag() : nullptr;
	if(auto dsf = dynamic_cast<TagLib::DSF::File *>(file)) return dsf->tag();
	if(auto dsdiff = dynamic_cast<TagLib::DSDIFF::File *>(file)) return dsdiff->ID3v2Tag(create);
	if(auto tta = dynamic_cast<TagLib::TrueAudio::File *>(file)) return tta->ID3v2Tag(create);
	return nullptr;
}

std::vector<Chapter> readID3v2Chapters(TagLib::ID3v2::Tag *tag) {
	std::vector<Chapter> chapters;
	if(!tag) return chapters;
	const TagLib::ID3v2::FrameList &frames = tag->frameList("CHAP");
	if(frames.isEmpty()) return chapters;

	auto chapterOf = [](const TagLib::ID3v2::ChapterFrame *frame) {
		Chapter chapter;
		chapter.start = frame->startTime();
		chapter.end = frame->endTime();
		const TagLib::ID3v2::FrameList &titles = frame->embeddedFrameList("TIT2");
		if(!titles.isEmpty()) chapter.title = toNSString(titles.front()->toString());
		const TagLib::ByteVector elementID = frame->elementID();
		chapter.identifier = [[NSString alloc] initWithBytes:elementID.data() length:elementID.size() encoding:NSISOLatin1StringEncoding];
		return chapter;
	};

	// In the top-level table of contents' order when it gives one,
	// otherwise by start.
	const TagLib::ID3v2::TableOfContentsFrame *toc = nullptr;
	for(const auto frame : tag->frameList("CTOC")) {
		auto candidate = dynamic_cast<const TagLib::ID3v2::TableOfContentsFrame *>(frame);
		if(candidate && candidate->isTopLevel()) {
			toc = candidate;
			break;
		}
	}
	if(toc && toc->isOrdered()) {
		for(const auto &elementID : toc->childElements()) {
			if(auto frame = TagLib::ID3v2::ChapterFrame::findByElementID(tag, elementID)) {
				chapters.push_back(chapterOf(frame));
			}
		}
	}
	if(chapters.empty()) {
		for(const auto frame : frames) {
			if(auto chapter = dynamic_cast<const TagLib::ID3v2::ChapterFrame *>(frame)) {
				chapters.push_back(chapterOf(chapter));
			}
		}
		std::stable_sort(chapters.begin(), chapters.end(), [](const Chapter &a, const Chapter &b) { return a.start < b.start; });
	}
	return chapters;
}

std::vector<Chapter> readMatroskaChapters(TagLib::Matroska::File *file) {
	std::vector<Chapter> chapters;
	TagLib::Matroska::Chapters *all = file->chapters();
	if(!all) return chapters;
	const auto &editions = all->chapterEditionList();
	if(editions.isEmpty()) return chapters;
	const TagLib::Matroska::ChapterEdition *edition = &editions.front();
	for(const auto &candidate : editions) {
		if(candidate.isDefault()) {
			edition = &candidate;
			break;
		}
	}
	for(const auto &source : edition->chapterList()) {
		if(source.isHidden()) continue;
		Chapter chapter;
		chapter.start = source.timeStart() / 1000000;
		chapter.end = source.timeEnd() / 1000000;
		if(!source.displayList().isEmpty()) chapter.title = toNSString(source.displayList().front().string());
		chapter.identifier = @(source.uid());
		chapters.push_back(chapter);
	}
	return chapters;
}

std::vector<Chapter> readChapters(TagLib::File *file) {
	if(auto matroska = dynamic_cast<TagLib::Matroska::File *>(file)) return readMatroskaChapters(matroska);
	return readID3v2Chapters(id3v2TagOf(file, false));
}

// The chapters a write was handed, ordered by start, each ending where the
// next starts and the last where the file does.
std::vector<Chapter> chaptersFromArray(NSArray *array, unsigned long long length) {
	std::vector<Chapter> chapters;
	for(NSDictionary *item in array) {
		if(![item isKindOfClass:[NSDictionary class]]) continue;
		Chapter chapter;
		chapter.start = [item[CogChapterStart] unsignedLongLongValue];
		chapter.end = [item[CogChapterEnd] unsignedLongLongValue];
		NSString *title = item[CogChapterTitle];
		chapter.title = [title isKindOfClass:[NSString class]] ? title : @"";
		chapter.identifier = item[CogChapterID];
		chapters.push_back(chapter);
	}
	std::stable_sort(chapters.begin(), chapters.end(), [](const Chapter &a, const Chapter &b) { return a.start < b.start; });
	for(size_t i = 0; i < chapters.size(); ++i) {
		if(i + 1 < chapters.size()) {
			chapters[i].end = chapters[i + 1].start;
		} else if(length > chapters[i].start) {
			chapters[i].end = length;
		} else if(chapters[i].end < chapters[i].start) {
			chapters[i].end = chapters[i].start;
		}
	}
	return chapters;
}

void writeID3v2Chapters(TagLib::ID3v2::Tag *tag, const std::vector<Chapter> &chapters) {
	tag->removeFrames("CTOC");
	tag->removeFrames("CHAP");
	if(chapters.empty()) return;

	TagLib::ByteVectorList children;
	NSMutableSet *used = [NSMutableSet new];
	unsigned int next = 0;
	for(const Chapter &chapter : chapters) {
		NSString *elementID = [chapter.identifier isKindOfClass:[NSString class]] ? chapter.identifier : nil;
		if(![elementID length] || [used containsObject:elementID] || ![elementID canBeConvertedToEncoding:NSISOLatin1StringEncoding]) {
			do {
				elementID = [NSString stringWithFormat:@"chp%u", next++];
			} while([used containsObject:elementID]);
		}
		[used addObject:elementID];
		NSData *idData = [elementID dataUsingEncoding:NSISOLatin1StringEncoding];
		TagLib::ByteVector idBytes((const char *)[idData bytes], (unsigned int)[idData length]);

		auto title = new TagLib::ID3v2::TextIdentificationFrame("TIT2", TagLib::String::UTF8);
		title->setText(toString(chapter.title));
		TagLib::ID3v2::FrameList embedded;
		embedded.append(title);
		// 0xFFFFFFFF offsets: the times are what count.
		tag->addFrame(new TagLib::ID3v2::ChapterFrame(idBytes, (unsigned int)chapter.start, (unsigned int)chapter.end, 0xFFFFFFFF, 0xFFFFFFFF, embedded));
		children.append(idBytes);
	}
	auto toc = new TagLib::ID3v2::TableOfContentsFrame("toc", children);
	toc->setIsTopLevel(true);
	toc->setIsOrdered(true);
	tag->addFrame(toc);
}

unsigned long long randomUID() {
	unsigned long long uid = 0;
	while(uid == 0) arc4random_buf(&uid, sizeof(uid));
	return uid;
}

void writeMatroskaChapters(TagLib::Matroska::File *file, const std::vector<Chapter> &chapters) {
	TagLib::Matroska::Chapters *all = file->chapters(!chapters.empty());
	if(!all) return;
	all->clear();
	if(chapters.empty()) return;
	TagLib::List<TagLib::Matroska::Chapter> list;
	for(const Chapter &chapter : chapters) {
		unsigned long long uid = [chapter.identifier isKindOfClass:[NSNumber class]] ? [chapter.identifier unsignedLongLongValue] : 0;
		if(uid == 0) uid = randomUID();
		TagLib::List<TagLib::Matroska::Chapter::Display> displays;
		displays.append(TagLib::Matroska::Chapter::Display(toString(chapter.title), "eng"));
		list.append(TagLib::Matroska::Chapter(chapter.start * 1000000, chapter.end * 1000000, displays, uid));
	}
	all->addChapterEdition(TagLib::Matroska::ChapterEdition(list, true, false, randomUID()));
}

NSArray *chapterArray(const std::vector<Chapter> &chapters) {
	NSMutableArray *array = [NSMutableArray new];
	for(const Chapter &chapter : chapters) {
		NSMutableDictionary *item = [@{ CogChapterStart: @(chapter.start), CogChapterEnd: @(chapter.end), CogChapterTitle: chapter.title ?: @"" } mutableCopy];
		if(chapter.identifier) item[CogChapterID] = chapter.identifier;
		[array addObject:item];
	}
	return array;
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
		TagLib::FileRef f([[url path] fileSystemRepresentation], false);
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
	NSString *path = [url path];
	NSString *reason = nil;
	BOOL saved = NO;

	// A copy beside the file keeps the extension, which is how TagLib picks
	// the format, and the folder, so the replace is a rename.
	NSString *copyName = [NSString stringWithFormat:@".%@.cog-%@.%@", [[path lastPathComponent] stringByDeletingPathExtension], [[NSUUID UUID] UUIDString], [path pathExtension]];
	NSURL *copyURL = [[url URLByDeletingLastPathComponent] URLByAppendingPathComponent:copyName];
	NSError *copyError = nil;
	if([fileManager copyItemAtURL:url toURL:copyURL error:&copyError]) {
		if(saveTagsAtPath([copyURL path], tags, replace, &reason)) {
			NSError *replaceError = nil;
			saved = [fileManager replaceItemAtURL:url withItemAtURL:copyURL backupItemName:nil options:0 resultingItemURL:nil error:&replaceError];
			if(!saved) reason = [replaceError localizedDescription] ?: @"The saved copy could not replace the file.";
		}
		if(!saved) [fileManager removeItemAtURL:copyURL error:nil];
	} else {
		// The folder isn't writable, but the file may be: save in place,
		// with a copy to put back if TagLib fails.
		NSURL *backupURL = [[NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES] URLByAppendingPathComponent:copyName];
		NSData *original = [NSData dataWithContentsOfURL:url options:NSDataReadingMappedIfSafe error:nil];
		if(original && [original writeToURL:backupURL atomically:NO]) {
			saved = saveTagsAtPath(path, tags, replace, &reason);
			if(!saved && ![original writeToURL:url atomically:NO]) {
				ALog(@"Could not restore %@ after a failed save; the original is at %@", path, [backupURL path]);
				reason = [NSString stringWithFormat:@"%@ The original was kept at %@.", reason, [backupURL path]];
				backupURL = nil;
			}
			if(backupURL) [fileManager removeItemAtURL:backupURL error:nil];
		} else {
			reason = [copyError localizedDescription] ?: @"The file could not be copied before saving.";
		}
	}

	[sandboxBroker endFolderAccess:sbHandle];

	if(!saved && error) *error = saveError(url, reason ?: @"The tags could not be saved.");
	return saved;
}

@end
