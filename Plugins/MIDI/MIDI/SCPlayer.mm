#include "SCPlayer.h"

#include <vector>

SCPlayer::SCPlayer()
: MIDIPlayer() {
	initialized = false;
	_controlROM = NULL;
	_pcmROM = NULL;
	for(unsigned int i = 0; i < 3; ++i) {
		_synth[i] = NULL;
	}
}

SCPlayer::~SCPlayer() {
	shutdown();
}

void SCPlayer::send_event(uint32_t b) {
	uint32_t port = (b >> 24) & 0xFF;
	if(port > 2) port = 0;
	_synth[port]->midi_input(b & 0xFF, (b >> 8) & 0xFF, (b >> 16) & 0xFF);
}

void SCPlayer::send_sysex(const uint8_t *event, size_t size, size_t port) {
	_synth[0]->midi_input_sysex(event, size);
	_synth[1]->midi_input_sysex(event, size);
	_synth[2]->midi_input_sysex(event, size);
}

void SCPlayer::render(float *out, unsigned long count) {
	for(unsigned long i = 0; i < count; ++i) {
		float sampleOut[6];
		_synth[0]->get_next_sample(sampleOut + 0);
		_synth[1]->get_next_sample(sampleOut + 2);
		_synth[2]->get_next_sample(sampleOut + 4);
		out[0] = sampleOut[0] + sampleOut[2] + sampleOut[4];
		out[1] = sampleOut[1] + sampleOut[3] + sampleOut[5];
		out += 2;
	}
}

void SCPlayer::shutdown() {
	for(int i = 0; i < 3; i++) {
		delete _synth[i];
		_synth[i] = NULL;
	}
	delete _pcmROM;
	_pcmROM = NULL;
	delete _controlROM;
	_controlROM = NULL;
	initialized = false;
}

bool SCPlayer::startup() {
	if(initialized) return true;

	NSBundle *bundle = [NSBundle bundleWithIdentifier:@"net.kode54.midi"];
	NSString *controlROMPath = [bundle pathForResource:@"r00233567_control" ofType:@"bin"];
	NSString *cpuROMPath = [bundle pathForResource:@"r15199858_main_mcu" ofType:@"bin"];
	NSString *pcmROMPath0 = [bundle pathForResource:@"r15209359_pcm_1" ofType:@"bin"];
	NSString *pcmROMPath1 = [bundle pathForResource:@"r15279813_pcm_2" ofType:@"bin"];

	_controlROM = new EmuSC::ControlRom(std::string([controlROMPath UTF8String]), std::string([cpuROMPath UTF8String]));
	_pcmROM = new EmuSC::PcmRom({ std::string([pcmROMPath0 UTF8String]), std::string([pcmROMPath1 UTF8String]) }, *_controlROM);

	for(int i = 0; i < 3; i++) {
		_synth[i] = new EmuSC::Synth(*_controlROM, *_pcmROM);
		_synth[i]->set_audio_format(uSampleRate, 2);
		if(i > 0) {
			_synth[i]->set_param(EmuSC::SystemParam::DeviceID, (uint8_t)(17 + i));
		}
	}

	initialized = true;

	setFilterMode(mode, reverb_chorus_disabled);

	return true;
}

unsigned int SCPlayer::send_event_needs_time() {
	return 0;
}
