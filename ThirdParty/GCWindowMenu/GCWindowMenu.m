///**********************************************************************************************************************************
///  GCWindowMenu.m
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

#import "GCWindowMenu.h"
#import "GCOneShotEffectTimer.h"

@interface GCWindowMenu (Private)

+ (void)		popUpWindowMenu:(GCWindowMenu*) menu withEvent:(NSEvent*) event;
+ (void)		popUpWindowMenu:(GCWindowMenu*) menu atPoint:(NSPoint) loc withEvent:(NSEvent*) event;

- (void)		trackWithEvent:(NSEvent*) event;
- (NSEvent*)	transmogrify:(NSEvent*) event;

@end


#define kGCDefaultWindowMenuSize  (NSMakeRect(0, 0, 100, 28 ))
#define kGCMenuContentInset			2


@implementation GCWindowMenu

///*********************************************************************************************************************
///
/// method:			popUpWindowMenu:withEvent:forView:
/// scope:			private class method
/// overrides:		
/// description:	pops up a custom popup menu, tracks it, then hides it again with a fadeout
/// 
/// parameters:		<menu> the custom popup window to display
///					<event> the event to start the display with (usually from a mouse down)
/// result:			none
///
/// notes:			the menu is positioned with its top, left point just to the left of, and slightly below, the
///					point given in the event
///
///********************************************************************************************************************

+ (void)	popUpWindowMenu:(GCWindowMenu*) menu withEvent:(NSEvent*) event
{
	NSPoint loc = [event locationInWindow];
	loc.x -= 10;
	loc.y -= 5;
	
	[self popUpWindowMenu:menu atPoint:loc withEvent:event];
}


///*********************************************************************************************************************
///
/// method:			popUpWindowMenu:atPoint:withEvent:forView:
/// scope:			private class method
/// overrides:		
/// description:	pops up a custom popup menu, tracks it, then hides it again with a fadeout
/// 
/// parameters:		<menu> the custom popup window to display
///					<loc> the location within the window at which to display the menu (top, left of menu)
///					<event> the event to start the display with (usually from a mouse down)
/// result:			none
///
/// notes:			
///
///********************************************************************************************************************

+ (void)	popUpWindowMenu:(GCWindowMenu*) menu atPoint:(NSPoint) loc withEvent:(NSEvent*) event
{
	if ( menu == nil )
		menu = [GCWindowMenu windowMenu];
	
	[menu retain];
	
	loc = [[event window] convertBaseToScreen:loc];
	[menu setFrameTopLeftPoint:loc];
	[[event window] addChildWindow:menu ordered:NSWindowAbove];
	
	// show the "menu"
	
	[menu orderFront:self];
	
	// track the menu (keeps control in its own event loop):

	[menu trackWithEvent:event];
	
	// all done, tear down - remove with a fade effect
	
	[GCOneShotEffectTimer oneShotWithTime:0.15 forDelegate:menu];
	[menu release];
}



///*********************************************************************************************************************
///
/// method:			windowMenu
/// scope:			public class method
/// overrides:		
/// description:	makes a window menu that can be popped up using the above methods.
/// 
/// parameters:		none
/// result:			a new poppable window menu
///
/// notes:			this method just makes an empy window with the default size. It's up to you to add some useful
///					content before displaying it
///
///********************************************************************************************************************

+ (GCWindowMenu*)	windowMenu
{
	GCWindowMenu* fi =  [[GCWindowMenu alloc]  initWithContentRect:NSZeroRect
												styleMask:NSBorderlessWindowMask
												backing:NSBackingStoreBuffered
												defer:YES];
	
	// note - because windows are all sent a -close message at quit time, set it
	// not to be released at that time, otherwise the release from the autorelease pool
	// will cause a crash due to the stale reference

	[fi setReleasedWhenClosed:NO];	// **** important!! ****
	return [fi autorelease];
}


///*********************************************************************************************************************
///
/// method:			windowMenuWithContentView:
/// scope:			public class method
/// overrides:		
/// description:	makes a window menu that can be popped up using the above methods.
/// 
/// parameters:		<view> the view to display within the menu
/// result:			a new poppable window menu containing the given view
///
/// notes:			the window is sized to fit the frame of the view you pass.
///
///********************************************************************************************************************

+ (GCWindowMenu*)   windowMenuWithContentView:(NSView*) view
{
	GCWindowMenu* menu = [self windowMenu];
	
	[menu setMainView:view sizeToFit:YES];
	return menu;
}


///*********************************************************************************************************************
///
/// method:			popUpAtPoint:withEvent:forView:
/// scope:			public instance method
/// overrides:		
/// description:	pops up a custom popup menu, tracks it, then hides it again with a fadeout
/// 
/// parameters:		<p> the location within the window at which to display the menu (top, left of menu)
///					<event> the event to start the display with (usually from a mouse down in some view)
/// result:			none
///
/// notes:			
///
///********************************************************************************************************************

- (void)			popUpAtPoint:(NSPoint) p withEvent:(NSEvent*) event
{
	[[self class] popUpWindowMenu:self atPoint:p withEvent:event];
}


///*********************************************************************************************************************
///
/// method:			popUpWithEvent:forView:
/// scope:			public instance method
/// overrides:		
/// description:	pops up a custom popup menu, tracks it, then hides it again with a fadeout
/// 
/// parameters:		<event> the event to start the display with (usually from a mouse down in some view)
/// result:			none
///
/// notes:			
///
///********************************************************************************************************************

- (void)			popUpWithEvent:(NSEvent*) event
{
	[[self class] popUpWindowMenu:self withEvent:event];
}



///*********************************************************************************************************************
///
/// method:			initWithContentRect:styleMask:backing:defer:
/// scope:			public instance method
/// overrides:		NSWindow
/// description:	designated initializer.
/// 
/// parameters:		<> see NSWindow
/// result:			the window
///
/// notes:			
///
///********************************************************************************************************************

- (id)	initWithContentRect:(NSRect) contentRect
		styleMask:(unsigned int) styleMask
		backing:(NSBackingStoreType) bufferingType
		defer:(BOOL) deferCreation
{
	if ((self = [super initWithContentRect:contentRect
						styleMask:styleMask
						backing:bufferingType
						defer:deferCreation]) != nil )
	{
		[self setLevel:NSPopUpMenuWindowLevel];
		[self setHasShadow:YES];
		[self setAlphaValue:0.95];
		[self setReleasedWhenClosed:YES];
		[self setFrame:kGCDefaultWindowMenuSize display:NO];
		
		_mainView = nil;
		_passFirstClick = YES;
		_oneShotTracking = YES;
	}
	
	return self;
}



///*********************************************************************************************************************
///
/// method:			trackWithEvent:
/// scope:			public instance method
/// overrides:		
/// description:	track the mouse in the menu
/// 
/// parameters:		<event> the initial starting event (will usually be a mouse down)
/// result:			none
///
/// notes:			tracking calls the main view's usual mouseDown/dragged/up methods, and tries to do so as compatibly
///					as possible with the usual view behaviours.
///
///********************************************************************************************************************

- (void)			trackWithEvent:(NSEvent*) event
{
	// tracks the "menu" by keeping control until a mouse up (or down, if menu 'clicked' into being)
	
	NSLog(@"starting tracking; initial event = %@", event);
		
	//[NSEvent startPeriodicEventsAfterDelay:1.0 withPeriod:0.1];
	
	NSTimeInterval startTime = [event timestamp];
	
	[self setAcceptsMouseMovedEvents:YES];
	
	if ( _passFirstClick )
	{
		[[self mainView] mouseDown:[self transmogrify:event]];
	
		// the view may have trapped the mouse down and implemented its own tracking.
		// Standard NSControls do that for example. In that case we don't want to track
		// ourselves, so need to detect that and abort. 
		
		if ([[self currentEvent] timestamp] - startTime > 0.25 )
			return;
	}
	
	NSEvent*		theEvent;
	BOOL			keepOn = YES;
	unsigned int	mask;
	BOOL			invertedTracking = NO;
	
	mask = NSLeftMouseUpMask | NSLeftMouseDraggedMask |
			NSRightMouseUpMask | NSRightMouseDraggedMask |
			NSAppKitDefinedMask | NSFlagsChangedMask |
			NSScrollWheelMask;
 
	while (keepOn)
	{
		theEvent = [self transmogrify:[self nextEventMatchingMask:mask]];

		switch ([theEvent type])
		{
			case NSMouseMovedMask:
				[[self mainView] mouseMoved:theEvent];
				break;
				
			case NSRightMouseDragged:
			case NSLeftMouseDragged:
				[[self mainView] mouseDragged:theEvent];
				break;
			
			case NSRightMouseUp:
			case NSLeftMouseUp:
				// if this is within a very short time of the mousedown, leave the menu up but track it
				// using mouse moved and mouse down to end.
				
				if ([theEvent timestamp] - startTime < 0.25 || !_passFirstClick )
				{
					invertedTracking = YES;
					mask |= ( NSLeftMouseDownMask | NSRightMouseDownMask | NSMouseMovedMask );
				}
				else
				{
					[[self mainView] mouseUp:theEvent];
					keepOn = NO;
				}
				break;
				
			case NSRightMouseDown:
			case NSLeftMouseDown:
				if ( ! NSPointInRect([theEvent locationInWindow], [[self mainView] frame]))
					keepOn = NO;
				else
				{
					[[self mainView] mouseDown:theEvent];
					
					if ( _oneShotTracking )
						keepOn = NO;
				}
				break;

			case NSPeriodic:
				break;
				
			case NSFlagsChanged:
				[[self mainView] flagsChanged:theEvent];
				break;
				
			case NSAppKitDefined:
				if([theEvent subtype] == NSApplicationDeactivatedEventType )
					keepOn = NO;
				break;
				
			case NSScrollWheel:
				[[self mainView] scrollWheel:theEvent];
				break;

			default:
				/* Ignore any other kind of event. */
				break;
		}
	}
	
	[self discardEventsMatchingMask:NSAnyEventMask beforeEvent:theEvent];
		
	//[NSEvent stopPeriodicEvents];
	NSLog(@"tracking ended");
}



///*********************************************************************************************************************
///
/// method:			transmogrify:
/// scope:			private instance method
/// overrides:		
/// description:	convert the event to the local window if necessary
/// 
/// parameters:		<event> an event
/// result:			the same event, or a modified version
///
/// notes:			ensures that events received while tracking are always targetted at the right window
///
///********************************************************************************************************************

- (NSEvent*)		transmogrify:(NSEvent*) event
{
	if(([event window] != self) && [event isMouseEventType])
	{
		NSPoint glob = [[event window] convertBaseToScreen:[event locationInWindow]];

		return [NSEvent mouseEventWithType:	[event type]
						location:			[self convertScreenToBase:glob]
						modifierFlags:		[event modifierFlags]
						timestamp:			[event timestamp]
						windowNumber:		[self windowNumber]
						context:			[event context]
						eventNumber:		[event eventNumber]
						clickCount:			[event clickCount]
						pressure:			[event pressure]];
	}
	else
		return event;
}


///*********************************************************************************************************************
///
/// method:			canBecomeMainWindow
/// scope:			public instance method
/// overrides:		NSWindow
/// description:	
/// 
/// parameters:		none
/// result:			return YES
///
/// notes:			
///
///********************************************************************************************************************

- (BOOL)			canBecomeMainWindow
{
	return NO;
}


///*********************************************************************************************************************
///
/// method:			setMainView:sizeToFit:
/// scope:			public instance method
/// overrides:		
/// description:	sets the pop-up window's content to the given view, and optionally sizes the window to fit
/// 
/// parameters:		<aView> any view already created to be displayed in the menu
///					<stf> if YES, window is sized to the view's frame. If NO, the window size is not changed
/// result:			none
///
/// notes:			main view is additionally retained so outlet from NIB may be directly passed in
///
///********************************************************************************************************************

- (void)			setMainView:(NSView*) aView sizeToFit:(BOOL) stf
{
	[aView retain];
	[_mainView release];
	_mainView = aView;
	
	// add as a subview which retains it as well
	
	[[self contentView] addSubview:aView];
	
	// if stf, position the view at top, left corner of the window and
	// make the window the size of the view
	
	if ( stf )
	{
		NSRect fr = [self frameRectForContentRect:NSInsetRect( [aView frame], -kGCMenuContentInset, -kGCMenuContentInset )];
	
		fr.origin = NSZeroPoint;
		[aView setFrameOrigin:NSMakePoint( kGCMenuContentInset, kGCMenuContentInset )];
		[self setFrame:fr display:YES];
	}
	
	[_mainView setNeedsDisplay:YES];
	
	// if the view added is an NSControl, set first click to NO by default
	
	if ([aView isKindOfClass:[NSControl class]])
		[self setMainViewWantsFirstClick:NO];
}


///*********************************************************************************************************************
///
/// method:			mainView
/// scope:			public instance method
/// overrides:		
/// description:	get the main view
/// 
/// parameters:		none
/// result:			the main view
///
/// notes:			
///
///********************************************************************************************************************

- (NSView*)			mainView
{
	return _mainView;
}


///*********************************************************************************************************************
///
/// method:			setMainViewWantsFirstClick:
/// scope:			public instance method
/// overrides:		
/// description:	sets whether the main view should receive a mouse down on entry to the tracking loop
/// 
/// parameters:		<firstClick> YES to get the first click
/// result:			none
///
/// notes:			normally should be YES (the default). However views such as NSControl derivatives that implement
///					their own tracking should set NO. If NO, the popup can only be operated by clicking to open, then
///					clicking and dragging within - the continuous click to open, drag through and release operation
///					wont work because the control doesn't get a mouse down to start with.
///
///********************************************************************************************************************

- (void)			setMainViewWantsFirstClick:(BOOL) firstClick
{
	_passFirstClick = firstClick;
}


///*********************************************************************************************************************
///
/// method:			setShouldCloseWhenViewTrackingReturns:
/// scope:			public instance method
/// overrides:		
/// description:	sets whether popup should close or remain visible after main view completes its own tracking
/// 
/// parameters:		<cmup> YES close on return from view tracking, NO to remain visble
/// result:			none
///
/// notes:			this affects tracking with views that implement their own tracking, such as NSControl. If YES, you
///					get one shot at the control - after operating it, it will be hidden. If NO, the control may be
///					changed as often as you want but you must manually click outside the menu to close it.
///
///********************************************************************************************************************

- (void)			setShouldCloseWhenViewTrackingReturns:(BOOL) cmup
{
	_oneShotTracking = cmup;
}


///*********************************************************************************************************************
///
/// method:			oneShotHasReachedInverse:
/// scope:			public instance method
/// overrides:		NSObject (OneShotDelegate)
/// description:	callback from fade out effect
/// 
/// parameters:		<relpos> goes from 1..0
/// result:			none
///
/// notes:			
///
///********************************************************************************************************************

- (void)		oneShotHasReachedInverse:(float) relpos
{
	[self setAlphaValue:relpos];
}


///*********************************************************************************************************************
///
/// method:			oneShotComplete
/// scope:			public instance method
/// overrides:		NSObject (OneShotDelegate)
/// description:	callback from fade out effect
/// 
/// parameters:		none
/// result:			none
///
/// notes:			removes the window from screen - oneshot will then release it
///
///********************************************************************************************************************

- (void)		oneShotComplete
{
	[[self parentWindow] removeChildWindow:self];
	[self orderOut:self];
}





@end


@implementation NSEvent (GCAdditions)

///*********************************************************************************************************************
///
/// method:			isMouseEventType:
/// scope:			public instance method
/// overrides:		
/// description:	checks event to see if it's any mouse event
/// 
/// parameters:		none
/// result:			YES if the event is a mouse event of any kind
///
/// notes:			
///
///********************************************************************************************************************

- (BOOL)	isMouseEventType
{
	// returns YES if type is any mouse type
	
	NSEventType t = [self type];
	
	return ( t == NSLeftMouseDown		||
			 t == NSLeftMouseUp			||
			 t == NSRightMouseDown		||
			 t == NSRightMouseUp		||
			 t == NSLeftMouseDragged	||
			 t == NSRightMouseDragged   ||
			 t == NSOtherMouseDown		||
			 t == NSOtherMouseUp		||
			 t == NSOtherMouseDragged );
}

@end
