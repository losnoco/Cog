//
//  NSData+MD5.m
//  Cog
//
//  Created by Christopher Snowhill on 10/9/13.
//
//

#import <CommonCrypto/CommonDigest.h>

#import "NSData+MD5.h"

@implementation NSData (MD5)

- (NSString*)MD5
{
    // Create pointer to the string as UTF8
    const void * ptr = [self bytes];
    NSUInteger len = [self length];
    
    // Create byte array of unsigned chars
    unsigned char md5Buffer[CC_MD5_DIGEST_LENGTH];
    
    // Create 16 byte MD5 hash value, store in buffer
    CC_MD5(ptr, len, md5Buffer);
    
    // Convert MD5 value in the buffer to NSString of hex values
    NSMutableString *output = [NSMutableString stringWithCapacity:CC_MD5_DIGEST_LENGTH * 2];
    for(int i = 0; i < CC_MD5_DIGEST_LENGTH; i++)
        [output appendFormat:@"%02x",md5Buffer[i]];
    
    return output;
}

@end
