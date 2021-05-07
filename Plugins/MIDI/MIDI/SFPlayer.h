//
//  SFPlayer.h
//  MIDI
//
//  Created by Christopher Snowhill on 5/3/21.
//  Copyright © 2021 Christopher Snowhill. All rights reserved.
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

    const char * GetLastError() const;

private:
    virtual void send_event(uint32_t b);
    virtual void render(float * out, unsigned long count);

    virtual void shutdown();
    virtual bool startup();

    std::string       _last_error;

    fluid_settings_t * _settings[3];
    fluid_synth_t    * _synth[3];
    std::string        sSoundFontName;
    std::string        sFileSoundFontName;

    unsigned           uInterpolationMethod;
};

#endif /* SFPlayer_h */
