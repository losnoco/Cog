#include "TSFPlayer.h"

#include <stdlib.h>

#include <string>

#include <chrono>
#include <map>
#include <mutex>
#include <thread>
#include <list>

#include "tsf/tsf.h"

#define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))

struct TSF_Cached_SoundFont {
	unsigned long ref_count;
	std::chrono::steady_clock::time_point time_released;
	struct tsf *_synth;
	std::list<struct tsf *> referenced;
	TSF_Cached_SoundFont() : _synth(nullptr) { }
	TSF_Cached_SoundFont(const TSF_Cached_SoundFont &in) {
		ref_count = in.ref_count;
		time_released = in.time_released;
		_synth = in._synth;
		referenced.resize(in.referenced.size());
		std::copy(in.referenced.begin(), in.referenced.end(), referenced.begin());
	}
	TSF_Cached_SoundFont(struct tsf *in) : _synth(in), ref_count(1) { }
};

static std::mutex *Cache_Lock;

static std::map<std::string, TSF_Cached_SoundFont> *Cache_List;

static bool Cache_Running = false;

static std::thread *Cache_Thread = NULL;

static void cache_run();

static void cache_init() {
	Cache_Lock = new std::mutex;
	Cache_List = new std::map<std::string, TSF_Cached_SoundFont>;
	Cache_Thread = new std::thread(cache_run);
}

static void cache_deinit() {
	Cache_Running = false;
	Cache_Thread->join();
	delete Cache_Thread;

	for(auto it = Cache_List->begin(); it != Cache_List->end(); ++it) {
		auto &vec = it->second.referenced;
		for(auto itr = vec.begin(); itr != vec.end(); ++itr)
			tsf_close(*itr);
		if(it->second._synth)
			tsf_close(it->second._synth);
	}
	delete Cache_List;
}

static struct tsf * cache_open_font(const char *path) {
	struct tsf *synth = nullptr;

	std::lock_guard<std::mutex> lock(*Cache_Lock);

	auto &entry = (*Cache_List)[path];

	if(!entry._synth) {
		synth = tsf_load_filename(path);
		if(synth) {
			entry._synth = synth;
			entry.ref_count = 1;
			synth = tsf_copy(synth);
		} else {
			Cache_List->erase(path);
		}
	} else {
		synth = tsf_copy(entry._synth);
		++(entry.ref_count);
	}

	if(synth) {
		entry.referenced.push_back(synth);
	}

	return synth;
}

static void cache_close_font(struct tsf *synth) {
	std::lock_guard<std::mutex> lock(*Cache_Lock);

	for(auto it = Cache_List->begin(); it != Cache_List->end(); ++it) {
		auto &vec = it->second.referenced;
		auto itr = std::find(vec.begin(), vec.end(), synth);
		if(itr != vec.end()) {
			tsf_close(*itr);
			vec.erase(itr);
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
						if(it->second._synth)
							tsf_close(it->second._synth);
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

static class TSF_Initializer {
	std::mutex lock;

	bool initialized;

	std::string base_path;

	public:
	TSF_Initializer()
	: initialized(false) {
	}

	~TSF_Initializer() {
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
			initialized = true;
		}
		return initialized;
	}
} g_initializer;

TSFPlayer::TSFPlayer()
: MIDIPlayer() {
	_synth = nullptr;

	if(!g_initializer.initialize()) throw std::runtime_error("Unable to initialize TinySoundFont");
}

TSFPlayer::~TSFPlayer() {
	shutdown();
}

void TSFPlayer::send_event(uint32_t b) {
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
		case 8: tsf_channel_note_off(_synth, channel, event[1]); break;
		case 9: tsf_channel_note_on(_synth, channel, event[1], (float)(event[2]) / 127.0); break;
		case 11: tsf_channel_midi_control(_synth, channel, event[1], event[2]); break;
		case 12: tsf_channel_set_presetnumber(_synth, channel, event[1], channel == 9 || channel == (9 + 16) || channel == (9 + 32)); break;
		case 14: tsf_channel_set_pitchwheel(_synth, channel, (unsigned)((event[2] & 0x7F) << 7) + (unsigned)(event[1] & 0x7F)); break;
		default: break;
	}
}

void TSFPlayer::send_sysex(const uint8_t *data, size_t size, size_t port) {
}

void TSFPlayer::render(float *out, unsigned long count) {
	tsf_render_float(_synth, out, (int)count);
}

void TSFPlayer::setSoundFont(const char *in) {
	sSoundFontName = in;
	shutdown();
}

void TSFPlayer::setFileSoundFont(const char *in) {
	sFileSoundFontName = in;
	shutdown();
}

void TSFPlayer::shutdown() {
	cache_close_font(_synth);
}

bool TSFPlayer::startup() {
	if(_synth) return true;

	const auto &soundFont = sFileSoundFontName.length() ? sFileSoundFontName : sSoundFontName;
	_synth = cache_open_font(soundFont.c_str());
	if(!_synth) {
		return false;
	}
	tsf_set_output(_synth, TSF_STEREO_INTERLEAVED, uSampleRate, -10.0);
	tsf_reset(_synth);

	for (int i = 0; i < 48; ++i)
	{
		tsf_channel_midi_control(_synth, i, 121, 1);
		tsf_channel_set_presetnumber(_synth, i, 0, i == 9 || i == (9 + 16) || i == (9 + 32));
	}

	return true;
}
