//
//  SHA256Digest.h
//  Cog
//
//  Created by Christopher Snowhill on 2/22/22.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface SHA256Digest : NSObject

+ (NSData *)digestBytes:(const void *)bytes length:(size_t)length;
+ (NSData *)digestData:(const NSData *)data;

+ (NSString *)digestBytesAsString:(const void *)bytes length:(size_t)length;
+ (NSString *)digestDataAsString:(const NSData *)data;

@end

NS_ASSUME_NONNULL_END
