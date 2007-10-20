//
// APLDecoder.h

#import <Cocoa/Cocoa.h>

#import "Plugin.h"

@class APLFile;

@interface APLDecoder : NSObject <CogDecoder> {
	id<CogSource> source;
	id<CogDecoder> decoder;
	
	int bytesPerFrame; //Number of bytes per frame, ie channels * (bitsPerSample/8)
	int bytesPerSecond; //Number of bytes per second, ie bytesPerFrame * sampleRate
	int bytePosition; //Current position in bytes.
	
	
	
	double trackStart;
	double trackEnd; //miliseconds until end of track.
	double trackLength; //track len in miliseconds
	
	APLFile *apl;
}

@end
