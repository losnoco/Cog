#ifndef __BMPlayer_h__
#define __BMPlayer_h__

#include "MIDIPlayer.h"

#include "../../../ThirdParty/BASS/bassmidi.h"

class BMPlayer : public MIDIPlayer
{
public:
	// zero variables
	BMPlayer();

	// close, unload
	virtual ~BMPlayer();

	// configuration
	void setSoundFont( const char * in );
	void setFileSoundFont( const char * in );
	void setSincInterpolation(bool enable = true);

private:
	void send_event(uint32_t b);
    void render(float * out, unsigned long count);
    
    virtual void send_event(uint32_t b, uint32_t sample_offset);
	virtual void render_512(float * out);

	virtual void shutdown();
	virtual bool startup();
    
    void compound_presets( std::vector<BASS_MIDI_FONTEX> & out, std::vector<BASS_MIDI_FONTEX> & in, std::vector<long> & channels );

	void reset_parameters();

    std::vector<HSOUNDFONT> _soundFonts;
    std::string        sSoundFontName;
    std::string        sFileSoundFontName;
    
    typedef struct BASS_event
    {
        uint32_t b;
        uint32_t sample_offset;
        BASS_event() : b(0), sample_offset(0) { }
        BASS_event(uint32_t _b, uint32_t _sample_offset) : b(_b), sample_offset(_sample_offset) { }
    } BASS_event;
    std::vector<BASS_event> _eventQueue;

	HSTREAM            _stream[3];

	bool               bSincInterpolation;

    bool               bank_lsb_overridden;
    uint8_t            bank_lsb_override[48];
};

#endif
