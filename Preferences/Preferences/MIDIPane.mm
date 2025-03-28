//
//  MIDIPane.m
//  General
//
//  Created by Christopher Snowhill on 10/15/13.
//
//

#import "MIDIPane.h"

#import "SandboxBroker.h"

#import "AppController.h"

#import "AUPlayerView.h"

@implementation MIDIPane {
	NSTimer *startupTimer;
}

- (void)awakeFromNib {
	__block MIDIPane *_self = self;
	startupTimer = [NSTimer timerWithTimeInterval:0.2 repeats:YES block:^(NSTimer * _Nonnull timer) {
		NSUInteger selectedItem = [_self->midiPluginControl indexOfSelectedItem];
		NSArray *arrangedObjects = [_self->midiPluginBehaviorArrayController arrangedObjects];
		if([arrangedObjects count] > selectedItem) {
			NSDictionary *selectedInfo = arrangedObjects[selectedItem];
			[_self->midiPluginSetupButton setEnabled:[[selectedInfo objectForKey:@"configurable"] boolValue]];
			[timer invalidate];
		}
	}];
	[[NSRunLoop mainRunLoop] addTimer:startupTimer forMode:NSRunLoopCommonModes];
}

- (void)dealloc {
	[startupTimer invalidate];
	startupTimer = nil;
}

- (NSString *)title {
	return NSLocalizedPrefString(@"Synthesis");
}

- (NSImage *)icon {
	if(@available(macOS 11.0, *))
		return [NSImage imageWithSystemSymbolName:@"pianokeys" accessibilityDescription:nil];
	return [[NSImage alloc] initWithContentsOfFile:[[NSBundle bundleForClass:[self class]] pathForImageResource:@"midi"]];
}

- (IBAction)setSoundFont:(id)sender {
	NSArray *fileTypes = @[@"sf2", @"sf2pack", @"sflist", @"sf3"];
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	[panel setAllowsMultipleSelection:NO];
	[panel setCanChooseDirectories:NO];
	[panel setCanChooseFiles:YES];
	[panel setFloatingPanel:YES];
	[panel setAllowedFileTypes:fileTypes];
	NSString *oldPath = [[NSUserDefaults standardUserDefaults] stringForKey:@"soundFontPath"];
	if(oldPath != nil)
		[panel setDirectoryURL:[NSURL fileURLWithPath:oldPath]];
	NSInteger result = [panel runModal];
	if(result == NSModalResponseOK) {
		[[NSUserDefaults standardUserDefaults] setValue:[[panel URL] path] forKey:@"soundFontPath"];

		id sandboxBrokerClass = NSClassFromString(@"SandboxBroker");
		NSURL *pathUrl = [panel URL];
		if(![[sandboxBrokerClass sharedSandboxBroker] areAllPathsSafe:@[pathUrl]]) {
			id appControllerClass = NSClassFromString(@"AppController");
			[appControllerClass globalShowPathSuggester];
		}
	}
}

- (IBAction)setMidiPlugin:(id)sender {
	NSUInteger selectedItem = [midiPluginControl indexOfSelectedItem];
	NSDictionary *selectedInfo = [midiPluginBehaviorArrayController arrangedObjects][selectedItem];
	[midiPluginSetupButton setEnabled:[[selectedInfo objectForKey:@"configurable"] boolValue]];
}

static OSType getOSType(const char *in_) {
	const unsigned char *in = (const unsigned char *)in_;
	OSType v = (in[0] << 24) + (in[1] << 16) + (in[2] << 8) + in[3];
	return v;
}

- (IBAction)setupPlugin:(id)sender {
	NSUInteger selectedItem = [midiPluginControl indexOfSelectedItem];
	NSDictionary *selectedInfo = [midiPluginBehaviorArrayController arrangedObjects][selectedItem];
	if(![[selectedInfo objectForKey:@"configurable"] boolValue])
		return;

	NSString *plugin = [selectedInfo objectForKey:@"preference"];
	const char *cplugin = [plugin UTF8String];

	AudioComponentDescription cd = { 0 };

	cd.componentType = kAudioUnitType_MusicDevice;
	cd.componentSubType = getOSType(cplugin);
	cd.componentManufacturer = getOSType(cplugin + 4);

	AudioComponent comp = NULL;

	comp = AudioComponentFindNext(comp, &cd);

	if(!comp)
		return;

	AudioUnit au = NULL;

	OSStatus error;

	error = AudioComponentInstanceNew(comp, &au);

	if(error != noErr)
		return;

	/*error = AudioUnitInitialize(au);
	if(error != noErr)
		return;*/

	AUPluginUI * pluginUI = new AUPluginUI(plugin, au);

	if(!pluginUI->window_opened()) {
		delete pluginUI;
	}
}

@end
