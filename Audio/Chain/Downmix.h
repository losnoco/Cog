//
//  Downmix.h
//  Cog
//
//  Created by Christopher Snowhill on 2/05/22.
//  Copyright 2022 __LoSnoCo__. All rights reserved.
//

#import <CoreAudio/CoreAudio.h>
#import <Foundation/Foundation.h>

#import "HeadphoneFilter.h"

@interface DownmixProcessor : NSObject {
	HeadphoneFilter *hFilter;

	AudioStreamBasicDescription inputFormat;
	AudioStreamBasicDescription outputFormat;

	uint32_t inConfig;
	uint32_t outConfig;
}

- (id)initWithInputFormat:(AudioStreamBasicDescription)inf inputConfig:(uint32_t)iConfig andOutputFormat:(AudioStreamBasicDescription)outf outputConfig:(uint32_t)oConfig;

- (void)process:(const void *)inBuffer frameCount:(size_t)frames output:(void *)outBuffer;

@end
