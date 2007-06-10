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

- (BOOL)open:(id<CogSource>)source
{
	NSLog(@"Opening!");
	
	NSURL						*url				= [source url];
	OSErr						error; 
	Handle						dataRef; 
	OSType						dataRefType; 
	Movie						soundToPlay;
	AudioChannelLayout*			layout				= nil;
	UInt32						size				= 0;

	NSLog(@"EnterMovies...");		
	EnterMovies();


	NSLog(@"Creating new data reference...");
	error = QTNewDataReferenceFromCFURL((CFURLRef)url, 0, &dataRef, &dataRefType);
	NSLog(@"   %d",error);
	
	NSLog(@"Creating new movie...");
	short fileID = movieInDataForkResID; 
	short flags = 0; 
	error = NewMovieFromDataRef(&soundToPlay, flags, &fileID, dataRef, dataRefType);
	NSLog(@"   %d",error);

	NSLog(@"Setting movie active...");
	SetMovieActive(soundToPlay, TRUE);

	NSLog(@"Beginning extraction session...");
	error = MovieAudioExtractionBegin(soundToPlay, 0, &_extractionSessionRef); 
	NSLog(@"   %d",error);

	NSLog(@"Getting property info...");
	error = MovieAudioExtractionGetPropertyInfo(_extractionSessionRef,
			kQTPropertyClass_MovieAudioExtraction_Audio,
			kQTMovieAudioExtractionAudioPropertyID_AudioChannelLayout,
			NULL, &size, NULL);
	NSLog(@"   %d",error);
		
	if (error == noErr) {
		// Allocate memory for the channel layout
		layout = (AudioChannelLayout *) calloc(1, size);
		if (layout == nil) {
			error = memFullErr;
			NSLog(@"Oops, out of memory");
		}
		// Get the layout for the current extraction configuration.
		// This will have already been expanded into channel descriptions.
		NSLog(@"Getting property...");
		error = MovieAudioExtractionGetProperty(_extractionSessionRef,
				kQTPropertyClass_MovieAudioExtraction_Audio,
				kQTMovieAudioExtractionAudioPropertyID_AudioChannelLayout,
				size, layout, nil);   
		NSLog(@"   %d",error);
	}
	
	NSLog(@"Getting audio stream basic description (absd)...");
	error = MovieAudioExtractionGetProperty(_extractionSessionRef,
			kQTPropertyClass_MovieAudioExtraction_Audio,
			kQTMovieAudioExtractionAudioPropertyID_AudioStreamBasicDescription,
			sizeof (_asbd), &_asbd, nil);
	NSLog(@"   %d",error);
	
	NSLog(@"   format flags   = %d",_asbd.mFormatFlags);
	NSLog(@"   sample rate    = %f",_asbd.mSampleRate);
	NSLog(@"   b/packet       = %d",_asbd.mBytesPerPacket);
	NSLog(@"   f/packet       = %d",_asbd.mFramesPerPacket);
	NSLog(@"   b/frame        = %d",_asbd.mBytesPerFrame);
	NSLog(@"   channels/frame = %d",_asbd.mChannelsPerFrame);
	NSLog(@"   b/channel      = %d",_asbd.mBitsPerChannel);
	
	if (_asbd.mChannelsPerFrame != 2) {
		NSLog(@"Cannot import non-stereo audio!");
	}
	
	_asbd.mFormatID = kAudioFormatLinearPCM;
	_asbd.mFormatFlags = kAudioFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsBigEndian;
//	_asbd.mChannelsPerFrame = 2;
	_asbd.mBitsPerChannel = 8*sizeof(int);
	_asbd.mBytesPerFrame = (_asbd.mBitsPerChannel/8) * _asbd.mChannelsPerFrame;
	_asbd.mBytesPerPacket = _asbd.mBytesPerFrame;
	_asbd.mFramesPerPacket = 1;
	
	NSLog(@"Setting new _asbd...");
	error = MovieAudioExtractionSetProperty(_extractionSessionRef,
			kQTPropertyClass_MovieAudioExtraction_Audio,
			kQTMovieAudioExtractionAudioPropertyID_AudioStreamBasicDescription,
			sizeof (_asbd), &_asbd);
	NSLog(@"   %d",error);
	
	
	NSLog(@"   format flags   = %d",_asbd.mFormatFlags);
	NSLog(@"   sample rate    = %f",_asbd.mSampleRate);
	NSLog(@"   b/packet       = %d",_asbd.mBytesPerPacket);
	NSLog(@"   f/packet       = %d",_asbd.mFramesPerPacket);
	NSLog(@"   b/frame        = %d",_asbd.mBytesPerFrame);
	NSLog(@"   channels/frame = %d",_asbd.mChannelsPerFrame);
	NSLog(@"   b/channel      = %d",_asbd.mBitsPerChannel);

	_totalFrames = _asbd.mSampleRate * ((float) GetMovieDuration(soundToPlay) / (float) GetMovieTimeScale(soundToPlay));

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
		
	return YES;
}

- (int) fillBuffer:(void *)buf ofSize:(UInt32)size
{
	OSErr error;
	UInt32				extractionFlags			= 0;
	AudioBufferList		buffer;
	UInt32 numFrames = size / _asbd.mBytesPerFrame;
	
	buffer.mNumberBuffers = 1;
	buffer.mBuffers[0].mNumberChannels = _asbd.mChannelsPerFrame;
	buffer.mBuffers[0].mDataByteSize = size;
	
	buffer.mBuffers[0].mData = buf;
		
	error = MovieAudioExtractionFillBuffer(_extractionSessionRef, &numFrames, &buffer, &extractionFlags);
	if (error) {
		NSLog(@"   %d",error);
		NSLog(@"   Extraction flags = %d (complete? %d)",extractionFlags,kQTMovieAudioExtractionComplete);
	}
	
	return numFrames * _asbd.mBytesPerFrame;
}

- (void)close
{
	OSErr						error; 

	NSLog(@"Ending extraction session...");
	error = MovieAudioExtractionEnd(_extractionSessionRef);
	NSLog(@"   %d",error);

	NSLog(@"ExitMovies...");
	ExitMovies();
}

- (double) seekToTime:(double)milliseconds
{
	return 0.0;
}

+ (NSArray *)fileTypes
{
	NSMutableArray *extensions = [NSMutableArray array];

	Component component = NULL;
	ComponentDescription looking;
	NSCharacterSet *spaceSet = [NSCharacterSet characterSetWithCharactersInString:@" '"];

	looking.componentType = MovieImportType;
	looking.componentSubType = 0; // Any subtype is OK
	looking.componentManufacturer = 0; // Any manufacturer is OK
	looking.componentFlags = movieImportSubTypeIsFileExtension;
	looking.componentFlagsMask = movieImportSubTypeIsFileExtension;

	while (component = FindNextComponent(component, &looking)) {
		ComponentDescription description;
		
		if (GetComponentInfo(component, &description, NULL, NULL, NULL) == noErr) {
			
			NSString *HFSType = NSFileTypeForHFSTypeCode(description.componentSubType);
			
			NSLog(@"Extension?: %@", HFSType);
			[extensions addObject:[HFSType stringByTrimmingCharactersInSet:spaceSet]];
			
			// the extension is present in the description.componentSubType field, which really holds
			// a 32-bit number. you need to convert that to a string, and trim off any trailing spaces.
			// here's a quickie...
			char ext[5] = {0};
			NSString *extension;

			bcopy(&description.componentSubType, ext, 4);

			extension = [[NSString stringWithCString:ext] stringByTrimmingCharactersInSet:spaceSet];

			// do something with extension here ...    
			[extensions addObject:extension];
		}
	}
	
	return extensions;
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:_asbd.mChannelsPerFrame],@"channels",
		[NSNumber numberWithInt:_asbd.mBitsPerChannel],@"bitsPerSample",
		[NSNumber numberWithInt:0],@"bitrate",
		[NSNumber numberWithFloat:_asbd.mSampleRate],@"sampleRate",
		[NSNumber numberWithDouble:_totalFrames/(_asbd.mSampleRate/1000.0)],@"length",
		[NSNumber numberWithBool:NO], @"seekable",
		@"big", @"endian",
		nil];
}



@end
