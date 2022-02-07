//
//  AdPlugDecoder.h
//  AdPlug
//
//  Created by Christopher Snowhill on 1/27/18.
//  Copyright 2018 __LoSnoCo__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <libAdPlug/adplug.h>
#import <libAdPlug/opl.h>

#import "Plugin.h"

@interface AdPlugDecoder : NSObject <CogDecoder> {
	CPlayer* m_player;
	Copl* m_emu;

	unsigned int subsong, samples_todo;

	id<CogSource> source;
	unsigned long current_pos;
	unsigned long length;
}

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;
- (void)cleanUp;
@end
