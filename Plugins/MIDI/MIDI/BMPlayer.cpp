#include "BMPlayer.h"

#include <sflist.h>

#include <stdlib.h>

#include <string>

#include <dlfcn.h>

#include <chrono>
#include <map>
#include <mutex>
#include <thread>

#define SF2PACK

#define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))

struct Cached_SoundFont {
	unsigned long ref_count;
	std::chrono::steady_clock::time_point time_released;
	HSOUNDFONT handle;
	sflist_presets *presetlist;
	Cached_SoundFont()
	: handle(0), presetlist(0) {
	}
};

static std::mutex Cache_Lock;

static std::map<std::string, Cached_SoundFont> Cache_List;

static bool Cache_Running = false;

static std::thread *Cache_Thread = NULL;

static void cache_run();

static void cache_init() {
	Cache_Thread = new std::thread(cache_run);
}

static void cache_deinit() {
	Cache_Running = false;
	Cache_Thread->join();
	delete Cache_Thread;

	for(auto it = Cache_List.begin(); it != Cache_List.end(); ++it) {
		if(it->second.handle)
			BASS_MIDI_FontFree(it->second.handle);
		if(it->second.presetlist)
			sflist_free(it->second.presetlist);
	}
}

static HSOUNDFONT cache_open_font(const char *path) {
	HSOUNDFONT font = NULL;

	std::lock_guard<std::mutex> lock(Cache_Lock);

	Cached_SoundFont &entry = Cache_List[path];

	if(!entry.handle) {
		font = BASS_MIDI_FontInit(path, 0);
		if(font) {
			entry.handle = font;
			entry.ref_count = 1;
		} else {
			Cache_List.erase(path);
		}
	} else {
		font = entry.handle;
		++entry.ref_count;
	}

	return font;
}

static sflist_presets *sflist_open_file(const char *path) {
	sflist_presets *presetlist;
	FILE *f;
	char *sflist_file;
	char *separator;
	size_t length;
	char error[sflist_max_error];
	char base_path[32768];
	strcpy(base_path, path);
	separator = strrchr(base_path, '/');
	if(separator)
		*separator = '\0';
	else
		base_path[0] = '\0';

	f = fopen(path, "r");
	if(!f) return 0;

	fseek(f, 0, SEEK_END);
	length = ftell(f);
	fseek(f, 0, SEEK_SET);

	sflist_file = (char *)malloc(length + 1);
	if(!sflist_file) {
		fclose(f);
		return 0;
	}

	length = fread(sflist_file, 1, length, f);
	fclose(f);

	sflist_file[length] = '\0';

	presetlist = sflist_load(sflist_file, strlen(sflist_file), base_path, error);

	free(sflist_file);

	return presetlist;
}

static sflist_presets *cache_open_list(const char *path) {
	sflist_presets *presetlist = NULL;

	std::lock_guard<std::mutex> lock(Cache_Lock);

	Cached_SoundFont &entry = Cache_List[path];

	if(!entry.presetlist) {
		presetlist = sflist_open_file(path);
		if(presetlist) {
			entry.presetlist = presetlist;
			entry.ref_count = 1;
		} else {
			Cache_List.erase(path);
		}
	} else {
		presetlist = entry.presetlist;
		++entry.ref_count;
	}

	return presetlist;
}

static void cache_close_font(HSOUNDFONT handle) {
	std::lock_guard<std::mutex> lock(Cache_Lock);

	for(auto it = Cache_List.begin(); it != Cache_List.end(); ++it) {
		if(it->second.handle == handle) {
			if(--it->second.ref_count == 0)
				it->second.time_released = std::chrono::steady_clock::now();
			break;
		}
	}
}

static void cache_close_list(sflist_presets *presetlist) {
	std::lock_guard<std::mutex> lock(Cache_Lock);

	for(auto it = Cache_List.begin(); it != Cache_List.end(); ++it) {
		if(it->second.presetlist == presetlist) {
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
			std::lock_guard<std::mutex> lock(Cache_Lock);
			for(auto it = Cache_List.begin(); it != Cache_List.end();) {
				if(it->second.ref_count == 0) {
					auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.time_released);
					if(elapsed.count() >= 10) {
						if(it->second.handle)
							BASS_MIDI_FontFree(it->second.handle);
						if(it->second.presetlist)
							sflist_free(it->second.presetlist);
						it = Cache_List.erase(it);
						continue;
					}
				}
				++it;
			}
		}

		std::this_thread::sleep_for(dura);
	}
}

static class Bass_Initializer {
	std::mutex lock;

	bool initialized;

	std::string base_path;

	public:
	Bass_Initializer()
	: initialized(false) {
	}

	~Bass_Initializer() {
		if(initialized) {
			cache_deinit();
			// BASS_Free(); // this is only called on shutdown anyway
		}
	}

	bool check_initialized() {
		std::lock_guard<std::mutex> lock(this->lock);
		return initialized;
	}

	void set_base_path() {
		Dl_info info;
		dladdr((void *)&BASS_Init, &info);
		base_path = info.dli_fname;
		size_t slash = base_path.find_last_of('/');
		base_path.erase(base_path.begin() + slash + 1, base_path.end());
	}

	void load_plugin(const char *name) {
		std::string full_path = base_path;
		full_path += name;
		BASS_PluginLoad(full_path.c_str(), 0);
	}

	bool initialize() {
		std::lock_guard<std::mutex> lock(this->lock);
		if(!initialized) {
#ifdef SF2PACK
			set_base_path();
			load_plugin("libbassflac.dylib");
			load_plugin("libbasswv.dylib");
			load_plugin("libbassopus.dylib");
			load_plugin("libbass_mpc.dylib");
#endif
			BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
			initialized = !!BASS_Init(0, 44100, 0, NULL, NULL);
			if(initialized) {
				BASS_SetConfigPtr(BASS_CONFIG_MIDI_DEFFONT, NULL);
				BASS_SetConfig(BASS_CONFIG_MIDI_VOICES, 256);
				cache_init();
			}
		}
		return initialized;
	}
} g_initializer;

BMPlayer::BMPlayer()
: MIDIPlayer() {
	_stream = NULL;
	bSincInterpolation = false;
	_presetList = 0;

	if(!g_initializer.initialize()) throw std::runtime_error("Unable to initialize BASS");
}

BMPlayer::~BMPlayer() {
	shutdown();
}

void BMPlayer::setSincInterpolation(bool enable) {
	bSincInterpolation = enable;

	shutdown();
}

void BMPlayer::send_event(uint32_t b) {
	uint8_t event[3];
	event[0] = static_cast<uint8_t>(b);
	event[1] = static_cast<uint8_t>(b >> 8);
	event[2] = static_cast<uint8_t>(b >> 16);
	unsigned port = (b >> 24) & 0x7F;
	if(port > 2) port = 0;
	const unsigned channel = (b & 0x0F) + port * 16;
	const unsigned command = b & 0xF0;
	const unsigned event_length = (command >= 0xF8 && command <= 0xFF) ? 1 : ((command == 0xC0 || command == 0xD0) ? 2 : 3);
	BASS_MIDI_StreamEvents(_stream, BASS_MIDI_EVENTS_RAW + 1 + channel, event, event_length);
}

void BMPlayer::send_sysex(const uint8_t *data, size_t size, size_t port) {
	BASS_MIDI_StreamEvents(_stream, BASS_MIDI_EVENTS_RAW, data, static_cast<unsigned int>(size));
}

void BMPlayer::render(float *out, unsigned long count) {
	BASS_ChannelGetData(_stream, out, BASS_DATA_FLOAT | (unsigned int)(count * sizeof(float) * 2));
}

void BMPlayer::setSoundFont(const char *in) {
	sSoundFontName = in;
	shutdown();
}

void BMPlayer::setFileSoundFont(const char *in) {
	sFileSoundFontName = in;
	shutdown();
}

void BMPlayer::shutdown() {
	if(_stream) BASS_StreamFree(_stream);
	_stream = NULL;
	for(unsigned long i = 0; i < _soundFonts.size(); ++i) {
		cache_close_font(_soundFonts[i]);
	}
	_soundFonts.resize(0);
	if(_presetList) {
		cache_close_list(_presetList);
		_presetList = 0;
	}
}

bool BMPlayer::startup() {
	if(_stream) return true;

	_stream = BASS_MIDI_StreamCreate(48, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | (bSincInterpolation ? BASS_MIDI_SINCINTER : 0), (unsigned int)uSampleRate);
	if(!_stream) {
		return false;
	}
	BASS_MIDI_StreamEvent(_stream, 9, MIDI_EVENT_DEFDRUMS, 1);
	BASS_MIDI_StreamEvent(_stream, 9 + 16, MIDI_EVENT_DEFDRUMS, 1);
	BASS_MIDI_StreamEvent(_stream, 9 + 32, MIDI_EVENT_DEFDRUMS, 1);
	std::vector<BASS_MIDI_FONTEX2> presetList;
	if(sFileSoundFontName.length()) {
		HSOUNDFONT font = cache_open_font(sFileSoundFontName.c_str());
		if(!font) {
			shutdown();
			return false;
		}
		_soundFonts.push_back(font);
		presetList.push_back({ font, -1, -1, -1, 0, 0, 0, 48 });
	}

	if(sSoundFontName.length()) {
		std::string ext;
		size_t dot = sSoundFontName.find_last_of('.');
		if(dot != std::string::npos) ext.assign(sSoundFontName.begin() + dot + 1, sSoundFontName.end());
		if(!strcasecmp(ext.c_str(), "sf2") || !strcasecmp(ext.c_str(), "sf3")
#ifdef SF2PACK
		   || !strcasecmp(ext.c_str(), "sf2pack")
#endif
		) {
			HSOUNDFONT font = cache_open_font(sSoundFontName.c_str());
			if(!font) {
				shutdown();
				return false;
			}
			_soundFonts.push_back(font);
			presetList.push_back({ font, -1, -1, -1, 0, 0, 0, 48 });
		} else if(!strcasecmp(ext.c_str(), "sflist") || !strcasecmp(ext.c_str(), "json")) {
			_presetList = cache_open_list(sSoundFontName.c_str());
			if(!_presetList) {
				shutdown();
				return false;
			}
			for(unsigned int i = 0, j = _presetList->count; i < j; ++i) {
				presetList.push_back(_presetList->presets[i]);
			}
		}
	}

	BASS_MIDI_StreamSetFonts(_stream, &presetList[0], (unsigned int)presetList.size() | BASS_MIDI_FONT_EX2);

	return true;
}
