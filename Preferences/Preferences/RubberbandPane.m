//
//  Rubberband.m
//  Preferences
//
//  Created by Christopher Snowhill on 2/9/25.
//

#import <Foundation/Foundation.h>

#import "RubberbandPane.h"

@implementation RubberbandPane

- (NSString *)title {
	return NSLocalizedPrefString(@"Rubber Band");
}

- (NSImage *)icon {
	NSImage *icon = [[NSImage alloc] initWithContentsOfFile:[[NSBundle bundleForClass:[self class]] pathForImageResource:@"rubberband"]];
	[icon setTemplate:YES];
	return icon;
}

- (void)awakeFromNib {
	[self changeState:self];
}

- (IBAction)changeState:(id)sender {
	NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
	BOOL engineR3 = [[defaults stringForKey:@"rubberbandEngine"] isEqualToString:@"finer"];

	[windowBehavior reinitWithEngine:engineR3];

	if(engineR3) {
		NSString *window = [defaults stringForKey:@"rubberbandWindow"];
		if([window isEqualToString:@"long"]) {
			[defaults setValue:@"standard" forKey:@"rubberbandWindow"];
		}
	}
}

@end

@implementation RubberbandEngineArrayController
- (void)awakeFromNib {
	[self removeObjects:[self arrangedObjects]];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"EngineDisabled", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"disabled"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"EngineFaster", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"faster"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"EngineFiner", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"finer"}];
}

@end

@implementation RubberbandTransientsArrayController
- (void)awakeFromNib {
	[self removeObjects:[self arrangedObjects]];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"TransientsCrisp", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"crisp"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"TransientsMixed", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"mixed"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"TransientsSmooth", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"smooth"}];
}

@end

@implementation RubberbandDetectorArrayController
- (void)awakeFromNib {
	[self removeObjects:[self arrangedObjects]];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"DetectorCompound", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"compound"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"DetectorPercussive", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"percussive"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"DetectorSoft", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"soft"}];
}

@end

@implementation RubberbandPhaseArrayController
- (void)awakeFromNib {
	[self removeObjects:[self arrangedObjects]];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"PhaseLaminar", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"laminar"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"PhaseIndependent", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"independent"}];
}

@end

@implementation RubberbandWindowArrayController
- (void)awakeFromNib {
	BOOL engineR3 = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] stringForKey:@"rubberbandEngine"] isEqualToString:@"finer"];
	[self reinitWithEngine:engineR3];
}

- (void)reinitWithEngine:(BOOL)engineR3 {
	[self removeObjects:[self arrangedObjects]];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"WindowStandard", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"standard"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"WindowShort", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"short"}];

	if(!engineR3) {
		[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"WindowLong", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"long"}];
	}
}

@end

@implementation RubberbandSmoothingArrayController
- (void)awakeFromNib {
	[self removeObjects:[self arrangedObjects]];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"SmoothingOff", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"off"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"SmoothingOn", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"on"}];
}

@end

@implementation RubberbandFormantArrayController
- (void)awakeFromNib {
	[self removeObjects:[self arrangedObjects]];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"FormantShifted", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"shifted"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"FormantPreserved", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"preserved"}];
}

@end

@implementation RubberbandPitchArrayController
- (void)awakeFromNib {
	[self removeObjects:[self arrangedObjects]];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"PitchHighSpeed", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"highspeed"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"PitchHighQuality", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"highquality"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"PitchHighConsistency", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"highconsistency"}];
}

@end

@implementation RubberbandChannelsArrayController
- (void)awakeFromNib {
	[self removeObjects:[self arrangedObjects]];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"ChannelsApart", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"apart"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"ChannelsTogether", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"together"}];
}

@end
