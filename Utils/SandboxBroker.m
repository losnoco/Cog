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

static SandboxBroker *kSharedSandboxBroker = nil;

@interface SandboxEntry : NSObject {
	SandboxToken *_token;
	NSInteger _refCount;
	NSURL *_secureUrl;
	NSString *_path;
	BOOL _isFolder;
};

@property(readonly) SandboxToken *token;

@property NSURL *secureUrl;

@property(readonly) NSString *path;

@property NSInteger refCount;

@property(readonly) BOOL isFolder;

- (id)initWithToken:(SandboxToken *)token;
@end

@implementation SandboxEntry
- (id)initWithToken:(SandboxToken *)token {
	SandboxEntry *obj = [super init];
	if(obj) {
		obj->_refCount = 1;
		obj->_secureUrl = nil;
		obj->_token = token;
		obj->_path = token.path;
		obj->_isFolder = token.folder;
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
	return _path;
}

- (BOOL)isFolder {
	return _isFolder;
}
@end

@implementation SandboxBroker

+ (id)sharedSandboxBroker {
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		kSharedSandboxBroker = [self new];
	});
	return kSharedSandboxBroker;
}

+ (NSPersistentContainer *)sharedPersistentContainer {
	return [NSClassFromString(@"PlaylistController") sharedPersistentContainer];
}

+ (NSURL *_Nullable)urlWithoutFragment:(NSURL *)url {
	if(![url isFileURL]) return url;

	NSString *s = [url path];
	if(!s) return nil; // Cool, the resource no longer exists!

	NSRange fragmentRange = [s rangeOfString:@"#"
									 options:NSBackwardsSearch];

	if(fragmentRange.location != NSNotFound) {
		// Chop the fragment.
		NSString *newURLString = [s substringToIndex:fragmentRange.location];

		return [NSURL fileURLWithPath:newURLString];
	} else {
		return url;
	}
}

- (id)init {
	self = [super init];
	if(self) {
		storage = [NSMutableArray new];
	}

	return self;
}

- (void)shutdown {
	for(SandboxEntry *obj in storage) {
		if([obj secureUrl]) {
			[[obj secureUrl] stopAccessingSecurityScopedResource];
		}
	}
}

+ (BOOL)isPath:(NSURL *)path aSubdirectoryOf:(NSURL *)directory {
	NSArray *pathComponents = [path pathComponents];
	NSArray *directoryComponents = [directory pathComponents];

	if([pathComponents count] < [directoryComponents count])
		return NO;

	for(size_t i = 0; i < [directoryComponents count]; ++i) {
		if(![pathComponents[i] isEqualToString:directoryComponents[i]])
			return NO;
	}

	return YES;
}

- (SandboxEntry *)recursivePathTest:(NSURL *)url {
	__block SandboxEntry *ret = nil;

	NSPersistentContainer *pc = [SandboxBroker sharedPersistentContainer];
	
	NSPredicate *folderPredicate = [NSPredicate predicateWithFormat:@"folder == NO"];
	NSPredicate *filePredicate = [NSPredicate predicateWithFormat:@"path == %@", [url path]];

	[pc.viewContext performBlockAndWait:^{
		NSPredicate *predicate = [NSCompoundPredicate andPredicateWithSubpredicates:@[folderPredicate, filePredicate]];

		NSFetchRequest *request = [NSFetchRequest fetchRequestWithEntityName:@"SandboxToken"];
		request.predicate = predicate;

		NSError *error = nil;
		NSArray *results = [pc.viewContext executeFetchRequest:request error:&error];
		if(results && [results count] > 0) {
			ret = [[SandboxEntry alloc] initWithToken:results[0]];
		}

		if(!ret) {
			predicate = [NSPredicate predicateWithFormat:@"folder == YES"];

			NSSortDescriptor *sortDescriptor = [NSSortDescriptor sortDescriptorWithKey:@"path.length" ascending:NO];

			request = [NSFetchRequest fetchRequestWithEntityName:@"SandboxToken"];
			request.sortDescriptors = @[sortDescriptor];
			request.predicate = predicate;

			error = nil;
			results = [pc.viewContext executeFetchRequest:request error:&error];

			if(results && [results count] > 0) {
				for(SandboxToken *token in results) {
					if(token.path && [SandboxBroker isPath:url aSubdirectoryOf:[NSURL fileURLWithPath:token.path]]) {
						SandboxEntry *entry = [[SandboxEntry alloc] initWithToken:token];

						ret = entry;
						break;
					}
				}
			}
		}
	}];

	if(ret) {
		NSDictionary *dict = [NSURL resourceValuesForKeys:@[NSURLVolumeURLKey] fromBookmarkData:ret.token.bookmark];
		if(dict) {
			NSURL *volumeUrl = dict[NSURLVolumeURLKey];
			if(volumeUrl && [volumeUrl isFileURL] && [volumeUrl path]) {
				if(![[NSFileManager defaultManager] fileExistsAtPath:[volumeUrl path]]) {
					return nil;
				}
			}
		}
	}

	if(ret) {
		BOOL isStale;
		NSError *err = nil;
		NSURL *secureUrl = [NSURL URLByResolvingBookmarkData:ret.token.bookmark options:NSURLBookmarkResolutionWithSecurityScope relativeToURL:nil bookmarkDataIsStale:&isStale error:&err];
		if(!secureUrl && err) {
			ALog(@"Failed to access bookmark for URL: %@, error: %@", ret.token.path, [err localizedDescription]);
			return nil;
		}

		[pc.viewContext performBlockAndWait:^{
			ret.secureUrl = secureUrl;
		}];

		return ret;
	}

	return nil;
}

static inline void dispatch_sync_reentrant(dispatch_queue_t queue, dispatch_block_t block) {
	if(dispatch_queue_get_label(queue) == dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL)) {
		block();
	} else {
		dispatch_sync(queue, block);
	}
}

static inline void dispatch_async_reentrant(dispatch_queue_t queue, dispatch_block_t block) {
	if(dispatch_queue_get_label(queue) == dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL)) {
		block();
	} else {
		dispatch_async(queue, block);
	}
}

- (void)addFolderIfMissing:(NSURL *)folderUrl {
	if(![folderUrl isFileURL]) return;

	dispatch_async_reentrant(dispatch_get_main_queue(), ^{
		SandboxEntry *_entry = nil;

		for(SandboxEntry *entry in self->storage) {
			if(entry.path && entry.isFolder && [SandboxBroker isPath:folderUrl aSubdirectoryOf:[NSURL fileURLWithPath:entry.path]]) {
				_entry = entry;
				break;
			}
		}

		if(!_entry) {
			_entry = [self recursivePathTest:folderUrl];
		}

		if(!_entry) {
			NSError *err = nil;
			NSData *bookmark = [folderUrl bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope includingResourceValuesForKeys:nil relativeToURL:nil error:&err];
			if(!bookmark && err) {
				ALog(@"Failed to add bookmark for URL: %@, with error: %@", folderUrl, [err localizedDescription]);
				return;
			}

			NSPersistentContainer *pc = [SandboxBroker sharedPersistentContainer];

			[pc.viewContext performBlockAndWait:^{
				SandboxToken *token = [NSEntityDescription insertNewObjectForEntityForName:@"SandboxToken" inManagedObjectContext:pc.viewContext];

				if(token) {
					token.path = [folderUrl path];
					token.bookmark = bookmark;
					[SandboxBroker cleanupFolderAccess];
				}
			}];
		}
	});
}

- (void)addFileIfMissing:(NSURL *)fileUrl {
	if(![fileUrl isFileURL]) return;

	NSURL *url = [SandboxBroker urlWithoutFragment:fileUrl];
	if(!url) return;

	dispatch_async_reentrant(dispatch_get_main_queue(), ^{
		SandboxEntry *_entry = nil;

		for(SandboxEntry *entry in self->storage) {
			if(entry.path) {
				if((entry.isFolder && [SandboxBroker isPath:url aSubdirectoryOf:[NSURL fileURLWithPath:entry.path]]) ||
				   (!entry.isFolder && [url isEqualTo:[NSURL fileURLWithPath:entry.path]])) {
					_entry = entry;
					break;
				}
			}
		}

		if(!_entry) {
			_entry = [self recursivePathTest:url];
		}

		if(!_entry) {
			NSError *err = nil;
			NSData *bookmark = [fileUrl bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope includingResourceValuesForKeys:nil relativeToURL:nil error:&err];
			if(!bookmark && err) {
				ALog(@"Failed to add bookmark for URL: %@, with error: %@", url, [err localizedDescription]);
				return;
			}

			NSPersistentContainer *pc = [SandboxBroker sharedPersistentContainer];

			[pc.viewContext performBlockAndWait:^{
				SandboxToken *token = [NSEntityDescription insertNewObjectForEntityForName:@"SandboxToken" inManagedObjectContext:pc.viewContext];

				if(token) {
					token.path = [url path];
					token.bookmark = bookmark;
					token.folder = NO;
				}
			}];
		}
	});
}

- (void)requestFolderForFile:(NSURL *)fileUrl {
	if(![fileUrl isFileURL]) return;
	NSURL *folderUrl = [fileUrl URLByDeletingLastPathComponent];

	dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
		SandboxEntry *_entry = nil;

		for(SandboxEntry *entry in self->storage) {
			if(entry.path && entry.isFolder && [SandboxBroker isPath:folderUrl aSubdirectoryOf:[NSURL fileURLWithPath:entry.path]]) {
				_entry = entry;
				break;
			}
		}

		if(!_entry) {
			_entry = [self recursivePathTest:folderUrl];
		}

		if(!_entry) {
			static BOOL warnedYet = NO;

			if(!warnedYet) {
				NSAlert *alert = [NSAlert new];
				[alert setMessageText:NSLocalizedString(@"GrantPathTitle", @"Title of file dialog for granting folder access")];
				[alert setInformativeText:NSLocalizedString(@"GrantPathMessage", @"Message to new users regarding file permissions")];
				[alert addButtonWithTitle:NSLocalizedString(@"GrantPathOK", @"OK button text")];
				[alert addButtonWithTitle:NSLocalizedString(@"GrantPathStopWarning", @"Button to stop warnings for session")];
				
				if([alert runModal] == NSAlertSecondButtonReturn) {
					warnedYet = YES;
				}
			}

			NSOpenPanel *panel = [NSOpenPanel openPanel];
			[panel setAllowsMultipleSelection:NO];
			[panel setCanChooseDirectories:YES];
			[panel setCanChooseFiles:NO];
			[panel setFloatingPanel:YES];
			[panel setDirectoryURL:folderUrl];
			[panel setTitle:NSLocalizedString(@"GrantPathTitle", @"Title of file dialog for granting folder access")];
			NSInteger result = [panel runModal];
			if(result == NSModalResponseOK) {
				NSURL *folderUrl = [panel URL];
				NSError *err = nil;
				NSData *bookmark = [folderUrl bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope includingResourceValuesForKeys:nil relativeToURL:nil error:&err];
				if(!bookmark && err) {
					ALog(@"Failed to add bookmark for URL: %@, with error: %@", folderUrl, [err localizedDescription]);
					return;
				}

				NSPersistentContainer *pc = [SandboxBroker sharedPersistentContainer];

				SandboxToken *token = [NSEntityDescription insertNewObjectForEntityForName:@"SandboxToken" inManagedObjectContext:pc.viewContext];

				if(token) {
					token.path = [folderUrl path];
					token.bookmark = bookmark;
					[SandboxBroker cleanupFolderAccess];
				}
			}
		}
	});
}

+ (void)cleanupFolderAccess {
	NSPersistentContainer *pc = [SandboxBroker sharedPersistentContainer];

	NSSortDescriptor *sortDescriptor = [NSSortDescriptor sortDescriptorWithKey:@"path.length" ascending:YES];
	NSFetchRequest *request = [NSFetchRequest fetchRequestWithEntityName:@"SandboxToken"];
	request.sortDescriptors = @[sortDescriptor];

	[pc.viewContext performBlock:^{
		NSError *error = nil;
		NSArray *results = [pc.viewContext executeFetchRequest:request error:&error];
		NSMutableArray *resultsCopy = nil;
		if(results) {
			resultsCopy = [results mutableCopy];
		}

		BOOL isUpdated = NO;

		if(resultsCopy && [resultsCopy count]) {
			for(NSUInteger i = 0; i < [resultsCopy count] - 1; ++i) {
				SandboxToken *token = resultsCopy[i];
				NSURL *url = [NSURL fileURLWithPath:token.path];
				for(NSUInteger j = i + 1; j < [resultsCopy count];) {
					SandboxToken *compareToken = resultsCopy[j];
					if([SandboxBroker isPath:[NSURL fileURLWithPath:compareToken.path] aSubdirectoryOf:url]) {
						[pc.viewContext deleteObject:compareToken];
						isUpdated = YES;
						[resultsCopy removeObjectAtIndex:j];
					} else {
						++j;
					}
				}
			}
		}

		if(isUpdated) {
			NSError *error;
			[pc.viewContext save:&error];
			if(error) {
				ALog(@"Error saving data: %@", [error localizedDescription]);
			}
		}
	}];
}

- (const void *_Nullable)beginFolderAccess:(NSURL *)fileUrl {
	if(!fileUrl) return NULL;
	NSURL *folderUrl = [SandboxBroker urlWithoutFragment:fileUrl];
	if(!folderUrl || ![folderUrl isFileURL]) return NULL;

	__block SandboxEntry *_entry = nil;

	NSString *sandboxPath = [folderUrl path];

	dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
		for(SandboxEntry *entry in self->storage) {
			if(entry.path) {
				if((entry.isFolder && [SandboxBroker isPath:folderUrl aSubdirectoryOf:[NSURL fileURLWithPath:entry.path]]) ||
				   (!entry.isFolder && [entry.path isEqualToString:sandboxPath])) {
					entry.refCount += 1;
					_entry = entry;
					break;
				}
			}
		}

		if(!_entry) {
			_entry = [self recursivePathTest:folderUrl];
			if(_entry)
				[self->storage addObject:_entry];
		}

		if(_entry && _entry.secureUrl) {
			[_entry.secureUrl startAccessingSecurityScopedResource];
		}
	});

	if(_entry)
		return CFBridgingRetain(_entry);
	else
		return NULL;
}

- (void)endFolderAccess:(const void *_Nullable)handle {
	if(!handle) return;
	SandboxEntry *entry = CFBridgingRelease(handle);
	if(!entry) return;

	dispatch_async_reentrant(dispatch_get_main_queue(), ^{
		if(entry.refCount > 1) {
			entry.refCount -= 1;
		} else {
			if(entry.secureUrl) {
				[entry.secureUrl stopAccessingSecurityScopedResource];
				entry.secureUrl = nil;
			}
			entry.refCount = 0;

			[self->storage removeObject:entry];
		}
	});
}

- (BOOL)areAllPathsSafe:(NSArray *)urls {
	for(NSURL *url in urls) {
		if(![url isFileURL]) continue;
		if(![self recursivePathTest:url]) {
			return NO;
		}
	}
	return YES;
}

@end
