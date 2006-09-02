//
//  FileTreeDelegate.h
//  BindTest
//
//  Created by Zaphod Beeblebrox on 8/20/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "UKKQueue/UKKQueue.h"

@interface FileTreeWatcher : NSObject {
	UKKQueue *kqueue;
	id delegate;
	
	NSMutableArray *watchedPaths;
}

@end
