//
//  SilenceSource.m
//  SilenceSource
//
//  Created by Christopher Snowhill on 2/8/15.
//  Copyright 2015 __NoWork, LLC__. All rights reserved.
//

#import "SilenceSource.h"


@implementation SilenceSource

- (BOOL)open:(NSURL *)url
{
	[self setURL:url];
    
	return YES;
}

- (BOOL)seekable
{
	return YES;
}

- (BOOL)seek:(long)position whence:(int)whence
{
    return YES;
}

- (long)tell
{
    return 0;
}

- (long)read:(void *)buffer amount:(long)amount
{
    memset(buffer, 0, amount);
    return amount;
}

- (void)close
{
}

- (NSURL *)url
{
	return _url;
}

- (NSString *)mimeType
{
	return @"audio/x-silence";
}

- (void)setURL:(NSURL *)url
{
	_url = url;
}


+ (NSArray *)schemes
{
	return @[@"silence"];
}

- (void)dealloc {
	[self close];
	[self setURL:nil];
}

@end
