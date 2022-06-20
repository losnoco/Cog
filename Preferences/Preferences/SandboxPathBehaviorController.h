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

@end

NS_ASSUME_NONNULL_END
