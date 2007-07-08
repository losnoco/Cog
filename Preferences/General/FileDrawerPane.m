//
//  FileDrawerPane.m
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "FileDrawerPane.h"


@implementation FileDrawerPane

- (void)awakeFromNib
{
	[self setName:NSLocalizedStringFromTableInBundle(@"File Drawer", nil, [NSBundle bundleForClass:[self class]], @"") ];
	[self setIcon:@"file_drawer"];
	
	[rootPathTextView setStringValue:[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"fileDrawerRootPath"]];
}

- (IBAction)openSheet:(id)sender
{
	NSOpenPanel *p;
	
	p = [NSOpenPanel openPanel];
	
	[p setCanChooseDirectories:YES];
	[p setCanChooseFiles:NO];
	[p setAllowsMultipleSelection:NO];
	
	[p beginSheetForDirectory:nil file:nil types:nil modalForWindow:[view window] modalDelegate:self didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:) contextInfo:NULL];
	
}

- (void)openPanelDidEnd:(NSOpenPanel *)panel returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
	if (returnCode == NSOKButton)
	{
		[rootPathTextView setStringValue:[panel filename]];
		[[NSUserDefaults standardUserDefaults] setObject:[panel filename] forKey:@"fileDrawerRootPath"];
	}
}

@end
