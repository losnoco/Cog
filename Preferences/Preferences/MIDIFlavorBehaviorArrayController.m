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

	[self addObject:
	      [NSDictionary dictionaryWithObjectsAndKeys:
	                    @"Default (auto)", @"name", @"default", @"preference", nil]];

	[self addObject:
	      [NSDictionary dictionaryWithObjectsAndKeys:
	                    @"General MIDI", @"name", @"gm", @"preference", nil]];

	[self addObject:
	      [NSDictionary dictionaryWithObjectsAndKeys:
	                    @"General MIDI 2", @"name", @"gm2", @"preference", nil]];

	[self addObject:
	      [NSDictionary dictionaryWithObjectsAndKeys:
	                    @"Roland SC-55", @"name", @"sc55", @"preference", nil]];

	[self addObject:
	      [NSDictionary dictionaryWithObjectsAndKeys:
	                    @"Roland SC-88", @"name", @"sc88", @"preference", nil]];

	[self addObject:
	      [NSDictionary dictionaryWithObjectsAndKeys:
	                    @"Roland SC-88 Pro", @"name", @"sc88pro", @"preference", nil]];

	[self addObject:
	      [NSDictionary dictionaryWithObjectsAndKeys:
	                    @"Roland SC-8850", @"name", @"sc8850", @"preference", nil]];

	[self addObject:
	      [NSDictionary dictionaryWithObjectsAndKeys:
	                    @"Yamaha XG", @"name", @"xg", @"preference", nil]];
}

@end
