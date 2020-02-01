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
    __Verify_noErr(AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize));
	int nDevices = propsize / sizeof(AudioDeviceID);
	AudioDeviceID *devids = malloc(propsize);
    __Verify_noErr(AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize, devids));
	int i;
    
    theAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
    AudioDeviceID systemDefault;
    propsize = sizeof(systemDefault);
    __Verify_noErr(AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize, &systemDefault));
	
	NSDictionary *defaultDevice = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"outputDevice"];
	
    theAddress.mScope = kAudioDevicePropertyScopeOutput;
    
	for (i = 0; i < nDevices; ++i) {
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

		NSDictionary *deviceInfo = @{
			@"name": [NSString stringWithString:(__bridge NSString*)name],
			@"deviceID": [NSNumber numberWithUnsignedInt:devids[i]],
		};
		[self addObject:deviceInfo];
        
        CFRelease(name);
		
		if (defaultDevice) {
			if (([[defaultDevice objectForKey:@"deviceID"] isEqualToNumber:[deviceInfo objectForKey:@"deviceID"]]) ||
				([[defaultDevice objectForKey:@"name"] isEqualToString:[deviceInfo objectForKey:@"name"]])) {
				[self setSelectedObjects:[NSArray arrayWithObject:deviceInfo]];
				// Update `outputDevice`, in case the ID has changed.
				[[NSUserDefaults standardUserDefaults] setObject:deviceInfo forKey:@"outputDevice"];
			}
		}
        else {
            if ( devids[i] == systemDefault ) {
                [self setSelectedObjects:[NSArray arrayWithObject:deviceInfo]];
            }
        }
	}
	free(devids);
	
		
	if (!defaultDevice)
		[self setSelectionIndex:0];
}

@end
