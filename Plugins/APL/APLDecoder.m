#import "APLDecoder.h"
#import "APLFile.h"

#import "Logging.h"

@implementation APLDecoder

+ (NSArray *)fileTypes {	
	return [NSArray arrayWithObject:@"apl"];
}

+ (NSArray *)mimeTypes {	
	return [NSArray arrayWithObjects:@"application/x-apl", nil];
}

+ (float)priority {
    return 1.0;
}

- (NSDictionary *)properties {
	NSMutableDictionary *properties = [[decoder properties] mutableCopy];
	
	//Need to alter length
	[properties setObject:[NSNumber numberWithLong:trackLength] forKey:@"totalFrames"];
	return properties;
}

- (BOOL)open:(id<CogSource>)s
{
	//DLog(@"Loading apl...");
	if (![[s url] isFileURL])
		return NO;
	
	NSURL *url = [s url];
	[s close];
	
	apl = [APLFile createWithFile:[url path]];
	
	//Kind of a hackish way of accessing outside classes.
	source = [NSClassFromString(@"AudioSource") audioSourceForURL:[apl file]];
	
	if (![source open:[apl file]]) {
		ALog(@"Could not open source for file '%@' referenced in apl", [apl file]);
		return NO;
	}
	decoder = [NSClassFromString(@"AudioDecoder") audioDecoderForSource:source];
	
	if (![decoder open:source]) {
		ALog(@"Could not open decoder for source for apl");
		return NO;
	}
	
	NSDictionary *properties = [decoder properties];
	int bitsPerSample = [[properties objectForKey:@"bitsPerSample"] intValue];
	int channels = [[properties objectForKey:@"channels"] intValue];
//	float sampleRate = [[properties objectForKey:@"sampleRate"] floatValue];
	
	
	bytesPerFrame = (bitsPerSample/8) * channels;
	
	if ([apl endBlock] > [apl startBlock])
		trackEnd = [apl endBlock]; //([apl endBlock] / sampleRate) * 1000.0;
	else 
		trackEnd = [[properties objectForKey:@"totalFrames"] doubleValue]; //!!? double?
		
	trackStart = [apl startBlock];
	trackLength = trackEnd - trackStart;
	
	[self seek: 0];
	
	//Note: Should register for observations of the decoder, but laziness consumes all.
	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
	return YES;
}

- (void)close {
	if (decoder) {
		[decoder close];
		decoder = nil;
	}
	
    source = nil;
    apl = nil;
}


- (long)seek:(long)frame
{
	if (frame > trackEnd - trackStart) {
		//need a better way of returning fail.
		return -1;
	}
	
	frame += trackStart;
	
	framePosition = [decoder seek:frame];

	return framePosition;
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
	if (framePosition + frames > trackEnd)
		frames = trackEnd - framePosition;

	if (!frames) {
		DLog(@"APL readAudio Returning 0");
		return 0;
	}

	int n = [decoder readAudio:buf frames:frames];
	framePosition += n;
	return n;
}


@end
