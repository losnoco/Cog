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
	
	void *inputBuffer;
	void *outputBuffer;
	
	//Temporary for callback use
	int inputBufferSize;
	//end
	
	int outputSize;
	int maxInputSize;
	
	AudioStreamBasicDescription inputFormat;
	AudioStreamBasicDescription outputFormat;
}

- (void *)outputBuffer;
- (void *)inputBuffer;

- (void)setupWithInputFormat:(AudioStreamBasicDescription)inputFormat outputFormat:(AudioStreamBasicDescription)outputFormat;
- (void)cleanUp;

- (int)convert:(int)amount;

- (int)maxInputSize;

@end
