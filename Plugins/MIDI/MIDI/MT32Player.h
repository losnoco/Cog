#ifndef __MT32Player_h__
#define __MT32Player_h__

#include "MIDIPlayer.h"

#include <string>

#include "../../../Frameworks/munt/munt/mt32emu/src/mt32emu.h"

class MT32Player : public MIDIPlayer {
	public:
	// zero variables
	MT32Player(bool gm = false, unsigned gm_set = 0);

	// close, unload
	virtual ~MT32Player();

	// configuration
	void setBasePath(const char *in);

	protected:
	virtual void send_event(uint32_t b);
	virtual void render(float *out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();

	private:
	MT32Emu::Synth *_synth;
	std::string sBasePath;

	MT32Emu::File *controlRomFile, *pcmRomFile;
	const MT32Emu::ROMImage *controlRom, *pcmRom;

	bool bGM;
	unsigned uGMSet;

	void reset();

	MT32Emu::File *openFile(const char *filename);
};

#endif
