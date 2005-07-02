//
//  UpdateController.h
//  Cog
//
//  Created by Vincent Spader on 3/26/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "MacPADSocket.h"


//Set to 0 for every startup
#ifndef DAYS_BETWEEN_CHECKS
#define DAYS_BETWEEN_CHECKS 0
#endif

@interface UpdateController : NSObject {
	IBOutlet NSWindow* updateWindow;
	IBOutlet NSButton* okayButton;
	IBOutlet NSProgressIndicator* checkingIndicator;
	IBOutlet NSTextField *statusView;
	IBOutlet NSButton *autoCheckButton;
	
	BOOL checkInBackground;
	BOOL updateAvailable;
	NSString *downloadURL;
	
	MacPADSocket *macPAD;
}

- (void)setCheckAtStartup: (BOOL)shouldCheck;

- (void)checkForUpdate;

- (void)macPADCheckFinished:(NSNotification *)aNotification;
- (void)macPADErrorOccurred:(NSNotification *)aNotification;
- (void)updateDisplay:(MacPADSocket *)socket info:(NSDictionary *)info;

- (IBAction)openUpdateWindow:(id)sender;
- (IBAction)okay:(id)sender;
- (IBAction)takeBoolFromObject:(id)sender;

- (void)setDownloadURL:(NSString *)d;

@end
