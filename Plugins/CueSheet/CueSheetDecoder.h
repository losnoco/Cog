//
//  CueSheetDecoder.h
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "Plugin.h"

@class CueSheetTrack;

@interface CueSheetDecoder : NSObject <CogDecoder> {
	id<CogSource> source;
	id<CogDecoder> decoder;

	int bytesPerSecond;
	int bytePosition;
	double trackEnd;
	
	CueSheetTrack *track;
}

@end
