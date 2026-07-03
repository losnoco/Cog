//
//  CogRemoteControlServerBridgeInterface.h
//  Cog
//
//  Created by Kevin López Brante on 2026-07-01.
//
//  Hand-written mirror of CogRemoteControl.framework's Swift bridge class
//  (RemoteControlServerBridge.swift). The app can't import the framework's
//  module or generated -Swift.h — its macOS 13 deployment floor is above the
//  app's 10.15 — so the ObjC interface is declared here and resolved through
//  the weak link at runtime. Selectors are pinned on the Swift side with
//  @objc(...) and must stay in sync with this file.
//

#import <Foundation/Foundation.h>

#import "../CogRemoteControl/CogRemoteControlTarget.h"

NS_ASSUME_NONNULL_BEGIN

API_AVAILABLE(macos(13.0))
@interface CogRemoteControlServerBridge : NSObject

/// Starts the MCP server on 127.0.0.1:<port>. The completion receives an
/// error description, or nil on success.
+ (void)startWithPort:(NSInteger)port
               target:(id<CogRemoteControlTarget>)target
           completion:(void (^)(NSString *_Nullable errorDescription))completion;

+ (void)stopWithCompletion:(void (^_Nullable)(void))completion;

@end

NS_ASSUME_NONNULL_END
