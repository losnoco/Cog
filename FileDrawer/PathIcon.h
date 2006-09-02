//
//  FileIcon.h
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/20/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface PathIcon : NSObject {
	NSString *path;
	NSImage *icon;
}

-(id)initWithPath:(NSString *)p;

@end
