//
//  ScriptAdditions.h
//  Cog
//
//  Created by Christopher Snowhill on 2/21/22.
//

#ifndef ScriptAdditions_h
#define ScriptAdditions_h

@interface NSApplication (APLApplicationExtensions)
- (id)playbackStart:(NSScriptCommand *)command;
- (id)playbackPause:(NSScriptCommand *)command;
- (id)playbackStop:(NSScriptCommand *)command;
- (id)playbackPrevious:(NSScriptCommand *)command;
- (id)playbackNext:(NSScriptCommand *)command;
@end

#endif /* ScriptAdditions_h */
