//
//  TagLibID3v2Reader.h
//  TagLib Plugin
//
//  Created by Christopher Snowhill on 2/8/22.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface TagLibID3v2Reader : NSObject
+ (NSDictionary *)metadataForTag:(NSData *)tagBlock;
// The CHAP/CTOC chapters of an ID3v2 tag block, in the order the metadata
// writer reads them from a file: the top-level ordered table of contents'
// order when it gives one, else by start. The same CogChapter dictionaries
// the writer answers, so a decoder reading the tag agrees with the editor.
+ (NSArray *)chaptersForTag:(NSData *)tagBlock;
@end

NS_ASSUME_NONNULL_END
