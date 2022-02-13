//
//  AudioMetadataReader.h
//  CogAudio
//
//  Created by Vincent Spader on 2/24/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface AudioPropertiesReader : NSObject {
}

+ (NSDictionary *)propertiesForURL:(NSURL *)url;
+ (NSDictionary *)propertiesForURL:(NSURL *)url skipCue:(BOOL)skip;

@end
