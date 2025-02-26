//
//  AudioDecoder.h
//  CogAudio
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <CogAudio/Plugin.h>

@interface AudioDecoder : NSObject {
}

+ (id<CogDecoder>)audioDecoderForSource:(id<CogSource>)source;
+ (id<CogDecoder>)audioDecoderForSource:(id<CogSource>)source skipCue:(BOOL)skip;

@end
