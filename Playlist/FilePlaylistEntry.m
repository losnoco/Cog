//
//  FilePlaylistEntry.m
//  Cog
//
//  Created by Vincent Spader on 3/12/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "FilePlaylistEntry.h"


@implementation FilePlaylistEntry

@synthesize fragment;

- (void)setURL:(NSURL *)url
{
	FSPathMakeRef((UInt8 *)[[url path] fileSystemRepresentation], &fileRef, NULL);
	self.fragment = [url fragment];
}

- (NSURL *)URL
{
    UInt8 path[PATH_MAX];

	OSStatus status = FSRefMakePath(&fileRef, (UInt8*)path, sizeof(path)); 
	if (status != noErr) 
		return nil;
		
	NSString *after = @"";
	if (self.fragment != nil) {
		after = [@"#" stringByAppendingString:self.fragment];
	}
    return [NSURL URLWithString:[[[NSURL fileURLWithPath: [NSString stringWithUTF8String:(const char *)path]] absoluteString] stringByAppendingString:after]]; 
}

@end
