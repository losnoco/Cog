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

#include <AppKit/AppKit.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioUnit/AudioUnitCarbonView.h>
#include <Carbon/Carbon.h>

@class NSWindowDeleter;

class AUPluginUI {
	public:
	AUPluginUI(NSString *name, AudioUnit& sampler);
	~AUPluginUI();

	bool window_opened() const;

	private:
	AudioUnit& au;
	NSString* name;
	int prefheight;
	int prefwidth;

	bool opened;
	bool mapped;
	bool resizable;
	int min_width;
	int min_height;
	int req_width;
	int req_height;
	int alo_width;
	int alo_height;

	/* Cocoa */

	NSWindowDeleter* cocoa_window;
	NSView* au_view;
	NSRect last_au_frame;

	bool test_cocoa_view_support();
	int create_cocoa_view();

	bool plugin_class_valid(Class pluginClass);
};

#endif
