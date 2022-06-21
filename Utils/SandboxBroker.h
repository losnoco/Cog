//
//  SandboxBroker.h
//  Cog
//
//  Created by Christopher Snowhill on 6/20/22.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface SandboxBroker : NSObject {
	NSMutableArray *storage;
}

+ (SandboxBroker *)sharedSandboxBroker;

- (id)init;
- (void)shutdown;

- (const void *)beginFolderAccess:(NSURL *)fileUrl;
- (void)endFolderAccess:(const void *)handle;

@end

NS_ASSUME_NONNULL_END
