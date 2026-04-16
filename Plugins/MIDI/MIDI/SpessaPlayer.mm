#include "SpessaPlayer.h"

#import "Plugin.h"

#include <stdlib.h>

#include <string>

#include <chrono>
#include <map>
#include <mutex>
#include <thread>
#include <list>

#define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))

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

struct Spessa_Cached_SoundFont {
	unsigned long ref_count;
	std::chrono::steady_clock::time_point time_released;
	SS_SoundBank *bank;
	Spessa_Cached_SoundFont() : bank(nullptr) { }
	Spessa_Cached_SoundFont(const Spessa_Cached_SoundFont &in) {
		ref_count = in.ref_count;
		time_released = in.time_released;
		bank = in.bank;
	}
	Spessa_Cached_SoundFont(SS_SoundBank *in) : bank(in), ref_count(1) { }
};

static std::mutex *Cache_Lock;

static std::map<std::string, Spessa_Cached_SoundFont> *Cache_List;

static bool Cache_Running = false;

static std::thread *Cache_Thread = NULL;

static void cache_run();

static void cache_init() {
	Cache_Lock = new std::mutex;
	Cache_List = new std::map<std::string, Spessa_Cached_SoundFont>;
	Cache_Thread = new std::thread(cache_run);
}

static void cache_deinit() {
	Cache_Running = false;
	Cache_Thread->join();
	delete Cache_Thread;

	for(auto it = Cache_List->begin(); it != Cache_List->end(); ++it) {
		if(it->second.bank)
			ss_soundbank_free(it->second.bank);
	}
	delete Cache_List;
}

static SS_SoundBank *cache_open_font(const char *path) {
	SS_SoundBank *bank = nullptr;

	std::lock_guard<std::mutex> lock(*Cache_Lock);

	auto &entry = (*Cache_List)[path];

	if(!entry.bank) {
		SS_File *bankFile = cog_file_open(path);
		if(bankFile) {
			bank = ss_soundbank_load(bankFile);
			if(bank) {
				entry.bank = bank;
				entry.ref_count = 1;
			} else {
				Cache_List->erase(path);
			}
			ss_file_close(bankFile);
		} else {
			Cache_List->erase(path);
		}
	} else {
		bank = entry.bank;
		++(entry.ref_count);
	}

	return bank;
}

static void cache_close_font(SS_SoundBank *bank) {
	std::lock_guard<std::mutex> lock(*Cache_Lock);

	for(auto it = Cache_List->begin(); it != Cache_List->end(); ++it) {
		if(it->second.bank == bank) {
			if(--it->second.ref_count == 0)
				it->second.time_released = std::chrono::steady_clock::now();
			break;
		}
	}
}

static void cache_run() {
	std::chrono::milliseconds dura(250);

	Cache_Running = true;

	while(Cache_Running) {
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

		{
			std::lock_guard<std::mutex> lock(*Cache_Lock);
			for(auto it = Cache_List->begin(); it != Cache_List->end();) {
				if(it->second.ref_count == 0) {
					auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.time_released);
					if(elapsed.count() >= 10) {
						if(it->second.bank)
							ss_soundbank_free(it->second.bank);
						it = Cache_List->erase(it);
						continue;
					}
				}
				++it;
			}
		}

		std::this_thread::sleep_for(dura);
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
			cache_deinit();
		}
	}

	bool check_initialized() {
		std::lock_guard<std::mutex> lock(this->lock);
		return initialized;
	}

	bool initialize() {
		std::lock_guard<std::mutex> lock(this->lock);
		if(!initialized) {
			cache_init();
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

	if(!g_initializer.initialize()) throw std::runtime_error("Unable to initialize SpessaSynth");
}

SpessaPlayer::~SpessaPlayer() {
	shutdown();
}


void SpessaPlayer::send_event(uint32_t b) {
	send_event_time(b, 0);
}

void SpessaPlayer::send_sysex(const uint8_t *data, size_t size, size_t port) {
	send_sysex_time(data, size, port, 0);
}

void SpessaPlayer::send_event_time(uint32_t b, unsigned int time) {
	double t = playerTime + ((double)time) / dSampleRate;
	uint8_t event[3];
	event[0] = static_cast<uint8_t>(b);
	event[1] = static_cast<uint8_t>(b >> 8);
	event[2] = static_cast<uint8_t>(b >> 16);
	unsigned port = (b >> 24) & 0x7F;
	if(port > 2) port = 0;
	const unsigned channel = (b & 0x0F) + port * 16;
	const unsigned command = (b & 0xF0) >> 4;
	switch(command)
	{
		case 8: ss_processor_note_off(_synth, channel, event[1], t); break;
		case 9: ss_processor_note_on(_synth, channel, event[1], event[2], t); break;
		case 10: ss_processor_poly_pressure(_synth, channel, event[1], event[2], t); break;
		case 11: ss_processor_control_change(_synth, channel, event[1], event[2], t); break;
		case 12: ss_processor_program_change(_synth, channel, event[1], t); break;
		case 13: ss_processor_channel_pressure(_synth, channel, event[1], t); break;
		case 14: ss_processor_pitch_wheel(_synth, channel, ((unsigned)(event[2] & 0x7F) << 7) + (unsigned)(event[1] & 0x7F), -1, t); break;
		default: break;
	}
}

void SpessaPlayer::send_sysex_time(const uint8_t *data, size_t size, size_t port, unsigned int time) {
	if (size < 2 || data[0] != 0xF0 || data[size - 1] != 0xF7) return;
	double t = playerTime + ((double)time) / dSampleRate;
	ss_processor_sysex(_synth, data + 1, size - 2, t);
}

void SpessaPlayer::render(float *out, unsigned long count) {
	const double timeAdd = (double)count / dSampleRate;
	ss_processor_render_interleaved(_synth, out, (uint32_t)count);
	playerTime += timeAdd;
}

void SpessaPlayer::setSoundFont(const char *in) {
	sSoundFontName = in;
	shutdown();
}

void SpessaPlayer::setFileSoundFont(const char *in) {
	sFileSoundFontName = in;
	shutdown();
}

void SpessaPlayer::setEmbeddedBank(const uint8_t *embedded_bank, size_t bank_size, uint16_t bank_offset) {
	embeddedBank.assign(embedded_bank, embedded_bank + bank_size);
	bankOffset = bank_offset;
}

void SpessaPlayer::setInterpolation(SS_InterpolationType interp)
{
	this->interp = interp;
	shutdown();
}

void SpessaPlayer::shutdown() {
	if (_synth) {
		ss_processor_remove_soundbank(_synth, "fileBank", true);
		ss_processor_remove_soundbank(_synth, "globalBank", true);
		ss_processor_free(_synth); _synth = nullptr;
	}
	for (auto it = _banks.begin(); it != _banks.end(); ++it)
		cache_close_font(*it);
	_banks.resize(0);
}

bool SpessaPlayer::startup() {
	if(_synth) return true;

	SS_SoundBank* fileBank = nullptr;
	if (sFileSoundFontName.length())
		fileBank = cache_open_font(sFileSoundFontName.c_str());

	SS_SoundBank* globalBank = nullptr;
	if (sSoundFontName.length())
		globalBank = cache_open_font(sSoundFontName.c_str());

	if (!fileBank && !globalBank) {
		return false;
	}

	SS_SoundBank* _embeddedBank = NULL;
	if (embeddedBank.size()) {
		SS_File *embedFile = ss_file_open_from_memory(embeddedBank.data(), embeddedBank.size(), false);
		if(embedFile) {
			_embeddedBank = ss_soundbank_load(embedFile);
			ss_file_close(embedFile);
		}
	}

	if (fileBank) _banks.push_back(fileBank);
	if (globalBank) _banks.push_back(globalBank);

	SS_ProcessorOptions opts;
	opts.enable_effects = true;
	opts.voice_cap = 512;
	opts.interpolation = interp;

	_synth = ss_processor_create(round(dSampleRate), &opts);
	if(!_synth) {
		return false;
	}

	/* This bank will be owned */
	if (_embeddedBank && !ss_processor_load_soundbank(_synth, _embeddedBank, "embeddedBank", bankOffset))
		return false;

	if (fileBank && !ss_processor_load_soundbank(_synth, fileBank, "fileBank", 0))
		return false;
	if (globalBank && !ss_processor_load_soundbank(_synth, globalBank, "globalBank", 0))
		return false;

	playerTime = 0;

	return true;
}

unsigned int SpessaPlayer::send_event_needs_time() {
	return outputMax;
}
