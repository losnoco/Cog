//
//  Downmix.h
//  Cog
//
//  Created by Christopher Snowhill on 2/05/22.
//  Copyright 2022 __LoSnoCo__. All rights reserved.
//

#import <CoreAudio/CoreAudioTypes.h>
#import <Foundation/Foundation.h>

@interface DownmixProcessor : NSObject {
	AudioStreamBasicDescription inputFormat;
	AudioStreamBasicDescription outputFormat;

	uint32_t inConfig;
	uint32_t outConfig;
}

/* Fit mono to the output's center channel, where it has one, rather than to
 * its front left and right. */
@property(nonatomic) BOOL monoToCenter;

- (id)initWithInputFormat:(AudioStreamBasicDescription)inf inputConfig:(uint32_t)iConfig andOutputFormat:(AudioStreamBasicDescription)outf outputConfig:(uint32_t)oConfig;

- (void)process:(const void *)inBuffer frameCount:(size_t)frames output:(void *)outBuffer;

@end
