///**********************************************************************************************************************************
///  GCWindowMenu.h
///  GCDrawKitUI
///
///  Created by graham on 27/03/07.
///  Released under the Creative Commons license 2006 Apptree.net.
///
/// 
///  This work is licensed under the Creative Commons Attribution-ShareAlike 2.5 License.
///  To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/2.5/ or send a letter to
///  Creative Commons, 543 Howard Street, 5th Floor, San Francisco, California, 94105, USA.
///
///**********************************************************************************************************************************

#import <Cocoa/Cocoa.h>


@interface GCWindowMenu : NSWindow
{
	NSView*			_mainView;
	BOOL			_passFirstClick;
	BOOL			_oneShotTracking;
}

+ (GCWindowMenu*)	windowMenu;
+ (GCWindowMenu*)   windowMenuWithContentView:(NSView*) view;

- (void)			popUpAtPoint:(NSPoint) p withEvent:(NSEvent*) event;
- (void)			popUpWithEvent:(NSEvent*) event;

- (void)			setMainView:(NSView*) aView sizeToFit:(BOOL) stf;
- (NSView*)			mainView;

- (void)			setMainViewWantsFirstClick:(BOOL) firstClick;
- (void)			setShouldCloseWhenViewTrackingReturns:(BOOL) cmup;

@end


@interface NSEvent (GCAdditions)

- (BOOL)			isMouseEventType;

@end