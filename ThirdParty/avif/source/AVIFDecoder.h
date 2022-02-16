//
//  AVIFDecoder.h
//  AVIFQuickLook
//
//  Created by lizhuoli on 2019/4/15.
//  Copyright © 2019 dreampiggy. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface AVIFDecoder : NSObject

+ (nullable CGImageRef)createAVIFImageAtPath:(nonnull NSString *)path CF_RETURNS_RETAINED;
+ (nullable CGImageRef)createAVIFImageWithData:(nonnull NSData *)data CF_RETURNS_RETAINED;
+ (BOOL)isAVIFFormatForData:(nullable NSData *)data;

@end

NS_ASSUME_NONNULL_END
