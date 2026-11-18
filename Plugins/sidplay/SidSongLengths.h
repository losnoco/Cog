//
//  SidSongLengths.h
//  sidplay
//
//  Song lengths from the High Voltage SID Collection's database.
//

#import <Foundation/Foundation.h>

#import <sidplayfp/SidTune.h>

// The collection lists how long each song of each of its tunes plays in
// DOCUMENTS/Songlengths.md5, by the MD5 of the tune's file. A tune that lies
// in the collection, in any folder below the one holding DOCUMENTS, or that
// has a Songlengths.md5 in a folder above it, plays for those lengths.
@interface SidSongLengths : NSObject

// The lengths in seconds of the songs of `tune`, which was read from `url`,
// in song order; nil if no database above `url` lists the tune.
+ (NSArray<NSNumber *> *)lengthsForTune:(SidTune *)tune url:(NSURL *)url;

// The length in seconds of song `song` (from 1) of `tune`, or 0 if unknown.
+ (double)lengthOfSong:(unsigned int)song ofTune:(SidTune *)tune url:(NSURL *)url;

// The lengths a database line gives after its "=", as "0:56 4:33.108 1:02".
+ (NSArray<NSNumber *> *)lengthsFromString:(NSString *)string;

// The lengths by MD5 that a database's contents list.
+ (NSDictionary<NSString *, NSString *> *)entriesFromData:(NSData *)data;

@end
