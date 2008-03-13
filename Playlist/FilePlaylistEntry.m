//
//  FilePlaylistEntry.m
//  Cog
//
//  Created by Vincent Spader on 3/12/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "FilePlaylistEntry.h"


@implementation FilePlaylistEntry

- (void)setURL:(NSURL *)url
{
	FSPathMakeRef((UInt8 *)[[url path] fileSystemRepresentation], &fileRef, NULL);
}

- (NSURL *)URL
{
    UInt8 path[PATH_MAX];

	OSStatus status = FSRefMakePath(&fileRef, (UInt8*)path, sizeof(path)); 
	if (status != noErr) 
		return nil;
		
    return [NSURL fileURLWithPath: [NSString stringWithUTF8String:(const char *)path]]; 
}

@end
