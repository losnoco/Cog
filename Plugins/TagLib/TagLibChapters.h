//
//  TagLibChapters.h
//  TagLib
//
//  Chapters through TagLib, shared by the metadata writer and the ID3v2
//  tag reader. One reader, so the editor, the playlist's chapter rows and
//  the decoder that plays them agree on the chapters and their order by
//  construction.
//

#import <Foundation/Foundation.h>

#ifdef __cplusplus
#include <vector>

namespace TagLib {
class File;
namespace ID3v2 {
class Tag;
}
namespace Matroska {
class File;
}
namespace MP4 {
class File;
}
}

// One chapter: start and end in milliseconds, a title, and the identity a
// read keeps so a write preserves it (an ID3v2 element ID as NSString, or a
// Matroska UID as NSNumber).
struct Chapter {
	unsigned long long start = 0; // milliseconds
	unsigned long long end = 0;
	NSString *title = @"";
	id identifier = nil; // NSString element ID (ID3v2) or NSNumber UID (Matroska)
};

// The ID3v2 tag of any file kind that keeps one, created when asked.
TagLib::ID3v2::Tag *id3v2TagOf(TagLib::File *file, bool create);

// The CHAP/CTOC chapters of an ID3v2 tag: the top-level ordered table of
// contents' order when it gives one, else by start.
std::vector<Chapter> readID3v2Chapters(TagLib::ID3v2::Tag *tag);

// The chapters of any file TagLib can read them from (ID3v2, Matroska, MP4).
std::vector<Chapter> readChapters(TagLib::File *file);

// The chapters a write was handed, ordered by start, each ending where the
// next starts and the last where the file does.
std::vector<Chapter> chaptersFromArray(NSArray *array, unsigned long long length);

void writeID3v2Chapters(TagLib::ID3v2::Tag *tag, const std::vector<Chapter> &chapters);
void writeMatroskaChapters(TagLib::Matroska::File *file, const std::vector<Chapter> &chapters);
void writeMP4Chapters(TagLib::MP4::File *file, const std::vector<Chapter> &chapters);

// The chapters as the writer and the tag reader answer them: an array of
// dictionaries under the CogChapter keys.
NSArray *chapterArray(const std::vector<Chapter> &chapters);
#endif