//
//  SandboxBroker.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/27/21.
//

#import <Foundation/Foundation.h>

#import <Cocoa/Cocoa.h>

#import "SandboxBroker.h"
#import "Logging.h"

static NSURL * urlWithoutFragment(NSURL * u) {
    NSString * s = [u path];
    
    NSString* lastComponent = [u lastPathComponent];

    // Find that last component in the string from the end to make sure
    // to get the last one
    NSRange fragmentRange = [s rangeOfString:lastComponent
                                     options:NSBackwardsSearch];

    // Chop the fragment.
    NSString* newURLString = [s substringToIndex:fragmentRange.location + fragmentRange.length];
    
    return [NSURL fileURLWithPath:newURLString];
}

@interface SandboxEntry : NSObject <NSSecureCoding> {
    NSInteger _pathRefCount;
    NSInteger _secRefCount;
    NSData *_bookmark;
    NSURL *_secureUrl;
};

@property(class, readonly) BOOL supportsSecureCoding;

@property(readonly) NSData * bookmark;

@property NSURL * secureUrl;

@property NSInteger pathRefCount;
@property NSInteger secRefCount;

- (id) initWithCoder:(NSCoder *)coder;
- (void) encodeWithCoder:(NSCoder *)coder;

- (id) initWithBookmark:(NSData *)bookmark;
@end

@implementation SandboxEntry
- (id) initWithCoder:(NSCoder *)coder {
    SandboxEntry * obj = [super init];
    if (obj) {
        obj->_pathRefCount = [coder decodeIntegerForKey:@"refCount"];
        obj->_bookmark = [coder decodeObjectForKey:@"bookmark"];
        obj->_secRefCount = 0;
        obj->_secureUrl = nil;
    }
    return obj;
}

- (void) encodeWithCoder:(NSCoder *)coder {
    [coder encodeInteger:_pathRefCount forKey:@"refCount"];
    [coder encodeObject:_bookmark forKey:@"bookmark"];
}

- (id) initWithBookmark:(NSData *)bookmark {
    SandboxEntry * obj = [super init];
    if (obj) {
        obj->_pathRefCount = 1;
        obj->_secRefCount = 0;
        obj->_secureUrl = nil;
        obj->_bookmark = bookmark;
    }
    return obj;
}

+ (BOOL) supportsSecureCoding {
    return YES;
}

- (NSInteger) pathRefCount {
    return _pathRefCount;
}

- (void) setPathRefCount:(NSInteger)pathRefCount {
    _pathRefCount = pathRefCount;
}

- (NSInteger) secRefCount {
    return _secRefCount;
}

- (void) setSecRefCount:(NSInteger)secRefCount {
    _secRefCount = secRefCount;
}

- (NSURL *) secureUrl {
    return _secureUrl;
}

- (void) setSecureUrl:(NSURL *)url {
    _secureUrl = url;
}

- (NSData *) bookmark {
    return _bookmark;
}
@end

@implementation SandboxBroker

+ (id) sharedSandboxBroker {
    static SandboxBroker *theSharedSandboxBroker = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        theSharedSandboxBroker = [[self alloc] init];
    });
    return theSharedSandboxBroker;
}

- (id) init {
    id _self = [super init];
    if (_self) {
        NSData *archiveData = [[NSUserDefaults standardUserDefaults] valueForKey:@"fileBookmarks"];
        
        if (archiveData) {
            NSError * err = nil;
            
            // It's an NSDictionary, containing SandboxEntry objects, referenced by NSURL keys
            NSSet * classes = [NSSet setWithObjects:[NSDictionary class], [SandboxEntry class], [NSURL class], nil];

            NSDictionary * _storage = [NSKeyedUnarchiver unarchivedObjectOfClasses:classes fromData:archiveData error:&err];
        
            if (err) {
                ALog("bookmark archive error - %@", err);
            }
            
            if (_storage) {
                storage = [_storage mutableCopy];
                return _self;
            }
        }
        
        storage = [[NSMutableDictionary alloc] init];
    }
    
    return _self;
}

- (void) shutdown {
    NSError * err = nil;
    
    NSArray * allKeys = [[storage allKeys] copy];

    for (NSURL * folderUrl in allKeys) {
        SandboxEntry * obj = [storage objectForKey:folderUrl];
        if ([obj secureUrl]) {
            [[obj secureUrl] stopAccessingSecurityScopedResource];
        }
    }
    
    NSData * archiveData = [NSKeyedArchiver archivedDataWithRootObject:storage requiringSecureCoding:YES error:&err];
    
    if (err) {
        ALog("error archiving bookmarks - %@", err);
    } else {
        [[NSUserDefaults standardUserDefaults] setObject:archiveData forKey:@"fileBookmarks"];
    }
}

- (void) addBookmarkToDictionary:(NSURL *)fileUrl {
    __block NSURL * folderUrl = [urlWithoutFragment(fileUrl) URLByDeletingLastPathComponent];
    
    @synchronized(self) {
        __block SandboxEntry * obj = [storage objectForKey:folderUrl];
        if (!obj) {
            NSOpenPanel *p;
            
            p = [NSOpenPanel openPanel];
            
            [p setCanChooseFiles:NO];
            [p setCanChooseDirectories:YES];
            [p setAllowsMultipleSelection:YES];
            [p setResolvesAliases:YES];
            
            [p setDirectoryURL:folderUrl];
            [p setTitle:@"Press OK to grant access to this folder"];
            
            errorState = nil;
            
            [p beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) {
                if ( result == NSModalResponseOK ) {
                    folderUrl = [[p URLs] objectAtIndex:0];
                    
                    NSError * err = nil;
                    NSData * bookmark = [folderUrl bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope includingResourceValuesForKeys:nil relativeToURL:nil error:&err];
                    if (!bookmark && err) {
                        ALog("failed to bookmark path %@ with error %@", folderUrl, err);
                        self->errorState = err;
                        [NSApp stopModal];
                        return;
                    }
                    
                    obj = [[SandboxEntry alloc] initWithBookmark:bookmark];
                    [self->storage setObject:obj forKey:folderUrl];
                    [NSApp stopModal];
                } else {
                    ALog("user rejected file access prompt");
                    self->errorState = [NSError errorWithDomain:NSCocoaErrorDomain code:NSFileReadNoPermissionError userInfo:nil];
                    [NSApp stopModal];
                    return;
                }
            }];
            
            [NSApp runModalForWindow:[NSApp mainWindow]];
        } else {
            [obj setPathRefCount:[obj pathRefCount] + 1];
            [storage setObject:obj forKey:folderUrl];
        }
    }
}

- (void) removeBookmarkForURL:(NSURL *)fileUrl {
    NSURL * folderUrl = [urlWithoutFragment(fileUrl) URLByDeletingLastPathComponent];
    
    @synchronized (self) {
        SandboxEntry * obj = [storage objectForKey:folderUrl];
        if (obj) {
            NSInteger refCount = [obj pathRefCount];
            if (refCount > 1) {
                [obj setPathRefCount:refCount - 1];
                [storage setObject:obj forKey:folderUrl];
            } else {
                if ([obj secureUrl]) {
                    [[obj secureUrl] stopAccessingSecurityScopedResource];
                }
                [storage removeObjectForKey:folderUrl];
            }
        }
    }
}

- (void) beginFolderAccess:(NSURL *)fileUrl {
    NSURL * folderUrl = [urlWithoutFragment(fileUrl) URLByDeletingLastPathComponent];
    
    @synchronized (self) {
        SandboxEntry * obj = [storage objectForKey:folderUrl];
        if (!obj) {
            errorState = nil;
            
            [self addBookmarkToDictionary:fileUrl];
            
            obj = [storage objectForKey:folderUrl];
            
            if (errorState) {
                ALog(@"error granting permission to folder - %@", errorState);
                return;
            }
        }
        
        if (obj) {
            NSInteger refCount = [obj secRefCount];
            if (refCount > 0) {
                [obj setSecRefCount:refCount + 1];
                [storage setObject:obj forKey:folderUrl];
            } else {
                NSError * err = nil;
                BOOL isStale = NO;
                NSURL * secureUrl = [NSURL URLByResolvingBookmarkData:[obj bookmark] options:NSURLBookmarkResolutionWithSecurityScope relativeToURL:nil bookmarkDataIsStale:&isStale error:&err];
                if (!secureUrl && err)
                {
                    ALog("failed to access bookmark for %@ with error %@", folderUrl, err);
                    return;
                }
                
                [secureUrl startAccessingSecurityScopedResource];
                
                [obj setSecureUrl:secureUrl];
                [obj setSecRefCount:1];
                [storage setObject:obj forKey:folderUrl];
            }
        }
    }
}

- (void) endFolderAccess:(NSURL *)fileUrl {
    NSURL * folderUrl = [urlWithoutFragment(fileUrl) URLByDeletingLastPathComponent];
    
    @synchronized (self) {
        SandboxEntry * obj = [storage objectForKey:folderUrl];
        if (obj) {
            NSInteger refCount = [obj secRefCount];
            if (refCount > 1) {
                [obj setSecRefCount:refCount - 1];
                [storage setObject:obj forKey:folderUrl];
            } else {
                if ([obj secureUrl]) {
                    [[obj secureUrl] stopAccessingSecurityScopedResource];
                    [obj setSecureUrl:nil];
                }
                
                [obj setSecRefCount:0];
                
                [storage setObject:obj forKey:folderUrl];
            }
        }
    }
}

@end
