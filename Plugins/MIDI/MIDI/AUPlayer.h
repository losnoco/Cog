#ifndef __AUPlayer_h__
#define __AUPlayer_h__

#include "MIDIPlayer.h"

//#include <string>

#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreAudio/CoreAudioTypes.h>

class AUPluginUI;

class AUPlayer : public MIDIPlayer
{
public:
	// zero variables
	AUPlayer();

	// close, unload
	virtual ~AUPlayer();

	// configuration
    void setSoundFont( const char * in );
    /*void setFileSoundFont( const char * in );*/
    //void showDialog();
    
    typedef void (*callback)(OSType uSubType, OSType uManufacturer, const char * name);
    static void enumComponents(callback cbEnum);
    
    void setComponent(OSType uSubType, OSType uManufacturer);

protected:
	virtual void send_event(uint32_t b, uint32_t sample_offset);
	virtual void render_512(float * out);

	virtual void shutdown();
	virtual bool startup();
    
private:
    void loadSoundFont(const char * name);
    
    std::string        sSoundFontName;
    /*std::string        sFileSoundFontName;*/

    AudioTimeStamp mTimeStamp;

    AudioUnit samplerUnit[3];
    
    bool samplerUIinitialized[3];
    AUPluginUI * samplerUI[3];
    
    AudioBufferList *bufferList;
    
    float *audioBuffer;
    
    OSType componentSubType, componentManufacturer;
};

#endif
