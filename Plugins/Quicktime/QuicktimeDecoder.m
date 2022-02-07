//
//  QuicktimeDecoder.m
//  Quicktime
//
//  Created by Vincent Spader on 6/10/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "QuicktimeDecoder.h"
#import "Quicktime/QuicktimeComponents.h"

@implementation QuicktimeDecoder

- (BOOL)open:(id<CogSource>)source {
	NSLog(@"Opening!");

	NSURL *url = [source url];
	OSErr error;
	Handle dataRef;
	OSType dataRefType;

	NSLog(@"EnterMovies...");
	EnterMovies();

	NSLog(@"Creating new data reference...");
	error = QTNewDataReferenceFromCFURL((CFURLRef)url, 0, &dataRef, &dataRefType);
	NSLog(@"   %d", error);

	NSLog(@"Creating new movie...");
	short fileID = movieInDataForkResID;
	short flags = 0; // newMovieDontResolveDataRefs | newMovieDontAskUnresolvedDataRefs;
	error = NewMovieFromDataRef(&_movie, flags, &fileID, dataRef, dataRefType);
	if(error != noErr) {
		NSLog(@"   %d", error);
		return NO;
	}

	NSLog(@"Setting movie active...");
	SetMovieActive(_movie, TRUE);

	NSLog(@"Beginning extraction session...");
	error = MovieAudioExtractionBegin(_movie, 0, &_extractionSessionRef);
	if(error != noErr) {
		NSLog(@"   %d", error);
		return NO;
	}

	NSLog(@"Getting audio stream basic description (absd)...");
	error = MovieAudioExtractionGetProperty(_extractionSessionRef,
	                                        kQTPropertyClass_MovieAudioExtraction_Audio,
	                                        kQTMovieAudioExtractionAudioPropertyID_AudioStreamBasicDescription,
	                                        sizeof(_asbd), &_asbd, nil);
	if(error != noErr) {
		NSLog(@"   %d", error);
		return NO;
	}

	NSLog(@"bits per sample: %i", _asbd.mBitsPerChannel);
	_asbd.mFormatID = kAudioFormatLinearPCM;
	_asbd.mFormatFlags = kAudioFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsBigEndian;
	//	_asbd.mBitsPerChannel = 8*sizeof(int);
	_asbd.mBytesPerFrame = (_asbd.mBitsPerChannel / 8) * _asbd.mChannelsPerFrame;
	_asbd.mBytesPerPacket = _asbd.mBytesPerFrame;
	_asbd.mFramesPerPacket = 1;

	NSLog(@"Setting new _asbd...");
	error = MovieAudioExtractionSetProperty(_extractionSessionRef,
	                                        kQTPropertyClass_MovieAudioExtraction_Audio,
	                                        kQTMovieAudioExtractionAudioPropertyID_AudioStreamBasicDescription,
	                                        sizeof(_asbd), &_asbd);
	if(error != noErr) {
		NSLog(@"   %d", error);
		return NO;
	}
	/* THIS IS BROKEN
	error = MovieAudioExtractionGetProperty(_extractionSessionRef,
	                                        kQTPropertyClass_SoundDescription,
	                                        kQTSoundDescriptionPropertyID_BitRate,
	                                        sizeof(_bitrate),&_bitrate,nil);
	if (error != noErr) {
	    NSLog(@"   %d",error);
	    _bitrate = 0;
	}
	*/

	_totalFrames = _asbd.mSampleRate * ((float)GetMovieDuration(_movie) / (float)GetMovieTimeScale(_movie));

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size {
	OSErr error;
	UInt32 extractionFlags = 0;
	AudioBufferList buffer;
	UInt32 numFrames = size / _asbd.mBytesPerFrame;

	buffer.mNumberBuffers = 1;
	buffer.mBuffers[0].mNumberChannels = _asbd.mChannelsPerFrame;
	buffer.mBuffers[0].mDataByteSize = size;

	buffer.mBuffers[0].mData = buf;

	error = MovieAudioExtractionFillBuffer(_extractionSessionRef, &numFrames, &buffer, &extractionFlags);
	if(error) {
		NSLog(@"   %d", error);
		NSLog(@"   Extraction flags = %d (complete? %d)", extractionFlags, kQTMovieAudioExtractionComplete);
	}

	return numFrames * _asbd.mBytesPerFrame;
}

- (void)close {
	OSErr error;

	NSLog(@"Ending extraction session...");
	error = MovieAudioExtractionEnd(_extractionSessionRef);
	NSLog(@"   %d", error);

	NSLog(@"ExitMovies...");
	ExitMovies();
}

- (double)seekToTime:(double)milliseconds {
	OSErr error;
	TimeRecord timeRec;

	timeRec.scale = GetMovieTimeScale(_movie);
	timeRec.base = NULL;
	timeRec.value.hi = 0;
	timeRec.value.lo = (milliseconds / 1000.0) * timeRec.scale;

	error = MovieAudioExtractionSetProperty(_extractionSessionRef, kQTPropertyClass_MovieAudioExtraction_Movie, kQTMovieAudioExtractionMoviePropertyID_CurrentTime, sizeof(timeRec), &timeRec);
	if(error) {
		NSLog(@"Error seeking! %i", error);
		return 0.0;
	}

	return milliseconds;
}

+ (NSArray *)fileTypes {
	NSMutableArray *extensions = [NSMutableArray array];

	Component component = NULL;
	ComponentDescription looking;
	NSCharacterSet *spaceSet = [NSCharacterSet characterSetWithCharactersInString:@" '"];

	looking.componentType = MovieImportType;
	looking.componentSubType = 0; // Any subtype is OK
	looking.componentManufacturer = 0; // Any manufacturer is OK
	looking.componentFlags = movieImportSubTypeIsFileExtension;
	looking.componentFlagsMask = movieImportSubTypeIsFileExtension;

	while(component = FindNextComponent(component, &looking)) {
		ComponentDescription description;

		if(GetComponentInfo(component, &description, NULL, NULL, NULL) == noErr) {
			NSString *HFSType = NSFileTypeForHFSTypeCode(description.componentSubType);

			NSLog(@"Extension?: %@", HFSType);
			[extensions addObject:[HFSType stringByTrimmingCharactersInSet:spaceSet]];

			// the extension is present in the description.componentSubType field, which really holds
			// a 32-bit number. you need to convert that to a string, and trim off any trailing spaces.
			// here's a quickie...
			char ext[5] = { 0 };
			NSString *extension;

			bcopy(&description.componentSubType, ext, 4);

			extension = [[NSString stringWithCString:ext] stringByTrimmingCharactersInSet:spaceSet];

			// do something with extension here ...
			[extensions addObject:extension];
		}
	}

	return extensions;
}

+ (NSArray *)mimeTypes {
	return nil;
}

- (NSDictionary *)properties {
	return [NSDictionary dictionaryWithObjectsAndKeys:
	                     [NSNumber numberWithInt:_asbd.mChannelsPerFrame], @"channels",
	                     [NSNumber numberWithInt:_asbd.mBitsPerChannel], @"bitsPerSample",
	                     [NSNumber numberWithInt:0 /*_bitrate*/], @"bitrate",
	                     [NSNumber numberWithFloat:_asbd.mSampleRate], @"sampleRate",
	                     [NSNumber numberWithDouble:_totalFrames / (_asbd.mSampleRate / 1000.0)], @"length",
	                     [NSNumber numberWithBool:YES], @"seekable",
	                     @"big", @"endian",
	                     nil];
}

@end
