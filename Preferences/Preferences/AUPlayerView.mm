//
//  AUPlayerView.mm
//  MIDI
//
//  Created by Christopher Snowhill on 1/29/16.
//  Copyright © 2016-2022 Christopher Snowhill. All rights reserved.
//

#import <AudioUnit/AUCocoaUIView.h>
#import <CoreAudioKit/AUGenericView.h>

#import "AUPlayerView.h"

@interface NSWindowDeleter : NSWindow {
	AUPluginUI *parent;
}

- (void)setParentObject:(AUPluginUI *)object;
- (void)dealloc;
@end

@implementation NSWindowDeleter

- (void)setParentObject:(AUPluginUI *)object {
	parent = object;
}

- (void)dealloc {
	delete parent;
}
@end

AUPluginUI::AUPluginUI(NSString *_name, AudioUnit &_au)
: name(_name), au(_au), mapped(false), resizable(false), min_width(0), min_height(0), req_width(0), req_height(0), alo_width(0), alo_height(0) {
	opened = false;
	cocoa_window = nil;
	au_view = nil;

	if(test_cocoa_view_support()) {
		create_cocoa_view();
	}

	if(au_view) {
		cocoa_window = [[NSWindowDeleter alloc] initWithContentRect:NSMakeRect(0, 0, req_width, req_height)
														  styleMask:(NSWindowStyleMaskTitled |
																	 NSWindowStyleMaskClosable)
															backing:NSBackingStoreBuffered
															  defer:NO];

		[cocoa_window setParentObject:this];

		[cocoa_window setAutodisplay:YES];
		[cocoa_window setTitle:@"AU Plug-in"];
		[cocoa_window setOneShot:YES];

		[cocoa_window setContentView:au_view];

		NSDictionary *midiPluginSettings = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"midiPluginSettings"];
		if(midiPluginSettings) {
			NSDictionary *theSettings = [midiPluginSettings objectForKey:name];
			if(theSettings) {
				CFDictionaryRef cdict = (__bridge CFDictionaryRef)theSettings;
				AudioUnitSetProperty(au,
									 kAudioUnitProperty_ClassInfo,
									 kAudioUnitScope_Global,
									 0,
									 &cdict,
									 sizeof(cdict));
			}
		}

		[cocoa_window orderFront:cocoa_window];

		opened = true;
	}
}

AUPluginUI::~AUPluginUI() {
	if(opened) {
		NSMutableDictionary *midiPluginSettings = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"midiPluginSettings"] mutableCopy];
		if(!midiPluginSettings) {
			midiPluginSettings = [NSMutableDictionary new];
		}
		CFDictionaryRef outClassInfo = nil;
		UInt32 objectSize = sizeof(outClassInfo);
		OSErr err = AudioUnitGetProperty(au,
										 kAudioUnitProperty_ClassInfo,
										 kAudioUnitScope_Global,
										 0,
										 &outClassInfo,
										 &objectSize);
		if(err == noErr) {
			NSDictionary *dict = (__bridge NSDictionary *)outClassInfo;
			[midiPluginSettings setObject:dict forKey:name];
			[[[NSUserDefaultsController sharedUserDefaultsController] defaults] setObject:midiPluginSettings forKey:@"midiPluginSettings"];
			dict = nil;
			CFRelease(outClassInfo);
		}
	}

	AudioUnitUninitialize(au);
	AudioComponentInstanceDispose(au);
	au = NULL;
}

bool AUPluginUI::test_cocoa_view_support() {
	UInt32 dataSize = 0;
	Boolean isWritable = 0;
	OSStatus err = AudioUnitGetPropertyInfo(au,
	                                        kAudioUnitProperty_CocoaUI, kAudioUnitScope_Global,
	                                        0, &dataSize, &isWritable);

	return dataSize > 0 && err == noErr;
}

bool AUPluginUI::plugin_class_valid(Class pluginClass) {
	if([pluginClass conformsToProtocol:@protocol(AUCocoaUIBase)]) {
		if([pluginClass instancesRespondToSelector:@selector(interfaceVersion)] &&
		   [pluginClass instancesRespondToSelector:@selector(uiViewForAudioUnit:withSize:)]) {
			return true;
		}
	}
	return false;
}

int AUPluginUI::create_cocoa_view() {
	bool wasAbleToLoadCustomView = false;
	AudioUnitCocoaViewInfo *cocoaViewInfo = NULL;
	UInt32 numberOfClasses = 0;
	UInt32 dataSize;
	Boolean isWritable;
	NSString *factoryClassName = 0;
	NSURL *CocoaViewBundlePath = NULL;

	OSStatus result = AudioUnitGetPropertyInfo(au,
	                                           kAudioUnitProperty_CocoaUI,
	                                           kAudioUnitScope_Global,
	                                           0,
	                                           &dataSize,
	                                           &isWritable);

	numberOfClasses = (dataSize - sizeof(CFURLRef)) / sizeof(CFStringRef);

	// Does view have custom Cocoa UI?

	if((result == noErr) && (numberOfClasses > 0)) {
		cocoaViewInfo = (AudioUnitCocoaViewInfo *)malloc(dataSize);

		if(AudioUnitGetProperty(au,
		                        kAudioUnitProperty_CocoaUI,
		                        kAudioUnitScope_Global,
		                        0,
		                        cocoaViewInfo,
		                        &dataSize) == noErr) {
			CocoaViewBundlePath = (__bridge NSURL *)cocoaViewInfo->mCocoaAUViewBundleLocation;

			// we only take the first view in this example.
			factoryClassName = (__bridge NSString *)cocoaViewInfo->mCocoaAUViewClass[0];
		} else {
			if(cocoaViewInfo != NULL) {
				free(cocoaViewInfo);
				cocoaViewInfo = NULL;
			}
		}
	}

	// [A] Show custom UI if view has it

	if(CocoaViewBundlePath && factoryClassName) {
		NSBundle *viewBundle = [NSBundle bundleWithPath:[CocoaViewBundlePath path]];

		if(viewBundle == NULL) {
			return -1;
		} else {
			Class factoryClass = [viewBundle classNamed:factoryClassName];
			if(!factoryClass) {
				return -1;
			}

			// make sure 'factoryClass' implements the AUCocoaUIBase protocol
			if(!plugin_class_valid(factoryClass)) {
				return -1;
			}
			// make a factory
			id factory = [factoryClass new];
			if(factory == NULL) {
				return -1;
			}

			// make a view
			au_view = [factory uiViewForAudioUnit:au withSize:NSZeroSize];

			// cleanup
			if(cocoaViewInfo) {
				UInt32 i;
				for(i = 0; i < numberOfClasses; i++)
					CFRelease(cocoaViewInfo->mCocoaAUViewClass[i]);

				free(cocoaViewInfo);
			}
			wasAbleToLoadCustomView = true;
		}
	}

	if(!wasAbleToLoadCustomView) {
		// load generic Cocoa view
		au_view = [[AUGenericView alloc] initWithAudioUnit:au];
		[(AUGenericView *)au_view setShowsExpertParameters:1];
	}

	// Get the initial size of the new AU View's frame
	NSRect frame = [au_view frame];
	min_width = req_width = CGRectGetWidth(NSRectToCGRect(frame));
	min_height = req_height = CGRectGetHeight(NSRectToCGRect(frame));
	resizable = [au_view autoresizingMask];

	return 0;
}

bool AUPluginUI::window_opened() const {
	return opened;
}
