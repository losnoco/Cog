//
// APLDecoder.h

#import <Cocoa/Cocoa.h>

#import "Plugin.h"

@class APLFile;

@interface APLDecoder : NSObject <CogDecoder> {
	id<CogSource> source;
	id<CogDecoder> decoder;

	int bytesPerFrame; // Number of bytes per frame, ie channels * (bitsPerSample/8)
	long framePosition; // current position in frames

	long trackStart;
	long trackEnd; // frames until end of track.
	long trackLength; // track len in frames

	APLFile *apl;
}

@end
