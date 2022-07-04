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

+ (NSURL *)urlWithoutFragment:(NSURL *)url;
+ (BOOL)isPath:(NSURL *)path aSubdirectoryOf:(NSURL *)directory;

- (id)init;
- (void)shutdown;

- (void)addFolderIfMissing:(NSURL *)folderUrl;
- (void)addFileIfMissing:(NSURL *)fileUrl;

- (void)requestFolderForFile:(NSURL *)fileUrl;

- (const void *)beginFolderAccess:(NSURL *)fileUrl;
- (void)endFolderAccess:(const void *)handle;

- (BOOL)areAllPathsSafe:(NSArray *)urls;

@end

NS_ASSUME_NONNULL_END
