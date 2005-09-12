//
//  Controller.h
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/7/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface SoundController : NSObject {	
	InputController *input;
	OutputController *output;
	Converter *converter;
	
	NSLock *outputLock;
	NSLock *inputLock;
	
	Semaphore *conversionSemaphore;
	Semaphore *ioSemaphore;
	
	NSMutableArray *amountConverted;
	unsigned int amountPlayed; //when amountPlayed > amountConverted[0], amountPlayed -= amountConverted[0], pop(amountConverted[0]), song changed
}

- (void)convertedAmount:(int)amount; //called by converter...same thread?
- (void)playedAmount:(int)amount; //called by outputcontroller...different thread

@end
