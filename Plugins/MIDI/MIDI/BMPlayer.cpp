#include "BMPlayer.h"

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
    Cached_SoundFont() : handle( 0 ) { }
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
        BASS_MIDI_FontFree( it->second.handle );
    }
}

static HSOUNDFONT cache_open( const char * path )
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

static void cache_close( HSOUNDFONT handle )
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
                        BASS_MIDI_FontFree( it->second.handle );
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

void BMPlayer::send_event(uint32_t b, uint32_t sample_offset)
{
    _eventQueue.push_back(BASS_event(b, sample_offset));
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

void BMPlayer::render_512(float *out)
{
    unsigned long queue_index = 0;
    unsigned long queue_count = _eventQueue.size();
    uint32_t current = 0;
    uint32_t next = 512;
    if (queue_count)
        next = _eventQueue[0].sample_offset;
    do
    {
        if (next > 512)
            next = 512;
        
        if (next > current)
            render(out + current * 2, next - current);
        
        current = next;
        
        while (queue_index < queue_count && _eventQueue[queue_index].sample_offset <= next)
        {
            send_event(_eventQueue[queue_index].b);
            queue_index++;
        }
        if (queue_index < queue_count)
            next = _eventQueue[queue_index].sample_offset;
        else
            next = 512;
    } while (current < 512);
    
    if (queue_index < queue_count)
    {
        memmove(&_eventQueue[0], &_eventQueue[queue_index], sizeof(BASS_event) * (queue_count - queue_index));
        _eventQueue.resize(queue_count - queue_index);
    }
    else
        _eventQueue.resize(0);
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
		cache_close( _soundFonts[i] );
    }
	_soundFonts.resize( 0 );
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
			HSOUNDFONT font = cache_open( sSoundFontName.c_str() );
			if ( !font )
			{
				shutdown();
				return false;
			}
            _soundFonts.push_back( font );
			presetList.push_back( {font, -1, -1, -1, 0, 0} );
		}
		else if ( !strcasecmp( ext.c_str(), "sflist" ) )
		{
			FILE * fl = fopen( sSoundFontName.c_str(), "r" );
			if ( fl )
			{
                std::string path, temp;
                char name[32768];
                char *nameptr;
                size_t slash = sSoundFontName.find_last_of('/');
                if ( slash != std::string::npos ) path.assign( sSoundFontName.begin(), sSoundFontName.begin() + slash + 1 );
				while ( !feof( fl ) )
				{
                    std::vector<BASS_MIDI_FONTEX> presets;
                    
					if ( !fgets( name, 32767, fl ) ) break;
					name[32767] = 0;
					char * cr = strchr( name, '\n' );
					if ( cr ) *cr = 0;
                    cr = strchr( name, '\r' );
                    if ( cr ) *cr = 0;
                    cr = strchr( name, '|' );
                    if ( cr )
                    {
                        std::vector<BASS_MIDI_FONTEX> nested_presets;
                        std::vector<long> channels;
                        bool valid = true;
                        bool pushed_back = true;
                        char *endchr;
                        nameptr = cr + 1;
                        *cr = 0;
                        cr = name;
                        while ( *cr && valid )
                        {
                            switch ( *cr++ )
                            {
                                case 'p':
                                {
                                    // patch override - "p[db#,]dp#=[sb#,]sp#" ex. "p0,5=0,1"
                                    // may be used once per preset group
                                    pushed_back = false;
                                    long dbank = 0;
                                    long dpreset = strtol( cr, &endchr, 10 );
                                    if ( endchr == cr )
                                    {
                                        valid = false;
                                        break;
                                    }
                                    if ( *endchr == ',' )
                                    {
                                        dbank = dpreset;
                                        cr = endchr + 1;
                                        dpreset = strtol( cr, &endchr, 10 );
                                        if ( endchr == cr )
                                        {
                                            valid = false;
                                            break;
                                        }
                                    }
                                    if ( *endchr != '=' )
                                    {
                                        valid = false;
                                        break;
                                    }
                                    cr = endchr + 1;
                                    long sbank = -1;
                                    long spreset = strtol( cr, &endchr, 10 );
                                    if ( endchr == cr )
                                    {
                                        valid = false;
                                        break;
                                    }
                                    if ( *endchr == ',' )
                                    {
                                        sbank = spreset;
                                        cr = endchr + 1;
                                        spreset = strtol( cr, &endchr, 10 );
                                        if ( endchr == cr )
                                        {
                                            valid = false;
                                            break;
                                        }
                                    }
                                    if ( *endchr && *endchr != ';' && *endchr != '&' )
                                    {
                                        valid = false;
                                        break;
                                    }
                                    cr = endchr;
                                    nested_presets.push_back( { 0, (int) spreset, (int) sbank, (int) dpreset, (int) dbank, 0 } );
                                }
                                break;
                                    
                                case 'c':
                                {
                                    // channel override - implemented using bank LSB, which is disabled from
                                    // actual use. - format "c#[-#]" ex. "c16" (range is 1-48)
                                    // may be used multiple times per preset group
                                    pushed_back = false;
                                    long channel_start = strtol(cr, &endchr, 10);
                                    long channel_end;
                                    if ( endchr == cr )
                                    {
                                        valid = false;
                                        break;
                                    }
                                    if ( channel_start < 1 || channel_start > 48 )
                                    {
                                        valid = false;
                                        break;
                                    }
                                    channel_end = channel_start;
                                    if ( *endchr == '-' )
                                    {
                                        channel_end = strtol(cr, &endchr, 10);
                                        if ( channel_end <= channel_start || channel_end > 48 )
                                        {
                                            valid = false;
                                            break;
                                        }
                                    }
                                    for ( auto it = channels.begin(); it != channels.end(); ++it )
                                    {
                                        if ( *it >= channel_start || *it <= channel_end )
                                        {
                                            valid = false;
                                            break;
                                        }
                                    }
                                    if ( *endchr && *endchr != ';' )
                                    {
                                        valid = false;
                                        break;
                                    }
                                    cr = endchr;
                                    for ( long channel = channel_start; channel <= channel_end; ++channel )
                                        channels.push_back( channel );
                                }
                                break;
                                    
                                case '&':
                                {
                                    // separates preset groups per SoundFont bank
                                    if ( !pushed_back )
                                    {
                                        compound_presets( presets, nested_presets, channels );
                                        nested_presets.clear(); channels.clear();
                                        pushed_back = true;
                                    }
                                }
                                break;
                                    
                                case ';':
                                    // separates preset items
                                    break;
                                    
                                default:
                                    // invalid command character
                                    valid = false;
                                    break;
                            }
                        }
                        if ( !pushed_back && valid )
                            compound_presets( presets, nested_presets, channels );
                        if ( !valid )
                        {
                            presets.clear();
                            presets.push_back( { 0, -1, -1, -1, 0, 0 } );
                            memset( bank_lsb_override, 0, sizeof(bank_lsb_override) );
                        }
                    }
                    else
                    {
                        presets.push_back( { 0, -1, -1, -1, 0, 0 } );
                        nameptr = name;
                    }
					if ( nameptr[0] == '/' )
					{
						temp = nameptr;
					}
					else
					{
                        temp = path;
                        temp += nameptr;
					}
					HSOUNDFONT font = cache_open( temp.c_str() );
					if ( !font )
					{
						fclose( fl );
						shutdown();
						return false;
					}
                    for ( auto it = presets.begin(); it != presets.end(); ++it )
                    {
                        it->font = font;
                        presetList.push_back( *it );
                    }
                    _soundFonts.push_back( font );
				}
				fclose( fl );
			}
			else
			{
				return false;
			}
		}
	}

	if ( sFileSoundFontName.length() )
	{
		HSOUNDFONT font = cache_open( sFileSoundFontName.c_str() );
		if ( !font )
		{
			shutdown();
			return false;
		}
        _soundFonts.push_back( font );
		presetList.push_back( { font, -1, -1, -1, 0, 0 } );
	}

    std::vector< BASS_MIDI_FONTEX > fonts;
	for ( unsigned long i = 0, j = presetList.size(); i < j; ++i )
	{
        fonts.push_back( presetList[j - i - 1] );
	}
	BASS_MIDI_StreamSetFonts( _stream[0], &fonts[0], (unsigned int) fonts.size() | BASS_MIDI_FONT_EX );
    BASS_MIDI_StreamSetFonts( _stream[1], &fonts[0], (unsigned int) fonts.size() | BASS_MIDI_FONT_EX );
    BASS_MIDI_StreamSetFonts( _stream[2], &fonts[0], (unsigned int) fonts.size() | BASS_MIDI_FONT_EX );

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
