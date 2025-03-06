#import "OutputsArrayController.h"

#import <CoreAudio/AudioHardware.h>

@implementation OutputsArrayController

- (void)awakeFromNib {
	[self removeObjects:[self arrangedObjects]];

	[self setSelectsInsertedObjects:NO];

	NSDictionary *defaultDeviceInfo = @{
		@"name": @"System Default Device",
		@"deviceID": @(-1),
	};
	[self addObject:defaultDeviceInfo];

	NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
	NSDictionary *defaultDevice = [defaults objectForKey:@"outputDevice"];
	NSString *defaultDeviceName = defaultDevice ? defaultDevice[@"name"] : @"";
	NSNumber *defaultDeviceIDNum = defaultDevice ? defaultDevice[@"deviceID"] : @(-1);
	int defaultDeviceID = defaultDeviceIDNum ? [defaultDeviceIDNum intValue] : -1;

	[self enumerateAudioOutputsUsingBlock:
		 ^(NSString *deviceName, AudioDeviceID deviceID, AudioDeviceID systemDefaultID, BOOL *stop) {
		NSDictionary *deviceInfo = @{
			@"name": deviceName,
			@"deviceID": @(deviceID),
		};

		[self addObject:deviceInfo];

		if(defaultDeviceID != -1) {
			if((deviceID == (AudioDeviceID)defaultDeviceID) ||
			   ([deviceName isEqualToString:defaultDeviceName])) {
				[self setSelectedObjects:@[deviceInfo]];
				// Update `outputDevice`, in case the ID has changed.
				if(deviceID != (AudioDeviceID)defaultDeviceID) {
					[defaults setObject:deviceInfo forKey:@"outputDevice"];
				}
			}
		}
	}];

	if(defaultDeviceID == -1) {
		[self setSelectionIndex:0];
	}
}

- (void)enumerateAudioOutputsUsingBlock:(void(NS_NOESCAPE ^ _Nonnull)(NSString *deviceName, AudioDeviceID deviceID, AudioDeviceID systemDefaultID, BOOL *stop))block {
	UInt32 propsize;
	AudioObjectPropertyAddress theAddress = {
		.mSelector = kAudioHardwarePropertyDevices,
		.mScope = kAudioObjectPropertyScopeGlobal,
		.mElement = kAudioObjectPropertyElementMaster
	};

	__Verify_noErr(AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize));
	UInt32 nDevices = propsize / (UInt32)sizeof(AudioDeviceID);
	AudioDeviceID *devids = malloc(propsize);
	__Verify_noErr(AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize, devids));

	theAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
	AudioDeviceID systemDefault;
	propsize = sizeof(systemDefault);
	__Verify_noErr(AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize, &systemDefault));

	theAddress.mScope = kAudioDevicePropertyScopeOutput;

	for(UInt32 i = 0; i < nDevices; ++i) {
		CFStringRef name = NULL;
		propsize = sizeof(name);
		theAddress.mSelector = kAudioDevicePropertyDeviceNameCFString;
		__Verify_noErr(AudioObjectGetPropertyData(devids[i], &theAddress, 0, NULL, &propsize, &name));
		if(!name) {
			continue;
		}

		propsize = 0;
		theAddress.mSelector = kAudioDevicePropertyStreamConfiguration;
		__Verify_noErr(AudioObjectGetPropertyDataSize(devids[i], &theAddress, 0, NULL, &propsize));

		if(propsize < sizeof(UInt32)) {
			CFRelease(name);
			continue;
		}

		AudioBufferList *bufferList = (AudioBufferList *)malloc(propsize);
		__Verify_noErr(AudioObjectGetPropertyData(devids[i], &theAddress, 0, NULL, &propsize, bufferList));
		UInt32 bufferCount = bufferList->mNumberBuffers;
		free(bufferList);

		if(!bufferCount) {
			CFRelease(name);
			continue;
		}

		BOOL stop = NO;
		block([NSString stringWithString:(__bridge NSString *)name],
		      devids[i],
		      systemDefault,
		      &stop);

		CFRelease(name);

		if(stop) {
			break;
		}
	}

	free(devids);
}

@end
