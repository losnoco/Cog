//
//  BufferChain.h
//  CogNew
//
//  Created by Vincent Spader on 1/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "InputNode.h"

#import "AudioPlayer.h"

@interface BufferChain : NSObject {
	InputNode *inputNode;
	
	NSURL *streamURL;
	id userInfo;
	
	id finalNode; //Final buffer in the chain.
	
	id controller;
}

- (id)initWithController:(id)c;
- (void)buildChain;
- (BOOL)open:(NSURL *)url withOutputFormat:(AudioStreamBasicDescription)outputFormat;
- (void)seek:(double)time;

- (void)launchThreads;

- (id)finalNode;

- (id)userInfo;
- (void)setUserInfo:(id)i;

- (NSURL *)streamURL;
- (void)setStreamURL:(NSURL *)url;

- (void)setShouldContinue:(BOOL)s;

- (void)initialBufferFilled;

- (void)endOfInputReached;


@end
