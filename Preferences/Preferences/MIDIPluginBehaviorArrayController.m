//
//  MIDIPluginBehaviorArrayController.m
//  General
//
//  Created by Christopher Snowhill on 03/26/14.
//
//

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudio/CoreAudioTypes.h>

#import "MIDIPluginBehaviorArrayController.h"

typedef void (*callback)(void *context, OSType uSubType, OSType uManufacturer, const char *name);
static void enumComponents(callback cbEnum, void *context) {
	AudioComponentDescription cd = { 0 };
	cd.componentType = kAudioUnitType_MusicDevice;

	AudioComponent comp = NULL;

	const char *bytes;
	char bytesBuffer[512];

	comp = AudioComponentFindNext(comp, &cd);

	while(comp != NULL) {
		AudioComponentDescription tcd;
		CFStringRef cfName;
		AudioComponentCopyName(comp, &cfName);
		bytes = CFStringGetCStringPtr(cfName, kCFStringEncodingUTF8);
		if(!bytes) {
			CFStringGetCString(cfName, bytesBuffer, sizeof(bytesBuffer) - 1, kCFStringEncodingUTF8);
			bytesBuffer[sizeof(bytesBuffer) - 1] = '\0';
			bytes = bytesBuffer;
		}
		AudioComponentGetDescription(comp, &tcd);
		if(tcd.componentManufacturer != kAudioUnitManufacturer_Apple) {
			cbEnum(context, tcd.componentSubType, tcd.componentManufacturer, bytes);
		}
		CFRelease(cfName);
		comp = AudioComponentFindNext(comp, &cd);
	}
}

static void copyOSType(char *out, OSType in) {
	out[0] = (in >> 24) & 0xFF;
	out[1] = (in >> 16) & 0xFF;
	out[2] = (in >> 8) & 0xFF;
	out[3] = in & 0xFF;
}

static void enumCallback(void *context, OSType uSubType, OSType uManufacturer, const char *name) {
	id pself = (__bridge id)context;

	char pref[9];

	copyOSType(pref, uSubType);
	copyOSType(pref + 4, uManufacturer);
	pref[8] = '\0';

	[pself addObject:
	       [NSDictionary dictionaryWithObjectsAndKeys:
	                     [NSString stringWithUTF8String:name], @"name",
	                     [NSString stringWithUTF8String:pref], @"preference", nil]];
}

@implementation MIDIPluginBehaviorArrayController
- (void)awakeFromNib {
	[self removeObjects:[self arrangedObjects]];

	[self addObject:@{@"name": @"BASSMIDI", @"preference": @"BASSMIDI"}];

	[self addObject:@{@"name": @"DMX Generic", @"preference": @"DOOM0000"}];
	[self addObject:@{@"name": @"DMX Doom 1", @"preference": @"DOOM0001"}];
	[self addObject:@{@"name": @"DMX Doom 2", @"preference": @"DOOM0002"}];
	[self addObject:@{@"name": @"DMX Raptor", @"preference": @"DOOM0003"}];
	[self addObject:@{@"name": @"DMX Strife", @"preference": @"DOOM0004"}];
	[self addObject:@{@"name": @"DMXOPL", @"preference": @"DOOM0005"}];

	[self addObject:@{@"name": @"OPL3Windows", @"preference": @"OPL3W000"}];

	enumComponents(enumCallback, (__bridge void *)(self));
}

@end
