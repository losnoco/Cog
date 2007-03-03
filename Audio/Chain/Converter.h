//
//  ConverterNode.h
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <CoreAudio/AudioHardware.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>
@interface Converter : NSObject
{
	AudioConverterRef converter;
	
	void *outputBuffer;
	
	//Temporary for callback use
	void *inputBuffer;
	int inputBufferSize;
	BOOL needsReset;
	//end
	
	int outputSize;
	
	AudioStreamBasicDescription inputFormat;
	AudioStreamBasicDescription outputFormat;
}

- (void *)outputBuffer;

- (void)setupWithInputFormat:(AudioStreamBasicDescription)inputFormat outputFormat:(AudioStreamBasicDescription)outputFormat;
- (void)cleanUp;

- (int)convert:(void *)input amount:(int)inputSize;

@end
