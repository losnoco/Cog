//
//  MIDIFlavorBehaviorArrayController.m
//  General
//
//  Created by Christopher Snowhill on 04/12/16.
//
//

#import "MIDIFlavorBehaviorArrayController.h"

@implementation MIDIFlavorBehaviorArrayController
- (void)awakeFromNib {
	[self removeObjects:[self arrangedObjects]];

	[self addObject:@{@"name": @"Default (auto)", @"preference": @"default"}];

	[self addObject:@{@"name": @"General MIDI", @"preference": @"gm"}];

	[self addObject:@{@"name": @"General MIDI 2", @"preference": @"gm2"}];

	[self addObject:@{@"name": @"Roland SC-55", @"preference": @"sc55"}];

	[self addObject:@{@"name": @"Roland SC-88", @"preference": @"sc88"}];

	[self addObject:@{@"name": @"Roland SC-88 Pro", @"preference": @"sc88pro"}];

	[self addObject:@{@"name": @"Roland SC-8850", @"preference": @"sc8850"}];

	[self addObject:@{@"name": @"Yamaha XG", @"preference": @"xg"}];
}

@end
