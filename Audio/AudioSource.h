//
//  AudioDecoder.h
//  CogAudio
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "PluginController.h"

@interface AudioSource : NSObject {
}

+ (id<CogSource>)audioSourceForURL:(NSURL *)url;

@end
