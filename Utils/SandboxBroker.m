//
//  SandboxBroker.m
//  Cog
//
//  Created by Christopher Snowhill on 6/20/22.
//

#import <Foundation/Foundation.h>

#import <Cocoa/Cocoa.h>

#import "SandboxBroker.h"

#import "Logging.h"

#import "Cog-Swift.h"

#import "PlaylistController.h"

static SandboxBroker *__sharedSandboxBroker = nil;

@interface NSApplication (SandboxBrokerExtension)
- (SandboxBroker *)sharedSandboxBroker;
@end

@implementation NSApplication (SandboxBrokerExtension)
- (SandboxBroker *)sharedSandboxBroker {
	return __sharedSandboxBroker;
}
@end

static NSURL *urlWithoutFragment(NSURL *u) {
	NSString *s = [u path];

	NSString *lastComponent = [u lastPathComponent];

	// Find that last component in the string from the end to make sure
	// to get the last one
	NSRange fragmentRange = [s rangeOfString:lastComponent
	                                 options:NSBackwardsSearch];

	// Chop the fragment.
	NSString *newURLString = [s substringToIndex:fragmentRange.location + fragmentRange.length];

	return [NSURL fileURLWithPath:newURLString];
}

@interface SandboxEntry : NSObject {
	SandboxToken *_token;
	NSInteger _refCount;
	NSURL *_secureUrl;
};

@property(readonly) SandboxToken *token;

@property NSURL *secureUrl;

@property(readonly) NSString *path;

@property NSInteger refCount;

- (id)initWithToken:(SandboxToken *)token;
@end

@implementation SandboxEntry
- (id)initWithToken:(SandboxToken *)token {
	SandboxEntry *obj = [super init];
	if(obj) {
		obj->_refCount = 1;
		obj->_secureUrl = nil;
		obj->_token = token;
	}
	return obj;
}

- (NSInteger)refCount {
	return _refCount;
}

- (void)setRefCount:(NSInteger)refCount {
	_refCount = refCount;
}

- (NSURL *)secureUrl {
	return _secureUrl;
}

- (void)setSecureUrl:(NSURL *)url {
	_secureUrl = url;
}

- (SandboxToken *)token {
	return _token;
}

- (NSString *)path {
	return _token.path;
}
@end

@implementation SandboxBroker

+ (id)sharedSandboxBroker {
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		__sharedSandboxBroker = [[self alloc] init];
	});
	return [NSApp sharedSandboxBroker];
}

- (id)init {
	id _self = [super init];
	if(_self) {
		storage = [[NSMutableArray alloc] init];
	}

	return _self;
}

- (void)shutdown {
	for(SandboxEntry *obj in storage) {
		if([obj secureUrl]) {
			[[obj secureUrl] stopAccessingSecurityScopedResource];
		}
	}
}

- (void)recursivePathTest:(NSURL *)url removing:(BOOL)removing {
	NSArray *pathComponents = [url pathComponents];

	NSSortDescriptor *sortDescriptor = [NSSortDescriptor sortDescriptorWithKey:@"path" ascending:NO];

	for(size_t i = [pathComponents count]; i > 0; --i) {
		NSArray *partialComponents = [pathComponents subarrayWithRange:NSMakeRange(0, i)];
		NSURL *partialUrl = [NSURL fileURLWithPathComponents:partialComponents];
		NSString *matchString = [[partialUrl path] stringByAppendingString:@"*"];

		NSPredicate *predicate = [NSPredicate predicateWithFormat:@"path like %@", matchString];

		NSArray *matchingObjects = [storage filteredArrayUsingPredicate:predicate];

		if(matchingObjects && [matchingObjects count] > 0) {
			if([matchingObjects count] > 1) {
				matchingObjects = [matchingObjects sortedArrayUsingDescriptors:@[sortDescriptor]];
			}

			for(SandboxEntry *entry in matchingObjects) {
				if([entry.path isEqualToString:[partialUrl path]]) {
					if(!removing) {
						entry.refCount += 1;
						return;
					} else {
						if(entry.refCount > 1) {
							entry.refCount -= 1;
							return;
						} else {
							if(entry.secureUrl) {
								[entry.secureUrl stopAccessingSecurityScopedResource];
								entry.secureUrl = nil;
							}
							entry.refCount = 0;

							[storage removeObject:entry];
							return;
						}
					}
				}
			}
		}
	}

	if(removing) return;

	NSPersistentContainer *pc = [NSApp sharedPersistentContainer];

	for(size_t i = [pathComponents count]; i > 0; --i) {
		NSArray *partialComponents = [pathComponents subarrayWithRange:NSMakeRange(0, i)];
		NSURL *partialUrl = [NSURL fileURLWithPathComponents:partialComponents];
		NSString *matchString = [[partialUrl path] stringByAppendingString:@"*"];

		NSPredicate *predicate = [NSPredicate predicateWithFormat:@"path like %@", matchString];

		NSFetchRequest *request = [NSFetchRequest fetchRequestWithEntityName:@"SandboxToken"];
		request.predicate = predicate;
		request.sortDescriptors = @[sortDescriptor];

		NSError *error = nil;
		NSArray *results = [pc.viewContext executeFetchRequest:request error:&error];

		if(results && [results count] > 0) {
			for(SandboxToken *token in results) {
				if([token.path isEqualToString:[partialUrl path]]) {
					SandboxEntry *entry = [[SandboxEntry alloc] initWithToken:token];

					[storage addObject:entry];

					BOOL isStale;
					NSError *err = nil;
					NSURL *secureUrl = [NSURL URLByResolvingBookmarkData:token.bookmark options:NSURLBookmarkResolutionWithSecurityScope relativeToURL:nil bookmarkDataIsStale:&isStale error:&err];
					if(!secureUrl && err) {
						ALog(@"Failed to access bookmark for URL: %@, error: %@", token.path, [err localizedDescription]);
						return;
					}

					entry.secureUrl = secureUrl;

					[secureUrl startAccessingSecurityScopedResource];

					return;
				}
			}
		}
	}

	return;
}

- (void)beginFolderAccess:(NSURL *)fileUrl {
	NSURL *folderUrl = [urlWithoutFragment(fileUrl) URLByDeletingLastPathComponent];
	if(![folderUrl isFileURL]) return;
	if(![NSApp respondsToSelector:@selector(sharedPersistentContainer)]) return;

	@synchronized(self) {
		[self recursivePathTest:folderUrl removing:NO];
	}
}

- (void)endFolderAccess:(NSURL *)fileUrl {
	NSURL *folderUrl = [urlWithoutFragment(fileUrl) URLByDeletingLastPathComponent];
	if(![folderUrl isFileURL]) return;

	@synchronized(self) {
		[self recursivePathTest:folderUrl removing:YES];
	}
}

@end
