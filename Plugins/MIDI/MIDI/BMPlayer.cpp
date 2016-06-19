#include "BMPlayer.h"

#include <sflist.h>

#include <stdlib.h>

#include <string>

#include <dlfcn.h>

#include <map>
#include <chrono>
#include <thread>
#include <mutex>

#define SF2PACK

#define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))

struct Cached_SoundFont
{
    unsigned long ref_count;
    std::chrono::steady_clock::time_point time_released;
    HSOUNDFONT handle;
    sflist_presets * presetlist;
    Cached_SoundFont() : handle( 0 ), presetlist( 0 ) { }
};

static std::mutex Cache_Lock;

static std::map<std::string, Cached_SoundFont> Cache_List;

static bool Cache_Running = false;

static std::thread * Cache_Thread = NULL;

static void cache_run();

static void cache_init()
{
    Cache_Thread = new std::thread( cache_run );
}

static void cache_deinit()
{
    Cache_Running = false;
    Cache_Thread->join();
    delete Cache_Thread;
    
    for ( auto it = Cache_List.begin(); it != Cache_List.end(); ++it )
    {
        if ( it->second.handle )
            BASS_MIDI_FontFree( it->second.handle );
        if ( it->second.presetlist )
            sflist_free( it->second.presetlist );
    }
}

static HSOUNDFONT cache_open_font( const char * path )
{
    HSOUNDFONT font = NULL;

    std::lock_guard<std::mutex> lock( Cache_Lock );

    Cached_SoundFont & entry = Cache_List[ path ];
    
    if ( !entry.handle )
    {
        font = BASS_MIDI_FontInit( path, 0 );
        if ( font )
        {
            entry.handle = font;
            entry.ref_count = 1;
        }
        else
        {
            Cache_List.erase( path );
        }
    }
    else
    {
        font = entry.handle;
        ++entry.ref_count;
    }
    
    return font;
}

static sflist_presets * sflist_open_file( const char * path )
{
    sflist_presets * presetlist;
    FILE * f;
    char * sflist_file;
    char * separator;
    size_t length;
    char error[sflist_max_error];
    char base_path[32768];
    strcpy(base_path, path);
    separator = strrchr(base_path, '/');
    if (separator)
        *separator = '\0';
    else
        base_path[0] = '\0';
    
    f = fopen( path, "r" );
    if (!f) return 0;
    
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    sflist_file = (char *) malloc(length + 1);
    if (!sflist_file)
    {
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

static sflist_presets * cache_open_list( const char * path )
{
    sflist_presets * presetlist = NULL;
    
    std::lock_guard<std::mutex> lock( Cache_Lock );
    
    Cached_SoundFont & entry = Cache_List[ path ];
    
    if ( !entry.presetlist )
    {
        
        presetlist = sflist_open_file( path );
        if ( presetlist )
        {
            entry.presetlist = presetlist;
            entry.ref_count = 1;
        }
        else
        {
            Cache_List.erase( path );
        }
    }
    else
    {
        presetlist = entry.presetlist;
        ++entry.ref_count;
    }
    
    return presetlist;
}

static void cache_close_font( HSOUNDFONT handle )
{
    std::lock_guard<std::mutex> lock( Cache_Lock );
    
    for ( auto it = Cache_List.begin(); it != Cache_List.end(); ++it )
    {
        if ( it->second.handle == handle )
        {
            if ( --it->second.ref_count == 0 )
                it->second.time_released = std::chrono::steady_clock::now();
            break;
        }
    }
}

static void cache_close_list( sflist_presets * presetlist )
{
    std::lock_guard<std::mutex> lock( Cache_Lock );
    
    for ( auto it = Cache_List.begin(); it != Cache_List.end(); ++it )
    {
        if ( it->second.presetlist == presetlist )
        {
            if ( --it->second.ref_count == 0 )
                it->second.time_released = std::chrono::steady_clock::now();
            break;
        }
    }
}

static void cache_run()
{
    std::chrono::milliseconds dura( 250 );
    
    Cache_Running = true;
    
    while ( Cache_Running )
    {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        
        {
            std::lock_guard<std::mutex> lock( Cache_Lock );
            for ( auto it = Cache_List.begin(); it != Cache_List.end(); )
            {
                if ( it->second.ref_count == 0 )
                {
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds> ( now - it->second.time_released );
                    if ( elapsed.count() >= 10 )
                    {
                        if ( it->second.handle )
                            BASS_MIDI_FontFree( it->second.handle );
                        if ( it->second.presetlist )
                            sflist_free( it->second.presetlist );
                        it = Cache_List.erase( it );
                        continue;
                    }
                }
                ++it;
            }
        }
        
        std::this_thread::sleep_for( dura );
    }
}

static class Bass_Initializer
{
    std::mutex lock;

	bool initialized;
    
    std::string base_path;

public:
	Bass_Initializer() : initialized(false)
    {
    }

	~Bass_Initializer()
	{
		if ( initialized )
		{
            cache_deinit();
			BASS_Free();
		}
	}

	bool check_initialized()
	{
        std::lock_guard<std::mutex> lock( this->lock );
		return initialized;
	}
    
    void set_base_path()
    {
        Dl_info info;
        dladdr( (void*) &BASS_Init, &info );
        base_path = info.dli_fname;
        size_t slash = base_path.find_last_of( '/' );
        base_path.erase( base_path.begin() + slash + 1, base_path.end() );
    }
    
    void load_plugin(const char * name)
    {
        std::string full_path = base_path;
        full_path += name;
        BASS_PluginLoad( full_path.c_str(), 0 );
    }

	bool initialize()
	{
        std::lock_guard<std::mutex> lock( this->lock );
		if ( !initialized )
		{
#ifdef SF2PACK
            set_base_path();
            load_plugin( "libbassflac.dylib" );
            load_plugin( "libbasswv.dylib" );
            load_plugin( "libbassopus.dylib" );
            load_plugin( "libbass_mpc.dylib" );
#endif
			BASS_SetConfig( BASS_CONFIG_UPDATEPERIOD, 0 );
			initialized = !!BASS_Init( 0, 44100, 0, NULL, NULL );
			if ( initialized )
			{
				BASS_SetConfigPtr( BASS_CONFIG_MIDI_DEFFONT, NULL );
				BASS_SetConfig( BASS_CONFIG_MIDI_VOICES, 256 );
                cache_init();
			}
		}
		return initialized;
	}
} g_initializer;

BMPlayer::BMPlayer() : MIDIPlayer()
{
	memset(_stream, 0, sizeof(_stream));
	bSincInterpolation = false;
    _presetList = 0;

	if ( !g_initializer.initialize() ) throw std::runtime_error( "Unable to initialize BASS" );
}

BMPlayer::~BMPlayer()
{
	shutdown();
}

void BMPlayer::setSincInterpolation(bool enable)
{
	bSincInterpolation = enable;

	shutdown();
}

void BMPlayer::send_event(uint32_t b)
{
	if (!(b & 0x80000000))
	{
		unsigned char event[ 3 ];
		event[ 0 ] = (unsigned char)b;
		event[ 1 ] = (unsigned char)( b >> 8 );
		event[ 2 ] = (unsigned char)( b >> 16 );
		unsigned port = (b >> 24) & 0x7F;
		unsigned channel = b & 0x0F;
		unsigned command = b & 0xF0;
		unsigned event_length = ( command == 0xC0 || command == 0xD0 ) ? 2 : 3;
        if ( port > 2 ) port = 2;
        if ( bank_lsb_overridden && command == 0xB0 && event[ 1 ] == 0x20 ) return;
		BASS_MIDI_StreamEvents( _stream[port], BASS_MIDI_EVENTS_RAW + 1 + channel, event, event_length );
	}
	else
	{
		uint32_t n = b & 0xffffff;
		const uint8_t * data;
        std::size_t size, port;
		mSysexMap.get_entry( n, data, size, port );
		if ( port > 2 ) port = 2;
		BASS_MIDI_StreamEvents( _stream[port], BASS_MIDI_EVENTS_RAW, data, (unsigned int) size );
        if ( port == 0 )
        {
            BASS_MIDI_StreamEvents( _stream[1], BASS_MIDI_EVENTS_RAW, data, (unsigned int) size );
            BASS_MIDI_StreamEvents( _stream[2], BASS_MIDI_EVENTS_RAW, data, (unsigned int) size );
        }
	}
}

void BMPlayer::render(float * out, unsigned long count)
{
    float buffer[1024];
    while (count)
    {
        unsigned long todo = count;
        if ( todo > 512 )
            todo = 512;
        memset(out, 0, todo * sizeof(float) * 2);
        for (unsigned long i = 0; i < 3; ++i)
        {
            BASS_ChannelGetData( _stream[i], buffer, BASS_DATA_FLOAT | (unsigned int) ( todo * sizeof( float ) * 2 ) );
            for (unsigned long j = 0; j < todo * 2; ++j)
            {
                out[j] += buffer[j];
            }
        }
        out += todo * 2;
        count -= todo;
    }
}

void BMPlayer::setSoundFont( const char * in )
{
	sSoundFontName = in;
	shutdown();
}

void BMPlayer::setFileSoundFont( const char * in )
{
	sFileSoundFontName = in;
	shutdown();
}

void BMPlayer::shutdown()
{
	if ( _stream[2] ) BASS_StreamFree( _stream[2] );
    if ( _stream[1] ) BASS_StreamFree( _stream[1] );
    if ( _stream[0] ) BASS_StreamFree( _stream[0] );
    memset(_stream, 0, sizeof(_stream));
	for ( unsigned long i = 0; i < _soundFonts.size(); ++i )
	{
		cache_close_font( _soundFonts[i] );
    }
	_soundFonts.resize( 0 );
    if ( _presetList )
    {
        cache_close_list( _presetList );
        _presetList = 0;
    }
}

void BMPlayer::compound_presets( std::vector<BASS_MIDI_FONTEX> & out, std::vector<BASS_MIDI_FONTEX> & in, std::vector<long> & channels )
{
    if ( !in.size() )
        in.push_back( { 0, -1, -1, -1, 0, 0 } );
    if ( channels.size() )
    {
        for ( auto pit = in.begin(); pit != in.end(); ++pit )
        {
            for ( auto it = channels.begin(); it != channels.end(); ++it )
            {
                bank_lsb_override[ *it - 1 ] = *it;
                
                int dbanklsb = (int) *it;
                pit->dbanklsb = dbanklsb;
                out.push_back( *pit );
            }
        }
    }
    else
    {
        for ( auto pit = in.begin(); pit != in.end(); ++pit )
        {
            out.push_back( *pit );
        }
    }
}

bool BMPlayer::startup()
{
	if ( _stream[0] && _stream[1] && _stream[2] ) return true;

	_stream[0] = BASS_MIDI_StreamCreate( 16, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | ( bSincInterpolation ? BASS_MIDI_SINCINTER : 0 ), (unsigned int) uSampleRate );
    _stream[1] = BASS_MIDI_StreamCreate( 16, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | ( bSincInterpolation ? BASS_MIDI_SINCINTER : 0 ), (unsigned int) uSampleRate );
    _stream[2] = BASS_MIDI_StreamCreate( 16, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | ( bSincInterpolation ? BASS_MIDI_SINCINTER : 0 ), (unsigned int) uSampleRate );
	if (!_stream[0] || !_stream[1] || !_stream[2])
	{
		return false;
	}
    memset( bank_lsb_override, 0, sizeof( bank_lsb_override ) );
    std::vector<BASS_MIDI_FONTEX> presetList;
    if ( sFileSoundFontName.length() )
    {
        HSOUNDFONT font = cache_open_font( sFileSoundFontName.c_str() );
        if ( !font )
        {
            shutdown();
            return false;
        }
        _soundFonts.push_back( font );
        presetList.push_back( { font, -1, -1, -1, 0, 0 } );
    }
    
    if (sSoundFontName.length())
	{
        std::string ext;
        size_t dot = sSoundFontName.find_last_of('.');
        if (dot != std::string::npos) ext.assign( sSoundFontName.begin() +  dot + 1, sSoundFontName.end() );
		if ( !strcasecmp( ext.c_str(), "sf2" )
#ifdef SF2PACK
			|| !strcasecmp( ext.c_str(), "sf2pack" )
#endif
			)
		{
			HSOUNDFONT font = cache_open_font( sSoundFontName.c_str() );
			if ( !font )
			{
				shutdown();
				return false;
			}
            _soundFonts.push_back( font );
			presetList.push_back( {font, -1, -1, -1, 0, 0} );
		}
		else if ( !strcasecmp( ext.c_str(), "sflist" ) || !strcasecmp( ext.c_str(), "json" ) )
		{
            _presetList = cache_open_list( sSoundFontName.c_str() );
            if ( !_presetList )
            {
                shutdown();
                return false;
            }
            for (unsigned int i = 0, j = _presetList->count; i < j; ++i)
            {
                presetList.push_back( _presetList->presets[i] );
            }
		}
	}

	BASS_MIDI_StreamSetFonts( _stream[0], &presetList[0], (unsigned int) presetList.size() | BASS_MIDI_FONT_EX );
    BASS_MIDI_StreamSetFonts( _stream[1], &presetList[0], (unsigned int) presetList.size() | BASS_MIDI_FONT_EX );
    BASS_MIDI_StreamSetFonts( _stream[2], &presetList[0], (unsigned int) presetList.size() | BASS_MIDI_FONT_EX );

	reset_parameters();

	return true;
}

void BMPlayer::reset_parameters()
{
    bank_lsb_overridden = false;
    for ( unsigned int i = 0; i < 48; ++i )
    {
        if (bank_lsb_override[i])
            bank_lsb_overridden = true;
        BASS_MIDI_StreamEvent( _stream[i/16], i%16, MIDI_EVENT_BANK_LSB, bank_lsb_override[i] );
    }
}
