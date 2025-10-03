//
//  EqualizerWindowController.m
//  Cog
//
//  Created by Christopher Snowhill on 2/13/22.
//

#import "EqualizerWindowController.h"

#import "json.h"

#import "Logging.h"

static const NSString *equalizerGenre = @"";
static const NSString *equalizerDefaultGenre = @"Flat";
static NSArray *equalizer_presets_processed = nil;
static NSDictionary *equalizer_presets_by_name = nil;
static json_value *equalizer_presets = NULL;

static NSString *_cog_equalizer_type = @"Cog EQ library file v1.0";

static NSArray *_cog_equalizer_items(void) {
	return @[@"name", @"hz32", @"hz64", @"hz128", @"hz256", @"hz512", @"hz1000", @"hz2000", @"hz4000", @"hz8000", @"hz16000", @"preamp"];
}

static NSArray *_cog_equalizer_band_settings(void) {
	return @[@"eqPreamp", @"eq20Hz", @"eq25Hz", @"eq31p5Hz", @"eq40Hz", @"eq50Hz", @"eq63Hz", @"eq80Hz", @"eq100Hz", @"eq125Hz", @"eq160Hz", @"eq200Hz", @"eq250Hz", @"eq315Hz", @"eq400Hz", @"eq500Hz", @"eq630Hz", @"eq800Hz", @"eq1kHz", @"eq1p2kHz", @"eq1p6kHz", @"eq2kHz", @"eq2p5kHz", @"eq3p1kHz", @"eq4kHz", @"eq5kHz", @"eq6p3kHz", @"eq8kHz", @"eq10kHz", @"eq12kHz", @"eq16kHz", @"eq20kHz"];
}

static const float cog_equalizer_bands[10] = { 32, 64, 128, 256, 512, 1000, 2000, 4000, 8000, 16000 };
static NSArray *cog_equalizer_items = nil;
static NSArray *cog_equalizer_band_settings = nil;
static NSString *cog_equalizer_extra_genres = @"altGenres";

static const float apple_equalizer_bands[31] = { 20, 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800, 1000, 1200, 1600, 2000, 2500, 3100, 4000, 5000, 6300, 8000, 10000, 12000, 16000, 20000 };

static inline float interpolatePoint(const NSDictionary *preset, float freqTarget) {
	if(!cog_equalizer_items)
		cog_equalizer_items = _cog_equalizer_items();

	// predict extra bands! lpc was too broken, quadra was broken, let's try simple linear steps
	if(freqTarget < cog_equalizer_bands[0]) {
		float work[14];
		float work_freq[14];
		for(unsigned int i = 0; i < 10; ++i) {
			work[9 - i] = [[preset objectForKey:[cog_equalizer_items objectAtIndex:1 + i]] floatValue];
			work_freq[9 - i] = cog_equalizer_bands[i];
		}
		for(unsigned int i = 10; i < 14; ++i) {
			work[i] = work[i - 1] + (work[i - 1] - work[i - 2]) * 1.05;
			work_freq[i] = work_freq[i - 1] + (work_freq[i - 1] - work_freq[i - 2]) * 1.05;
		}
		for(unsigned int i = 0; i < 13; ++i) {
			if(freqTarget >= work_freq[13 - i] &&
			   freqTarget < work_freq[12 - i]) {
				float freqLow = work_freq[13 - i];
				float freqHigh = work_freq[12 - i];
				float valueLow = work[13 - i];
				float valueHigh = work[12 - i];

				float delta = (freqTarget - freqLow) / (freqHigh - freqLow);

				return valueLow + (valueHigh - valueLow) * delta;
			}
		}

		return work[13];
	} else if(freqTarget > cog_equalizer_bands[9]) {
		float work[14];
		float work_freq[14];
		for(unsigned int i = 0; i < 10; ++i) {
			work[i] = [[preset objectForKey:[cog_equalizer_items objectAtIndex:1 + i]] floatValue];
			work_freq[i] = cog_equalizer_bands[i];
		}
		for(unsigned int i = 10; i < 14; ++i) {
			work[i] = work[i - 1] + (work[i - 1] - work[i - 2]) * 1.05;
			work_freq[i] = work_freq[i - 1] + (work_freq[i - 1] - work_freq[i - 2]) * 1.05;
		}
		for(unsigned int i = 0; i < 13; ++i) {
			if(freqTarget >= work_freq[i] &&
			   freqTarget < work_freq[i + 1]) {
				float freqLow = work_freq[i];
				float freqHigh = work_freq[i + 1];
				float valueLow = work[i];
				float valueHigh = work[i + 1];

				float delta = (freqTarget - freqLow) / (freqHigh - freqLow);

				return valueLow + (valueHigh - valueLow) * delta;
			}
		}

		return work[13];
	}

	// Pick the extremes
	if(freqTarget == cog_equalizer_bands[0])
		return [[preset objectForKey:[cog_equalizer_items objectAtIndex:1]] floatValue];
	else if(freqTarget == cog_equalizer_bands[9])
		return [[preset objectForKey:[cog_equalizer_items objectAtIndex:10]] floatValue];

	// interpolation time! linear is fine for this

	for(size_t i = 0; i < 9; ++i) {
		if(freqTarget >= cog_equalizer_bands[i] &&
		   freqTarget < cog_equalizer_bands[i + 1]) {
			float freqLow = cog_equalizer_bands[i];
			float freqHigh = cog_equalizer_bands[i + 1];
			float valueLow = [[preset objectForKey:[cog_equalizer_items objectAtIndex:i + 1]] floatValue];
			float valueHigh = [[preset objectForKey:[cog_equalizer_items objectAtIndex:i + 2]] floatValue];

			float delta = (freqTarget - freqLow) / (freqHigh - freqLow);

			return valueLow + (valueHigh - valueLow) * delta;
		}
	}

	return 0.0;
}

static void interpolateBands(float *out, const NSDictionary *preset) {
	for(size_t i = 0; i < 31; ++i) {
		out[i] = interpolatePoint(preset, apple_equalizer_bands[i]);
	}
}

static float getPreamp(const NSDictionary *preset) {
	return [[preset objectForKey:[cog_equalizer_items objectAtIndex:11]] floatValue];
}

static void loadPresets(void) {
	if([equalizer_presets_processed count]) return;

	NSURL *url = [[NSBundle mainBundle] URLForResource:@"Cog.q1" withExtension:@"json"];

	NSFileHandle *fileHandle = [NSFileHandle fileHandleForReadingAtPath:[url path]];
	if(fileHandle) {
		NSError *err;
		NSData *data;
		if(@available(macOS 10.15, *)) {
			data = [fileHandle readDataToEndOfFileAndReturnError:&err];
		} else {
			data = [fileHandle readDataToEndOfFile];
			err = nil;
		}
		if(!err && data) {
			equalizer_presets = json_parse(data.bytes, data.length);

			if(equalizer_presets->type == json_object &&
			   equalizer_presets->u.object.length == 2 &&
			   strncmp(equalizer_presets->u.object.values[0].name, "type", equalizer_presets->u.object.values[0].name_length) == 0 &&
			   equalizer_presets->u.object.values[0].value->type == json_string &&
			   strncmp(equalizer_presets->u.object.values[0].value->u.string.ptr, [_cog_equalizer_type UTF8String], equalizer_presets -> u.object.values[0].value->u.string.length) == 0 &&
			   strncmp(equalizer_presets->u.object.values[1].name, "presets", equalizer_presets->u.object.values[1].name_length) == 0 &&
			   equalizer_presets->u.object.values[1].value->type == json_array) {
				// Got the array of presets
				NSMutableArray *array = [NSMutableArray new];
				NSMutableDictionary *dict = [NSMutableDictionary new];

				size_t count = equalizer_presets->u.object.values[1].value->u.array.length;
				json_value **values = equalizer_presets->u.object.values[1].value->u.array.values;

				cog_equalizer_items = _cog_equalizer_items();

				const size_t cog_object_minimum = [cog_equalizer_items count];

				for(size_t i = 0; i < count; ++i) {
					if(values[i]->type == json_object) {
						NSMutableArray<NSString *> *extraGenres = [NSMutableArray new];
						size_t object_items = values[i]->u.object.length;
						json_object_entry *object_entry = values[i]->u.object.values;
						size_t requiredItemsPresent = 0;
						if(object_items >= cog_object_minimum) {
							NSMutableDictionary *equalizerItem = [NSMutableDictionary new];
							for(size_t j = 0; j < object_items; ++j) {
								NSString *key = [NSString stringWithUTF8String:object_entry[j].name];
								NSInteger index = [cog_equalizer_items indexOfObject:key];
								if(index != NSNotFound) {
									if(index == 0 && object_entry[j].value->type == json_string) {
										NSString *name = [NSString stringWithUTF8String:object_entry[j].value->u.string.ptr];
										[equalizerItem setObject:name forKey:key];
										++requiredItemsPresent;
									} else if(object_entry[j].value->type == json_integer) {
										int64_t value = object_entry[j].value->u.integer;
										float floatValue = ((value <= 401 && value >= 1) ? ((float)(value - 201) / 10.0) : 0.0);
										[equalizerItem setObject:@(floatValue) forKey:key];
										++requiredItemsPresent;
									}
								} else if([key isEqualToString:cog_equalizer_extra_genres]) {
									// Process alternate genre matches
									if(object_entry[j].value->type == json_array) {
										size_t value_count = object_entry[j].value->u.array.length;
										json_value **values = object_entry[j].value->u.array.values;
										for(size_t k = 0; k < value_count; ++k) {
											if(values[k]->type == json_string) {
												[extraGenres addObject:[NSString stringWithUTF8String:values[i]->u.string.ptr]];
											}
										}
									}
								}
							}

							if(requiredItemsPresent == cog_object_minimum) {
								// Add the base item
								NSDictionary *outItem = [NSDictionary dictionaryWithDictionary:equalizerItem];
								[array addObject:outItem];
								[dict setObject:outItem forKey:[outItem objectForKey:@"name"]];

								// Add the alternate genres, if any
								for(NSString *genre in extraGenres) {
									[dict setObject:outItem forKey:genre];
								}
							}
						}
					}
				}

				equalizer_presets_processed = [NSArray arrayWithArray:array];
				equalizer_presets_by_name = [NSDictionary dictionaryWithDictionary:dict];
			}
		}
		[fileHandle closeFile];

		json_value_free(equalizer_presets);
		equalizer_presets = NULL;
	}
}

void equalizerApplyGenre(AudioUnit au, const NSString *genre) {
	equalizerGenre = genre;
	if([[NSUserDefaults standardUserDefaults] boolForKey:@"GraphicEQtrackgenre"]) {
		loadPresets();

		NSDictionary *preset = [equalizer_presets_by_name objectForKey:genre];
		if(!preset) {
			// Find a match
			if(genre && ![genre isEqualToString:@""]) {
				NSUInteger matchLength = 0;
				NSString *lowerCaseGenre = [genre lowercaseString];
				for(NSString *key in [equalizer_presets_by_name allKeys]) {
					NSString *lowerCaseKey = [key lowercaseString];
					if([lowerCaseGenre containsString:lowerCaseKey]) {
						if([key length] > matchLength) {
							matchLength = [key length];
							preset = [equalizer_presets_by_name objectForKey:key];
						}
					}
				}
			}

			if(!preset) {
				preset = [equalizer_presets_by_name objectForKey:equalizerDefaultGenre];
			}
		}
		if(preset) {
			NSInteger index = [equalizer_presets_processed indexOfObject:preset];
			[[NSUserDefaults standardUserDefaults] setInteger:index forKey:@"GraphicEQpreset"];

			equalizerApplyPreset(au, preset);
		}
	}
}

void equalizerLoadPreset(AudioUnit au) {
	NSInteger index = [[NSUserDefaults standardUserDefaults] integerForKey:@"GraphicEQpreset"];
	if(index >= 0 && index < [equalizer_presets_processed count]) {
		NSDictionary *preset = [equalizer_presets_processed objectAtIndex:index];
		equalizerApplyPreset(au, preset);
	} else if(au) {
		@synchronized(cog_equalizer_band_settings) {
			if(!cog_equalizer_band_settings)
				cog_equalizer_band_settings = _cog_equalizer_band_settings();
		}

		AudioUnitSetParameter(au, kGraphicEQParam_NumberOfBands, kAudioUnitScope_Global, 0, 1, 0);
		for(NSInteger i = 1; i < [cog_equalizer_band_settings count]; ++i) {
			float value = [[NSUserDefaults standardUserDefaults] floatForKey:[cog_equalizer_band_settings objectAtIndex:i]];
			AudioUnitSetParameter(au, (int)(i - 1), kAudioUnitScope_Global, 0, value, 0);
		}
	}
}

void equalizerApplyPreset(AudioUnit au, const NSDictionary *preset) {
	if(preset) {
		@synchronized(cog_equalizer_band_settings) {
			if(!cog_equalizer_band_settings)
				cog_equalizer_band_settings = _cog_equalizer_band_settings();
		}

		if(au) {
			AudioUnitParameterValue paramValue = 0;
			if(AudioUnitGetParameter(au, kGraphicEQParam_NumberOfBands, kAudioUnitScope_Global, 0, &paramValue))
				return;
		}

		float presetValues[31];
		interpolateBands(presetValues, preset);

		float preamp = getPreamp(preset);

		[[NSUserDefaults standardUserDefaults] setFloat:preamp forKey:[cog_equalizer_band_settings objectAtIndex:0]];
		if(au) {
			AudioUnitSetParameter(au, kGraphicEQParam_NumberOfBands, kAudioUnitScope_Global, 0, 1, 0);
		}
		for(unsigned int i = 0; i < 31; ++i) {
			[[NSUserDefaults standardUserDefaults] setFloat:presetValues[i] forKey:[cog_equalizer_band_settings objectAtIndex:i + 1]];
			if(au) {
				AudioUnitSetParameter(au, i, kAudioUnitScope_Global, 0, presetValues[i], 0);
			}
		}
	}
}

@implementation EqPresetBehaviorArrayController

- (void)awakeFromNib {
	[self removeObjects:[self arrangedObjects]];

	loadPresets();

	for(NSDictionary *preset in equalizer_presets_processed) {
		[self addObject:@{ @"name": [preset objectForKey:@"name"], @"preference": [preset objectForKey:@"name"] }];
	}

	[self addObject:@{ @"name": @"Custom", @"preference": @"Custom" }];
}

@end

@implementation EqualizerSlider

- (void)awakeFromNib {
	[self setTrackFillColor:[NSColor systemGrayColor]];
}

@end

@interface EqualizerWindowController ()

@end

@implementation EqualizerWindowController

+ (void)initialize {
	@synchronized(cog_equalizer_band_settings) {
		if(!cog_equalizer_band_settings)
			cog_equalizer_band_settings = _cog_equalizer_band_settings();
	}
}

- (id)init {
	return [super initWithWindowNibName:@"Equalizer"];
}

- (void)windowDidLoad {
	[super windowDidLoad];

	[self changePreset:presetSelector];

	[self handleMouseEvents];
}

- (void)setEQ:(AudioUnit)au {
	self->au = au;
}

- (IBAction)toggleWindow:(id)sender {
	if([[self window] isVisible])
		[[self window] orderOut:self];
	else
		[self showWindow:self];
}

- (IBAction)toggleEnable:(id)sender {
}

- (IBAction)toggleTracking:(id)sender {
	equalizerApplyGenre(au, equalizerGenre);

	[self changePreset:presetSelector];
}

- (IBAction)flattenEQ:(id)sender {
	NSDictionary *preset = [equalizer_presets_by_name objectForKey:equalizerDefaultGenre];
	NSInteger index = [equalizer_presets_processed indexOfObject:preset];

	[presetSelector selectItemAtIndex:index];
	[[NSUserDefaults standardUserDefaults] setInteger:index forKey:@"GraphicEQpreset"];

	[self changePreset:presetSelector];
}

- (EqualizerSlider *)sliderForIndex:(NSInteger)index {
	switch(index) {
		case 0:
			return eqPreamp;
		case 1:
			return eq20Hz;
		case 2:
			return eq25Hz;
		case 3:
			return eq31p5Hz;
		case 4:
			return eq40Hz;
		case 5:
			return eq50Hz;
		case 6:
			return eq63Hz;
		case 7:
			return eq80Hz;
		case 8:
			return eq100Hz;
		case 9:
			return eq125Hz;
		case 10:
			return eq160Hz;
		case 11:
			return eq200Hz;
		case 12:
			return eq250Hz;
		case 13:
			return eq315Hz;
		case 14:
			return eq400Hz;
		case 15:
			return eq500Hz;
		case 16:
			return eq630Hz;
		case 17:
			return eq800Hz;
		case 18:
			return eq1kHz;
		case 19:
			return eq1p2kHz;
		case 20:
			return eq1p6kHz;
		case 21:
			return eq2kHz;
		case 22:
			return eq2p5kHz;
		case 23:
			return eq3p1kHz;
		case 24:
			return eq4kHz;
		case 25:
			return eq5kHz;
		case 26:
			return eq6p3kHz;
		case 27:
			return eq8kHz;
		case 28:
			return eq10kHz;
		case 29:
			return eq12kHz;
		case 30:
			return eq16kHz;
		case 31:
			return eq20kHz;
		default:
			return nil;
	}
}

- (IBAction)levelPreamp:(id)sender {
	float preamp = [eqPreamp floatValue];

	float maxValue = 0.0;
	for(NSInteger i = 1; i < [cog_equalizer_band_settings count]; ++i) {
		float value = [[self sliderForIndex:i] floatValue];
		if(value > maxValue) maxValue = value;
	}

	if(maxValue > 0.0 && preamp != -maxValue) {
		[presetSelector selectItemAtIndex:[equalizer_presets_processed count]];
		[[NSUserDefaults standardUserDefaults] setInteger:[equalizer_presets_processed count] forKey:@"GraphicEQpreset"];

		[eqPreamp setFloatValue:-maxValue];
		[[NSUserDefaults standardUserDefaults] setFloat:-maxValue forKey:[cog_equalizer_band_settings objectAtIndex:0]];
	}
}

- (IBAction)adjustSlider:(id)sender {
	NSInteger tag = [sender tag];

	NSInteger count = [equalizer_presets_processed count];
	if([[NSUserDefaults standardUserDefaults] integerForKey:@"GraphicEQpreset"] != count) {
		[[NSUserDefaults standardUserDefaults] setInteger:count forKey:@"GraphicEQpreset"];
		[presetSelector selectItemAtIndex:count];
	}

	if(tag == 0) {
		float preamp = [eqPreamp floatValue];
		[[NSUserDefaults standardUserDefaults] setFloat:preamp forKey:[cog_equalizer_band_settings objectAtIndex:0]];
	} else if(tag < [cog_equalizer_band_settings count]) {
		float value = [sender floatValue];
		[[NSUserDefaults standardUserDefaults] setFloat:value forKey:[cog_equalizer_band_settings objectAtIndex:tag]];
		if(au)
			AudioUnitSetParameter(au, (int)(tag - 1), kAudioUnitScope_Global, 0, value, 0);
	}
}

- (void)changePreset:(id)sender {
	NSInteger index = [sender indexOfSelectedItem];

	if(index >= 0 && index < [equalizer_presets_processed count]) {
		NSDictionary *preset = [equalizer_presets_processed objectAtIndex:index];

		equalizerApplyPreset(au, preset);
	}
}

- (void)handleMouseEvents {
	[NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskRightMouseDown | NSEventMaskRightMouseDragged
	                                      handler:^NSEvent *_Nullable(NSEvent *_Nonnull theEvent) {
		                                      if([theEvent window] == [self window]) {
			                                      NSPoint event_location = [theEvent locationInWindow];
			                                      NSPoint local_point = [self.window.contentView convertPoint:event_location fromView:nil];

			                                      for(NSInteger i = 0; i < [cog_equalizer_band_settings count]; ++i) {
				                                      NSSlider *slider = [self sliderForIndex:i];
				                                      if(NSPointInRect(local_point, [slider frame])) {
					                                      float sliderPosition = (MAX(MIN(local_point.y, 344.0), 40.0) - 40.0) / 152.0 - 1.0;
					                                      [slider setFloatValue:sliderPosition * 20.0];
					                                      [self adjustSlider:slider];
					                                      break;
				                                      }
			                                      }
		                                      }

		                                      return theEvent;
	                                      }];
}

@end
