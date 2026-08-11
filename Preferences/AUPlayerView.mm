//
//  AUPlayerView.mm
//  MIDI
//
//  Created by Christopher Snowhill on 1/29/16.
//  Copyright © 2016-2022 Christopher Snowhill. All rights reserved.
//

#import <AudioUnit/AUCocoaUIView.h>
#import <CoreAudioKit/AUGenericView.h>

/* Where requestViewControllerWithCompletionHandler: lives -- it is a category
 * on AUAudioUnit, not part of AudioToolbox's own declaration of the class. */
#import <CoreAudioKit/AUViewController.h>

#import "AUPlayerView.h"

@interface NSWindowDeleter : NSWindow<NSWindowDelegate> {
	AUPluginUI *parent;
}

- (void)setParentObject:(AUPluginUI *)object;
@end

@implementation NSWindowDeleter

- (void)setParentObject:(AUPluginUI *)object {
	parent = object;
	self.delegate = self;
}

/*
 * This discovery brought to you by much stupid rambling from the Google AI bot, and some
 * enlightening information: The UI window's lifetime is managed by the plugin. I cannot
 * make it self-releasing, because that will double-release it on the plugin and crash
 * the host. Instead, we wait for the close notification, time a deletion shortly after
 * it closes, and then return immediately.
 *
 * Should have figured when UIs were crashing on me, even my own plug-in's UI. Here, and
 * I thought it was a buggy plug-in crashing on me, when I wasn't managing lifetimes
 * correctly at all. Lesson learned.
 */
- (void)windowWillClose:(NSNotification *)notification {
	self.delegate = nil;
	__block AUPluginUI *blockParent = parent;
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
		delete blockParent;
	});
}
@end

AUPluginUI::AUPluginUI(NSString *_name, AVAudioUnit *_node)
: node(_node), name(_name), resizable(false), min_width(0), min_height(0), req_width(0), req_height(0) {
	cocoa_window = nil;
	au_view = nil;
	au_view_controller = nil;

	au = [node audioUnit];
	auUnit = [node AUAudioUnit];

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

	open();
}

AUPluginUI::~AUPluginUI() {
	save_settings();

	au_view_controller = nil;
	au_view = nil;

	AudioUnitUninitialize(au);

	/* No AudioComponentInstanceDispose: the AVAudioUnit owns the instance and
	 * disposing of it here as well would be a second release of the same thing. */
	au = NULL;
	auUnit = nil;
	node = nil;
}

void AUPluginUI::save_settings() {
	if(!au) return;

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
	if(err == noErr && outClassInfo) {
		NSDictionary *dict = (__bridge NSDictionary *)outClassInfo;
		[midiPluginSettings setObject:dict forKey:name];
		[[[NSUserDefaultsController sharedUserDefaultsController] defaults] setObject:midiPluginSettings forKey:@"midiPluginSettings"];
		dict = nil;
		CFRelease(outClassInfo);
	}
}

/* Asks the unit for its interface, whichever kind it has.
 *
 * An app extension's view lives in its own process and there is no view class
 * on this side to instantiate, so the only way to it is to ask the unit --
 * asynchronously, because the answer has to come back across a connection.
 * A v2 unit answers nil, and the Cocoa path below is what it wanted anyway. */
void AUPluginUI::open() {
	if(auUnit) {
		AUPluginUI *self_ = this;
		[auUnit requestViewControllerWithCompletionHandler:^(NSViewController *viewController) {
			dispatch_async(dispatch_get_main_queue(), ^{
				if(viewController && viewController.view) {
					self_->au_view_controller = viewController;
					self_->present(viewController.view);
				} else {
					self_->open_cocoa();
				}
			});
		}];
		return;
	}

	open_cocoa();
}

void AUPluginUI::open_cocoa() {
	if(test_cocoa_view_support()) {
		create_cocoa_view();
	}

	/* create_cocoa_view gives up rather than falling through when a unit
	 * advertises a Cocoa view whose bundle will not load. The generic view
	 * always works, and an unconfigurable plugin is worse than a plain one. */
	if(!au_view) {
		au_view = [[AUGenericView alloc] initWithAudioUnit:au];
		[(AUGenericView *)au_view setShowsExpertParameters:1];

		NSRect frame = [au_view frame];
		min_width = req_width = CGRectGetWidth(NSRectToCGRect(frame));
		min_height = req_height = CGRectGetHeight(NSRectToCGRect(frame));
	}

	if(!au_view) {
		/* Nothing to show, and nothing else will ever delete this. */
		delete this;
		return;
	}

	present(au_view);
}

void AUPluginUI::present(NSView *view) {
	NSRect frame = [view frame];
	req_width = CGRectGetWidth(NSRectToCGRect(frame));
	req_height = CGRectGetHeight(NSRectToCGRect(frame));
	if(req_width <= 0) req_width = 480;
	if(req_height <= 0) req_height = 320;

	/* Strong while it is being built: the member is weak, so without this the
	 * window would be released the moment it was assigned. Ordering it in is
	 * what hands ownership to AppKit, and closing it is what gives it up
	 * again -- which deallocates it, which deletes this object. */
	NSWindowDeleter *window = [[NSWindowDeleter alloc] initWithContentRect:NSMakeRect(0, 0, req_width, req_height)
	                                                            styleMask:(NSWindowStyleMaskTitled |
	                                                                       NSWindowStyleMaskClosable)
	                                                              backing:NSBackingStoreBuffered
	                                                                defer:NO];

	[window setParentObject:this];
	[window setReleasedWhenClosed:NO];
	[window setTitle:name ? name : @"AU Plug-in"];
	[window setContentView:view];
	[window center];
	[window orderFront:window];

	cocoa_window = window;
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
			free(cocoaViewInfo);
			cocoaViewInfo = NULL;
		}
	}

	// [A] Show custom UI if view has it

	if(CocoaViewBundlePath && factoryClassName) {
		NSBundle *viewBundle = [NSBundle bundleWithPath:[CocoaViewBundlePath path]];

		if(viewBundle) {
			Class factoryClass = [viewBundle classNamed:factoryClassName];

			// make sure 'factoryClass' implements the AUCocoaUIBase protocol
			if(factoryClass && plugin_class_valid(factoryClass)) {
				// make a factory
				id factory = [factoryClass new];
				if(factory) {
					// make a view
					au_view = [factory uiViewForAudioUnit:au withSize:NSZeroSize];
					wasAbleToLoadCustomView = (au_view != nil);
				}
			}
		}
	}

	/* Freed on every path, and the bundle URL released with the class names.
	 * Both used to escape whenever the custom view did not load. */
	if(cocoaViewInfo) {
		for(UInt32 i = 0; i < numberOfClasses; i++)
			CFRelease(cocoaViewInfo->mCocoaAUViewClass[i]);
		if(cocoaViewInfo->mCocoaAUViewBundleLocation)
			CFRelease(cocoaViewInfo->mCocoaAUViewBundleLocation);
		free(cocoaViewInfo);
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
