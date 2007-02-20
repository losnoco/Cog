#import "OutputsArrayController.h"

@implementation OutputsArrayController

- (void)awakeFromNib
{
	NSLog(@"initOutputDeviceList");
	[self removeObjects:[self arrangedObjects]];
	
	[self setSelectsInsertedObjects:NO];
			
	NSLog(@"OutputCoreAudio.setup()");
	UInt32 propsize;
	verify_noerr(AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &propsize, NULL));
	int nDevices = propsize / sizeof(AudioDeviceID);	
	AudioDeviceID *devids = malloc(propsize);
	verify_noerr(AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &propsize, devids));
	int i;
	NSLog(@"Number of devices: %d", nDevices);
	for (i = 0; i < nDevices; ++i) {
		char name[64];
		UInt32 maxlen = 64;
		verify_noerr(AudioDeviceGetProperty(devids[i], 0, false, kAudioDevicePropertyDeviceName, &maxlen, name));
		NSLog(@"Device: %d %s", devids[i], name);
		
		// Ignore devices that have no output channels:
		// This tells us the size of the buffer required to hold the information about the channels
		UInt32 propSize;
		verify_noerr(AudioDeviceGetPropertyInfo(devids[i], 0, false, kAudioDevicePropertyStreamConfiguration, &propSize, NULL));
		// Knowing the size of the required buffer, we can determine how many channels there are
		// without actually allocating a buffer and requesting the information.
		// (we don't care about the exact number of channels, only if there are more than zero or not)
		if (propSize <= sizeof(UInt32)) continue;

		NSObject *deviceInfo = [NSDictionary dictionaryWithObjectsAndKeys:
			[NSString stringWithCString:name], @"name",
			[NSNumber numberWithLong:devids[i]], @"deviceID",
			nil];
		[self addObject:deviceInfo];
		[deviceInfo release];
	}
	free(devids);
	
	[self setSelectionIndex:0];
}

@end
