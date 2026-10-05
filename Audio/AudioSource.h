//
//  AudioSource.h
//  CogAudio
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <CogAudio/PluginController.h>

@interface AudioSource : NSObject {
}

+ (id<CogSource>)audioSourceForURL:(NSURL *)url;

@end
