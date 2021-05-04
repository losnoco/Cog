//
//  SFPlayer.cpp
//  MIDI
//
//  Created by Christopher Snowhill on 5/3/21.
//  Copyright © 2021 Christopher Snowhill. All rights reserved.
//

#include "SFPlayer.h"

#include <string.h>

#define _countof(x) (sizeof((x))/sizeof(((x)[0])))

static const uint8_t sysex_gm_reset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
static const uint8_t sysex_gm2_reset[]= { 0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7 };
static const uint8_t sysex_gs_reset[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
static const uint8_t sysex_xg_reset[] = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

static bool is_gs_reset(const unsigned char * data, unsigned long size)
{
    if ( size != _countof( sysex_gs_reset ) ) return false;

    if ( memcmp( data, sysex_gs_reset, 5 ) != 0 ) return false;
    if ( memcmp( data + 7, sysex_gs_reset + 7, 2 ) != 0 ) return false;
    if ( ( ( data[ 5 ] + data[ 6 ] + 1 ) & 127 ) != data[ 9 ] ) return false;
    if ( data[ 10 ] != sysex_gs_reset[ 10 ] ) return false;

    return true;
}

SFPlayer::SFPlayer() : MIDIPlayer()
{
    _synth = 0;
    uInterpolationMethod = FLUID_INTERP_DEFAULT;

    reset_drums();

    synth_mode = mode_gm;

    _settings = new_fluid_settings();

    fluid_settings_setnum(_settings, "synth.gain", 1.0);
    fluid_settings_setnum(_settings, "synth.sample-rate", 44100);
    fluid_settings_setint(_settings, "synth.midi-channels", 48);
}

SFPlayer::~SFPlayer()
{
    if (_synth) delete_fluid_synth(_synth);
    if (_settings) delete_fluid_settings(_settings);
}

void SFPlayer::setInterpolationMethod(unsigned method)
{
    shutdown();
    uInterpolationMethod = method;
    if ( _synth ) fluid_synth_set_interp_method( _synth, -1, method );
}

void SFPlayer::send_event(uint32_t b)
{
    if (!(b & 0x80000000))
    {
        int param2 = (b >> 16) & 0xFF;
        int param1 = (b >> 8) & 0xFF;
        int cmd = b & 0xF0;
        int chan = b & 0x0F;
        int port = (b >> 24) & 0x7F;

        if ( port && port < 3 )
            chan += port * 16;

        switch (cmd)
        {
        case 0x80:
            fluid_synth_noteoff(_synth, chan, param1);
            break;
        case 0x90:
            fluid_synth_noteon(_synth, chan, param1, param2);
            break;
        case 0xA0:
            break;
        case 0xB0:
            fluid_synth_cc(_synth, chan, param1, param2);
            if ( param1 == 0 || param1 == 0x20 )
            {
                unsigned bank = channel_banks[ chan ];
                if ( param1 == 0x20 ) bank = ( bank & 0x3F80 ) | ( param2 & 0x7F );
                else bank = ( bank & 0x007F ) | ( ( param2 & 0x7F ) << 7 );
                channel_banks[ chan ] = bank;
                if ( synth_mode == mode_xg )
                {
                    if ( bank == 16256 ) drum_channels [chan] = 1;
                    else drum_channels [chan] = 0;
                }
                else if ( synth_mode == mode_gm2 )
                {
                    if ( bank == 15360 )
                        drum_channels [chan] = 1;
                    else if ( bank == 15488 )
                        drum_channels [chan] = 0;
                }
            }
            break;
        case 0xC0:
            if ( drum_channels [chan] ) fluid_synth_bank_select(_synth, chan, 16256 /*DRUM_INST_BANK*/);
            fluid_synth_program_change(_synth, chan, param1);
            break;
        case 0xD0:
            fluid_synth_channel_pressure(_synth, chan, param1);
            break;
        case 0xE0:
            fluid_synth_pitch_bend(_synth, chan, (param2 << 7) | param1);
            break;
        }
    }
    else
    {
        uint32_t n = b & 0xffffff;
        const uint8_t * data;
        size_t size, port;
        mSysexMap.get_entry( n, data, size, port );
        if (port >= 3)
            port = 0;
        if ( ( size == _countof( sysex_gm_reset ) && !memcmp( data, sysex_gm_reset, _countof( sysex_gm_reset ) ) ) ||
            ( size == _countof( sysex_gm2_reset ) && !memcmp( data, sysex_gm2_reset, _countof( sysex_gm2_reset ) ) ) ||
            is_gs_reset( data, size ) ||
            ( size == _countof( sysex_xg_reset ) && !memcmp( data, sysex_xg_reset, _countof( sysex_xg_reset ) ) ) )
        {
            fluid_synth_system_reset( _synth );
            reset_drums();
            synth_mode = ( size == _countof( sysex_xg_reset ) ) ? mode_xg :
                         ( size == _countof( sysex_gs_reset ) ) ? mode_gs :
                         ( data [4] == 0x01 )                   ? mode_gm :
                                                                  mode_gm2;
        }
        else if ( synth_mode == mode_gs && size == 11 &&
            data [0] == 0xF0 && data [1] == 0x41 && data [3] == 0x42 &&
            data [4] == 0x12 && data [5] == 0x40 && (data [6] & 0xF0) == 0x10 &&
            data [10] == 0xF7)
        {
            if (data [7] == 2)
            {
                // GS MIDI channel to part assign
                gs_part_to_ch [ ( port * 16 ) + ( data [6] & 15 ) ] = data [8];
            }
            else if ( data [7] == 0x15 )
            {
                // GS part to rhythm allocation
                unsigned int drum_channel = gs_part_to_ch [ ( port * 16 ) + ( data [6] & 15 ) ];
                if ( drum_channel < 16 )
                {
                    drum_channels [ ( port * 16 ) + drum_channel ] = data [8];
                    if ( port == 0 )
                    {
                        drum_channels [ 16 + drum_channel ] = data [8];
                        drum_channels [ 32 + drum_channel ] = data [8];
                    }
                }
            }
        }
    }
}

void SFPlayer::render(float * out, unsigned long count)
{
    fluid_synth_write_float(_synth, count, out, 0, 2, out, 1, 2);
}

void SFPlayer::setSoundFont( const char * in )
{
    sSoundFontName = in;
    shutdown();
}

void SFPlayer::setFileSoundFont( const char * in )
{
    sFileSoundFontName = in;
    shutdown();
}

void SFPlayer::shutdown()
{
    if (_synth) delete_fluid_synth(_synth);
    _synth = 0;
}

bool SFPlayer::startup()
{
    if ( _synth ) return true;

    fluid_settings_setnum(_settings, "synth.sample-rate", uSampleRate);

    _synth = new_fluid_synth(_settings);
    if (!_synth)
    {
        _last_error = "Out of memory";
        return false;
    }
    fluid_synth_set_interp_method( _synth, -1, uInterpolationMethod );
    reset_drums();
    synth_mode = mode_gm;
    if (sSoundFontName.length())
    {
        std::string ext;
        size_t dot = sSoundFontName.find_last_of( '.' );
        if ( dot != std::string::npos )
            ext.assign( sSoundFontName.begin() + dot + 1, sSoundFontName.end() );
        if ( !strcasecmp( ext.c_str(), "sf2" ) )
        {
            if ( FLUID_FAILED == fluid_synth_sfload( _synth, sSoundFontName.c_str(), 1) )
            {
                shutdown();
                _last_error = "Failed to load SoundFont bank: ";
                _last_error += sSoundFontName;
                return false;
            }
        }
        else if ( !strcasecmp( ext.c_str(), "sflist" ) )
        {
            FILE * fl = fopen( sSoundFontName.c_str(), "r" );
            if ( fl )
            {
                std::string path, temp;
                char name[32768];
                size_t slash = sSoundFontName.find_last_of( '\\' );
                if ( slash != std::string::npos )
                    path.assign( sSoundFontName.begin(), sSoundFontName.begin() + slash + 1 );
                while ( !feof( fl ) )
                {
                    if ( !fgets( name, 32767, fl ) ) break;
                    name[32767] = 0;
                    char * cr = strchr( name, '\n' );
                    if ( cr ) *cr = 0;
                    cr = strchr( name, '\r' );
                    if ( cr ) *cr = 0;
                    if ( name[0] == '/' )
                    {
                        temp = name;
                    }
                    else
                    {
                        temp = path;
                        temp += name;
                    }
                    if ( FLUID_FAILED == fluid_synth_sfload( _synth, temp.c_str(), 1 ) )
                    {
                        fclose( fl );
                        shutdown();
                        _last_error = "Failed to load SoundFont bank: ";
                        _last_error += temp;
                        return false;
                    }
                }
                fclose( fl );
            }
            else
            {
                _last_error = "Failed to open SoundFont list: ";
                _last_error += sSoundFontName;
                return false;
            }
        }
    }

    if ( sFileSoundFontName.length() )
    {
        if ( FLUID_FAILED == fluid_synth_sfload(_synth, sFileSoundFontName.c_str(), 1) )
        {
            shutdown();
            _last_error = "Failed to load SoundFont bank: ";
            _last_error += sFileSoundFontName;
            return false;
        }
    }

    _last_error = "";

    return true;
}

void SFPlayer::reset_drums()
{
    static const uint8_t part_to_ch[16] = { 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15 };

    memset( drum_channels, 0, sizeof( drum_channels ) );
    drum_channels [9] = 1;
    drum_channels [9+16] = 1;
    drum_channels [9+32] = 1;

    memcpy( gs_part_to_ch, part_to_ch, sizeof( gs_part_to_ch ) );
    memcpy( gs_part_to_ch+16, part_to_ch, sizeof( gs_part_to_ch ) );
    memcpy( gs_part_to_ch+32, part_to_ch, sizeof( gs_part_to_ch ) );

    memset( channel_banks, 0, sizeof( channel_banks ) );

    if ( _synth )
    {
        for ( unsigned i = 0; i < 48; ++i )
        {
            if ( drum_channels [i] )
            {
                fluid_synth_bank_select( _synth, i, 16256 /*DRUM_INST_BANK*/);
            }
        }
    }
}

const char * SFPlayer::GetLastError() const
{
    if ( _last_error.length() ) return _last_error.c_str();
    return NULL;
}
