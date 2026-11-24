//
//  TagLibChapters.mm
//  TagLib
//
//  Chapters through TagLib: ID3v2 CHAP/CTOC frames, Matroska editions, and
//  MP4's two kinds. Shared by the metadata writer, which saves them, and
//  the ID3v2 tag reader, which reads them from a tag block the minimp3
//  decoder has already read. One reader, so the editor, the playlist's
//  chapter rows and the decoder that plays them agree on the chapters and
//  their order by construction.
//

#import "TagLibChapters.h"

#import "Plugin.h"

#import <tag/aifffile.h>
#import <tag/chapterframe.h>
#import <tag/dsdifffile.h>
#import <tag/dsffile.h>
#import <tag/id3v2tag.h>
#import <tag/matroskachapter.h>
#import <tag/matroskachapteredition.h>
#import <tag/matroskachapters.h>
#import <tag/matroskafile.h>
#import <tag/mp4chapter.h>
#import <tag/mp4file.h>
#import <tag/mpegfile.h>
#import <tag/tableofcontentsframe.h>
#import <tag/textidentificationframe.h>
#import <tag/trueaudiofile.h>
#import <tag/wavfile.h>

#include <algorithm>
#include <vector>

namespace {

TagLib::String toString(NSString *string) {
	return TagLib::String([string UTF8String] ?: "", TagLib::String::UTF8);
}

NSString *toNSString(const TagLib::String &string) {
	return [NSString stringWithUTF8String:string.toCString(true)] ?: @"";
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

// MP4 chapters carry a title and a start; each ends where the next starts,
// and the last where the file does. The QuickTime track when there is one,
// as Apple's players show it, otherwise the Nero list.
std::vector<Chapter> readMP4Chapters(TagLib::MP4::File *file) {
	std::vector<Chapter> chapters;
	TagLib::MP4::ChapterList list = file->qtChapters();
	if(list.isEmpty()) list = file->neroChapters();
	for(const auto &source : list) {
		Chapter chapter;
		chapter.start = source.startTime() > 0 ? (unsigned long long)source.startTime() : 0;
		chapter.title = toNSString(source.title());
		chapters.push_back(chapter);
	}
	std::stable_sort(chapters.begin(), chapters.end(), [](const Chapter &a, const Chapter &b) { return a.start < b.start; });
	const unsigned long long length = file->audioProperties() ? (unsigned long long)file->audioProperties()->lengthInMilliseconds() : 0;
	for(size_t i = 0; i < chapters.size(); ++i) {
		chapters[i].end = i + 1 < chapters.size() ? chapters[i + 1].start : std::max(length, chapters[i].start);
	}
	return chapters;
}

unsigned long long randomUID() {
	unsigned long long uid = 0;
	while(uid == 0) arc4random_buf(&uid, sizeof(uid));
	return uid;
}

} // namespace

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

std::vector<Chapter> readChapters(TagLib::File *file) {
	if(auto matroska = dynamic_cast<TagLib::Matroska::File *>(file)) return readMatroskaChapters(matroska);
	if(auto mp4 = dynamic_cast<TagLib::MP4::File *>(file)) return readMP4Chapters(mp4);
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

// Both kinds, so every player finds the same chapters; an empty list
// removes them.
void writeMP4Chapters(TagLib::MP4::File *file, const std::vector<Chapter> &chapters) {
	TagLib::MP4::ChapterList list;
	for(const Chapter &chapter : chapters) {
		list.append(TagLib::MP4::Chapter(toString(chapter.title), (long long)chapter.start));
	}
	file->setQtChapters(list);
	file->setNeroChapters(list);
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
