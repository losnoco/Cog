//
//  SHA256Digest.m
//  Cog
//
//  Created by Christopher Snowhill on 2/22/22.
//

#import "SHA256Digest.h"

#import <CommonCrypto/CommonDigest.h>

@implementation SHA256Digest

+ (NSData *)digestBytes:(const void *)bytes length:(size_t)length {
	uint8_t result[CC_SHA256_DIGEST_LENGTH];
	CC_SHA256_CTX ctx;
	CC_SHA256_Init(&ctx);
	CC_SHA256_Update(&ctx, bytes, (CC_LONG)length);
	CC_SHA256_Final(&result[0], &ctx);
	return [NSData dataWithBytes:&result[0] length:sizeof(result)];
}

+ (NSData *)digestData:(const NSData *)data {
	return [SHA256Digest digestBytes:[data bytes] length:[data length]];
}

+ (NSString *)digestBytesAsString:(const void *)bytes length:(size_t)length {
	NSData *hashData = [SHA256Digest digestBytes:bytes length:length];
	length = [hashData length];
	NSMutableString *result = [NSMutableString stringWithCapacity:length * 2];
	const uint8_t *values = (const uint8_t *)[hashData bytes];
	for(size_t i = 0; i < length; ++i) {
		[result appendFormat:@"%02x", values[i]];
	}
	return [NSString stringWithString:result];
}

+ (NSString *)digestDataAsString:(const NSData *)data {
	return [SHA256Digest digestBytesAsString:[data bytes] length:[data length]];
}

@end
