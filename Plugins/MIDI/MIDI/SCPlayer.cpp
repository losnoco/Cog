#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <vector>
#include <unistd.h>

#include "SCPlayer.h"

#define BLOCK_SIZE (512)

// YAY! OS X doesn't unload dylibs on dlclose, so we cache up to two sets of instances here

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static const unsigned int g_max_instances = 2;
static std::vector<unsigned int> g_instances_open;
static SCCore g_sampler[3 * g_max_instances];

SCPlayer::SCPlayer() : MIDIPlayer(), initialized(false), sccore_path(0)
{
    pthread_mutex_lock(&g_lock);
    while (g_instances_open.size() >= g_max_instances)
    {
        pthread_mutex_unlock(&g_lock);
        usleep(10000);
        pthread_mutex_lock(&g_lock);
    }
    unsigned int i;
    for (i = 0; i < g_max_instances; ++i)
    {
        if (std::find(g_instances_open.begin(), g_instances_open.end(), i) == g_instances_open.end())
            break;
    }
    g_instances_open.push_back(i);
    instance_id = i;
    sampler = &g_sampler[i * 3];
    pthread_mutex_unlock(&g_lock);
}

SCPlayer::~SCPlayer()
{
	shutdown();
    if (sccore_path)
        free(sccore_path);
    if (sampler)
    {
        pthread_mutex_lock(&g_lock);
        auto it = std::find(g_instances_open.begin(), g_instances_open.end(), instance_id);
        it = g_instances_open.erase(it);
        pthread_mutex_unlock(&g_lock);
    }
}

void SCPlayer::set_sccore_path(const char *path)
{
    size_t len;
    if (sccore_path) free(sccore_path);
    len = strlen(path);
    sccore_path = (char *) malloc(len + 1);
    if (sccore_path)
        memcpy(sccore_path, path, len + 1);
}

void SCPlayer::send_event(uint32_t b)
{
    send_event_time(b, 0);
}

void SCPlayer::send_sysex(const uint8_t * data, size_t size, size_t port)
{
    send_sysex_time(data, size, port, 0);
}

void SCPlayer::send_event_time(uint32_t b, unsigned int time)
{
    unsigned port = (b >> 24) & 0x7F;
	if ( port > 2 ) port = 0;
	sampler[port].TG_ShortMidiIn(b, time);
}

void SCPlayer::send_sysex_time(const uint8_t * data, size_t size, size_t port, unsigned int time)
{
    if ( port > 2 ) port = 0;
    sampler[port].TG_LongMidiIn(data, time);
    if (port == 0)
    {
        sampler[1].TG_LongMidiIn(data, time);
        sampler[2].TG_LongMidiIn(data, time);
    }
}

void SCPlayer::render(float * out, unsigned long count)
{
	memset(out, 0, count * sizeof(float) * 2);
	while (count)
	{
		float buffer[2][BLOCK_SIZE];
		unsigned long todo = count > BLOCK_SIZE ? BLOCK_SIZE : count;
		for (unsigned long i = 0; i < 3; ++i)
		{
			memset(buffer[0], 0, todo * sizeof(float));
			memset(buffer[1], 0, todo * sizeof(float));
		
			sampler[i].TG_setInterruptThreadIdAtThisTime();
			sampler[i].TG_Process(buffer[0], buffer[1], (unsigned int) todo);

			for (unsigned long j = 0; j < todo; ++j)
			{
				out[j * 2 + 0] += buffer[0][j];
				out[j * 2 + 1] += buffer[1][j];
			}
		}
		out += todo * 2;
		count -= todo;
	}
}

void SCPlayer::shutdown()
{
	for (int i = 0; i < 3; i++)
    {
        if (sampler[i].TG_deactivate)
        {
            sampler[i].TG_flushMidi();
            sampler[i].TG_deactivate();
        }
    }
	initialized = false;
}

bool SCPlayer::startup()
{
    int i;
    
    if (initialized) return true;
    
    if (!sccore_path) return false;

	for (i = 0; i < 3; i++)
	{
        if (!sampler[i].TG_initialize)
        {
            if (!sampler[i].Load(sccore_path, true))
                return false;
		
            if (sampler[i].TG_initialize(0) < 0)
                return false;
        }
		
		sampler[i].TG_activate(44100.0, 1024);
		sampler[i].TG_setMaxBlockSize(256);
		sampler[i].TG_setSampleRate((float)uSampleRate);
		sampler[i].TG_setSampleRate((float)uSampleRate);
		sampler[i].TG_setMaxBlockSize(BLOCK_SIZE);
	}
	
	initialized = true;
    
    for (int i = 0; i < 3; i++)
    {
        reset(i, 0);
    }
    
	return true;
}

unsigned int SCPlayer::get_playing_note_count()
{
	unsigned int total = 0;
	unsigned int i;

	if (!initialized)
		return 0;

	for (i = 0; i < 3; i++)
		total += sampler[i].TG_XPgetCurTotalRunningVoices();

	return total;
}

unsigned int SCPlayer::send_event_needs_time()
{
    return BLOCK_SIZE;
}
