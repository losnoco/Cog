//
//  CogDecoderMulti.h
//  CogAudio
//
//  Created by Christopher Snowhill on 10/21/13.
//
//

#import <Cocoa/Cocoa.h>
#import "Plugin.h"

@interface CogDecoderMulti : NSObject <CogDecoder> {
    NSArray *theDecoders;
    id<CogDecoder> theDecoder;
    NSMutableArray *cachedObservers;
}

-(id)initWithDecoders:(NSArray *)decoders;

@end
