//
//  AudioMetadataWriter.h
//  CogAudio
//
//  Created by Safari on 08/11/18.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface AudioMetadataWriter : NSObject {
}

+ (int)putMetadataInURL:(NSURL *)url;

@end
