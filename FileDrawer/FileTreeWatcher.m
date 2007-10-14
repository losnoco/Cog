//
//  FileTreeDelegate.m
//  BindTest
//
//  Created by Vincent Spader on 8/20/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "FileTreeWatcher.h"

#import "UKKQueue.h"

@implementation FileTreeWatcher

- (id)init
{
	self = [super init];
	if (self)
	{
		kqueue = [[UKKQueue alloc] init];
		[kqueue setDelegate:self];
		
		watchedPaths = [[NSMutableArray alloc] init];
	}
	
	return self;
}

- (void)dealloc
{
	[delegate release];
	[super dealloc];
}


- (void)addPath: (NSString *)path
{
	if ([watchedPaths containsObject:path] == NO) {
		[watchedPaths addObject:path];

		[kqueue addPath: path];
	}
}

- (void)removePath: (NSString *)path
{
	[watchedPaths removeObject:path];
	[kqueue removePath:path];
}

-(void) setDelegate: (id)d
{
	delegate = [d retain];
}


-(void) watcher: (id<UKFileWatcher>)kq receivedNotification: (NSString*)nm forPath: (NSString*)fpath
{
	[delegate refreshPath: fpath];
}

@end
