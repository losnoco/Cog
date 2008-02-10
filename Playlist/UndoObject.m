//
//  UndoObject.m
//  Cog
//
//  Created by Andre Reffhaug on 2/6/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "UndoObject.h"


@implementation UndoObject

-(void)setPath:(NSURL *)p
{
	[p retain];
	[path release];
	path = p;
}

-(void)setOrigin:(int)i
{
	origin = i;
}

-(void)setMovedTo:(int)i
{
	movedTo = i;
}
-(void)setOrigin:(int) i andPath:(NSURL *)p
{
	origin = i;
	[p retain];
	[path release];
	path = p;
}

-(int) origin
{
	return origin;
}

-(int) movedTo
{
	return movedTo;
}

-(NSURL *) path
{
	return path;
}

@end
