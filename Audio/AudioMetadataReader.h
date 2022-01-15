//
//  AudioMetadataReader.h
//  CogAudio
//
//  Created by Vincent Spader on 2/24/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface AudioMetadataReader : NSObject {

}

+ (NSDictionary *)metadataForURL:(NSURL *)url;
+ (NSDictionary *)metadataForURL:(NSURL *)url skipCue:(BOOL)skip;

@end
