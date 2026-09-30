//
//  MainWindow.m
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "MainWindow.h"

#import "AppController.h"

#import <CogAudio/AudioPlayer.h>

// NOTICE! We bury first time defaults that should depend on whether the install is fresh or not here
// so that they get created correctly depending on the situation.

// For instance, for the first option to get this treatment, we want time stretching to stay enabled
// for existing installations, but disable itself by default on new installs, to spare processing.

void showSentryConsent(NSWindow *window) {
	BOOL askedConsent = [[NSUserDefaults standardUserDefaults] boolForKey:@"sentryAskedConsent"];
	if(!askedConsent) {
		[window orderFront:window];

		NSAlert *alert = [NSAlert new];
		[alert setMessageText:NSLocalizedString(@"SentryConsentTitle", @"")];
		[alert setInformativeText:NSLocalizedString(@"SentryConsentText", @"")];
		[alert addButtonWithTitle:NSLocalizedString(@"ConsentNo", @"")];
		[alert addButtonWithTitle:NSLocalizedString(@"ConsentYes",@"")];

		[alert beginSheetModalForWindow:window completionHandler:^(NSModalResponse returnCode) {
			if(returnCode == NSAlertSecondButtonReturn) {
				[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"sentryConsented"];
			}
		}];
		
		[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"sentryAskedConsent"];
	}
}

// Descriptions for the audio output status in the status bar.

// "44.1 kHz", or "2.8224 MHz" for DSD, in the user's number format.
static NSString *sampleRateDescription(double sampleRate) {
	if(sampleRate >= 1000000.0) {
		return [NSString localizedStringWithFormat:@"%.6g MHz", sampleRate / 1000000.0];
	}
	if(sampleRate >= 1000.0) {
		return [NSString localizedStringWithFormat:@"%.6g kHz", sampleRate / 1000.0];
	}
	return [NSString localizedStringWithFormat:@"%.0f Hz", sampleRate];
}

static NSString *channelsDescription(UInt32 channels) {
	switch(channels) {
		case 1:
			return NSLocalizedString(@"Mono", @"One audio channel");
		case 2:
			return NSLocalizedString(@"Stereo", @"Two audio channels");
		default:
			return [NSString stringWithFormat:NSLocalizedString(@"%u ch", @"A number of audio channels, e.g. 6 ch"), (unsigned int)channels];
	}
}

// "Float32", "Int16", "DoP", or the format's four-character code if it is
// not PCM.
static NSString *sampleFormatName(AudioStreamBasicDescription format, BOOL isDoP) {
	if(isDoP) {
		return @"DoP";
	}
	if(format.mFormatID != kAudioFormatLinearPCM) {
		const UInt32 code = CFSwapInt32HostToBig(format.mFormatID);
		return [[NSString alloc] initWithBytes:&code length:sizeof(code) encoding:NSMacOSRomanStringEncoding] ?: @"?";
	}
	if(format.mFormatFlags & kAudioFormatFlagIsFloat) {
		return [NSString stringWithFormat:@"Float%u", (unsigned int)format.mBitsPerChannel];
	}
	if(format.mFormatFlags & kAudioFormatFlagIsSignedInteger) {
		return [NSString stringWithFormat:@"Int%u", (unsigned int)format.mBitsPerChannel];
	}
	return [NSString stringWithFormat:@"UInt%u", (unsigned int)format.mBitsPerChannel];
}

static NSString *nonMixableSuffix(AudioStreamBasicDescription format) {
	return (format.mFormatFlags & kAudioFormatFlagIsNonMixable) ? NSLocalizedString(@" · Non-mixable", @"Non-mixable Core Audio stream format") : @"";
}

// "Float32 · 88.2 kHz · Stereo", with the word size when samples are stored
// wider than they are ("Int24 in 32-bit words").
static NSString *formatDescription(AudioStreamBasicDescription format, BOOL isDoP) {
	NSString *sampleFormat = sampleFormatName(format, isDoP);
	if(!isDoP && format.mFormatID == kAudioFormatLinearPCM) {
		const BOOL nonInterleaved = !!(format.mFormatFlags & kAudioFormatFlagIsNonInterleaved);
		const UInt32 bytesPerSample = nonInterleaved ? format.mBytesPerFrame :
		                                              (format.mChannelsPerFrame ? format.mBytesPerFrame / format.mChannelsPerFrame : 0);
		const UInt32 wordBits = bytesPerSample * 8;
		if(wordBits > format.mBitsPerChannel) {
			sampleFormat = [NSString stringWithFormat:NSLocalizedString(@"%@ in %u-bit words", @"A sample format stored in wider words, e.g. Int24 in 32-bit words"), sampleFormat, (unsigned int)wordBits];
		}
	}
	return [NSString stringWithFormat:@"%@ · %@ · %@%@", sampleFormat, sampleRateDescription(format.mSampleRate), channelsDescription(format.mChannelsPerFrame), nonMixableSuffix(format)];
}

// "Float32 · 48 kHz", for the status bar itself.
static NSString *shortFormatDescription(AudioStreamBasicDescription format, BOOL isDoP) {
	return [NSString stringWithFormat:@"%@ · %@%@", sampleFormatName(format, isDoP), sampleRateDescription(format.mSampleRate), nonMixableSuffix(format)];
}

// "DSD64", after its rate as a multiple of 44.1 or 48 kHz.
static NSString *dsdName(double sampleRate) {
	for(NSNumber *base in @[@44100.0, @48000.0]) {
		const double multiple = sampleRate / base.doubleValue;
		if(multiple >= 1.0 && fabs(multiple - round(multiple)) < 1e-6) {
			return [NSString stringWithFormat:@"DSD%.0f", multiple];
		}
	}
	return @"DSD";
}

static NSString *encodingDescription(NSString *encoding) {
	if([encoding isEqualToString:@"lossless"]) {
		return NSLocalizedString(@"lossless", @"How a track is encoded");
	}
	if([encoding isEqualToString:@"lossy"]) {
		return NSLocalizedString(@"lossy", @"How a track is encoded");
	}
	if([encoding isEqualToString:@"synthesized"]) {
		return NSLocalizedString(@"synthesized", @"How a track is encoded: rendered while playing, as MIDI or chiptunes are");
	}
	// Decoders that cannot tell say "lossy/lossless".
	return nil;
}

// "FLAC (lossless) · Int16 · 44.1 kHz · Stereo".
static NSString *sourceDescription(AudioStreamBasicDescription format, NSString *codec, NSString *encoding) {
	NSMutableArray<NSString *> *parts = [NSMutableArray array];
	NSString *encodingName = encodingDescription(encoding);
	if(codec.length) {
		[parts addObject:encodingName ? [NSString stringWithFormat:@"%@ (%@)", codec, encodingName] : codec];
	} else if(encodingName) {
		[parts addObject:encodingName];
	}
	[parts addObject:format.mBitsPerChannel == 1 ? dsdName(format.mSampleRate) : sampleFormatName(format, NO)];
	[parts addObject:sampleRateDescription(format.mSampleRate)];
	[parts addObject:channelsDescription(format.mChannelsPerFrame)];
	return [parts componentsJoinedByString:@" · "];
}

static BOOL formatFromValue(id value, AudioStreamBasicDescription *format) {
	if(![value isKindOfClass:[NSValue class]]) {
		return NO;
	}
	[(NSValue *)value getValue:format size:sizeof(*format)];
	return YES;
}

// Each stream's format, identical streams counted ("Float32 · 48 kHz ·
// Stereo ×2"); nil if there are none.
static NSString *streamFormatsDescription(NSArray *values) {
	NSMutableArray<NSString *> *descriptions = [NSMutableArray array];
	NSCountedSet<NSString *> *counts = [NSCountedSet set];
	for(id value in values) {
		AudioStreamBasicDescription format;
		if(!formatFromValue(value, &format)) {
			continue;
		}
		NSString *description = formatDescription(format, NO);
		if(![counts containsObject:description]) {
			[descriptions addObject:description];
		}
		[counts addObject:description];
	}
	if(!descriptions.count) {
		return nil;
	}
	NSMutableArray<NSString *> *parts = [NSMutableArray array];
	for(NSString *description in descriptions) {
		const NSUInteger count = [counts countForObject:description];
		[parts addObject:count > 1 ? [NSString stringWithFormat:@"%@ ×%lu", description, (unsigned long)count] : description];
	}
	return [parts componentsJoinedByString:@" / "];
}

static NSString *signedDecibels(double decibels) {
	// With a real minus sign.
	return [[NSString localizedStringWithFormat:@"%+.1f", decibels] stringByReplacingOccurrencesOfString:@"-" withString:@"−"];
}

static NSString *percentage(double percent) {
	NSNumberFormatter *formatter = [NSNumberFormatter new];
	formatter.numberStyle = NSNumberFormatterPercentStyle;
	formatter.maximumFractionDigits = 1;
	return [formatter stringFromNumber:@(percent / 100.0)];
}

// One way Cog changes the samples, with how much where that is known.
static NSString *modificationDescription(NSString *modification, NSDictionary *status, double sourceRate, AudioStreamBasicDescription renderFormat) {
	if([modification isEqualToString:CogAudioOutputModificationResampling]) {
		return [NSString stringWithFormat:NSLocalizedString(@"Resampled from %@ to %@", @"Cog signal-integrity reason: source and output sample rates"),
		                                  sampleRateDescription(sourceRate), sampleRateDescription(renderFormat.mSampleRate)];
	}
	if([modification isEqualToString:CogAudioOutputModificationTrackGain]) {
		NSNumber *gain = status[CogAudioOutputTrackGainKey];
		return gain ? [NSString stringWithFormat:NSLocalizedString(@"ReplayGain or tagged volume: %@ dB", @"Cog signal-integrity reason: the track's gain"), signedDecibels(gain.doubleValue)] :
		              NSLocalizedString(@"ReplayGain or tagged volume applied", @"Cog signal-integrity reason");
	}
	if([modification isEqualToString:CogAudioOutputModificationChannelLayout]) {
		NSNumber *fitted = status[CogAudioOutputFittedChannelsKey];
		NSString *to = channelsDescription(renderFormat.mChannelsPerFrame);
		return fitted ? [NSString stringWithFormat:NSLocalizedString(@"Channels remapped from %@ to %@", @"Cog signal-integrity reason: channels before and after"), channelsDescription(fitted.unsignedIntValue), to] :
		                [NSString stringWithFormat:NSLocalizedString(@"Channels remapped to %@", @"Cog signal-integrity reason: the device's channels"), to];
	}
	if([modification isEqualToString:CogAudioOutputModificationVolume]) {
		NSNumber *volume = status[CogAudioOutputVolumeKey];
		return volume ? [NSString stringWithFormat:NSLocalizedString(@"Cog volume at %@", @"Cog signal-integrity reason: Cog's volume in percent"), percentage(volume.doubleValue)] :
		                NSLocalizedString(@"Cog volume is not 100%", @"Cog signal-integrity reason");
	}
	NSDictionary<NSString *, NSString *> *descriptions = @{
		CogAudioOutputModificationDSDToPCM: NSLocalizedString(@"DSD converted to PCM", @"Cog signal-integrity reason"),
		CogAudioOutputModificationHDCD: NSLocalizedString(@"HDCD decoded", @"Cog signal-integrity reason"),
		CogAudioOutputModificationPrecision: NSLocalizedString(@"Sample precision reduced", @"Cog signal-integrity reason"),
		CogAudioOutputModificationTimeStretch: NSLocalizedString(@"Tempo or pitch changed", @"Cog signal-integrity reason"),
		CogAudioOutputModificationFreeSurround: NSLocalizedString(@"Upmixed by FreeSurround", @"Cog signal-integrity reason"),
		CogAudioOutputModificationEqualizer: NSLocalizedString(@"Equalizer applied", @"Cog signal-integrity reason"),
		CogAudioOutputModificationHRTF: NSLocalizedString(@"HRTF headphone virtualization applied", @"Cog signal-integrity reason"),
	};
	return descriptions[modification] ?: modification;
}

@implementation MainWindow

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)windowStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation {
	self = [super initWithContentRect:contentRect styleMask:windowStyle backing:bufferingType defer:deferCreation];
	if(self) {
		[self setExcludedFromWindowsMenu:YES];
		[self setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
	}
	return self;
}

- (void)awakeFromNib {
	[super awakeFromNib];

	outputFormatField.accessibilityLabel = NSLocalizedString(@"Audio output status", @"Audio output status accessibility label");
	[self showAudioOutputStatus:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
	                                         selector:@selector(audioOutputStatusDidChange:)
	                                             name:CogAudioOutputStatusDidChangeNotification
	                                           object:nil];

	[playlistView setNextResponder:self];

	if(![[NSUserDefaults standardUserDefaults] boolForKey:@"miniMode"]) {
		showSentryConsent(self);
	}
}

- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self name:CogAudioOutputStatusDidChangeNotification object:nil];
}

- (void)audioOutputStatusDidChange:(NSNotification *)notification {
	[self showAudioOutputStatus:notification.userInfo];
}

// Whether Cog passes the decoded samples on unchanged, and in which formats
// they go from the decoder through Core Audio to the device.
- (void)showAudioOutputStatus:(NSDictionary *)status {
	AudioStreamBasicDescription renderFormat;
	if(!formatFromValue(status[CogAudioOutputRenderFormatKey], &renderFormat)) {
		outputFormatField.stringValue = NSLocalizedString(@"Audio: —", @"No active audio output information");
		outputFormatField.toolTip = NSLocalizedString(@"No active audio output.", @"No active audio output tooltip");
		return;
	}
	const BOOL isDoP = [status[CogAudioOutputDoPKey] boolValue];
	const BOOL exclusive = [status[CogAudioOutputExclusiveKey] boolValue];
	AudioStreamBasicDescription sourceFormat;
	const BOOL hasSource = formatFromValue(status[CogAudioOutputSourceFormatKey], &sourceFormat);
	NSString *unavailable = NSLocalizedString(@"Unavailable", @"A format Core Audio or the decoder did not report");

	NSMutableArray<NSString *> *lines = [NSMutableArray array];
	NSArray<NSString *> *modifications = status[CogAudioOutputModificationsKey];
	NSString *integrity;
	if(!modifications) {
		integrity = NSLocalizedString(@"Unknown", @"Unknown Cog signal-integrity state");
		[lines addObject:NSLocalizedString(@"Unknown: the decoder did not describe its samples.", @"Tooltip heading when Cog cannot tell whether it changes the samples")];
	} else if(!modifications.count) {
		integrity = NSLocalizedString(@"Bit perfect", @"Bit-perfect Cog signal-integrity state");
		[lines addObject:NSLocalizedString(@"Bit perfect: Cog passes the decoded samples on unchanged.", @"Tooltip heading when Cog does not change the samples")];
	} else {
		integrity = NSLocalizedString(@"Modified", @"Modified Cog signal-integrity state");
		[lines addObject:NSLocalizedString(@"Modified by Cog:", @"Tooltip heading before the ways Cog changes the samples")];
		for(NSString *modification in modifications) {
			[lines addObject:[@"• " stringByAppendingString:modificationDescription(modification, status, hasSource ? sourceFormat.mSampleRate : 0, renderFormat)]];
		}
	}
	[lines addObject:@""];

	[lines addObject:[NSString stringWithFormat:NSLocalizedString(@"Source: %@", @"Tooltip line: the track as decoded"),
	                                            hasSource ? sourceDescription(sourceFormat, status[CogAudioOutputSourceCodecKey], status[CogAudioOutputSourceEncodingKey]) : unavailable]];
	NSString *cogOutput = formatDescription(renderFormat, isDoP);
	[lines addObject:[NSString stringWithFormat:NSLocalizedString(@"Cog output: %@", @"Tooltip line: what Cog renders for Core Audio"), cogOutput]];

	// Core Audio's formats only where they differ from what they follow:
	// usually they are the same throughout.
	NSString *previous = cogOutput;
	NSString *coreAudio = streamFormatsDescription(status[CogAudioOutputVirtualFormatsKey]);
	if(coreAudio && ![coreAudio isEqualToString:cogOutput]) {
		[lines addObject:[NSString stringWithFormat:NSLocalizedString(@"Core Audio: %@", @"Tooltip line: the formats the system mixes in"), coreAudio]];
		previous = coreAudio;
	}
	NSMutableArray<NSString *> *qualifiers = [NSMutableArray array];
	if([status[CogAudioOutputSystemDefaultKey] boolValue]) {
		[qualifiers addObject:NSLocalizedString(@"system default", @"The output device is the system's default")];
	}
	[qualifiers addObject:exclusive ? NSLocalizedString(@"exclusive", @"Cog holds the output device for itself") :
	                                  NSLocalizedString(@"shared", @"The output device plays other apps' audio too")];
	NSString *deviceName = status[CogAudioOutputDeviceNameKey] ?: NSLocalizedString(@"Unnamed device", @"An output device without a name");
	NSString *device = [NSString stringWithFormat:@"%@ (%@)", deviceName, [qualifiers componentsJoinedByString:@", "]];
	NSString *physical = streamFormatsDescription(status[CogAudioOutputPhysicalFormatsKey]);
	if([physical isEqualToString:previous]) {
		device = [NSString stringWithFormat:NSLocalizedString(@"%@, same format", @"An output device whose format matches the line above"), device];
	} else {
		device = [NSString stringWithFormat:@"%@ · %@", device, physical ?: unavailable];
	}
	[lines addObject:[NSString stringWithFormat:NSLocalizedString(@"Device: %@", @"Tooltip line: the output device and the format it runs at"), device]];
	[lines addObject:@""];

	[lines addObject:NSLocalizedString(@"Covers Cog only; macOS and the device may still change the sound.", @"Tooltip note on what the signal-integrity verdict covers")];

	outputFormatField.stringValue = [NSString stringWithFormat:NSLocalizedString(@"%@ · %@%@", @"Signal integrity, Cog output format, and exclusive transport status"),
	                                                            integrity,
	                                                            shortFormatDescription(renderFormat, isDoP),
	                                                            exclusive ? NSLocalizedString(@" · Exclusive", @"Exclusive audio transport status") : @""];
	outputFormatField.toolTip = [lines componentsJoinedByString:@"\n"];
}

- (void)focusSearch:(id)sender {
	[self makeFirstResponder:searchField];
	NSRange range = NSMakeRange(0, searchField.stringValue.length);
	NSText *editor = searchField.currentEditor;
	if(editor) {
		editor.selectedRange = range;
	}
}

- (IBAction)openSearch:(id)sender {
	[self focusSearch:sender];
	// hack
	NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:0.125
													  target:self
													selector:@selector(focusSearch:)
													userInfo:nil
													 repeats:NO];
	[[NSRunLoop mainRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
}

@end
