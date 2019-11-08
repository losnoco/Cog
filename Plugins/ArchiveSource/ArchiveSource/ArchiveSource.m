//
//  ArchiveSource.m
//  ArchiveSource
//
//  Created by Christopher Snowhill on 10/4/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "ArchiveSource.h"

#import "Logging.h"

static NSString * path_unpack_string(NSString * src, NSRange * remainder)
{
    NSRange bar = [src rangeOfString:@"|"];
    if (bar.location != 0 || bar.length != 1)
        return nil;
    NSRange next = {
        .location = 1,
        .length = [src length] - 1
    };
    bar = [src rangeOfString:@"|" options:0 range:next];
    if (bar.length != 1)
        return nil;
    NSRange lengthRange = {
        .location = 1,
        .length = bar.location - 1
    };
    NSUInteger length = [[src substringWithRange:lengthRange] integerValue];
    if (length >= ([src length] - bar.location - 1))
        return nil;
    NSRange pathRange = {
        .location = bar.location + 1,
        .length = length
    };
    NSString * ret = [src substringWithRange:pathRange];
    remainder->location = bar.location + length + 1;
    remainder->length = [src length] - remainder->location;
    return ret;
}

static BOOL g_parse_unpack_path(NSString * src, NSString ** archive, NSString ** file, NSString ** type)
{
    NSRange typeRange;
    NSRange range;
    range = [src rangeOfString:@"|"];
    if (range.length != 1)
        return NO;
    range.length = [src length] - range.location;
    typeRange.location = 9;
    typeRange.length = range.location - 9;
    *type = [src substringWithRange:typeRange];
    NSString * url = [src substringWithRange:range];
    *archive = path_unpack_string(url, &range);
    if (!*archive)
        return NO;
    range.location++;
    range.length--;
    *file = [url substringWithRange:range];
    return YES;
}

@implementation ArchiveSource

- (id)init
{
    self = [super init];
    if ( self ) {
        fex = NULL;
    }
    return self;
}

- (BOOL)open:(NSURL *)url
{
	[self setURL:url];
	
    NSString * urlDecoded = [[url absoluteString] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
    
    NSString * type;
	NSString * archive;
    NSString * file;
    
    if (!g_parse_unpack_path(urlDecoded, &archive, &file, &type))
        return NO;
    
    if (![type isEqualToString:@"fex"])
        return NO;
    
    fex_err_t error;
    
    error = fex_open( &fex, [archive UTF8String] );
    if ( error ) {
        ALog(@"Error opening archive: %s", error);
        return NO;
    }
    
    while ( !fex_done( fex ) ) {
        if ( [file isEqualToString:[NSString stringWithUTF8String:fex_name( fex )]] )
            break;
        fex_next( fex );
    }
    
    if ( fex_done( fex ) )
        return NO;
    
    error = fex_data( fex, &data );
    if ( error ) {
        ALog(@"Error unpacking file from archive: %s", error);
        return NO;
    }
    
    offset = 0;
    size = fex_size( fex );
	
	return YES;
}

- (BOOL)seekable
{
	return YES;
}

- (BOOL)seek:(long)position whence:(int)whence
{
    switch (whence)
    {
        case SEEK_CUR:
            position += offset;
            break;
            
        case SEEK_END:
            position += size;
            break;
    }
    
    offset = position;
    
    return (offset <= size);
}

- (long)tell
{
	return offset;
}

- (long)read:(void *)buffer amount:(long)amount
{
    if ( offset >= size )
        return 0;
	if ( size - offset < amount )
        amount = size - offset;
    memcpy( buffer, (const uint8_t *)data + offset, amount );
    offset += amount;
    return amount;
}

- (void)close
{
    if ( fex ) {
        fex_close( fex );
        fex = NULL;
    }
}

- (NSURL *)url
{
	return _url;
}

- (NSString *)mimeType
{
	return nil;
}

- (void)setURL:(NSURL *)url
{
	_url = url;
}


+ (NSArray *)schemes
{
	return [NSArray arrayWithObject:@"unpack"];
}

- (void)dealloc {
	[self close];
}

@end
