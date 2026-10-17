//
//  PositionSlider.m
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "PositionSlider.h"
#import "Cog-Swift.h"
#import "TimeField.h"

static void *PositionSliderWaveformContext = &PositionSliderWaveformContext;

@implementation PositionSlider

+ (NSArray<NSString *> *)waveformKeys {
	return @[WaveformController.enabledKey, WaveformController.logScaleKey, WaveformController.rectifiedKey];
}

- (void)awakeFromNib {
	[super awakeFromNib];

	// The cell draws the waveform; the slider redraws it when it changes.
	[[NSNotificationCenter defaultCenter] addObserver:self
	                                         selector:@selector(waveformDidChange:)
	                                             name:WaveformController.didChangeNotification
	                                           object:WaveformController.shared];
	NSUserDefaultsController *defaults = [NSUserDefaultsController sharedUserDefaultsController];
	for(NSString *key in [PositionSlider waveformKeys]) {
		[defaults addObserver:self forKeyPath:[@"values." stringByAppendingString:key] options:0 context:PositionSliderWaveformContext];
	}
}

- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	NSUserDefaultsController *defaults = [NSUserDefaultsController sharedUserDefaultsController];
	for(NSString *key in [PositionSlider waveformKeys]) {
		[defaults removeObserver:self forKeyPath:[@"values." stringByAppendingString:key] context:PositionSliderWaveformContext];
	}
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if(context == PositionSliderWaveformContext) {
		[self setNeedsDisplay:YES];
	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (void)waveformDidChange:(NSNotification *)notification {
	[self setNeedsDisplay:YES];
}

/// The waveform settings, on the bar itself.
- (NSMenu *)menuForEvent:(NSEvent *)event {
	NSMenu *menu = [[NSMenu alloc] initWithTitle:@""];
	menu.autoenablesItems = NO;
	BOOL shown = [[NSUserDefaults standardUserDefaults] boolForKey:WaveformController.enabledKey];
	NSArray *items = @[@[NSLocalizedString(@"Show Waveform", @"Position slider menu item"), WaveformController.enabledKey],
	                   @[NSLocalizedString(@"Logarithmic Scale", @"Position slider menu item: waveform in decibels"), WaveformController.logScaleKey],
	                   @[NSLocalizedString(@"Rectified", @"Position slider menu item: waveform folded above its baseline"), WaveformController.rectifiedKey]];
	for(NSArray *entry in items) {
		NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:entry[0] action:@selector(toggleWaveformSetting:) keyEquivalent:@""];
		item.target = self;
		item.representedObject = entry[1];
		item.state = [[NSUserDefaults standardUserDefaults] boolForKey:entry[1]] ? NSControlStateValueOn : NSControlStateValueOff;
		// Scale and rectification only matter while the waveform shows.
		item.enabled = shown || [entry[1] isEqualToString:WaveformController.enabledKey];
		[menu addItem:item];
	}
	return menu;
}

- (IBAction)toggleWaveformSetting:(NSMenuItem *)sender {
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	[defaults setBool:![defaults boolForKey:sender.representedObject] forKey:sender.representedObject];
}

- (void)setDoubleValue:(double)value {
	self.positionTextField.currentTime = (long)value;

	[super setDoubleValue:value];
}

- (void)setMaxValue:(double)value {
	if(isnan(value) || isinf(value)) value = 0.0; // Clip invalid values from bad file playlist entries

	self.positionTextField.duration = (long)value;

	[super setMaxValue:value];
}

- (void)mouseDragged:(NSEvent *)theEvent {
	self.positionTextField.currentTime = (long)[self doubleValue];

	[super mouseDragged:theEvent];
}

@end
