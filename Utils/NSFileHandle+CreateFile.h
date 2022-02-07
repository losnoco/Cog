//
//  NSFileHandle+CreateFile.h
//  Cog
//
//  Created by Zaphod Beeblebrox on 3/13/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface NSFileHandle (CreateFile)

+ (id)fileHandleForWritingAtPath:(NSString *)path createFile:(BOOL)create;

@end
