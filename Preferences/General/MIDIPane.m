//
//  MIDIPane.m
//  General
//
//  Created by Christopher Snowhill on 10/15/13.
//
//

#import "MIDIPane.h"

@implementation MIDIPane

- (NSString *)title
{
	return NSLocalizedPrefString(@"Synthesis");
}

- (NSImage *)icon
{
    if (@available(macOS 11.0, *))
        return [NSImage imageWithSystemSymbolName:@"pianokeys" accessibilityDescription:nil];
	return [[NSImage alloc] initWithContentsOfFile:[[NSBundle bundleForClass:[self class]] pathForImageResource:@"midi"]];
}

- (IBAction)setSoundFont:(id)sender
{
    NSArray *fileTypes = @[@"sf2", @"sf2pack", @"sflist", @"json"];
    NSOpenPanel * panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection:NO];
    [panel setCanChooseDirectories:NO];
    [panel setCanChooseFiles:YES];
    [panel setFloatingPanel:YES];
    [panel setAllowedFileTypes:fileTypes];
    NSString * oldPath = [[NSUserDefaults standardUserDefaults] stringForKey:@"soundFontPath"];
    if ( oldPath != nil )
        [panel setDirectoryURL:[NSURL fileURLWithPath:oldPath]];
    NSInteger result = [panel runModal];
    if(result == NSModalResponseOK)
    {
        [[NSUserDefaults standardUserDefaults] setValue:[[panel URL] path] forKey:@"soundFontPath"];
    }
}

- (IBAction)setMidiPlugin:(id)sender
{
    NSString * plugin = [[NSUserDefaults standardUserDefaults] stringForKey:@"midi.plugin"];
    BOOL enabled = [plugin isEqualToString:@"Sc55rolD"] || [plugin isEqualToString:@"Sc55RoCl"];
    [midiFlavorControl setEnabled:enabled];
}

@end
