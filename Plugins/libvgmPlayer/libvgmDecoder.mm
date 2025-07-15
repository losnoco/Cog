//
//  libvgmDecoder.mm
//  libvgmPlayer
//
//  Created by Christopher Snowhill on 1/02/22.
//  Copyright 2022-2025 __LoSnoCo__. All rights reserved.
//

#import "libvgmDecoder.h"

#import "Logging.h"

#import "PlaylistController.h"

#import <libvgm/emu/EmuCores.h>
#import <libvgm/emu/Resampler.h>
#import <libvgm/emu/SoundDevs.h>
#import <libvgm/player/droplayer.hpp>
#import <libvgm/player/gymplayer.hpp>
#import <libvgm/player/s98player.hpp>
#import <libvgm/player/vgmplayer.hpp>
#import <libvgm/utils/FileLoader.h>
#import <libvgm/utils/MemoryLoader.h>

@implementation libvgmDecoder

#ifdef _DEBUG
const int logLevel = DEVLOG_DEBUG;
#else
const int logLevel = DEVLOG_INFO;
#endif

static UINT8 FilePlayCallback(PlayerBase* player, void* userParam, UINT8 evtType, void* evtParam) {
	libvgmDecoder* decoder = (__bridge libvgmDecoder*)(userParam);
	switch(evtType) {
		case PLREVT_START:
			// printf("Playback started.\n");
			break;
		case PLREVT_STOP:
			// printf("Playback stopped.\n");
			break;
		case PLREVT_LOOP: {
			// UINT32* curLoop = (UINT32*)evtParam;
			// if (player->GetState() & PLAYSTATE_SEEK)
			//	break;
		} break;
		case PLREVT_END:
			if([decoder trackEnded])
				break;
			[decoder setTrackEnded:YES];
			break;
	}
	return 0x00;
}

#include "yrw801.h"

static DATA_LOADER* RequestFileCallback(void* userParam, PlayerBase* player, const char* fileName) {
	DATA_LOADER* dLoad;
	if(strcmp(fileName, "yrw801.rom") == 0) {
		dLoad = MemoryLoader_Init(yrw801_rom, sizeof(yrw801_rom));
	} else {
		dLoad = FileLoader_Init(fileName);
	}
	UINT8 retVal = DataLoader_Load(dLoad);
	if(!retVal)
		return dLoad;
	DataLoader_Deinit(dLoad);
	return NULL;
}

static const char* LogLevel2Str(UINT8 level) {
	static const char* LVL_NAMES[6] = { " ??? ", "Error", "Warn ", "Info ", "Debug", "Trace" };
	if(level >= (sizeof(LVL_NAMES) / sizeof(LVL_NAMES[0])))
		level = 0;
	return LVL_NAMES[level];
}

static void PlayerLogCallback(void* userParam, PlayerBase* player, UINT8 level, UINT8 srcType,
                              const char* srcTag, const char* message) {
	if(level > logLevel)
		return; // don't print messages with higher verbosity than current log level
	if(srcType == PLRLOGSRC_PLR) {
		ALog(@"[%s] %s: %s", LogLevel2Str(level), player->GetPlayerName(), message);
	} else {
		ALog(@"[%s] %s %s: %s", LogLevel2Str(level), player->GetPlayerName(), srcTag, message);
	}
	return;
}

const int numChannels = 2;
const int numBitsPerSample = 24;
const int smplAlloc = 2048;
const int masterVol = 0x10000; // Fixed point 16.16

- (id)init {
	self = [super init];
	if(self) {
		fileData = NULL;
		dLoad = NULL;
		mainPlr = NULL;
	}
	return self;
}

- (BOOL)open:(id<CogSource>)s {
	[self setSource:s];

	// We need file-size to use GME
	if(![source seekable]) {
		return NO;
	}

	sampleRate = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthSampleRate"] doubleValue];
	if(sampleRate < 8000.0) {
		sampleRate = 44100.0;
	} else if(sampleRate > 192000.0) {
		sampleRate = 192000.0;
	}

	loopCount = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthDefaultLoopCount"] longValue];
	if(loopCount < 1) {
		loopCount = 1;
	} else if(loopCount > 10) {
		loopCount = 10;
	}

	fadeTime = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthDefaultFadeSeconds"] doubleValue];

	BOOL repeatOne = IsRepeatOneSet();
	uint32_t maxLoops = repeatOne ? 0 : (uint32_t)loopCount;

	mainPlr = new PlayerA;
	mainPlr->RegisterPlayerEngine(new VGMPlayer);
	mainPlr->RegisterPlayerEngine(new S98Player);
	mainPlr->RegisterPlayerEngine(new DROPlayer);
	mainPlr->RegisterPlayerEngine(new GYMPlayer);
	mainPlr->SetEventCallback(FilePlayCallback, (__bridge void*)(self));
	mainPlr->SetFileReqCallback(RequestFileCallback, NULL);
	mainPlr->SetLogCallback(PlayerLogCallback, NULL);
	{
		PlayerA::Config pCfg = mainPlr->GetConfiguration();
		pCfg.masterVol = masterVol;
		pCfg.loopCount = maxLoops;
		pCfg.fadeSmpls = (int)ceil(sampleRate * fadeTime); // fade over configured duration
		pCfg.endSilenceSmpls = sampleRate / 2; // 0.5 seconds of silence at the end
		pCfg.pbSpeed = 1.0;
		mainPlr->SetConfiguration(pCfg);
	}
	mainPlr->SetOutputSettings((int)ceil(sampleRate), numChannels, numBitsPerSample, smplAlloc);

	[source seek:0 whence:SEEK_END];
	size_t size = [source tell];
	[source seek:0 whence:SEEK_SET];

	fileData = (UINT8*)malloc(size);
	if(!fileData)
		return NO;

	size_t bytesRead = [source read:fileData amount:size];

	if(bytesRead != size)
		return NO;

	dLoad = MemoryLoader_Init(fileData, (unsigned int)size);
	if(!dLoad)
		return NO;

	DataLoader_SetPreloadBytes(dLoad, 0x100);
	if(DataLoader_Load(dLoad))
		return NO;

	if(mainPlr->LoadFile(dLoad))
		return NO;

	PlayerBase* player = mainPlr->GetPlayer();

	mainPlr->SetLoopCount(maxLoops);
	if(player->GetPlayerType() == FCC_VGM) {
		VGMPlayer* vgmplay = dynamic_cast<VGMPlayer*>(player);
		mainPlr->SetLoopCount(vgmplay->GetModifiedLoopCount(maxLoops));
	}

	length = player->Tick2Second(player->GetTotalTicks()) * sampleRate;

	[self setTrackEnded:NO];

	mainPlr->Start();

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (NSDictionary*)properties {
	return @{ @"bitrate": @(0),
		      @"sampleRate": @(sampleRate),
		      @"totalFrames": @(length),
		      @"bitsPerSample": @(numBitsPerSample),
		      @"channels": @(numChannels),
		      @"seekable": @(YES),
		      @"endian": @"host",
		      @"encoding": @"synthesized" };
}

- (NSDictionary *)metadata {
	return @{};
}

- (AudioChunk*)readAudio {
	if([self trackEnded])
		return nil;

	id audioChunkClass = NSClassFromString(@"AudioChunk");
	AudioChunk* chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];

	int frames = 1024;
	const size_t bytesPerFrame = [chunk format].mBytesPerFrame;
	uint8_t buffer[frames * bytesPerFrame];

	void* buf = (void*)buffer;

	BOOL repeatOne = IsRepeatOneSet();
	uint32_t maxLoops = repeatOne ? 0 : (uint32_t)loopCount;

	PlayerBase* player = mainPlr->GetPlayer();
	mainPlr->SetLoopCount(maxLoops);
	if(player->GetPlayerType() == FCC_VGM) {
		VGMPlayer* vgmplay = dynamic_cast<VGMPlayer*>(player);
		mainPlr->SetLoopCount(vgmplay->GetModifiedLoopCount(maxLoops));
	}

	double streamTimestamp = mainPlr->GetCurTime(0);

	UInt32 framesDone = 0;

	while(framesDone < frames) {
		UInt32 framesToDo = frames - framesDone;
		if(framesToDo > smplAlloc)
			framesToDo = smplAlloc;

		int numSamples = framesToDo * numChannels * (numBitsPerSample / 8);

		UINT32 numRendered = mainPlr->Render(numSamples, buf);

		buf = (void*)(((uint8_t*)buf) + numRendered);

		UINT32 framesRendered = numRendered / (numChannels * (numBitsPerSample / 8));

		framesDone += framesRendered;

		if(framesRendered < framesToDo)
			break;
	}

	[chunk setStreamTimestamp:streamTimestamp];
	[chunk assignSamples:buffer frameCount:framesDone];

	return chunk;
}

- (long)seek:(long)frame {
	[self setTrackEnded:NO];

	mainPlr->Seek(PLAYPOS_SAMPLE, (unsigned int)frame);

	return frame;
}

- (void)close {
	if(mainPlr) {
		mainPlr->Stop();
		mainPlr->UnloadFile();

		delete mainPlr;
		mainPlr = NULL;
	}
	if(dLoad) {
		DataLoader_Deinit(dLoad);
		dLoad = NULL;
	}
	if(fileData) {
		free(fileData);
		fileData = NULL;
	}
}

- (void)dealloc {
	[self close];
}

+ (NSArray*)fileTypes {
	return @[@"vgm", @"vgz", @"s98", @"dro", @"gym"];
}

+ (NSArray*)mimeTypes {
	return nil;
}

+ (float)priority {
	return 1.25;
}

+ (NSArray*)fileTypeAssociations {
	NSMutableArray* ret = [[NSMutableArray alloc] init];
	[ret addObject:@"libvgm Files"];
	[ret addObject:@"vg.icns"];
	[ret addObjectsFromArray:[self fileTypes]];

	return @[[NSArray arrayWithArray:ret]];
}

- (void)setSource:(id<CogSource>)s {
	source = s;
}

- (id<CogSource>)source {
	return source;
}

- (BOOL)trackEnded {
	return trackEnded;
}

- (void)setTrackEnded:(BOOL)ended {
	trackEnded = ended;
}

@end
