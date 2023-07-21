#import "APLDecoder.h"
#import "APLFile.h"

#import "Logging.h"

@implementation APLDecoder

+ (NSArray *)fileTypes {
	return @[@"apl"];
}

+ (NSArray *)mimeTypes {
	return @[@"application/x-apl"];
}

+ (float)priority {
	return 1.0;
}

+ (NSArray *)fileTypeAssociations {
	return @[
		@[@"APL Link File", @"song.icns", @"apl"]
	];
}

- (NSDictionary *)properties {
	NSMutableDictionary *properties = [[decoder properties] mutableCopy];

	// Need to alter length
	[properties setObject:@(trackLength) forKey:@"totalFrames"];
	return [NSDictionary dictionaryWithDictionary:properties];
}

- (NSDictionary *)metadata {
	return @{};
}

- (BOOL)open:(id<CogSource>)s {
	// DLog(@"Loading apl...");
	if(![[s url] isFileURL])
		return NO;

	NSURL *url = [s url];

	apl = [APLFile createWithFile:[url path]];

	// Kind of a hackish way of accessing outside classes.
	source = [NSClassFromString(@"AudioSource") audioSourceForURL:[apl file]];

	if(![source open:[apl file]]) {
		ALog(@"Could not open source for file '%@' referenced in apl", [apl file]);
		return NO;
	}
	decoder = [NSClassFromString(@"AudioDecoder") audioDecoderForSource:source skipCue:YES];

	if(![decoder open:source]) {
		ALog(@"Could not open decoder for source for apl");
		return NO;
	}

	NSDictionary *properties = [decoder properties];
	int bitsPerSample = [[properties objectForKey:@"bitsPerSample"] intValue];
	int channels = [[properties objectForKey:@"channels"] intValue];
	//	float sampleRate = [[properties objectForKey:@"sampleRate"] floatValue];

	isDSD = bitsPerSample == 1;

	bytesPerFrame = ((bitsPerSample + 7) / 8) * channels;

	if([apl endBlock] > [apl startBlock])
		trackEnd = [apl endBlock]; //([apl endBlock] / sampleRate) * 1000.0;
	else
		trackEnd = [[properties objectForKey:@"totalFrames"] doubleValue]; //!!? double?

	trackStart = [apl startBlock];
	trackLength = trackEnd - trackStart;

	[self seek:0];

	// Note: Should register for observations of the decoder, but laziness consumes all.
	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
	return YES;
}

- (void)close {
	if(decoder) {
		[decoder close];
		decoder = nil;
	}

	source = nil;
	apl = nil;
}

- (void)dealloc {
	[self close];
}

- (long)seek:(long)frame {
	if(frame > trackEnd - trackStart) {
		// need a better way of returning fail.
		return -1;
	}

	frame += trackStart;

	framePosition = [decoder seek:frame];

	return framePosition;
}

- (AudioChunk *)readAudio {
	int maxFrames = INT_MAX;

	if(framePosition + maxFrames >= trackEnd)
		maxFrames = (int)(trackEnd - framePosition);

	if(!maxFrames) {
		DLog(@"APL readAudio Returning 0");
		return nil;
	}

	size_t frameScale = isDSD ? 8 : 1;

	AudioChunk *chunk = [decoder readAudio];
	if(chunk.frameCount * frameScale > maxFrames) {
		[chunk setFrameCount:maxFrames / frameScale];
	}

	framePosition += chunk.frameCount * frameScale;
	return chunk;
}

- (BOOL)isSilence {
	if([decoder respondsToSelector:@selector(isSilence)])
		return [decoder isSilence];
	return NO;
}

@end
