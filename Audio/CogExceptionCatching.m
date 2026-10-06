//
//  CogExceptionCatching.m
//  CogAudio
//

#import "CogExceptionCatching.h"

NSException *CogCatchException(NS_NOESCAPE void (^block)(void)) {
	@try {
		block();
	}
	@catch(NSException *exception) {
		return exception;
	}
	return nil;
}
