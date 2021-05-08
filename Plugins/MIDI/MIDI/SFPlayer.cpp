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

SFPlayer::SFPlayer() : MIDIPlayer()
{
    _synth[0] = 0;
    _synth[1] = 0;
    _synth[2] = 0;
    uInterpolationMethod = FLUID_INTERP_DEFAULT;

    for (unsigned int i = 0; i < 3; ++i)
    {
        _settings[i] = new_fluid_settings();

        fluid_settings_setnum(_settings[i], "synth.gain", 0.2);
        fluid_settings_setnum(_settings[i], "synth.sample-rate", 44100);
        fluid_settings_setint(_settings[i], "synth.midi-channels", 16);
        fluid_settings_setint(_settings[i], "synth.device-id", 0x10 + i);
    }
}

SFPlayer::~SFPlayer()
{
    for (unsigned int i = 0; i < 3; ++i)
    {
        if (_synth[i]) delete_fluid_synth(_synth[i]);
        if (_settings[i]) delete_fluid_settings(_settings[i]);
    }
}

void SFPlayer::setInterpolationMethod(unsigned method)
{
    shutdown();
    uInterpolationMethod = method;
    for (unsigned int i = 0; i < 3; ++i)
        if ( _synth[i] ) fluid_synth_set_interp_method( _synth[i], -1, method );
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
        fluid_synth_t* _synth = this->_synth[0];

        if ( port && port < 3 )
            _synth = this->_synth[port];

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
            break;
        case 0xC0:
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
        if (data && size > 2 && data[0] == 0xF0 && data[size-1] == 0xF7)
        {
            ++data;
            size -= 2;
            fluid_synth_sysex(_synth[0], (const char *)data, size, NULL, NULL, NULL, 0);
            fluid_synth_sysex(_synth[1], (const char *)data, size, NULL, NULL, NULL, 0);
            fluid_synth_sysex(_synth[2], (const char *)data, size, NULL, NULL, NULL, 0);
        }
    }
}

void SFPlayer::render(float * out, unsigned long count)
{
    unsigned long done = 0;
    memset(out, 0, sizeof(float) * 2 * count);
    while (done < count)
    {
        float buffer[512 * 2];
        unsigned long todo = count - done;
        unsigned long i;
        if (todo > 512) todo = 512;
        for (unsigned long j = 0; j < 3; ++j)
        {
            memset(buffer, 0, sizeof(buffer));
            fluid_synth_write_float(_synth[j], todo, buffer, 0, 2, buffer, 1, 2);
            for (i = 0; i < todo; ++i)
            {
                out[i * 2 + 0] += buffer[i * 2 + 0];
                out[i * 2 + 1] += buffer[i * 2 + 1];
            }
        }
        out += todo * 2;
        done += todo;
    }
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
    for (unsigned int i = 0; i < 3; ++i)
    {
        if (_synth[i]) delete_fluid_synth(_synth[i]);
        _synth[i] = 0;
    }
}

bool SFPlayer::startup()
{
    if ( _synth[0] && _synth[1] && _synth[2] ) return true;

    for (unsigned int i = 0; i < 3; ++i)
    {
        fluid_settings_setnum(_settings[i], "synth.sample-rate", uSampleRate);
        _synth[i] = new_fluid_synth(_settings[i]);
        if (!_synth[i])
        {
            _last_error = "Out of memory";
            return false;
        }
        fluid_synth_set_interp_method( _synth[i], -1, uInterpolationMethod );
    }
    if (sSoundFontName.length())
    {
        std::string ext;
        size_t dot = sSoundFontName.find_last_of( '.' );
        if ( dot != std::string::npos )
            ext.assign( sSoundFontName.begin() + dot + 1, sSoundFontName.end() );
        if ( !strcasecmp( ext.c_str(), "sf2" ) )
        {
            for (unsigned i = 0; i < 3; ++i)
            {
                if ( FLUID_FAILED == fluid_synth_sfload( _synth[i], sSoundFontName.c_str(), 1) )
                {
                    shutdown();
                    _last_error = "Failed to load SoundFont bank: ";
                    _last_error += sSoundFontName;
                    return false;
                }
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
                    for (unsigned i = 0; i < 3; ++i)
                    {
                        if ( FLUID_FAILED == fluid_synth_sfload( _synth[i], temp.c_str(), 1 ) )
                        {
                            fclose( fl );
                            shutdown();
                            _last_error = "Failed to load SoundFont bank: ";
                            _last_error += temp;
                            return false;
                        }
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
        for (unsigned i = 0; i < 3; ++i)
        {
            if ( FLUID_FAILED == fluid_synth_sfload(_synth[i], sFileSoundFontName.c_str(), 1) )
            {
                shutdown();
                _last_error = "Failed to load SoundFont bank: ";
                _last_error += sFileSoundFontName;
                return false;
            }
        }
    }

    _last_error = "";

    return true;
}

const char * SFPlayer::GetLastError() const
{
    if ( _last_error.length() ) return _last_error.c_str();
    return NULL;
}
