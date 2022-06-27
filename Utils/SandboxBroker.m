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

static NSURL *_containerDirectory = nil;
static NSURL *_defaultMusicDirectory = nil;
static NSURL *_defaultDownloadsDirectory = nil;
static NSURL *_defaultMoviesDirectory = nil;

static NSURL *containerDirectory(void) {
	NSString *path = [@"~" stringByExpandingTildeInPath];
	return [NSURL fileURLWithPath:path];
}

// XXX this is only for comparison, not "escaping the sandbox"
static NSURL *pathEscape(NSString *path) {
	NSString *componentsToRemove = [NSString stringWithFormat:@"Library/Containers/%@/Data/", [[NSBundle mainBundle] bundleIdentifier]];
	NSRange rangeOfMatch = [path rangeOfString:componentsToRemove];
	if(rangeOfMatch.location != NSNotFound)
		path = [path stringByReplacingCharactersInRange:rangeOfMatch withString:@""];
	return [NSURL fileURLWithPath:path];
}

static NSURL *defaultMusicDirectory(void) {
	NSString *path = [NSSearchPathForDirectoriesInDomains(NSMusicDirectory, NSUserDomainMask, YES) lastObject];
	return pathEscape(path);
}

static NSURL *defaultDownloadsDirectory(void) {
	NSString *path = [NSSearchPathForDirectoriesInDomains(NSDownloadsDirectory, NSUserDomainMask, YES) lastObject];
	return pathEscape(path);
}

static NSURL *defaultMoviesDirectory(void) {
	NSString *path = [NSSearchPathForDirectoriesInDomains(NSMoviesDirectory, NSUserDomainMask, YES) lastObject];
	return pathEscape(path);
}

static SandboxBroker *kSharedSandboxBroker = nil;

@interface SandboxEntry : NSObject {
	SandboxToken *_token;
	NSInteger _refCount;
	NSURL *_secureUrl;
	NSString *_path;
};

@property(readonly) SandboxToken *token;

@property NSURL *secureUrl;

@property(readonly) NSString *path;

@property NSInteger refCount;

- (id)initWithToken:(SandboxToken *)token;
- (id)initWithStaticURL:(NSURL *)url;
@end

@implementation SandboxEntry
- (id)initWithToken:(SandboxToken *)token {
	SandboxEntry *obj = [super init];
	if(obj) {
		obj->_refCount = 1;
		obj->_secureUrl = nil;
		obj->_token = token;
		obj->_path = token.path;
	}
	return obj;
}

- (id)initWithStaticURL:(NSURL *)url {
	SandboxEntry *obj = [super init];
	if(obj) {
		obj->_refCount = 1;
		obj->_secureUrl = nil;
		obj->_token = nil;
		obj->_path = [url path];
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
@end

@implementation SandboxBroker

+ (id)sharedSandboxBroker {
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		kSharedSandboxBroker = [[self alloc] init];
	});
	return kSharedSandboxBroker;
}

+ (NSPersistentContainer *)sharedPersistentContainer {
	return [NSClassFromString(@"PlaylistController") sharedPersistentContainer];
}

+ (NSURL *)urlWithoutFragment:(NSURL *)url {
	if(![url isFileURL]) return url;

	NSNumber *isDirectory;

	BOOL success = [url getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];

	if(success && [isDirectory boolValue]) return url;

	NSString *s = [url path];

	NSString *lastComponent = [url lastPathComponent];

	// Find that last component in the string from the end to make sure
	// to get the last one
	NSRange fragmentRange = [s rangeOfString:lastComponent
	                                 options:NSBackwardsSearch];

	// Chop the fragment.
	NSString *newURLString = [s substringToIndex:fragmentRange.location + fragmentRange.length];

	return [NSURL fileURLWithPath:newURLString];
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
	SandboxEntry *ret = nil;

	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		_containerDirectory = containerDirectory();
		_defaultMusicDirectory = defaultMusicDirectory();
		_defaultDownloadsDirectory = defaultDownloadsDirectory();
		_defaultMoviesDirectory = defaultMoviesDirectory();
	});

	NSArray *urls = @[_containerDirectory, _defaultMusicDirectory, _defaultDownloadsDirectory, _defaultMoviesDirectory];

	for(NSURL *checkUrl in urls) {
		if([SandboxBroker isPath:url aSubdirectoryOf:checkUrl]) {
			return [[SandboxEntry alloc] initWithStaticURL:checkUrl];
		}
	}

	NSPersistentContainer *pc = [SandboxBroker sharedPersistentContainer];

	NSSortDescriptor *sortDescriptor = [NSSortDescriptor sortDescriptorWithKey:@"path.length" ascending:NO];

	NSFetchRequest *request = [NSFetchRequest fetchRequestWithEntityName:@"SandboxToken"];
	request.sortDescriptors = @[sortDescriptor];

	NSError *error = nil;
	NSArray *results = [pc.viewContext executeFetchRequest:request error:&error];
	if(results) results = [results copy];

	if(results && [results count] > 0) {
		for(SandboxToken *token in results) {
			if(token.path && [SandboxBroker isPath:url aSubdirectoryOf:[NSURL fileURLWithPath:token.path]]) {
				SandboxEntry *entry = [[SandboxEntry alloc] initWithToken:token];

				ret = entry;
				break;
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

		ret.secureUrl = secureUrl;

		return ret;
	}

	return nil;
}

- (const void *)beginFolderAccess:(NSURL *)fileUrl {
	NSURL *folderUrl = [[SandboxBroker urlWithoutFragment:fileUrl] URLByDeletingLastPathComponent];
	if(![folderUrl isFileURL]) return NULL;

	SandboxEntry *_entry = nil;

	@synchronized(self) {
		for(SandboxEntry *entry in storage) {
			if(entry.path && [SandboxBroker isPath:folderUrl aSubdirectoryOf:[NSURL fileURLWithPath:entry.path]]) {
				entry.refCount += 1;
				_entry = entry;
				break;
			}
		}

		if(!_entry) {
			_entry = [self recursivePathTest:folderUrl];
		}
	}

	if(_entry) {
		[storage addObject:_entry];

		if(_entry.secureUrl) {
			[_entry.secureUrl startAccessingSecurityScopedResource];
		}

		return CFBridgingRetain(_entry);
	} else {
		return NULL;
	}
}

- (void)endFolderAccess:(const void *)handle {
	if(!handle) return;
	SandboxEntry *entry = CFBridgingRelease(handle);
	if(!entry) return;

	@synchronized(self) {
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
		}
	}
}

- (BOOL)areAllPathsSafe:(NSArray *)urls {
	for(NSURL *url in urls) {
		if(![self recursivePathTest:url]) {
			return NO;
		}
	}
	return YES;
}

@end
