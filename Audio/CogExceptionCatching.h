//
//  CogExceptionCatching.h
//  CogAudio
//
//  Swift cannot catch an Objective-C exception, nor unwind through one
//  safely, and a plugin can raise one on a file it cannot parse. Swift calls
//  into the plugins through this, and handles what it returns.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// Runs `block`, and returns the Objective-C exception it raised, if any.
FOUNDATION_EXPORT NSException *_Nullable CogCatchException(NS_NOESCAPE void (^block)(void));

NS_ASSUME_NONNULL_END
