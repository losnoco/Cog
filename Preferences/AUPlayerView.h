//
//  AUPlayerView.h
//  MIDI
//
//  Created by Christopher Snowhill on 1/29/16.
//  Copyright © 2016-2022 Christopher Snowhill. All rights reserved.
//

#ifndef __AUPlayerView_h__
#define __AUPlayerView_h__

#include <string>
#include <vector>

#include <AVFoundation/AVFoundation.h>
#include <AppKit/AppKit.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioUnit/AudioUnitCarbonView.h>

@class NSWindowDeleter;

/* The settings window for one MIDI plugin.
 *
 * Owns the audio unit it is configuring, and saves its class info back to the
 * preferences when the window closes. Both kinds of unit end up here:
 *
 *   v2   a Cocoa view fetched through kAudioUnitProperty_CocoaUI, or the
 *        generic parameter view when the unit has none of its own.
 *   v3   a view controller the unit hands over asynchronously through
 *        requestViewControllerWithCompletionHandler:, which is the only way to
 *        get at an app extension's interface -- it lives in another process
 *        and there is no Cocoa view class here to load.
 *
 * The unit arrives as an AVAudioUnit rather than a bare AudioUnit because that
 * is the only thing that holds both faces of one instance: the v2 handle the
 * property calls need, and the AUAudioUnit that vends the v3 view. */
class AUPluginUI {
	public:
	AUPluginUI(NSString *name, AVAudioUnit *node);
	~AUPluginUI();

	private:
	/* Owns the instance. Releasing it is what disposes of the audio unit, so
	 * nothing here calls AudioComponentInstanceDispose itself. */
	AVAudioUnit *node;
	AudioUnit au;
	AUAudioUnit *auUnit;

	NSString *name;

	bool resizable;
	int min_width;
	int min_height;
	int req_width;
	int req_height;

	/* Cocoa */

	/* Weak on purpose. The window's dealloc is what deletes this object, so a
	 * strong reference here would be a cycle: the window could not deallocate
	 * while this held it, and this is not deleted until it does. Once the
	 * window has been ordered in, AppKit owns it. */
	__weak NSWindowDeleter *cocoa_window;

	NSView *au_view;

	/* Held for a v3 unit: the view belongs to it and dies with it. */
	NSViewController *au_view_controller;


	void open();
	void open_cocoa();
	void present(NSView *view);
	void save_settings();

	bool test_cocoa_view_support();
	int create_cocoa_view();

	bool plugin_class_valid(Class pluginClass);
};

#endif
