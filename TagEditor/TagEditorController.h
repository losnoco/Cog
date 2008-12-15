//
//  TagEditorController.h
//  Cog
//
//  Created by Safari on 08/11/17.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface TagEditorController : NSObject {

	IBOutlet NSWindow* tageditorWindow;
}
- (id)init;
- (void)showInfoForURL:(NSURL*)url;
- (IBAction)cancel:(id)sender;


@end
