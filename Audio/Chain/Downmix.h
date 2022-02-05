//
//  Downmix.h
//  Cog
//
//  Created by Christopher Snowhill on 2/05/22.
//  Copyright 2022 __LoSnoCo__. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreAudio/CoreAudio.h>

#import "HeadphoneFilter.h"

@interface DownmixProcessor : NSObject {
    HeadphoneFilter *hFilter;
    
    AudioStreamBasicDescription inputFormat;
    AudioStreamBasicDescription outputFormat;
}

- (id) initWithInputFormat:(AudioStreamBasicDescription)inf andOutputFormat:(AudioStreamBasicDescription)outf;

- (void) process:(const void*)inBuffer frameCount:(size_t)frames output:(void *)outBuffer;

@end

