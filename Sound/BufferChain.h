//
//  InputChain.h
//  CogNew
//
//  Created by Zaphod Beeblebrox on 1/4/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "InputNode.h"
#import "ConverterNode.h"
#import "SoundController.h"

@interface BufferChain : NSObject {
	InputNode *inputNode;
	ConverterNode *converterNode;

	NSArray *effects; //Not needed as of now, but for EFFECTS PLUGINS OF THE FUTURE!
	
	id finalNode; //Final buffer in the chain.
	
	id soundController;
}

- (id)initWithController:(id)c;
- (void)buildChain;
- (BOOL)open:(const char *)filename;

- (void)launchThreads;

- (id)finalNode;

@end
