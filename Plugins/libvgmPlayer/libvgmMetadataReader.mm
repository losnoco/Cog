//
//  libvgmMetadataReader.mm
//  libvgmPlayer
//
//  Created by Christopher Snowhill on 1/03/22.
//  Copyright 2022-2026 __LoSnoCo__. All rights reserved.
//

#import "libvgmMetadataReader.h"

#import "libvgmDecoder.h"

#import "Logging.h"

#import "NSDictionary+Optional.h"

#import <libvgm/player/droplayer.hpp>
#import <libvgm/player/gymplayer.hpp>
#import <libvgm/player/s98player.hpp>
#import <libvgm/player/vgmplayer.hpp>
#import <libvgm/utils/MemoryLoader.h>

@implementation libvgmMetadataReader

#ifdef _DEBUG
const int logLevel = DEVLOG_DEBUG;
#else
const int logLevel = DEVLOG_INFO;
#endif

static UINT8 FilePlayCallback(PlayerBase* player, void* userParam, UINT8 evtType, void* evtParam) {
	return 0x00;
}

static DATA_LOADER* RequestFileCallback(void* userParam, PlayerBase* player, const char* fileName) {
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

static std::string FCC2Str(UINT32 fcc) {
	std::string result(4, '\0');
	result[0] = (char)((fcc >> 24) & 0xFF);
	result[1] = (char)((fcc >> 16) & 0xFF);
	result[2] = (char)((fcc >> 8) & 0xFF);
	result[3] = (char)((fcc >> 0) & 0xFF);
	return result;
}

+ (NSArray*)fileTypes {
	return [libvgmDecoder fileTypes];
}

+ (NSArray*)mimeTypes {
	return [libvgmDecoder mimeTypes];
}

+ (float)priority {
	return [libvgmDecoder priority];
}

+ (NSDictionary*)metadataForURL:(NSURL*)url {
	id audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> source = [audioSourceClass audioSourceForURL:url];

	if(![source open:url])
		return 0;

	if(![source seekable])
		return 0;

	PlayerA mainPlr;
	mainPlr.RegisterPlayerEngine(new VGMPlayer);
	mainPlr.RegisterPlayerEngine(new S98Player);
	mainPlr.RegisterPlayerEngine(new DROPlayer);
	mainPlr.RegisterPlayerEngine(new GYMPlayer);
	mainPlr.SetEventCallback(FilePlayCallback, NULL);
	mainPlr.SetFileReqCallback(RequestFileCallback, NULL);
	mainPlr.SetLogCallback(PlayerLogCallback, NULL);
	mainPlr.SetOutputSettings(44100, 2, 24, 2048);

	[source seek:0 whence:SEEK_END];
	size_t size = [source tell];
	[source seek:0 whence:SEEK_SET];

	UINT8* fileData = (UINT8*)malloc(size);
	if(!fileData)
		return 0;

	size_t bytesRead = [source read:fileData amount:size];

	if(bytesRead != size) {
		free(fileData);
		return 0;
	}

	DATA_LOADER* dLoad = MemoryLoader_Init(fileData, (unsigned int)size);
	if(!dLoad) {
		free(fileData);
		return 0;
	}

	DataLoader_SetPreloadBytes(dLoad, 0x100);
	if(DataLoader_Load(dLoad)) {
		DataLoader_Deinit(dLoad);
		free(fileData);
		return 0;
	}

	if(mainPlr.LoadFile(dLoad)) {
		DataLoader_Deinit(dLoad);
		free(fileData);
		return 0;
	}

	NSString* system = @"";
	NSString* title = @"";
	NSString* artist = @"";
	NSString* album = @"";
	NSNumber* year = @(0);

	PlayerBase* player = mainPlr.GetPlayer();

	const char* const* tagList = player->GetTags();
	for(const char* const* t = tagList; *t; t += 2) {
		if(!strcmp(t[0], "TITLE"))
			title = guess_encoding_of_string(t[1]);
		else if(!strcmp(t[0], "ARTIST"))
			artist = guess_encoding_of_string(t[1]);
		else if(!strcmp(t[0], "GAME"))
			album = guess_encoding_of_string(t[1]);
		else if(!strcmp(t[0], "DATE")) {
			char* end;
			unsigned long theYear = strtoul(t[1], &end, 10);
			year = @(theYear);
		}
	}

	PLR_SONG_INFO sInf;
	player->GetSongInfo(sInf);

	system = [NSString stringWithFormat:@"%s v%X.%02X", FCC2Str(sInf.format).c_str(), sInf.fileVerMaj, sInf.fileVerMin];

	mainPlr.UnloadFile();
	DataLoader_Deinit(dLoad);
	free(fileData);

	const NSString* keys[] = {
		@"codec",
		@"album",
		@"title",
		@"artist",
		@"year"
	};
	const id values[] = {
		system,
		album,
		title,
		artist,
		year
	};
	return [NSDictionary initWithOptionalObjects:values forKeys:keys count:sizeof(keys) / sizeof(keys[0])];
}

@end
