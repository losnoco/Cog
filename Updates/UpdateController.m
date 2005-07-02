//
//  UpdateController.m
//  Cog
//
//  Created by Vincent Spader on 3/26/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "UpdateController.h"

@implementation UpdateController

- (void)checkForUpdate
{
	[okayButton setEnabled:NO];
	[checkingIndicator startAnimation:self];

	[statusView setStringValue:@"Checking for update..."];
	
	macPAD = [[MacPADSocket alloc] init];
	[macPAD setDelegate:self];

	[macPAD performCheckWithURL:[NSURL URLWithString:@"http://cogosx.sourceforge.net/Cog.plist"]];
}

- (void)updateDisplay:(MacPADSocket *)socket info:(NSDictionary *)info
{
	int result;
	result = [[info objectForKey:MacPADErrorCode] intValue];
	if (result == kMacPADResultNewVersion) //New version available
	{
		[statusView setStringValue:@"Update available!"];
		updateAvailable = YES;
	}
	else if (result == kMacPADResultNoNewVersion) //No new version available
	{
		[statusView setStringValue:@"No updates available."];
		updateAvailable = NO;
	}
	else //Error connecting to update server
	{
		[statusView setStringValue:@"Error connecting to update server."];
		updateAvailable = NO;
	}
	
	[self setDownloadURL:[macPAD productDownloadURL]];
	
    [macPAD release];
    macPAD = nil;
	
	[checkingIndicator stopAnimation:self];
	[okayButton setEnabled:YES];
	
//	DBLog(@"THINGS: %i %i", updateAvailable, checkInBackground);
	if (updateAvailable == YES && checkInBackground == YES)
	{
//		DBLog(@"SHOW THE GD WINDOW");
		[updateWindow makeKeyAndOrderFront: self];
	}
}

- (void)macPADErrorOccurred:(NSNotification *)aNotification
{
	DBLog(@"Update error occurred");
    [self updateDisplay:[aNotification object] info:[aNotification userInfo]];	
}

- (void)macPADCheckFinished:(NSNotification *)aNotification
{
//	DBLog(@"CHECK FINISHED");
    [self updateDisplay:[aNotification object] info:[aNotification userInfo]];
}

- (IBAction)openUpdateWindow:(id)sender
{
	[updateWindow makeKeyAndOrderFront: sender];

	checkInBackground = NO;
	
	[self checkForUpdate];
}

- (void)windowWillClose:(id)sender
{
}

- (void)checkForUpdateInBackground
{
	checkInBackground = YES;
	
	[self checkForUpdate];	
}

- (IBAction)okay:(id)sender
{
	if (updateAvailable == YES)
	{
		DBLog(@"OPENING URL: %@", downloadURL);
		[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:downloadURL]];
	}
	[updateWindow close];
}

-(void) awakeFromNib
{
	NSNumber *doCheck = [[NSUserDefaults standardUserDefaults] objectForKey: @"Cog:CheckForUpdateAtStartup"];
	NSNumber *lastCheckDateNum = [[NSUserDefaults standardUserDefaults] objectForKey: @"Cog:LastUpdateCheckDate"];
	NSDate *lastCheckDate = nil;
	
	if( doCheck == nil ) // No setting in prefs yet? First launch?
	{
		doCheck = [NSNumber numberWithBool:YES];
		
		// Save user's preference to prefs file:
		[[NSUserDefaults standardUserDefaults] setObject: doCheck forKey: @"Cog:CheckForUpdateAtStartup"];
	}
	
	[autoCheckButton setState: [doCheck boolValue]]; // Update prefs button
	
	// If user wants us to check for updates at startup, do so:
	if( [doCheck boolValue] )
	{
		NSTimeInterval  timeSinceLastCheck;
		
		// Determine how long since last check:
		if( lastCheckDateNum == nil )
			lastCheckDate = [NSDate distantPast];  // If there's no date in prefs, use something guaranteed to be past.
		else
			lastCheckDate = [NSDate dateWithTimeIntervalSinceReferenceDate: [lastCheckDateNum doubleValue]];
		timeSinceLastCheck = -[lastCheckDate timeIntervalSinceNow];
		
		// If last check was more than DAYS_BETWEEN_CHECKS days ago, check again now:
		if( timeSinceLastCheck > (3600 * 24 * DAYS_BETWEEN_CHECKS) )
		{
//			DBLog(@"CHECKING FOR UPDATE");
			[self checkForUpdateInBackground];
			[[NSUserDefaults standardUserDefaults] setObject: [NSNumber numberWithDouble: [NSDate timeIntervalSinceReferenceDate]] forKey: @"Cog:LastUpdateCheckDate"];
		}
	}
}

-(IBAction)	takeBoolFromObject: (id)sender
{
	if( [sender respondsToSelector: @selector(boolValue)] )
		[self setCheckAtStartup: [sender boolValue]];
	else
		[self setCheckAtStartup: [sender state]];
}

-(void)	setCheckAtStartup: (BOOL)shouldCheck
{
	NSNumber *doCheck = [NSNumber numberWithBool: shouldCheck];
	[[NSUserDefaults standardUserDefaults] setObject: doCheck forKey: @"Cog:CheckForUpdateAtStartup"];
	
	[autoCheckButton setState: shouldCheck];
	[[NSUserDefaults standardUserDefaults] setObject: [NSNumber numberWithDouble: 0] forKey: @"Cog:LastUpdateCheckDate"];
}


//BULLSHIT STUFF
- (void)setDownloadURL:(NSString *)d
{
	[d retain];
	[downloadURL release];
	downloadURL = d;
}

@end
