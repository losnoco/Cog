#ifndef __AUPlayer_h__
#define __AUPlayer_h__

#include "MIDIPlayer.h"

#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreAudio/CoreAudioTypes.h>

class AUPlayer : public MIDIPlayer
{
public:
	// zero variables
	AUPlayer();

	// close, unload
	virtual ~AUPlayer();

	// configuration
    void showDialog();
    
    typedef void (*callback)(const char * name);
    void enumComponents(callback cbEnum);
    
    void setComponent(const char * name);

protected:
	virtual void send_event(uint32_t b);
	virtual void render(float * out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();
    
private:
    AudioTimeStamp mTimeStamp;

    AudioUnit samplerUnit[3];
    
    AudioBufferList *bufferList;
    
    float *audioBuffer;
    
    char *mComponentName;
};

#endif
