//
//  SFPlayer.h
//  MIDI
//
//  Created by Christopher Snowhill on 5/3/21.
//  Copyright © 2021-2022 Christopher Snowhill. All rights reserved.
//

#ifndef SFPlayer_h
#define SFPlayer_h

#include "MIDIPlayer.h"

#include <fluidsynth.h>

class SFPlayer : public MIDIPlayer
{
public:
    // zero variables
    SFPlayer();

    // close, unload
    virtual ~SFPlayer();

    // configuration
    void setSoundFont( const char * in );
    void setFileSoundFont( const char * in );
    void setInterpolationMethod(unsigned method);
    void setDynamicLoading(bool enabled);

    const char * GetLastError() const;

private:
    virtual void send_event(uint32_t b);
    virtual void send_sysex(const uint8_t * data, size_t size, size_t port);
    virtual void render(float * out, unsigned long count);

    virtual void shutdown();
    virtual bool startup();

    std::string        _last_error;

    fluid_settings_t * _settings[3];
    fluid_synth_t    * _synth[3];
    std::string        sSoundFontName;
    std::string        sFileSoundFontName;

    unsigned           uInterpolationMethod;
    bool               bDynamicLoading;
};

#endif /* SFPlayer_h */
