//
//  SandboxBroker.h
//  Cog
//
//  Created by Christopher Snowhill on 2/27/21.
//

#ifndef SandboxBroker_h
#define SandboxBroker_h

@interface SandboxBroker : NSObject {
    NSMutableDictionary *storage;
    NSError *errorState;
}

+ (id) sharedSandboxBroker;

- (id) init;
- (void) shutdown;

- (void) addBookmarkToDictionary:(NSURL *)fileUrl;
- (void) removeBookmarkForURL:(NSURL *)fileUrl;

- (void) beginFolderAccess:(NSURL *)fileUrl;
- (void) endFolderAccess:(NSURL *)fileUrl;
@end

#endif /* SandboxBroker_h */
