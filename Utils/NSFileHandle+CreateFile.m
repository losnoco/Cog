//
//  NSFileHandle+CreateFile.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 3/13/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "NSFileHandle+CreateFile.h"


@implementation NSFileHandle (CreateFile)

+ (id)fileHandleForWritingAtPath:(NSString *)path createFile:(BOOL)create
{
	NSFileManager *manager = [NSFileManager defaultManager];

	if (![manager fileExistsAtPath:path])
		[manager createFileAtPath:path contents:nil attributes:nil];
	
	return [NSFileHandle fileHandleForWritingAtPath:path];
}

@end
