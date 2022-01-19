//
//  libvgmDecoder.mm
//  libvgmPlayer
//
//  Created by Christopher Snowhill on 1/02/22.
//  Copyright 2022 __LoSnoCo__. All rights reserved.
//

#import "libvgmDecoder.h"

#import "Logging.h"

#import "PlaylistController.h"

#import <libvgm/utils/MemoryLoader.h>
#import <libvgm/utils/FileLoader.h>
#import <libvgm/player/s98player.hpp>
#import <libvgm/player/droplayer.hpp>
#import <libvgm/player/vgmplayer.hpp>
#import <libvgm/player/gymplayer.hpp>
#import <libvgm/emu/Resampler.h>
#import <libvgm/emu/SoundDevs.h>
#import <libvgm/emu/EmuCores.h>

@implementation libvgmDecoder

#ifdef _DEBUG
const int logLevel = DEVLOG_DEBUG;
#else
const int logLevel = DEVLOG_INFO;
#endif

static UINT8 FilePlayCallback(PlayerBase* player, void* userParam, UINT8 evtType, void* evtParam)
{
	libvgmDecoder * decoder = (__bridge libvgmDecoder *)(userParam);
	switch(evtType)
	{
	case PLREVT_START:
		//printf("Playback started.\n");
		break;
	case PLREVT_STOP:
		//printf("Playback stopped.\n");
		break;
	case PLREVT_LOOP:
		{
			//UINT32* curLoop = (UINT32*)evtParam;
			//if (player->GetState() & PLAYSTATE_SEEK)
			//	break;
		}
		break;
	case PLREVT_END:
		if ([decoder trackEnded])
			break;
		[decoder setTrackEnded:YES];
		break;
	}
	return 0x00;
}

#include "yrw801.h"

static DATA_LOADER* RequestFileCallback(void* userParam, PlayerBase* player, const char* fileName)
{
	DATA_LOADER* dLoad;
	if (strcmp(fileName, "yrw801.rom") == 0)
	{
		dLoad = MemoryLoader_Init(yrw801_rom, sizeof(yrw801_rom));
	}
	else
	{
		dLoad = FileLoader_Init(fileName);
	}
	UINT8 retVal = DataLoader_Load(dLoad);
	if (! retVal)
		return dLoad;
	DataLoader_Deinit(dLoad);
	return NULL;
}

static const char* LogLevel2Str(UINT8 level)
{
	static const char* LVL_NAMES[6] = {" ??? ", "Error", "Warn ", "Info ", "Debug", "Trace"};
	if (level >= (sizeof(LVL_NAMES) / sizeof(LVL_NAMES[0])))
		level = 0;
	return LVL_NAMES[level];
}

static void PlayerLogCallback(void* userParam, PlayerBase* player, UINT8 level, UINT8 srcType,
	const char* srcTag, const char* message)
{
	if (level > logLevel)
		return;	// don't print messages with higher verbosity than current log level
	if (srcType == PLRLOGSRC_PLR)
    {
		ALog(@"[%s] %s: %s", LogLevel2Str(level), player->GetPlayerName(), message);
    }
	else
    {
		ALog(@"[%s] %s %s: %s", LogLevel2Str(level), player->GetPlayerName(), srcTag, message);
    }
	return;
}

const int sampleRate = 44100;
const int numChannels = 2;
const int numBitsPerSample = 24;
const int smplAlloc = 2048;
const int masterVol = 0x10000; // Fixed point 16.16

- (id)init
{
    self = [super init];
    if (self) {
		fileData = NULL;
		dLoad = NULL;
        mainPlr = NULL;
    }
    return self;
}

- (BOOL)open:(id<CogSource>)s
{
	[self setSource:s];
	
	//We need file-size to use GME
	if (![source seekable]) {
		return NO;
	}

    BOOL repeatOne = IsRepeatOneSet();
    uint32_t maxLoops = repeatOne ? 0 : 2;

	mainPlr = new PlayerA;
	mainPlr->RegisterPlayerEngine(new VGMPlayer);
	mainPlr->RegisterPlayerEngine(new S98Player);
	mainPlr->RegisterPlayerEngine(new DROPlayer);
	mainPlr->RegisterPlayerEngine(new GYMPlayer);
	mainPlr->SetEventCallback(FilePlayCallback, (__bridge void *)(self));
	mainPlr->SetFileReqCallback(RequestFileCallback, NULL);
	mainPlr->SetLogCallback(PlayerLogCallback, NULL);
	{
		PlayerA::Config pCfg = mainPlr->GetConfiguration();
		pCfg.masterVol = masterVol;
		pCfg.loopCount = maxLoops;
		pCfg.fadeSmpls = sampleRate * 4;	// fade over 4 seconds
		pCfg.endSilenceSmpls = sampleRate / 2;	// 0.5 seconds of silence at the end
		pCfg.pbSpeed = 1.0;
		mainPlr->SetConfiguration(pCfg);
	}
	mainPlr->SetOutputSettings(sampleRate, numChannels, numBitsPerSample, smplAlloc);

	[source seek:0 whence:SEEK_END];
	size_t size = [source tell];
	[source seek:0 whence:SEEK_SET];

	fileData = (UINT8*) malloc(size);
	if (!fileData)
		return NO;

	size_t bytesRead = [source read:fileData amount:size];

	if (bytesRead != size)
		return NO;

	dLoad = MemoryLoader_Init(fileData, (unsigned int)size);
	if (!dLoad)
		return NO;

	DataLoader_SetPreloadBytes(dLoad,0x100);
	if (DataLoader_Load(dLoad))
		return NO;

	if (mainPlr->LoadFile(dLoad))
		return NO;

	PlayerBase* player = mainPlr->GetPlayer();

    mainPlr->SetLoopCount(maxLoops);
    if (player->GetPlayerType() == FCC_VGM)
    {
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

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:0], @"bitrate",
		[NSNumber numberWithFloat:sampleRate], @"sampleRate",
		[NSNumber numberWithLong:length], @"totalFrames",
		[NSNumber numberWithInt:numBitsPerSample], @"bitsPerSample", //Samples are short
		[NSNumber numberWithInt:numChannels], @"channels", //output from gme_play is in stereo
		[NSNumber numberWithBool:[source seekable]], @"seekable",
		@"host", @"endian",
		nil];
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
	if ([self trackEnded])
		return 0;

	BOOL repeatOne = IsRepeatOneSet();
	uint32_t maxLoops = repeatOne ? 0 : 2;

    PlayerBase* player = mainPlr->GetPlayer();
	mainPlr->SetLoopCount(maxLoops);
	if (player->GetPlayerType() == FCC_VGM)
	{
		VGMPlayer* vgmplay = dynamic_cast<VGMPlayer*>(player);
		mainPlr->SetLoopCount(vgmplay->GetModifiedLoopCount(maxLoops));
	}
    
    UInt32 framesDone = 0;
    
    while (framesDone < frames)
    {
        UInt32 framesToDo = frames - framesDone;
        if (framesToDo > smplAlloc)
            framesToDo = smplAlloc;
        
        int numSamples = framesToDo * numChannels * (numBitsPerSample/8);

        mainPlr->Render(numSamples, buf);
        
        buf = (void *)(((uint8_t*)buf) + numSamples);
        
        framesDone += framesToDo;
    }

	return framesDone;
}

- (long)seek:(long)frame
{
    [self setTrackEnded:NO];
    
	mainPlr->Seek(PLAYPOS_SAMPLE, (unsigned int)frame);
	
	return frame;
}

- (void)close
{
	if (mainPlr) {
		mainPlr->Stop();
		mainPlr->UnloadFile();

		delete mainPlr;
		mainPlr = NULL;
	}
	if (dLoad) {
		DataLoader_Deinit(dLoad);
		dLoad = NULL;
	}
	if (fileData) {
		free(fileData);
		fileData = NULL;
	}
}

- (void)dealloc
{
    [self close];
}

+ (NSArray *)fileTypes 
{	
	return @[@"vgm", @"vgz", @"s98", @"dro", @"gym"];
}

+ (NSArray *)mimeTypes 
{	
	return nil;
}

+ (float)priority
{
    return 1.25;
}

+ (NSArray *)fileTypeAssociations
{
    NSMutableArray * ret = [[NSMutableArray alloc] init];
    [ret addObject:@"libvgm Files"];
    [ret addObject:@"vg.icns"];
    [ret addObjectsFromArray:[self fileTypes]];
    
    return @[[NSArray arrayWithArray:ret]];
}

- (void)setSource:(id<CogSource>)s
{
	source = s;
}

- (id<CogSource>)source
{
	return source;
}

- (BOOL)trackEnded
{
	return trackEnded;
}

- (void)setTrackEnded:(BOOL)ended
{
	trackEnded = ended;
}

@end
