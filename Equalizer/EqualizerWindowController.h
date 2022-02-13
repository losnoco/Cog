//
//  EqualizerWindowController.h
//  Cog
//
//  Created by Christopher Snowhill on 2/13/22.
//

#import <Cocoa/Cocoa.h>

#import <AppKit/AppKit.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>
#import <AudioUnit/AudioUnitCarbonView.h>

void equalizerApplyGenre(AudioUnit _Nullable au, const NSString *_Nonnull genre);
void equalizerLoadPreset(AudioUnit _Nullable au);
void equalizerApplyPreset(AudioUnit _Nullable au, const NSDictionary *_Nonnull preset);

NS_ASSUME_NONNULL_BEGIN

@interface EqPresetBehaviorArrayController : NSArrayController

@end

@interface EqualizerSlider : NSSlider

@end

@interface EqualizerWindowController : NSWindowController {
	IBOutlet NSPopUpButton *presetSelector;
	IBOutlet EqualizerSlider *eqPreamp;
	IBOutlet EqualizerSlider *eq20Hz;
	IBOutlet EqualizerSlider *eq25Hz;
	IBOutlet EqualizerSlider *eq31p5Hz;
	IBOutlet EqualizerSlider *eq40Hz;
	IBOutlet EqualizerSlider *eq50Hz;
	IBOutlet EqualizerSlider *eq63Hz;
	IBOutlet EqualizerSlider *eq80Hz;
	IBOutlet EqualizerSlider *eq100Hz;
	IBOutlet EqualizerSlider *eq125Hz;
	IBOutlet EqualizerSlider *eq160Hz;
	IBOutlet EqualizerSlider *eq200Hz;
	IBOutlet EqualizerSlider *eq250Hz;
	IBOutlet EqualizerSlider *eq315Hz;
	IBOutlet EqualizerSlider *eq400Hz;
	IBOutlet EqualizerSlider *eq500Hz;
	IBOutlet EqualizerSlider *eq630Hz;
	IBOutlet EqualizerSlider *eq800Hz;
	IBOutlet EqualizerSlider *eq1kHz;
	IBOutlet EqualizerSlider *eq1p2kHz;
	IBOutlet EqualizerSlider *eq1p6kHz;
	IBOutlet EqualizerSlider *eq2kHz;
	IBOutlet EqualizerSlider *eq2p5kHz;
	IBOutlet EqualizerSlider *eq3p1kHz;
	IBOutlet EqualizerSlider *eq4kHz;
	IBOutlet EqualizerSlider *eq5kHz;
	IBOutlet EqualizerSlider *eq6p3kHz;
	IBOutlet EqualizerSlider *eq8kHz;
	IBOutlet EqualizerSlider *eq10kHz;
	IBOutlet EqualizerSlider *eq12kHz;
	IBOutlet EqualizerSlider *eq16kHz;
	IBOutlet EqualizerSlider *eq20kHz;
	AudioUnit au;
}

- (void)setEQ:(AudioUnit _Nullable)au;

- (IBAction)toggleWindow:(id)sender;

- (IBAction)toggleEnable:(id)sender;
- (IBAction)toggleTracking:(id)sender;
- (IBAction)flattenEQ:(id)sender;
- (IBAction)levelPreamp:(id)sender;
- (IBAction)adjustSlider:(id)sender;
- (IBAction)changePreset:(id)sender;

@end

NS_ASSUME_NONNULL_END
