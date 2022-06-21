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
			if(!bookmarkUrl) {
				ALog(@"Stale bookmark for path: %@, with error: %@", token.path, [err localizedDescription]);
				continue;
			}
			[self addObject:@{ @"path": token.path, @"valid": (isStale ? NSLocalizedPrefString(@"ValidNo") : NSLocalizedPrefString(@"ValidYes")) }];
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

- (void)removePath:(NSString *)path {
	NSPersistentContainer *pc = [NSClassFromString(@"PlaylistController") sharedPersistentContainer];

	NSPredicate *predicate = [NSPredicate predicateWithFormat:@"path == %@", path];

	NSFetchRequest *request = [NSFetchRequest fetchRequestWithEntityName:@"SandboxToken"];
	request.predicate = predicate;

	NSError *error = nil;
	NSArray *results = [pc.viewContext executeFetchRequest:request error:&error];

	if(results && [results count] > 0) {
		for(SandboxToken *token in results) {
			[pc.viewContext deleteObject:token];
		}
	}

	NSArray *objects = [self arrangedObjects];

	for(NSDictionary *obj in objects) {
		if([[obj objectForKey:@"path"] isEqualToString:path]) {
			[self removeObject:obj];
			break;
		}
	}
}

- (void)removeStaleEntries {
	for(NSDictionary *entry in [[self arrangedObjects] copy]) {
		if([[entry objectForKey:@"valid"] isEqualToString:NSLocalizedPrefString(@"ValidNo")]) {
			[self removeObject:entry];
		}
	}
}

- (BOOL)matchesPath:(NSURL *)url {
	id sandboxBrokerClass = NSClassFromString(@"SandboxBroker");
	for(NSDictionary *entry in [self arrangedObjects]) {
		NSURL *testPath = [NSURL fileURLWithPath:[entry objectForKey:@"path"]];
		if([sandboxBrokerClass isPath:url aSubdirectoryOf:testPath])
			return YES;
	}
	return NO;
}

@end
