//
//  BufferChain.h
//  CogNew
//
//  Created by Vincent Spader on 1/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "InputNode.h"
#import "ConverterNode.h"
#import "SoundController.h"
#import "PlaylistEntry.h"

@interface BufferChain : NSObject {
	InputNode *inputNode;
	ConverterNode *converterNode;
	PlaylistEntry *playlistEntry;
	
	NSArray *effects; //Not needed as of now, but for EFFECTS PLUGINS OF THE FUTURE!
	
	id finalNode; //Final buffer in the chain.
	
	id soundController;
}

- (id)initWithController:(id)c;
- (void)buildChain;
- (BOOL)open:(PlaylistEntry *)pe;
- (void)seek:(double)time;

- (void)launchThreads;

- (id)finalNode;

@end
