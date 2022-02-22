//
//  ScriptAdditions.m
//  Cog
//
//  Created by Christopher Snowhill on 2/21/22.
//

#import <Cocoa/Cocoa.h>

#import "AppController.h"

@implementation NSApplication (APLApplicationExtensions)
- (id)playbackStart:(NSScriptCommand *)command {
	[(AppController *)[NSApp delegate] clickPlay];
	return [NSNumber numberWithBool:YES];
}
- (id)playbackPause:(NSScriptCommand *)command {
	[(AppController *)[NSApp delegate] clickPause];
	return [NSNumber numberWithBool:YES];
}
- (id)playbackStop:(NSScriptCommand *)command {
	[(AppController *)[NSApp delegate] clickStop];
	return [NSNumber numberWithBool:YES];
}
- (id)playbackPrevious:(NSScriptCommand *)command {
	[(AppController *)[NSApp delegate] clickPrev];
	return [NSNumber numberWithBool:YES];
}
- (id)playbackNext:(NSScriptCommand *)command {
	[(AppController *)[NSApp delegate] clickNext];
	return [NSNumber numberWithBool:YES];
}
@end
