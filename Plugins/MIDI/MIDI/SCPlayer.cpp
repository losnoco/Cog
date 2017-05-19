#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <vector>
#include <unistd.h>

#include "SCPlayer.h"

// YAY! OS X doesn't unload dylibs on dlclose, so we cache up to two sets of instances here

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static const unsigned int g_max_instances = 2;
static std::vector<unsigned int> g_instances_open;
static SCCore g_sampler[3 * g_max_instances];

SCPlayer::SCPlayer() : MIDIPlayer(), initialized(false), mode(sc_default), sccore_path(0)
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

static const uint8_t syx_reset_gm[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
static const uint8_t syx_reset_gm2[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7 };
static const uint8_t syx_reset_gs[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
static const uint8_t syx_reset_xg[] = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

static const uint8_t syx_gs_limit_bank_lsb[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x41, 0x00, 0x03, 0x00, 0xF7 };

static bool syx_equal(const uint8_t * a, const uint8_t * b)
{
	while (*a != 0xF7 && *b != 0xF7 && *a == *b)
	{
		a++; b++;
	}
	
	return *a == *b;
}

static bool syx_is_reset(const uint8_t * data)
{
	return syx_equal(data, syx_reset_gm) || syx_equal(data, syx_reset_gm2) || syx_equal(data, syx_reset_gs) || syx_equal(data, syx_reset_xg);
}

void SCPlayer::send_sysex(uint32_t port, const uint8_t * data)
{
	sampler[port].TG_LongMidiIn( data, 0 );
	if (syx_is_reset(data) && mode != sc_default)
	{
		reset(port);
	}
}

void SCPlayer::send_gs(uint32_t port, uint8_t * data)
{
	unsigned long i;
	unsigned char checksum = 0;
	for (i = 5; data[i+1] != 0xF7; ++i)
		checksum += data[i];
	checksum = (128 - checksum) & 127;
	data[i] = checksum;
	sampler[port].TG_LongMidiIn( data, 0 );
}

void SCPlayer::reset_sc(uint32_t port)
{
	unsigned int i;
	uint8_t message[11];
	
	memcpy(message, syx_gs_limit_bank_lsb, 11);

	message[7] = 1;

	switch (mode)
	{
		default: break;
			
		case sc_sc55:
			message[8] = 1;
			break;
			
		case sc_sc88:
			message[8] = 2;
			break;
			
		case sc_sc88pro:
			message[8] = 3;
			break;
			
		case sc_sc8850:
        case sc_default:
			message[8] = 4;
			break;
	}
	
	for (i = 0x41; i <= 0x49; ++i)
	{
		message[6] = i;
		send_gs(port, message);
	}
	message[6] = 0x40;
	send_gs(port, message);
	for (i = 0x4A; i <= 0x4F; ++i)
	{
		message[6] = i;
		send_gs(port, message);
	}
}

void SCPlayer::reset(uint32_t port)
{
	if (initialized)
	{
        sampler[port].TG_LongMidiIn(syx_reset_xg, 0); junk(port, 1024);
        sampler[port].TG_LongMidiIn(syx_reset_gm2, 0); junk(port, 1024);
        sampler[port].TG_LongMidiIn(syx_reset_gm, 0); junk(port, 1024);
        
        switch (mode)
		{
			case sc_gm:
				/*sampler[port].TG_LongMidiIn( syx_reset_gm, 0 );*/
				break;
				
			case sc_gm2:
				sampler[port].TG_LongMidiIn( syx_reset_gm2, 0 );
				break;
				
			case sc_sc55:
			case sc_sc88:
			case sc_sc88pro:
			case sc_sc8850:
            case sc_default:
                sampler[port].TG_LongMidiIn( syx_reset_gs, 0 ); junk(port, 1024);
				reset_sc(port);
				break;
				
			case sc_xg:
				sampler[port].TG_LongMidiIn( syx_reset_xg, 0 );
				break;
		}
        
        junk(port, 1024);
        
        {
            unsigned int i;
            for (i = 0; i < 16; ++i)
            {
                sampler[port].TG_ShortMidiIn(0x78B0 + i, 0);
                sampler[port].TG_ShortMidiIn(0x79B0 + i, 0);
                if (mode != sc_xg || i != 9)
                {
                    sampler[port].TG_ShortMidiIn(0x20B0 + i, 0);
                    sampler[port].TG_ShortMidiIn(0x00B0 + i, 0);
                    sampler[port].TG_ShortMidiIn(0xC0 + i, 0);
                }
            }
        }
        
        if (mode == sc_xg)
        {
            sampler[port].TG_ShortMidiIn(0x20B9, 0);
            sampler[port].TG_ShortMidiIn(0x7F00B9, 0);
            sampler[port].TG_ShortMidiIn(0xC9, 0);
        }
        
        junk(port, uSampleRate * 2 / 3);
	}
}

void SCPlayer::junk(uint32_t port, unsigned long count)
{
    float temp[2][1024];
    unsigned long i, j;
    for (i = 0, j = count / 1024; i < j; ++i)
    {
        memset(temp, 0, sizeof(temp));
        sampler[port].TG_setInterruptThreadIdAtThisTime();
        sampler[port].TG_Process(temp[0], temp[1], 1024);
    }
    count %= 1024;
    if (count)
    {
        memset(temp, 0, sizeof(temp));
        sampler[port].TG_setInterruptThreadIdAtThisTime();
        sampler[port].TG_Process(temp[0], temp[1], (unsigned int) count);
    }
}

void SCPlayer::set_mode(sc_mode m)
{
	mode = m;
	reset(0);
	reset(1);
	reset(2);
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
	if (!(b & 0x80000000))
	{
		unsigned port = (b >> 24) & 0x7F;
		if ( port > 2 ) port = 2;
		sampler[port].TG_ShortMidiIn(b, 0);
	}
	else
	{
		uint32_t n = b & 0xffffff;
		const uint8_t * data;
		std::size_t size, port;
		mSysexMap.get_entry( n, data, size, port );
		if ( port > 2 ) port = 2;
		send_sysex( (uint32_t) port, data );
		if ( port == 0 )
		{
			send_sysex( 1, data );
			send_sysex( 2, data );
		}
	}
}

void SCPlayer::render(float * out, unsigned long count)
{
	memset(out, 0, count * sizeof(float) * 2);
	while (count)
	{
		float buffer[2][4096];
		unsigned long todo = count > 4096 ? 4096 : count;
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
		sampler[i].TG_setMaxBlockSize(4096);
	}
	
	initialized = true;
    
    for (int i = 0; i < 3; i++)
    {
        reset(i);
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

bool SCPlayer::send_event_needs_time()
{
    return true;
}
