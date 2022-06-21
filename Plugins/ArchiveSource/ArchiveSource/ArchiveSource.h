//
//  ArchiveSource.h
//  ArchiveSource
//
//  Created by Christopher Snowhill on 10/4/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <File_Extractor/fex.h>

#import "Plugin.h"

@interface ArchiveSource : NSObject <CogSource> {
	fex_t *fex;

	const void *data;
	NSUInteger offset;
	NSUInteger size;

	NSURL *_url;

	const void *sbHandle;
}

@end
