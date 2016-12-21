#ifndef __SCPlayer_h__
#define __SCPlayer_h__

#include "MIDIPlayer.h"

#include "SCCore.h"

class SCPlayer : public MIDIPlayer
{
public:
	// zero variables
	SCPlayer();

	// close, unload
	virtual ~SCPlayer();

	unsigned int get_playing_note_count();

	typedef enum
	{
        sc_default = 0,
        sc_gm,
		sc_gm2,
		sc_sc55,
		sc_sc88,
		sc_sc88pro,
		sc_sc8850,
		sc_xg
	}
	sc_mode;
	
	void set_mode(sc_mode m);
    
void set_sccore_path(const char * path);

protected:
    virtual bool send_event_needs_time();
	virtual void send_event(uint32_t b);
	virtual void render(float * out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();
    
private:
	void send_sysex(uint32_t port, const uint8_t * data);
	void send_gs(uint32_t port, uint8_t * data);
	void reset_sc(uint32_t port);

	void reset(uint32_t port);

    void junk(uint32_t port, unsigned long count);
    
    unsigned int instance_id;
	bool initialized;
    SCCore * sampler;
	
	sc_mode mode;
    
    char * sccore_path;
};

#endif
