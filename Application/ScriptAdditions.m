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
	return @(YES);
}
- (id)playbackPause:(NSScriptCommand *)command {
	[(AppController *)[NSApp delegate] clickPause];
	return @(YES);
}
- (id)playbackStop:(NSScriptCommand *)command {
	[(AppController *)[NSApp delegate] clickStop];
	return @(YES);
}
- (id)playbackPrevious:(NSScriptCommand *)command {
	[(AppController *)[NSApp delegate] clickPrev];
	return @(YES);
}
- (id)playbackNext:(NSScriptCommand *)command {
	[(AppController *)[NSApp delegate] clickNext];
	return @(YES);
}
@end
