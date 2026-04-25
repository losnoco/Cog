#include "SpessaPlayer.h"

#import "Plugin.h"

#include <spessasynth_core/sflist.h>

#include <stdlib.h>

#include <string>

#include <chrono>
#include <cmath>
#include <list>
#include <map>
#include <mutex>
#include <thread>

#define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))

namespace {
typedef struct cog_file {
	id<CogSource> s;
	size_t size;
} cog_file;

static void cog_file_close(void *context) {
	cog_file *file = (cog_file *)context;
	[file->s close];
	delete file;
}

static bool cog_file_seek(void *context, size_t offset) {
	cog_file *file = (cog_file *)context;
	return [file->s seek:offset whence:SEEK_SET] == YES;
}

static size_t cog_file_size(void *context) {
	cog_file *file = (cog_file *)context;
	return file->size;
}

static size_t cog_file_read_bytes(void *context, uint8_t *out, size_t count) {
	cog_file *file = (cog_file *)context;
	return [file->s read:out amount:count];
}

static SS_File *cog_file_open(const char *path) {
	if(!strstr(path, "://")) {
		return ss_file_open_from_file(path);
	}

	id audioSourceClass = NSClassFromString(@"AudioSource");
	NSURL *url = [NSURL URLWithString:[NSString stringWithUTF8String:path]];
	id<CogSource> src = [audioSourceClass audioSourceForURL:url];
	if(![src open:url]) {
		return NULL;
	}

	SS_File_ReaderCallbacks cog_callbacks = {
		.close = &cog_file_close,
		.seek = &cog_file_seek,
		.size = &cog_file_size,
		.read_bytes = &cog_file_read_bytes
	};

	cog_file *file_context = new cog_file;
	file_context->s = src;

	[src seek:0 whence:SEEK_END];
	file_context->size = [src tell];
	[src seek:0 whence:SEEK_SET];

	SS_File *file = ss_file_open_from_callbacks(&cog_callbacks, file_context);
	if(!file) {
		[src close];
		delete file_context;
	}

	return file;
}

bool has_ext_ci(const char *path, const char *ext) {
	size_t plen = strlen(path);
	size_t elen = strlen(ext);
	if(plen < elen) return false;
	const char *tail = path + plen - elen;
	for(size_t i = 0; i < elen; i++) {
		char a = tail[i], b = ext[i];
		if(a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
		if(b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
		if(a != b) return false;
	}
	return true;
}

SS_FilteredBanks *open_sflist(const char *path) {
	char base_path[4096];

	const char *slash = strrchr(path, '/');
	if(slash) {
		size_t n = (size_t)(slash - path);
		if(n >= sizeof(base_path)) n = sizeof(base_path) - 1;
		memcpy(base_path, path, n);
		base_path[n] = '\0';
	} else {
		strcpy(base_path, ".");
	}

	SS_File *sflistFile = ss_file_open_from_file(path);
	if(sflistFile) {
		size_t sflistSize = ss_file_size(sflistFile);
		char *sflist = (char *)malloc(sflistSize);
		if(sflist && sflistSize) {
			ss_file_read_bytes(sflistFile, 0, (uint8_t *)sflist, sflistSize);
			ss_file_close(sflistFile);

			char err[sflist_max_error] = "";
			SS_FilteredBanks *banks = sflist_load(sflist, sflistSize, base_path, err);
			free(sflist);

			return banks;
		}
		ss_file_close(sflistFile);
	}

	return nullptr;
}

static SS_SoundBank *open_font(const char *path) {
	SS_File *bankFile = cog_file_open(path);
	if(bankFile) {
		SS_SoundBank *bank = ss_soundbank_load(bankFile);
		ss_file_close(bankFile);
		return bank;
	}
	return nullptr;
}
}

static class Spessa_Initializer {
	std::mutex lock;

	bool initialized;

	std::string base_path;

	public:
	Spessa_Initializer()
	: initialized(false) {
	}

	~Spessa_Initializer() {
		if(initialized) {
			/* Do nothing */
		}
	}

	bool check_initialized() {
		std::lock_guard<std::mutex> lock(this->lock);
		return initialized;
	}

	bool initialize() {
		std::lock_guard<std::mutex> lock(this->lock);
		if(!initialized) {
			ss_unit_converter_init();
			initialized = true;
		}
		return initialized;
	}
} g_initializer;

SpessaPlayer::SpessaPlayer()
: MIDIPlayer() {
	_synth = nullptr;
	interp = SS_INTERP_LINEAR;
	fileBankOffset = 0;

	if(!g_initializer.initialize()) throw std::runtime_error("Unable to initialize SpessaSynth");
}

SpessaPlayer::~SpessaPlayer() {
	shutdown();
}

void SpessaPlayer::setSoundFont(const char *in) {
	sSoundFontName = in;
	shutdown();
}

void SpessaPlayer::setFileBankOffset(uint16_t bank_offset) {
	fileBankOffset = bank_offset;
}

void SpessaPlayer::setFileSoundFont(const char *in) {
	sFileSoundFontName = in;
	shutdown();
}

void SpessaPlayer::setInterpolation(SS_InterpolationType interp) {
	this->interp = interp;
	shutdown();
}

void SpessaPlayer::shutdown() {
	if(_synth) {
		ss_processor_remove_soundbank(_synth, "fileBank", true);
		ss_processor_remove_soundbank(_synth, "globalBank", true);
		ss_processor_free(_synth);
		_synth = nullptr;
	}
	/* In case the synth didn't init completely, clean up */
	for(auto it = _banks.begin(); it != _banks.end(); ++it)
		ss_soundbank_free(*it);
	for(auto it = _filteredBanks.begin(); it != _filteredBanks.end(); ++it)
		ss_filtered_banks_free(*it, true);
	_banks.resize(0);
	_filteredBanks.resize(0);
	initialized = false;
}

bool SpessaPlayer::startup() {
	if(_synth) return true;

	SS_SoundBank *fileBank = nullptr;
	SS_FilteredBanks *filteredFileBank = nullptr;
	if(sFileSoundFontName.length()) {
		const char *path = sFileSoundFontName.c_str();
		if(has_ext_ci(path, ".sflist") || has_ext_ci(path, ".json"))
			filteredFileBank = open_sflist(path);
		else
			fileBank = open_font(path);
	}

	SS_SoundBank *globalBank = nullptr;
	SS_FilteredBanks *filteredGlobalBank = nullptr;
	if(sSoundFontName.length()) {
		const char *path = sSoundFontName.c_str();
		if(has_ext_ci(path, ".sflist") || has_ext_ci(path, ".json"))
			filteredGlobalBank = open_sflist(path);
		else
			globalBank = open_font(path);
	}

	const bool has_embedded = midi_file && midi_file->embedded_soundbank &&
	                          midi_file->embedded_soundbank_size > 0;

	if(!fileBank && !filteredFileBank && !globalBank && !filteredGlobalBank && !has_embedded) {
		return false;
	}

	if(fileBank) _banks.push_back(fileBank);
	if(globalBank) _banks.push_back(globalBank);

	if(filteredFileBank) _filteredBanks.push_back(filteredFileBank);
	if(filteredGlobalBank) _filteredBanks.push_back(filteredGlobalBank);

	SS_ProcessorOptions opts = {
		.enable_effects = true,
		.voice_cap = 512,
		.interpolation = interp,
		.preload_all_samples = false,
		.preload_instruments = false
	};

	_synth = ss_processor_create((uint32_t)std::lround(dSampleRate), &opts);
	if(!_synth) {
		return false;
	}

	if(fileBank) {
		if(!ss_processor_load_soundbank(_synth, fileBank, "fileBank", fileBankOffset, false))
			return false;
		std::erase(_banks, fileBank);
	}
	if(filteredFileBank) {
		if(!ss_processor_load_filtered_banks(_synth, filteredFileBank, "fileBank", false))
			return false;
		std::erase(_filteredBanks, filteredFileBank);
	}
	if(globalBank) {
		if(!ss_processor_load_soundbank(_synth, globalBank, "globalBank", 0, false))
			return false;
		std::erase(_banks, globalBank);
	}
	if(filteredGlobalBank) {
		if(!ss_processor_load_filtered_banks(_synth, filteredGlobalBank, "globalBank", false))
			return false;
		std::erase(_filteredBanks, filteredGlobalBank);
	}

	/* Embedded RMID soundbank is auto-loaded by ss_sequencer_load_midi. */

	initialized = true;
	return true;
}

void SpessaPlayer::renderChunk(float *out, uint32_t sample_count) {
	if(!_synth) return;
	ss_processor_render_interleaved(_synth, out, sample_count);
}
