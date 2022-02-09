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
@end

NS_ASSUME_NONNULL_END
