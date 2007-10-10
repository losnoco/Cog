//
//  CueSheet.h
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class CueSheetTrack;

@interface CueSheet : NSObject {
	NSArray *tracks;
}

+ (id)cueSheetWithFile: (NSString *)filename;

- (id)initWithFile:(NSString *)filename;

- (void)parseFile:(NSString *)filename;

- (NSArray *)tracks;

- (CueSheetTrack *)track:(NSString *)fragment;

@end
