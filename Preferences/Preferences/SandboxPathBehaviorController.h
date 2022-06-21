//
//  SandboxPathBehaviorController.h
//  Preferences
//
//  Created by Christopher Snowhill on 6/19/22.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface SandboxPathBehaviorController : NSArrayController

- (void)addUrl:(NSURL *)url;
- (void)removePath:(NSString *)path;
- (void)removeStaleEntries;

- (BOOL)matchesPath:(NSURL *)url;

@end

NS_ASSUME_NONNULL_END
