//
//  FileTreeDelegate.h
//  BindTest
//
//  Created by Vincent Spader on 8/20/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class UKKQueue;

@interface FileTreeWatcher : NSObject {
	UKKQueue *kqueue;
	id delegate;
	
	NSMutableArray *watchedPaths;
}

- (void)addPath: (NSString *)path;
- (void)removePath: (NSString *)path;

-(void) setDelegate: (id)d;

@end
