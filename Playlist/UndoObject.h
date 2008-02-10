//
//  UndoObject.h
//  Cog
//
//  Created by Andre Reffhaug on 2/6/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface UndoObject : NSObject {
	int origin;
	int movedTo;
	NSURL *path;
}

-(void)setPath:(NSURL *) p;
-(void)setOrigin:(int) i;
-(void)setMovedTo:(int) i;
-(void)setOrigin:(int) i andPath:(NSURL *)path;

-(int)origin;
-(int)movedTo;
-(NSURL *)path;

@end
