//
//  MIDIPluginState.m
//  Cog
//
//  Created by Christopher Snowhill on 9/30/26.
//

#import "MIDIPluginState.h"

#import "Logging.h"

static NSString *const MIDIPluginSettingsKey = @"midiPluginSettings";

static NSURL *stateDirectory(void) {
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
	NSString *basePath = [[paths firstObject] stringByAppendingPathComponent:@"Cog"];
	basePath = [basePath stringByAppendingPathComponent:@"MIDI Plugin State"];
	return [NSURL fileURLWithPath:basePath isDirectory:YES];
}

/* Plugin keys are OSType pairs and usually plain ASCII, but keep anything
 * else out of the file name. */
static NSURL *stateURL(NSString *plugin) {
	NSCharacterSet *allowed = [NSCharacterSet characterSetWithCharactersInString:@"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"];
	NSString *name = [plugin stringByAddingPercentEncodingWithAllowedCharacters:allowed] ?: @"unnamed";
	return [stateDirectory() URLByAppendingPathComponent:[name stringByAppendingPathExtension:@"plist"]];
}

static BOOL writeState(NSURL *url, NSDictionary *state) {
	NSError *error = nil;
	if(![[NSFileManager defaultManager] createDirectoryAtURL:stateDirectory() withIntermediateDirectories:YES attributes:nil error:&error]) {
		ALog(@"Could not create the MIDI plugin state folder: %@", error);
		return NO;
	}
	NSData *data = [NSPropertyListSerialization dataWithPropertyList:state format:NSPropertyListBinaryFormat_v1_0 options:0 error:&error];
	if(!data) {
		ALog(@"Could not serialize MIDI plugin state: %@", error);
		return NO;
	}
	if(![data writeToURL:url options:NSDataWritingAtomic error:&error]) {
		ALog(@"Could not write MIDI plugin state to %@: %@", url.path, error);
		return NO;
	}
	return YES;
}

static NSDictionary *readState(NSURL *url) {
	NSData *data = [NSData dataWithContentsOfURL:url options:NSDataReadingMappedIfSafe error:nil];
	if(!data) return nil;
	id state = [NSPropertyListSerialization propertyListWithData:data options:NSPropertyListImmutable format:NULL error:nil];
	return [state isKindOfClass:[NSDictionary class]] ? state : nil;
}

/* Moves midiPluginSettings out of the defaults. A state file that already
 * exists is newer than the default and is kept. The default is removed only
 * when every entry has been written and read back, so a failure loses
 * nothing and is retried next launch. */
static void migrateFromDefaults(void) {
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSDictionary *settings = [defaults dictionaryForKey:MIDIPluginSettingsKey];
	if(!settings) return;

	BOOL allSaved = YES;
	for(NSString *plugin in settings) {
		NSDictionary *state = settings[plugin];
		if(![plugin isKindOfClass:[NSString class]] || ![state isKindOfClass:[NSDictionary class]]) continue;
		NSURL *url = stateURL(plugin);
		if([[NSFileManager defaultManager] fileExistsAtPath:url.path]) continue;
		if(!writeState(url, state) || ![readState(url) isEqualToDictionary:state]) {
			allSaved = NO;
		}
	}

	if(allSaved) {
		[defaults removeObjectForKey:MIDIPluginSettingsKey];
		DLog(@"Moved %lu MIDI plugin states out of the user defaults", (unsigned long)settings.count);
	}
}

static NSMutableDictionary<NSString *, NSArray *> *cache(void) {
	static NSMutableDictionary *cache;
	static dispatch_once_t once;
	dispatch_once(&once, ^{
		cache = [NSMutableDictionary new];
		migrateFromDefaults();
	});
	return cache;
}

static NSDate *modificationDate(NSURL *url) {
	NSDate *date = nil;
	[url getResourceValue:&date forKey:NSURLContentModificationDateKey error:nil];
	return date;
}

void MIDIPluginStateMigrate(void) {
	(void)cache();
}

NSDictionary *MIDIPluginStateLoad(NSString *plugin) {
	NSMutableDictionary *entries = cache();
	NSURL *url = stateURL(plugin);
	[url removeCachedResourceValueForKey:NSURLContentModificationDateKey];
	NSDate *date = modificationDate(url);
	if(!date) return nil;

	@synchronized(entries) {
		NSArray *entry = entries[plugin];
		if(entry && [entry[0] isEqualToDate:date]) {
			return entry[1];
		}
	}

	NSDictionary *state = readState(url);
	if(state) {
		@synchronized(entries) {
			entries[plugin] = @[date, state];
		}
	}
	return state;
}

BOOL MIDIPluginStateSave(NSString *plugin, NSDictionary *state) {
	NSMutableDictionary *entries = cache();
	NSURL *url = stateURL(plugin);
	if(!writeState(url, state)) return NO;

	NSDate *date = modificationDate(url);
	@synchronized(entries) {
		if(date) {
			entries[plugin] = @[date, [state copy]];
		} else {
			[entries removeObjectForKey:plugin];
		}
	}
	return YES;
}
