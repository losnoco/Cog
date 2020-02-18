#import "OutputsArrayController.h"

@implementation OutputsArrayController

- (void)awakeFromNib
{
	[self removeObjects:[self arrangedObjects]];
	
	[self setSelectsInsertedObjects:NO];
	
	NSDictionary *defaultDevice = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"outputDevice"];
    NSString *defaultDeviceName = defaultDevice[@"name"];
    NSNumber *defaultDeviceIDNum = defaultDevice[@"deviceID"];
    AudioDeviceID defaultDeviceID = [defaultDeviceIDNum unsignedIntValue];

	[self enumerateAudioOutputsUsingBlock:
	 ^(NSString *deviceName, AudioDeviceID deviceID, AudioDeviceID systemDefaultID, BOOL *stop) {
		NSDictionary *deviceInfo = @{
			@"name": deviceName,
			@"deviceID": @(deviceID),
		};
		[self addObject:deviceInfo];
        		
		if (defaultDevice) {
			if ((deviceID == defaultDeviceID) ||
				([deviceName isEqualToString:defaultDeviceName])) {
				[self setSelectedObjects:[NSArray arrayWithObject:deviceInfo]];
				// Update `outputDevice`, in case the ID has changed.
				[[NSUserDefaults standardUserDefaults] setObject:deviceInfo forKey:@"outputDevice"];
			}
		}
        else {
            if (deviceID == systemDefaultID) {
                [self setSelectedObjects:[NSArray arrayWithObject:deviceInfo]];
            }
        }
	}];
		
	if (!defaultDevice) {
		[self setSelectionIndex:0];
	}
}

- (void)enumerateAudioOutputsUsingBlock:(void (NS_NOESCAPE ^ _Nonnull)(NSString *deviceName, AudioDeviceID deviceID, AudioDeviceID systemDefaultID, BOOL *stop))block
{
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
	
	for (UInt32 i = 0; i < nDevices; ++i) {
		CFStringRef name = NULL;
		propsize = sizeof(name);
		theAddress.mSelector = kAudioDevicePropertyDeviceNameCFString;
		__Verify_noErr(AudioObjectGetPropertyData(devids[i], &theAddress, 0, NULL, &propsize, &name));
		
		propsize = 0;
		theAddress.mSelector = kAudioDevicePropertyStreamConfiguration;
		__Verify_noErr(AudioObjectGetPropertyDataSize(devids[i], &theAddress, 0, NULL, &propsize));
		
		if (propsize < sizeof(UInt32)) continue;
		
		AudioBufferList * bufferList = (AudioBufferList *) malloc(propsize);
		__Verify_noErr(AudioObjectGetPropertyData(devids[i], &theAddress, 0, NULL, &propsize, bufferList));
		UInt32 bufferCount = bufferList->mNumberBuffers;
		free(bufferList);
		
		if (!bufferCount) continue;
		
		BOOL stop = NO;
		block([NSString stringWithString:(__bridge NSString *)name],
			  devids[i],
			  systemDefault,
			  &stop);
		
		CFRelease(name);
		
		if (stop) {
			break;
		}
	}
	
	free(devids);
}

@end
