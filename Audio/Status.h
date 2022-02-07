/*
 *  Status.h
 *  Cog
 *
 *  Created by Vincent Spader on 1/14/06.
 *  Copyright 2006 Vincent Spader. All rights reserved.
 *
 */

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, CogStatus) {
	CogStatusStopped = 0,
	CogStatusPaused,
	CogStatusPlaying,
	CogStatusStopping,
};
