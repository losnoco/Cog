//
//  AUPlayerView.m
//  Output
//
//  Created by Christopher Snowhill on 1/29/16.
//  Copyright © 2016-2022 Christopher Snowhill. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AudioUnit/AUCocoaUIView.h>
#import <CoreAudioKit/AUGenericView.h>

#import "AUPlayerView.h"

#import "json.h"

#import "Logging.h"

static NSString * equalizerGenre = @"";
static const NSString * equalizerDefaultGenre = @"Flat";
static NSArray * equalizer_presets_processed = nil;
static NSDictionary * equalizer_presets_by_name = nil;
static json_value * equalizer_presets = NULL;

static NSArray* _winamp_equalizer_items() {
    return @[@"name", @"hz70", @"hz180", @"hz320", @"hz600", @"hz1000", @"hz3000", @"hz6000", @"hz12000", @"hz14000", @"hz16000", @"preamp"];
}

static const float winamp_equalizer_bands[10] = { 70, 180, 320, 600, 1000, 3000, 6000, 12000, 14000, 16000 };
static NSArray* winamp_equalizer_items = nil;
static NSString* winamp_equalizer_extra_genres = @"altGenres";

static const float apple_equalizer_bands_31[31] = { 20, 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800, 1000, 1200, 1600, 2000, 2500, 3100, 4000, 5000, 6300, 8000, 10000, 12000, 16000, 20000 };
static const float apple_equalizer_bands_10[10] = { 32, 64, 128, 256, 512, 1000, 2000, 4000, 8000, 16000 };

static inline float quadra(float *p, float frac) { return (((((((((p[0] + p[2]) * 0.5) - p[1]) * frac) * 0.5) + p[1]) - ((p[2] + p[0] + (p[0] * 2.0)) / 2.0)) * frac) * 2.0) + p[0]; }

static inline float interpolatePoint(NSDictionary * preset, float freqTarget) {
    if (!winamp_equalizer_items)
        winamp_equalizer_items = _winamp_equalizer_items();

    // predict extra bands! lpc was too broken, so use quadratic interpolation!
    if (freqTarget <= winamp_equalizer_bands[0]) {
        float work[14];
        float work_freq[14];
        for (unsigned int i = 0; i < 10; ++i) {
            work[9 - i] = [[preset objectForKey:[winamp_equalizer_items objectAtIndex:1 + i]] floatValue];
            work_freq[9 - i] = winamp_equalizer_bands[i];
        }
        for (unsigned int i = 10; i < 14; ++i) {
            work[i] = quadra(work + i - 3, 0.94);
            work_freq[i] = quadra(work_freq + i - 3, 0.94);
        }
        for (unsigned int i = 0; i < 13; ++i) {
            if (freqTarget >= work_freq[13 - i] &&
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
    }
    else if (freqTarget >= winamp_equalizer_bands[9]) {
        float work[14];
        float work_freq[14];
        for (unsigned int i = 0; i < 10; ++i) {
            work[i] = [[preset objectForKey:[winamp_equalizer_items objectAtIndex:1 + i]] floatValue];
            work_freq[i] = winamp_equalizer_bands[i];
        }
        for (unsigned int i = 10; i < 14; ++i) {
            work[i] = quadra(work + i - 3, 0.94);
            work_freq[i] = quadra(work_freq + i - 3, 0.94);
        }
        for (unsigned int i = 0; i < 13; ++i) {
            if (freqTarget >= work_freq[i] &&
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
    
    // interpolation time! linear is fine for this
    
    for (size_t i = 0; i < 9; ++i) {
        if (freqTarget >= winamp_equalizer_bands[i] &&
            freqTarget < winamp_equalizer_bands[i + 1]) {
            float freqLow = winamp_equalizer_bands[i];
            float freqHigh = winamp_equalizer_bands[i + 1];
            float valueLow = [[preset objectForKey:[winamp_equalizer_items objectAtIndex:i + 1]] floatValue];
            float valueHigh = [[preset objectForKey:[winamp_equalizer_items objectAtIndex:i + 2]] floatValue];
            
            float delta = (freqTarget - freqLow) / (freqHigh - freqLow);
            
            return valueLow + (valueHigh - valueLow) * delta;
        }
    }
    
    return 0.0;
}

static void interpolateBandsTo10(float * out, NSDictionary * preset) {
    for (size_t i = 0; i < 10; ++i) {
        out[i] = interpolatePoint(preset, apple_equalizer_bands_10[i]);
    }
}

static void interpolateBandsTo31(float * out, NSDictionary * preset) {
    for (size_t i = 0; i < 31; ++i) {
        out[i] = interpolatePoint(preset, apple_equalizer_bands_31[i]);
    }
}

static void loadPresets()
{
    if ([equalizer_presets_processed count]) return;

    CFURLRef appUrlRef = CFBundleCopyResourceURL(CFBundleGetMainBundle(), CFSTR("Winamp.q1"), CFSTR("json"), NULL);

    CFStringRef macPath = CFURLCopyFileSystemPath(appUrlRef, kCFURLPOSIXPathStyle);

    NSFileHandle * fileHandle = [NSFileHandle fileHandleForReadingAtPath:(__bridge NSString *)macPath];
    if (fileHandle) {
        NSError *err;
        NSData *data;
        if (@available(macOS 10.15, *)) {
            data = [fileHandle readDataToEndOfFileAndReturnError:&err];
        }
        else {
            data = [fileHandle readDataToEndOfFile];
            err = nil;
        }
        if (!err && data) {
            equalizer_presets = json_parse(data.bytes, data.length);

            if (equalizer_presets->type == json_object &&
                equalizer_presets->u.object.length == 2 &&
                strncmp(equalizer_presets->u.object.values[0].name, "type", equalizer_presets->u.object.values[0].name_length) == 0 &&
                equalizer_presets->u.object.values[0].value->type == json_string &&
                strncmp(equalizer_presets->u.object.values[0].value->u.string.ptr, "Winamp EQ library file v1.1", equalizer_presets->u.object.values[0].value->u.string.length ) == 0 &&
                strncmp(equalizer_presets->u.object.values[1].name, "presets", equalizer_presets->u.object.values[1].name_length) == 0 &&
                equalizer_presets->u.object.values[1].value->type == json_array)
            {
                // Got the array of presets
                NSMutableArray * array = [[NSMutableArray alloc] init];
                NSMutableDictionary * dict = [[NSMutableDictionary alloc] init];

                size_t count = equalizer_presets->u.object.values[1].value->u.array.length;
                json_value ** values = equalizer_presets->u.object.values[1].value->u.array.values;

                winamp_equalizer_items = _winamp_equalizer_items();

                const size_t winamp_object_minimum = [winamp_equalizer_items count];

                for (size_t i = 0; i < count; ++i) {
                    if (values[i]->type == json_object) {
                        NSMutableArray<NSString *> * extraGenres = [[NSMutableArray alloc] init];
                        size_t object_items = values[i]->u.object.length;
                        json_object_entry * object_entry = values[i]->u.object.values;
                        size_t requiredItemsPresent = 0;
                        if (object_items >= winamp_object_minimum) {
                            NSMutableDictionary * equalizerItem = [[NSMutableDictionary alloc] init];
                            for (size_t j = 0; j < object_items; ++j) {
                                NSString * key = [NSString stringWithUTF8String:object_entry[j].name];
                                NSInteger index = [winamp_equalizer_items indexOfObject:key];
                                if (index != NSNotFound) {
                                    if (index == 0 && object_entry[j].value->type == json_string) {
                                        NSString * name = [NSString stringWithUTF8String:object_entry[j].value->u.string.ptr];
                                        [equalizerItem setObject:name forKey:key];
                                        ++requiredItemsPresent;
                                    }
                                    else if (object_entry[j].value->type == json_integer) {
                                        int64_t value = object_entry[j].value->u.integer;
                                        float floatValue = ((value <= 64 && value >= 1) ? ((float)(value - 33) / 32.0 * 12.0) : 0.0);
                                        [equalizerItem setObject:[NSNumber numberWithFloat:floatValue] forKey:key];
                                        ++requiredItemsPresent;
                                    }
                                }
                                else if ([key isEqualToString:winamp_equalizer_extra_genres]) {
                                    // Process alternate genre matches
                                    if (object_entry[j].value->type == json_array) {
                                        size_t value_count = object_entry[j].value->u.array.length;
                                        json_value ** values = object_entry[j].value->u.array.values;
                                        for (size_t k = 0; k < value_count; ++k) {
                                            if (values[k]->type == json_string) {
                                                [extraGenres addObject:[NSString stringWithUTF8String:values[i]->u.string.ptr]];
                                            }
                                        }
                                    }
                                }
                            }

                            if (requiredItemsPresent == winamp_object_minimum) {
                                // Add the base item
                                NSDictionary *outItem = [NSDictionary dictionaryWithDictionary:equalizerItem];
                                [array addObject:outItem];
                                [dict setObject:outItem forKey:[outItem objectForKey:@"name"]];

                                // Add the alternate genres, if any
                                for (NSString * genre in extraGenres) {
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
    CFRelease(macPath);
    CFRelease(appUrlRef);
}

void equalizerApplyGenre(AudioUnit au, NSString *genre) {
    equalizerGenre = genre;
    if ([[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"GraphicEQtrackgenre"]) {
        loadPresets();
        
        NSDictionary * preset = [equalizer_presets_by_name objectForKey:genre];
        if (!preset) {
            // Find a match
            if (genre && ![genre isEqualToString:@""])
            {
                NSUInteger matchLength = 0;
                NSString *lowerCaseGenre = [genre lowercaseString];
                for (NSString *key in [equalizer_presets_by_name allKeys]) {
                    NSString *lowerCaseKey = [key lowercaseString];
                    if ([lowerCaseGenre containsString:lowerCaseKey]) {
                        if ([key length] > matchLength)
                        {
                            matchLength = [key length];
                            preset = [equalizer_presets_by_name objectForKey:key];
                        }
                    }
                }
            }
            
            if (!preset) {
                preset = [equalizer_presets_by_name objectForKey:equalizerDefaultGenre];
            }
        }
        if (preset) {
            NSInteger index = [equalizer_presets_processed indexOfObject:preset];
            [[[NSUserDefaultsController sharedUserDefaultsController] defaults] setInteger:index forKey:@"GraphicEQpreset"];
            
            equalizerApplyPreset(au, preset);
        }
    }
}

void equalizerLoadPreset(AudioUnit au) {
    NSInteger index = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] integerForKey:@"GraphicEQpreset"];
    if (index >= 0 && index < [equalizer_presets_processed count]) {
        NSDictionary * preset = [equalizer_presets_processed objectAtIndex:index];
        equalizerApplyPreset(au, preset);
    }
}

void equalizerApplyPreset(AudioUnit au, NSDictionary * preset) {
    if (au && preset) {
        AudioUnitParameterValue paramValue = 0;
        if (AudioUnitGetParameter(au, kGraphicEQParam_NumberOfBands, kAudioUnitScope_Global, 0, &paramValue))
            return;

        size_t numberOfBands = paramValue ? 31 : 10;

        if (numberOfBands == 31) {
            float presetValues[31];
            interpolateBandsTo31(presetValues, preset);
            
            for (unsigned int i = 0; i < 31; ++i)
            {
                AudioUnitSetParameter(au, i, kAudioUnitScope_Global, 0, presetValues[i], 0);
            }
        }
        else if (numberOfBands == 10) {
            float presetValues[10];
            interpolateBandsTo10(presetValues, preset);
            
            for (unsigned int i = 0; i < 10; ++i)
            {
                AudioUnitSetParameter(au, i, kAudioUnitScope_Global, 0, presetValues[i], 0);
            }
        }
    }
}

@interface AUPluginUI (Private)
- (BOOL)test_cocoa_view_support;
- (int)create_cocoa_view;
- (BOOL)plugin_class_valid:(Class)pluginClass;
@end

@implementation AUPluginUI
- (id) initWithSampler:(AudioUnit)_au bringToFront:(BOOL)front orWindowNumber:(NSInteger)window
{
    self = [super init];
    if (self)
    {
        au = _au;
        
        windowOpen = NO;
        
        cocoa_window = nil;
        au_view = nil;
        
        if ([self test_cocoa_view_support])
        {
            [self create_cocoa_view];
        }
        
        if (au_view)
        {
            cocoa_window = [[AUPluginWindow alloc] initWithAuView:au_view withAu:au bringToFront:front relativeToWindow:window];
            
            if (cocoa_window) {
                windowOpen = YES;
                
                [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowClosed:) name:NSWindowWillCloseNotification object:cocoa_window];
            }
        }
    }
    
    return self;
}

- (void) dealloc
{
    if (windowOpen)
    {
        [self windowClosed:nil];
    }
    [cocoa_window close];
    cocoa_window = nil;
    au_view = nil;
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (BOOL) isOpen
{
    return windowOpen;
}

- (void) bringToFront
{
    [cocoa_window orderFront:cocoa_window];
}

- (void)windowClosed:(NSNotification*)notification
{
    [cocoa_window saveFrameUsingName:@"GraphicEQposition"];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    windowOpen = NO;
}

- (BOOL)test_cocoa_view_support
{
    UInt32 dataSize   = 0;
    Boolean isWritable = 0;
    OSStatus err = AudioUnitGetPropertyInfo(au,
                                            kAudioUnitProperty_CocoaUI, kAudioUnitScope_Global,
                                            0, &dataSize, &isWritable);
    
    return dataSize > 0 && err == noErr;
}

- (BOOL) plugin_class_valid: (Class)pluginClass
{
    if([pluginClass conformsToProtocol: @protocol(AUCocoaUIBase)]) {
        if([pluginClass instancesRespondToSelector: @selector(interfaceVersion)] &&
           [pluginClass instancesRespondToSelector: @selector(uiViewForAudioUnit:withSize:)]) {
            return true;
        }
    }
    return false;
}

- (int)create_cocoa_view
{
    bool wasAbleToLoadCustomView = false;
    AudioUnitCocoaViewInfo* cocoaViewInfo = NULL;
    UInt32               numberOfClasses = 0;
    UInt32     dataSize;
    Boolean    isWritable;
    NSString*	    factoryClassName = 0;
    NSURL*	            CocoaViewBundlePath = NULL;
    
    OSStatus result = AudioUnitGetPropertyInfo (au,
                                                kAudioUnitProperty_CocoaUI,
                                                kAudioUnitScope_Global,
                                                0,
                                                &dataSize,
                                                &isWritable );
    
    numberOfClasses = (dataSize - sizeof(CFURLRef)) / sizeof(CFStringRef);
    
    // Does view have custom Cocoa UI?
    
    if ((result == noErr) && (numberOfClasses > 0) ) {
        
        cocoaViewInfo = (AudioUnitCocoaViewInfo *)malloc(dataSize);
        
        if(AudioUnitGetProperty(au,
                                kAudioUnitProperty_CocoaUI,
                                kAudioUnitScope_Global,
                                0,
                                cocoaViewInfo,
                                &dataSize) == noErr) {
            
            CocoaViewBundlePath	= (__bridge NSURL *)cocoaViewInfo->mCocoaAUViewBundleLocation;
            
            // we only take the first view in this example.
            factoryClassName	= (__bridge NSString *)cocoaViewInfo->mCocoaAUViewClass[0];
        } else {
            if (cocoaViewInfo != NULL) {
                free (cocoaViewInfo);
                cocoaViewInfo = NULL;
            }
        }
    }
    
    // [A] Show custom UI if view has it
    
    if (CocoaViewBundlePath && factoryClassName) {
        NSBundle *viewBundle  	= [NSBundle bundleWithPath:[CocoaViewBundlePath path]];
        
        if (viewBundle == NULL) {
            return -1;
        } else {
            Class factoryClass = [viewBundle classNamed:factoryClassName];
            if (!factoryClass) {
                return -1;
            }
            
            // make sure 'factoryClass' implements the AUCocoaUIBase protocol
            if (![self plugin_class_valid: factoryClass]) {
                return -1;
            }
            // make a factory
            id factory = [[factoryClass alloc] init];
            if (factory == NULL) {
                return -1;
            }
            
            // make a view
            au_view = [factory uiViewForAudioUnit:au withSize:NSZeroSize];
            
            // cleanup
            if (cocoaViewInfo) {
                UInt32 i;
                for (i = 0; i < numberOfClasses; i++)
                    CFRelease(cocoaViewInfo->mCocoaAUViewClass[i]);
                
                free (cocoaViewInfo);
            }
            wasAbleToLoadCustomView = true;
        }
    }
    
    if (!wasAbleToLoadCustomView) {
        // load generic Cocoa view
        au_view = [[AUGenericView alloc] initWithAudioUnit:au];
        [(AUGenericView *)au_view setShowsExpertParameters:1];
    }
    
    return 0;
}

@end

@implementation AUPluginWindow
- (id) initWithAuView:(NSView *)_auView withAu:(AudioUnit)au bringToFront:(BOOL)front relativeToWindow:(NSInteger)window
{
    NSRect  frame = [_auView frame];
    CGFloat req_width  = frame.size.width;
    CGFloat req_height = frame.size.height;
    //BOOL resizable  = [_auView autoresizingMask];

    self = [super initWithContentRect:NSMakeRect(0, 0, req_width, req_height + 32)
                            styleMask:(NSWindowStyleMaskTitled |
                                       NSWindowStyleMaskClosable)
                              backing:NSBackingStoreBuffered
                                defer:NO];
    if (self)
    {
        self->au = au;
        auView = _auView;
        
        [self setAutodisplay:YES];
        [self setOneShot:YES];

        [self setReleasedWhenClosed:NO];
        
        NSRect topRect = NSMakeRect(0, 0, req_width, 40);
        
        topView = [[NSView alloc] initWithFrame:topRect];
        
        NSRect topFrame = NSMakeRect(0, req_height, req_width, topRect.size.height);
        NSRect newFrame = NSMakeRect(0, 0, req_width, req_height);
        
        topRect = NSMakeRect(0, 0, req_width, req_height + topRect.size.height);
        
        splitView = [[NSSplitView alloc] initWithFrame:topRect];
        
        [splitView setSubviews:@[topView, auView]];
        
        [self setContentView:splitView];
        
        [topView setFrame:topFrame];
        [auView setFrame:newFrame];
        
        BOOL enabled = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"GraphicEQenable"];
        
        NSButton *button = [NSButton checkboxWithTitle:@"Enabled" target:self action:@selector(toggleEnable:)];
        [button setState:enabled ? NSControlStateValueOn : NSControlStateValueOff];
        
        NSRect buttonFrame = [button frame];
        buttonFrame.origin = NSMakePoint( 18, 7 );
        [button setFrame:buttonFrame];
        
        [topView addSubview:button];

        enabled = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"GraphicEQtrackgenre"];
        NSButton *trackButton = [NSButton checkboxWithTitle:@"Tracking genre tags" target:self action:@selector(toggleTracking:)];
        [trackButton setState:enabled ? NSControlStateValueOn : NSControlStateValueOff];
        
        NSRect trackButtonFrame = [trackButton frame];
        trackButtonFrame.origin = NSMakePoint(buttonFrame.origin.x + buttonFrame.size.width + 4, 7);
        [trackButton setFrame:trackButtonFrame];
        
        [topView addSubview:trackButton];
        
        loadPresets();
        
        NSRect popupFrame = NSMakeRect(req_width - 320, 0, 308, 26);
        
        presetButton = [[NSPopUpButton alloc] initWithFrame:popupFrame];
        [topView addSubview:presetButton];
        [presetButton setAction:@selector(changePreset:)];
        [presetButton setTarget:self];

        NSMutableArray * array = [[NSMutableArray alloc] init];
        for (NSDictionary * preset in equalizer_presets_processed)
        {
            [array addObject:[preset objectForKey:@"name"]];
        }
        [array addObject:@"Custom"];
        [presetButton addItemsWithTitles:array];
        
        NSInteger presetSelected = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] integerForKey:@"GraphicEQpreset"];
        if (presetSelected < 0 || presetSelected >= [equalizer_presets_processed count])
            presetSelected = [equalizer_presets_processed count];
        
        [presetButton selectItemAtIndex:presetSelected];
        equalizerLoadPreset(au);
        
        NSTextField * presetLabel = [NSTextField labelWithString:@"Preset:"];
        
        NSRect labelFrame = [presetLabel frame];
        labelFrame.origin = NSMakePoint(popupFrame.origin.x - labelFrame.size.width - 2, 7);
        [presetLabel setFrame:labelFrame];
        [topView addSubview:presetLabel];
        
        [splitView adjustSubviews];
        
        [splitView setDelegate:self];
        
        [self setFrameUsingName:@"GraphicEQposition"];

        AUListenerCreateWithDispatchQueue(&listenerRef, 0.25, dispatch_get_main_queue(), ^void(void *inObject, const AudioUnitParameter *inParameter, AudioUnitParameterValue inValue) {
            AUPluginWindow * _self = (__bridge AUPluginWindow *) inObject;
            
            if (inParameter->mParameterID >= 0 && inParameter->mParameterID <= 31) {
                [_self->presetButton selectItemAtIndex:[equalizer_presets_processed count]];
            }
            else if (inParameter->mParameterID == kGraphicEQParam_NumberOfBands) {
                [self changePreset:self->presetButton];
            }
        });

        AudioUnitParameter param;
        
        param.mAudioUnit = au;
        param.mElement = 0;
        param.mScope = kAudioUnitScope_Global;
        
        for (unsigned int i = 0; i < 31; ++i) {
            param.mParameterID = i;
            AUListenerAddParameter(listenerRef, (__bridge void *)self, &param);
        }
        
        param.mParameterID = kGraphicEQParam_NumberOfBands;
        AUListenerAddParameter(listenerRef, (__bridge void *)self, &param);
        
        [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.GraphicEQpreset" options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial) context:nil];
        
        // Time to hack the auView
        NSButton * flattenButton = nil;
        NSPopUpButton * bandsButton = nil;
        NSArray * views = [auView subviews];
        
        for (NSView * view in views) {
            if ([view isKindOfClass:[NSPopUpButton class]]) {
                NSPopUpButton * popupButton = (NSPopUpButton *) view;
                bandsButton = popupButton;
            }
            else if ([view isKindOfClass:[NSButton class]]) {
                NSButton * button = (NSButton *) view;
                flattenButton = button;
            }
        }
        
        [flattenButton setHidden:YES];
        
        // Hacking done, showing window
        
        if (front)
        {
            [self orderFront:self];
        }
        else
            [self orderWindow:NSWindowBelow relativeTo:window];
    }
    
    return self;
}

- (void) dealloc
{
    if (listenerRef) {
        AUListenerDispose(listenerRef);
    }
    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.GraphicEQpreset" context:nil];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if ([keyPath isEqualToString:@"values.GraphicEQpreset"]) {
        NSInteger index = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] integerForKey:@"GraphicEQpreset"];
        
        if (index < 0 || index > [equalizer_presets_processed count])
            index = [equalizer_presets_processed count];
        
        NSInteger selectedIndex = [presetButton indexOfSelectedItem];
        
        // Don't want to get into an observe/modify loop here
        if (selectedIndex != index)
        {
            [presetButton selectItemAtIndex:index];
        
            [self changePreset:presetButton];
        }
    }
}

- (NSRect)splitView:(NSSplitView *)splitView effectiveRect:(NSRect)proposedEffectiveRect forDrawnRect:(NSRect)drawnRect ofDividerAtIndex:(NSInteger)dividerIndex
{
    return NSZeroRect;
}

- (void) toggleEnable:(id)sender
{
    BOOL enabled = [sender state] == NSControlStateValueOn;
    
    [[[NSUserDefaultsController sharedUserDefaultsController] defaults] setBool:enabled forKey:@"GraphicEQenable"];
}

- (void) toggleTracking:(id)sender
{
    BOOL enabled = [sender state] == NSControlStateValueOn;
    
    [[[NSUserDefaultsController sharedUserDefaultsController] defaults] setBool:enabled forKey:@"GraphicEQtrackgenre"];
    
    equalizerApplyGenre(au, equalizerGenre);
    
    [self changePreset:presetButton];
}

- (void) changePreset:(id)sender
{
    NSInteger index = [sender indexOfSelectedItem];
    
    if (index < [equalizer_presets_processed count])
    {
        NSDictionary * preset = [equalizer_presets_processed objectAtIndex:index];
        
        equalizerApplyPreset(au, preset);
        
        NSEvent * event = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDown location:NSMakePoint(0, 0) modifierFlags:0 timestamp:0 windowNumber:[self windowNumber] context:nil eventNumber:0 clickCount:1 pressure:1.0];

        [auView mouseDown:event];
        [auView mouseUp:event];
        
        [[[NSUserDefaultsController sharedUserDefaultsController] defaults] setInteger:index forKey:@"GraphicEQpreset"];
    }
    else
    {
        [[[NSUserDefaultsController sharedUserDefaultsController] defaults] setInteger:-1 forKey:@"GraphicEQpreset"];
    }
}

@end

