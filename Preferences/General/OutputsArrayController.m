#import "OutputsArrayController.h"

@implementation OutputsArrayController

- (void)awakeFromNib
{
	[self removeObjects:[self arrangedObjects]];
	
	[self setSelectsInsertedObjects:NO];
			
	UInt32 propsize;
    AudioObjectPropertyAddress theAddress = {
        .mSelector = kAudioHardwarePropertyDevices,
        .mScope = kAudioObjectPropertyScopeGlobal,
        .mElement = kAudioObjectPropertyElementMaster
    };
    verify_noerr(AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize));
	int nDevices = propsize / sizeof(AudioDeviceID);
	AudioDeviceID *devids = malloc(propsize);
    verify_noerr(AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize, devids));
	int i;
    
    theAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
    AudioDeviceID systemDefault;
    propsize = sizeof(systemDefault);
    verify_noerr(AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize, &systemDefault));
	
	NSDictionary *defaultDevice = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"outputDevice"];
	
    theAddress.mScope = kAudioDevicePropertyScopeOutput;
    
	for (i = 0; i < nDevices; ++i) {
        CFStringRef name = NULL;
        propsize = sizeof(name);
        theAddress.mSelector = kAudioDevicePropertyDeviceNameCFString;
		verify_noerr(AudioObjectGetPropertyData(devids[i], &theAddress, 0, NULL, &propsize, &name));
		
		// Ignore devices that have no output channels:
		// This tells us the size of the buffer required to hold the information about the channels
        theAddress.mSelector = kAudioDevicePropertyStreamConfiguration;
        verify_noerr(AudioObjectGetPropertyDataSize(devids[i], &theAddress, 0, NULL, &propsize));
		// Knowing the size of the required buffer, we can determine how many channels there are
		// without actually allocating a buffer and requesting the information.
		// (we don't care about the exact number of channels, only if there are more than zero or not)
		if (propsize <= sizeof(UInt32)) continue;

		NSDictionary *deviceInfo = [NSDictionary dictionaryWithObjectsAndKeys:
            [NSString stringWithString:(NSString*)name], @"name",
			[NSNumber numberWithLong:devids[i]], @"deviceID",
			nil];
		[self addObject:deviceInfo];
        
        CFRelease(name);
		
		if (defaultDevice) {
			if ([[defaultDevice objectForKey:@"deviceID"] isEqualToNumber:[deviceInfo objectForKey:@"deviceID"]]) {
				[self setSelectedObjects:[NSArray arrayWithObject:deviceInfo]];
			}
		}
        else {
            if ( devids[i] == systemDefault ) {
                [self setSelectedObjects:[NSArray arrayWithObject:deviceInfo]];
            }
        }

		[deviceInfo release];
	}
	free(devids);
	
		
	if (!defaultDevice)
		[self setSelectionIndex:0];
}

@end
