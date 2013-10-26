#include "../smp/smp.hpp"
#include "dsp.hpp"

namespace SuperFamicom {

void DSP::step(unsigned clocks) {
  clock += clocks;
}

void DSP::synchronize_smp() {
  while(clock >= 0) smp.enter();
}

void DSP::enter() {
  spc_dsp.run(1);
  step(24);

  signed count = spc_dsp.sample_count();
  if(count > 0) {
    for(unsigned n = 0; n < count; n += 2) smp.sample(samplebuffer[n + 0], samplebuffer[n + 1]);
    spc_dsp.set_output(samplebuffer, 8192);
  }
}

bool DSP::mute() {
  return spc_dsp.mute();
}

uint8_t DSP::read(uint8_t addr) {
  return spc_dsp.read(addr);
}

void DSP::write(uint8_t addr, uint8_t data) {
  spc_dsp.write(addr, data);
}

void DSP::power() {
  spc_dsp.init(smp.apuram);
  spc_dsp.reset();
  spc_dsp.set_output(samplebuffer, 8192);
}

void DSP::reset() {
  spc_dsp.soft_reset();
  spc_dsp.set_output(samplebuffer, 8192);
}

void DSP::channel_enable(unsigned channel, bool enable) {
  channel_enabled[channel & 7] = enable;
  unsigned mask = 0;
  for(unsigned i = 0; i < 8; i++) {
    if(channel_enabled[i] == false) mask |= 1 << i;
  }
  spc_dsp.mute_voices(mask);
}
    
void DSP::disable_surround(bool disable) {
  spc_dsp.disable_surround(disable);
}

DSP::DSP(struct SMP & p_smp)
    : smp( p_smp ), clock( 0 ) {
  for(unsigned i = 0; i < 8; i++) channel_enabled[i] = true;
}

}
