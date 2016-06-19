#ifndef __MSPlayer_h__
#define __MSPlayer_h__

#include "MIDIPlayer.h"

#include "interface.h"

class midisynth;

class MSPlayer : public MIDIPlayer
{
public:
	// zero variables
	MSPlayer();

	// close, unload
	virtual ~MSPlayer();

    void set_synth(unsigned int synth);
    void set_bank(unsigned int bank);
    void set_extp(unsigned int extp);
    
    typedef void (*enum_callback)(unsigned int synth, unsigned int bank, const char * name);
    
    void enum_synthesizers(enum_callback callback);
    
protected:
	virtual void send_event(uint32_t b);
	virtual void render(float * out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();
    
private:
    unsigned int synth_id;
    unsigned int bank_id;
    unsigned int extp;
    midisynth * synth;
};

#endif
