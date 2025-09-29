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

#import <File_Extractor/fex.h>

#import "SHA256Digest.h"

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

- (void)setupAU:(NSString *)plugin {
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

- (NSString *)romPath {
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
	NSString *basePath = [[paths firstObject] stringByAppendingPathComponent:@"Cog"];
	basePath = [basePath stringByAppendingPathComponent:@"Roms"];
	basePath = [basePath stringByAppendingPathComponent:@"Nuked-SC55"];
	return basePath;
}

- (void)removeNuked:(NSString *)romPath isDir:(BOOL)dir {
	NSError *error = nil;
	[[NSFileManager defaultManager] removeItemAtPath:romPath error:&error];

	NSAlert *alert = [[NSAlert alloc] init];
	[alert setMessageText:NSLocalizedPrefString(@"NukedInfoTitle")];
	if(error) {
		[alert setInformativeText:[NSString stringWithFormat:NSLocalizedPrefString(dir ? @"NukedErrorDirExistsError" : @"NukedErrorFileExistsError"), error]];
	} else {
		[alert setInformativeText:NSLocalizedPrefString(dir ? @"NukedInfoDirExists" : @"NukedErrorFileExists")];
	}
	[alert addButtonWithTitle:NSLocalizedPrefString(@"NukedOK")];
			
	[alert beginSheetModalForWindow:[view window] completionHandler:^(NSModalResponse returnCode) {
	}];
}

static NSString *nukedSc55mk2 = @"SC-55mk2";
static NSString *nukedSc55st = @"SC-55st";
static NSString *nukedSc55mk1 = @"SC-55mk1";
static NSString *nukedCm300 = @"CM-300/SCC-1";
static NSString *nukedJv880 = @"JV-880";
static NSString *nukedScb55 = @"SCB-55";
static NSString *nukedRlp3237 = @"RLP-3237";
static NSString *nukedSc155 = @"SC-155";
static NSString *nukedSc155mk2 = @"SC-155mk2";

- (NSDictionary *)nukedDevices {
	return @{nukedSc55mk2: @{@"count": @(5)},
			 nukedSc55st: @{@"count": @(0)},
			 nukedSc55mk1: @{@"count": @(5)},
			 nukedCm300: @{@"count": @(0)},
			 nukedJv880: @{@"count": @(0)},
			 nukedScb55: @{@"count": @(0)},
			 nukedRlp3237: @{@"count": @(0)},
			 nukedSc155: @{@"count": @(0)},
			 nukedSc155mk2: @{@"count": @(0)}};
}

- (NSDictionary *)nukedRomsets {
	return @{@"8a1eb33c7599b746c0c50283e4349a1bb1773b5c0ec0e9661219bf6c067d2042":
				 @{@"name": @"rom1.bin", @"type": nukedSc55mk2},
			 @"a4c9fd821059054c7e7681d61f49ce6f42ed2fe407a7ec1ba0dfdc9722582ce0":
				 @{@"name": @"rom2.bin", @"type": nukedSc55mk2},
			 @"b0b5f865a403f7308b4be8d0ed3ba2ed1c22db881b8a8326769dea222f6431d8":
				 @{@"name": @"rom_sm.bin", @"type": nukedSc55mk2},
			 @"c6429e21b9b3a02fbd68ef0b2053668433bee0bccd537a71841bc70b8874243b":
				 @{@"name": @"waverom1.bin", @"type": nukedSc55mk2},
			 @"5b753f6cef4cfc7fcafe1430fecbb94a739b874e55356246a46abe24097ee491":
				 @{@"name": @"waverom2.bin", @"type": nukedSc55mk2},

			 @"7e1bacd1d7c62ed66e465ba05597dcd60dfc13fc23de0287fdbce6cf906c6544":
				 @{@"name": @"sc55_rom1.bin", @"type": nukedSc55mk1},
			 @"effc6132d68f7e300aaef915ccdd08aba93606c22d23e580daf9ea6617913af1":
				 @{@"name": @"sc55_rom2.bin", @"type": nukedSc55mk1},
			 @"5655509a531804f97ea2d7ef05b8fec20ebf46216b389a84c44169257a4d2007":
				 @{@"name": @"sc55_waverom1.bin", @"type": nukedSc55mk1},
			 @"c655b159792d999b90df9e4fa782cf56411ba1eaa0bb3ac2bdaf09e1391006b1":
				 @{@"name": @"sc55_waverom2.bin", @"type": nukedSc55mk1},
			 @"334b2d16be3c2362210fdbec1c866ad58badeb0f84fd9bf5d0ac599baf077cc2":
				 @{@"name": @"sc55_waverom3.bin", @"type": nukedSc55mk1},

			 /*@"aabfcf883b29060198566440205f2fae1ce689043ea0fc7074842aaa4fd4823e":
				 @{@"name": @"jv880_rom1.bin", @"type": nukedJv880},
			 @"11852e60ff597633c754c5441c1e3e06793bcd951fcea2c4969ac3041d130fce":
				 @{@"name": @"jv880_rom2.bin", @"type": nukedJv880},
			 @"aa3101a76d57992246efeda282a2cb0c0f8fdb441c2eed2aa0b0fad4d81f3ad4":
				 @{@"name": @"jv880_waverom1.bin", @"type": nukedJv880},
			 @"a7b50bb47734ee9117fa16df1f257990a9a1a0b5ed420337ae4310eb80df75c8":
				 @{@"name": @"jv880_waverom2.bin", @"type": nukedJv880}*/};
}

- (void)importNuked:(NSString *)romPath {
	NSFileManager *defaultManager = [NSFileManager defaultManager];
	NSError *error = nil;
	[defaultManager createDirectoryAtPath:romPath withIntermediateDirectories:YES attributes:nil error:&error];
	if(error) {
		NSAlert *alert = [[NSAlert alloc] init];
		[alert setMessageText:NSLocalizedPrefString(@"NukedInfoTitle")];
		[alert setInformativeText:[NSString stringWithFormat:NSLocalizedPrefString(@"NukedErrorDirCreateError"), error]];
		[alert addButtonWithTitle:NSLocalizedPrefString(@"NukedOK")];

		[alert beginSheetModalForWindow:[view window] completionHandler:^(NSModalResponse returnCode) {
		}];

		[defaultManager removeItemAtPath:romPath error:&error];
		return;
	}

	NSArray *fileTypes = @[@"zip", @"rar", @"7z"];
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	[panel setAllowsMultipleSelection:NO];
	[panel setCanChooseDirectories:NO];
	[panel setCanChooseFiles:YES];
	[panel setFloatingPanel:YES];
	[panel setAllowedFileTypes:fileTypes];
	NSInteger result = [panel runModal];
	if(result == NSModalResponseOK) {
		NSURL *url = [panel URL];
		NSString *path = [url path];

		fex_t *fex = NULL;
		fex_err_t err = fex_open(&fex, [path UTF8String]);
		if(!err) {
			NSString *currentDevice = nil;
			NSDictionary *devices = [self nukedDevices];
			NSDictionary *romSets = [self nukedRomsets];
			NSMutableDictionary *foundSets = [[NSMutableDictionary alloc] init];

			while(!fex_done(fex)) {
				const void *data = NULL;
				err = fex_data(fex, &data);
				if(!err) {
					uint64_t size = fex_size(fex);
					NSData *itemData = [NSData dataWithBytes:data length:size];
					Class shaClass = NSClassFromString(@"SHA256Digest");
					NSString *hash = [shaClass digestDataAsString:itemData];
					NSDictionary *foundItem = romSets[hash];
					if(foundItem) {
						if(currentDevice) {
							if(![currentDevice isEqualToString:foundItem[@"type"]]) {
								break;
							}
						} else {
							currentDevice = foundItem[@"type"];
						}
						foundSets[hash] = @{@"data": itemData, @"name": foundItem[@"name"]};
					}
					fex_next(fex);
				}
			}
			if(!fex_done(fex) ||
			   !currentDevice ||
			   [devices[currentDevice][@"count"] integerValue] != [foundSets count]) {
				fex_close(fex);

				NSAlert *alert = [[NSAlert alloc] init];
				[alert setMessageText:NSLocalizedPrefString(@"NukedInfoTitle")];
				[alert setInformativeText:NSLocalizedPrefString(@"NukedErrorBrokenSet")];
				[alert addButtonWithTitle:NSLocalizedPrefString(@"NukedOK")];
						
				[alert beginSheetModalForWindow:[view window] completionHandler:^(NSModalResponse returnCode) {
				}];

				[defaultManager removeItemAtPath:romPath error:&error];
				return;
			}

			fex_close(fex);

			[foundSets enumerateKeysAndObjectsUsingBlock:^(id  _Nonnull key, id  _Nonnull obj, BOOL * _Nonnull stop) {
				NSString *itemPath = [romPath stringByAppendingPathComponent:obj[@"name"]];
				[defaultManager createFileAtPath:itemPath contents:obj[@"data"] attributes:nil];
			}];

			NSAlert *alert = [[NSAlert alloc] init];
			[alert setMessageText:NSLocalizedPrefString(@"NukedInfoTitle")];
			[alert setInformativeText:[NSString stringWithFormat:NSLocalizedPrefString(@"NukedInfoSetInstalled"), currentDevice]];
			[alert addButtonWithTitle:NSLocalizedPrefString(@"NukedOK")];

			[alert beginSheetModalForWindow:[view window] completionHandler:^(NSModalResponse returnCode) {
			}];
		}
	}
	return;
}

- (void)setupNuked {
	NSString *_romPath = [self romPath];
	NSFileManager *defaultManager = [NSFileManager defaultManager];
	BOOL dir = NO;
	if(![defaultManager fileExistsAtPath:_romPath isDirectory:&dir]) {
		// Import a ROM path
		[self importNuked:_romPath];
	} else {
		// Remove existing setup
		[self removeNuked:_romPath isDir:dir];
	}
}

- (IBAction)setupPlugin:(id)sender {
	NSUInteger selectedItem = [midiPluginControl indexOfSelectedItem];
	NSDictionary *selectedInfo = [midiPluginBehaviorArrayController arrangedObjects][selectedItem];
	if(![[selectedInfo objectForKey:@"configurable"] boolValue])
		return;

	NSString *plugin = [selectedInfo objectForKey:@"preference"];

	if([plugin isEqualToString:@"NukeSc55"]) {
		[self setupNuked];
	} else {
		[self setupAU:plugin];
	}
}

@end
