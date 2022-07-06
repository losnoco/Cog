//
//  SandboxPathBehaviorController.m
//  Preferences
//
//  Created by Christopher Snowhill on 6/19/22.
//

#import "PreferencePanePlugin.h"

#import "SandboxPathBehaviorController.h"

#import "PlaylistController.h"

#import <CoreData/CoreData.h>

#import "Logging.h"

#import "PlaylistController.h"

#import "SandboxBroker.h"

@interface SandboxToken : NSManagedObject
@property(nonatomic, strong) NSString *path;
@property(nonatomic, strong) NSData *bookmark;
@property(nonatomic) BOOL folder;
@end

@implementation SandboxPathBehaviorController
- (void)awakeFromNib {
	[self removeObjects:[self arrangedObjects]];

	NSSortDescriptor *sortDescriptor = [NSSortDescriptor sortDescriptorWithKey:@"path" ascending:YES];
	[self setSortDescriptors:@[sortDescriptor]];

	NSPersistentContainer *pc = [NSClassFromString(@"PlaylistController") sharedPersistentContainer];

	NSFetchRequest *request = [NSFetchRequest fetchRequestWithEntityName:@"SandboxToken"];

	NSError *error = nil;
	NSArray *results = [pc.viewContext executeFetchRequest:request error:&error];

	if(results && [results count] > 0) {
		for(SandboxToken *token in results) {
			BOOL isStale = YES;
			NSError *err = nil;
			NSURL *bookmarkUrl = [NSURL URLByResolvingBookmarkData:token.bookmark options:NSURLBookmarkResolutionWithSecurityScope relativeToURL:nil bookmarkDataIsStale:&isStale error:&err];
			[self addObject:@{ @"path": token.path, @"valid": ((!bookmarkUrl || isStale) ? NSLocalizedPrefString(@"ValidNo") : NSLocalizedPrefString(@"ValidYes")), @"isFolder": @(token.folder), @"token": token }];
		}
	}
}

- (void)addUrl:(NSURL *)url {
	NSError *err = nil;
	NSData *bookmark = [url bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope includingResourceValuesForKeys:nil relativeToURL:nil error:&err];
	if(!bookmark && err) {
		ALog(@"Failed to add bookmark for URL: %@, with error: %@", url, [err localizedDescription]);
		return;
	}

	NSPersistentContainer *pc = [NSClassFromString(@"PlaylistController") sharedPersistentContainer];

	SandboxToken *token = [NSEntityDescription insertNewObjectForEntityForName:@"SandboxToken" inManagedObjectContext:pc.viewContext];

	if(token) {
		token.path = [url path];
		token.bookmark = bookmark;
		[pc.viewContext save:&err];
		if(err) {
			ALog(@"Error saving bookmark: %@", [err localizedDescription]);
		} else {
			[self addObject:@{ @"path": [url path], @"valid": NSLocalizedPrefString(@"ValidYes") }];
		}
	}
}

- (void)removeToken:(id)token {
	NSPersistentContainer *pc = [NSClassFromString(@"PlaylistController") sharedPersistentContainer];

	NSArray *objects = [[self arrangedObjects] copy];
	
	BOOL updated = NO;

	for(NSDictionary *obj in objects) {
		if([[obj objectForKey:@"token"] isEqualTo:token]) {
			[self removeObject:obj];
			[pc.viewContext deleteObject:token];
			updated = YES;
			break;
		}
	}

	if(updated) {
		NSError *error;
		[pc.viewContext	save:&error];
		if(error) {
			ALog(@"Error deleting bookmark: %@", [error localizedDescription]);
		}
	}
}

- (void)removeStaleEntries {
	BOOL updated = NO;
	NSPersistentContainer *pc = [NSClassFromString(@"PlaylistController") sharedPersistentContainer];
	for(NSDictionary *entry in [[self arrangedObjects] copy]) {
		if([[entry objectForKey:@"valid"] isEqualToString:NSLocalizedPrefString(@"ValidNo")]) {
			[self removeObject:entry];
			[pc.viewContext deleteObject:[entry objectForKey:@"token"]];
			updated = YES;
		}
	}
	if(updated) {
		NSError *error;
		[pc.viewContext save:&error];
		if(error) {
			ALog(@"Error saving after removing stale bookmarks: %@", [error localizedDescription]);
		}
	}
}

- (BOOL)matchesPath:(NSURL *)url {
	id sandboxBrokerClass = NSClassFromString(@"SandboxBroker");
	for(NSDictionary *entry in [self arrangedObjects]) {
		if([[entry objectForKey:@"valid"] isEqualToString:NSLocalizedPrefString(@"ValidYes")]) {
			BOOL isFolder = [[entry objectForKey:@"isFolder"] boolValue];
			NSString *path = [entry objectForKey:@"path"];
			if(path && [path length]) {
				NSURL *testPath = [NSURL fileURLWithPath:[entry objectForKey:@"path"]];
				if((isFolder && [sandboxBrokerClass isPath:url aSubdirectoryOf:testPath]) ||
				   (!isFolder && [[url path] isEqualToString:path])) {
					return YES;
				}
			}
		}
	}
	return NO;
}

@end
